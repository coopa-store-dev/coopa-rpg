// main/rpg_battle.c —— 见 rpg_battle.h。
#include "rpg_battle.h"

#include <string.h>

#define NO_KEY 0xFF

// 按 kit_heat_t 下标:中辣、微辣、特辣
static const rpg_heat_rule_t RULES[KIT_HEATS] = {
    [KIT_HEAT_MID] = { 100, 100, RB_BAR_PERF_FROM, RB_BAR_PERF_TO, RB_FEINT_PCT, RB_FLEE_PCT, RB_REFUSE_NEED,
                       RB_BEAT_WIN, RB_WIGGLE_NEED },
    [KIT_HEAT_MILD] = { 80, 70, 750, 900, 15, 90, 7, 220, 6 },
    [KIT_HEAT_HOT] = { 125, 125, 790, 870, 40, 60, 12, 130, 10 },
};

const rpg_heat_rule_t *rpg_heat_rule(kit_heat_t h) {
    return &RULES[h < KIT_HEATS ? h : KIT_HEAT_MID];
}

static const rpg_heat_rule_t *rule(const rpg_battle_t *b) {
    return rpg_heat_rule((kit_heat_t)b->heat);
}

static uint32_t rnd(rpg_battle_t *b, uint32_t n) {
    uint32_t x = b->rng ? b->rng : 0x9E3779B9u;  // xorshift32
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    b->rng = x;
    return x % n;
}

rpg_key_t rpg_lane_key(rpg_lane_t lane) {
    switch (lane) {
        case RPG_LANE_HIGH: return RB_KEY_DOWN;  // 打头:蹲
        case RPG_LANE_LOW:  return RB_KEY_UP;    // 打脚:跳
        default:            return RB_KEY_OK;    // 打身子:挡
    }
}

bool rpg_battle_over(const rpg_battle_t *b) {
    return b->phase == RB_WON || b->phase == RB_LOST || b->phase == RB_FLED;
}

static void go(rpg_battle_t *b, rpg_bphase_t p) {
    b->phase = p;
    b->t = 0;
}

static void sp_add(rpg_battle_t *b, uint8_t n) {
    uint8_t max = rpg_sp_max(b->s);
    b->s->sp = (uint8_t)(b->s->sp + n > max ? max : b->s->sp + n);
}

static void perfect(rpg_battle_t *b) {
    b->perfects++;
    if (b->s->perfects < 0xFFFF) b->s->perfects++;
    sp_add(b, 1);
}

// 库巴打出的伤害:攻击 × mult10/10(辣条再 ×1.5,怕辣的冰淇淋 ×2.5)减防,±10% 浮动,至少 1。
static uint16_t kid_damage(rpg_battle_t *b, uint32_t mult10, bool ignore_def) {
    uint32_t atk = (uint32_t)rpg_atk(b->s) * mult10;
    if (b->spicy) atk = (b->info->flags & MONF_MELT) ? atk * RB_MELT_X10 / 10u : atk * 3u / 2u;
    atk /= 10u;
    uint32_t def = ignore_def ? 0 : b->info->def;
    uint32_t base = atk > def ? atk - def : 1;
    uint32_t v = base * (90u + rnd(b, 21)) / 100u;
    return (uint16_t)(v ? v : 1);
}

// 敌人这一仗的攻击(按辣度打折 / 加码)。
static uint32_t enemy_atk(const rpg_battle_t *b) {
    return b->info->atk * (uint32_t)rule(b)->atk_pct / 100u;
}

// 敌人打库巴不浮动:孩子能看懂「挡住 = 一半」。
static uint16_t enemy_base(const rpg_battle_t *b) {
    uint8_t def = rpg_def(b->s);
    uint32_t atk = enemy_atk(b);
    return (uint16_t)(atk > def ? atk - def : 1);
}

// 辣条按「出手攻击的次数」算,吃辣条的那一回合不算。
static void spicy_used(rpg_battle_t *b) {
    if (b->spicy) b->spicy--;
}

static void hurt_enemy(rpg_battle_t *b, uint16_t dmg) {
    b->last_dmg = dmg;
    b->ehp = b->ehp > dmg ? (uint16_t)(b->ehp - dmg) : 0;
}

static void hurt_kid(rpg_battle_t *b, uint16_t dmg) {
    b->last_dmg = dmg;
    b->s->hp = b->s->hp > dmg ? (uint16_t)(b->s->hp - dmg) : 0;
}

void rpg_battle_start(rpg_battle_t *b, rpg_save_t *s, rpg_mon_t mon, uint32_t seed) {
    memset(b, 0, sizeof(*b));
    b->s = s;
    b->mon = mon;
    b->info = rpg_mon(mon);
    b->heat = (uint8_t)kit_heat_from(s->heat);
    b->ehp = b->ehp_max = (uint16_t)(b->info->hp * (uint32_t)rule(b)->hp_pct / 100u);
    b->rng = seed;
    b->stage = (b->info->flags & MONF_SKEWER) ? RB_SKEWER_BALLS : 1;
    b->pend_key = NO_KEY;
    b->last_key = NO_KEY;
    rpg_meet(s, mon);
    go(b, RB_MENU);
}

// ---------------------------------------------------------------------------
// 菜单

uint8_t rpg_battle_rows(const rpg_battle_t *b) {
    uint8_t n = 0;
    if (b->phase == RB_SKILLS) {
        for (int k = 0; k < SK_COUNT; k++) n += rpg_has_skill(b->s, (rpg_skill_t)k);
        return (uint8_t)(n + 1);
    }
    if (b->phase == RB_ITEMS) {
        for (int k = 0; k < ITEM_COUNT; k++) n += b->s->items[k] > 0;
        return (uint8_t)(n + 1);
    }
    return 4;
}

int rpg_battle_row(const rpg_battle_t *b, uint8_t i) {
    if (b->phase == RB_SKILLS) {
        for (int k = 0; k < SK_COUNT; k++) {
            if (rpg_has_skill(b->s, (rpg_skill_t)k) && i-- == 0) return k;
        }
        return -1;
    }
    if (b->phase == RB_ITEMS) {
        for (int k = 0; k < ITEM_COUNT; k++) {
            if (b->s->items[k] && i-- == 0) return k;
        }
        return -1;
    }
    return i;
}

uint32_t rpg_battle_progress(const rpg_battle_t *b) {
    uint32_t dur;
    switch (b->phase) {
        case RB_BAR:        dur = RB_BAR_MS; break;
        case RB_MASH:       dur = RB_MASH_MS; break;
        case RB_MEMO_SHOW:  dur = RB_MEMO_SHOW_MS; break;
        case RB_MEMO_INPUT: dur = RB_MEMO_INPUT_MS; break;
        case RB_WINDUP:     dur = b->windup_ms; break;
        case RB_STRIKE:     dur = b->strike_ms; break;
        case RB_QUIZ:       dur = RB_QUIZ_MS; break;
        case RB_REFUSE:     dur = RB_REFUSE_MS; break;
        case RB_BEAT:       dur = RB_BEAT_MS; break;
        case RB_WIGGLE:     dur = RB_WIGGLE_MS; break;
        default:            return 0;
    }
    uint32_t p = b->t * 1000u / (dur ? dur : 1);
    return p > 1000u ? 1000u : p;
}

static uint32_t to_act(rpg_battle_t *b) {
    go(b, RB_ACT);
    return BE_PHASE;
}

// ---------------------------------------------------------------------------
// 我方出招

static uint32_t resolve_bar(rpg_battle_t *b, bool pressed) {
    uint32_t p = rpg_battle_progress(b);
    rpg_grade_t g = GRADE_WEAK;
    if (pressed && p >= rule(b)->perf_from && p <= rule(b)->perf_to) g = GRADE_PERFECT;
    else if (pressed && p >= RB_BAR_GOOD_FROM && p <= RB_BAR_GOOD_TO) g = GRADE_GOOD;
    b->grade = (uint8_t)g;
    if (g == GRADE_PERFECT) perfect(b);
    uint32_t ev = BE_GRADE;
    switch (b->act) {
        case ACT_HEAL: {
            uint16_t max = rpg_hp_max(b->s);
            uint16_t heal = (uint16_t)(max * (g == GRADE_PERFECT ? 8u : g == GRADE_GOOD ? 6u : 4u) / 10u);
            b->last_heal = heal;
            b->s->hp = (uint16_t)(b->s->hp + heal > max ? max : b->s->hp + heal);
            ev |= BE_HEAL;
            break;
        }
        case ACT_SLIPPER:
            hurt_enemy(b, kid_damage(b, g == GRADE_PERFECT ? 30 : g == GRADE_GOOD ? 25 : pressed ? 20 : 10, true));
            spicy_used(b);
            ev |= BE_ENEMY_HURT;
            break;
        default: {  // 拳头
            uint32_t m = g == GRADE_PERFECT ? 20 : g == GRADE_GOOD ? 15 : pressed ? 10 : 5;
            hurt_enemy(b, kid_damage(b, m, false));
            spicy_used(b);
            ev |= BE_ENEMY_HURT;
            break;
        }
    }
    return ev | to_act(b);
}

static uint32_t resolve_mash(rpg_battle_t *b) {
    uint32_t hits = b->taps / 2u;
    if (hits > RB_MASH_MAX_HITS) hits = RB_MASH_MAX_HITS;
    b->grade = hits >= 6 ? GRADE_PERFECT : hits >= 3 ? GRADE_GOOD : GRADE_WEAK;
    uint32_t ev = BE_GRADE;
    if (hits) {
        uint32_t dmg = 0;
        for (uint32_t i = 0; i < hits; i++) dmg += kid_damage(b, 6, false);
        hurt_enemy(b, (uint16_t)dmg);
        spicy_used(b);
        ev |= BE_ENEMY_HURT;
    } else {
        b->last_dmg = 0;
    }
    return ev | to_act(b);
}

static uint32_t resolve_beam(rpg_battle_t *b) {
    b->grade = b->memo_ok == RB_MEMO_LEN ? GRADE_PERFECT : b->memo_ok >= 2 ? GRADE_GOOD : GRADE_WEAK;
    if (b->grade == GRADE_PERFECT) perfect(b);
    uint32_t ev = BE_GRADE;
    if (b->memo_ok) {
        uint32_t dmg = 0;
        for (uint8_t i = 0; i < b->memo_ok; i++) dmg += kid_damage(b, 10, false);
        if (b->memo_ok == RB_MEMO_LEN) dmg = dmg * 3u / 2u;
        hurt_enemy(b, (uint16_t)dmg);
        spicy_used(b);
        ev |= BE_ENEMY_HURT;
    } else {
        b->last_dmg = 0;
    }
    return ev | to_act(b);
}

static uint32_t use_item(rpg_battle_t *b, rpg_item_t item) {
    b->act = ACT_ITEM;
    b->item = (uint8_t)item;
    b->s->items[item]--;
    uint32_t ev = BE_ITEM;
    if (item == ITEM_DUMPLING) {
        uint16_t max = rpg_hp_max(b->s);
        b->last_heal = (uint16_t)(b->s->hp + RPG_DUMPLING_HEAL > max ? max - b->s->hp : RPG_DUMPLING_HEAL);
        b->s->hp = (uint16_t)(b->s->hp + b->last_heal);
        ev |= BE_HEAL;
    } else if (item == ITEM_SPICY) {
        b->spicy = RPG_SPICY_TURNS;
        ev |= BE_SPICY;
    } else {
        sp_add(b, RPG_SODA_SP);
    }
    return ev | to_act(b);
}

static uint32_t use_skill(rpg_battle_t *b, rpg_skill_t sk) {
    uint8_t cost = rpg_skill_cost(sk);
    if (b->s->sp < cost) return BE_NO_SP;
    b->s->sp = (uint8_t)(b->s->sp - cost);
    b->taps = 0;
    switch (sk) {
        case SK_COMBO:
            b->act = ACT_COMBO;
            go(b, RB_MASH);
            return BE_PHASE;
        case SK_HEAL:
            b->act = ACT_HEAL;
            go(b, RB_BAR);
            return BE_PHASE;
        case SK_SLIPPER:
            b->act = ACT_SLIPPER;
            go(b, RB_BAR);
            return BE_PHASE | BE_SLIPPER;
        case SK_MOVE:  // 这一回合不出拳,敌人打过来时闪开一下就打回去一下
            b->act = ACT_MOVE;
            b->counter = true;
            return to_act(b);
        default:
            b->act = ACT_BEAM;
            b->memo_ok = 0;
            for (int i = 0; i < RB_MEMO_LEN; i++) b->memo[i] = (uint8_t)rnd(b, RPG_LANES);
            go(b, RB_MEMO_SHOW);
            return BE_PHASE;
    }
}

static uint32_t menu_input(rpg_battle_t *b, rpg_key_t key) {
    uint8_t n = rpg_battle_rows(b);
    if (key == RB_KEY_UP) {
        b->sel = (uint8_t)((b->sel + n - 1u) % n);
        return BE_CURSOR;
    }
    if (key == RB_KEY_DOWN) {
        b->sel = (uint8_t)((b->sel + 1u) % n);
        return BE_CURSOR;
    }
    int row = rpg_battle_row(b, b->sel);
    if (b->phase == RB_SKILLS || b->phase == RB_ITEMS) {
        if (row < 0) {  // 返回
            b->sel = b->phase == RB_SKILLS ? 1 : 2;
            go(b, RB_MENU);
            return BE_PHASE;
        }
        return b->phase == RB_SKILLS ? use_skill(b, (rpg_skill_t)row) : use_item(b, (rpg_item_t)row);
    }
    switch (row) {
        case 0:
            b->act = ACT_PUNCH;
            go(b, RB_BAR);
            return BE_PHASE;
        case 1:
        case 2:
            go(b, row == 1 ? RB_SKILLS : RB_ITEMS);
            b->sel = 0;
            return BE_PHASE;
        default:
            if (b->info->flags & MONF_BOSS) return BE_NO_FLEE;
            b->act = ACT_FLEE;
            if (rnd(b, 100) < rule(b)->flee_pct) {
                go(b, RB_FLED);
                return BE_PHASE | BE_FLED;
            }
            return BE_FLEE_FAIL | to_act(b);
    }
}

// ---------------------------------------------------------------------------
// 敌方回合

static uint8_t pick_lane(rpg_battle_t *b, uint8_t avoid) {
    uint8_t lanes[RPG_LANES], n = 0;
    for (uint8_t l = 0; l < RPG_LANES; l++) {
        if ((b->info->lanes & RPG_LANE_BIT(l)) && l != avoid) lanes[n++] = l;
    }
    return n ? lanes[rnd(b, n)] : avoid;
}

// 魔王 / 糖果大王:血 ≤ 1/2 第二阶段、≤ 1/5 第三阶段
#define MONF_STAGED (MONF_KING | MONF_CANDY)

static bool raining(const rpg_battle_t *b) {
    return (b->info->flags & MONF_STAGED) && b->stage == 2;
}

static uint32_t windup_start(rpg_battle_t *b) {
    b->lane = pick_lane(b, 0xFF);
    b->windup_ms = b->info->windup_ms;
    b->strike_ms = raining(b) ? (uint16_t)(b->info->strike_ms * 17u / 20u) : b->info->strike_ms;
    b->answered = false;
    b->pend_key = NO_KEY;
    b->feinted = false;
    b->feint_at = 0;
    if ((b->info->flags & MONF_FEINT) && rnd(b, 100) < rule(b)->feint_pct) {
        uint8_t to = pick_lane(b, b->lane);
        if (to != b->lane) {
            b->feint_at = (uint16_t)(b->windup_ms * 55u / 100u);
            b->feint_to = to;
        }
    }
    go(b, RB_WINDUP);
    return BE_PHASE | BE_WINDUP;
}

static void make_quiz(rpg_battle_t *b) {
    uint16_t ans;
    int16_t cand[4];
    b->qmul = rnd(b, 2) == 0;
    if (b->qmul) {
        b->qa = (uint8_t)(2 + rnd(b, 8));
        b->qb = (uint8_t)(2 + rnd(b, 8));
        ans = (uint16_t)(b->qa * b->qb);
        cand[0] = (int16_t)(ans + b->qb);
        cand[1] = (int16_t)(ans - b->qb);
        cand[2] = (int16_t)(ans + b->qa);
        cand[3] = (int16_t)(ans + 1);
    } else {
        b->qa = (uint8_t)(11 + rnd(b, 39));
        b->qb = (uint8_t)(11 + rnd(b, 39));
        ans = (uint16_t)(b->qa + b->qb);
        cand[0] = (int16_t)(ans + 10);
        cand[1] = (int16_t)(ans - 10);
        cand[2] = (int16_t)(ans + 1);
        cand[3] = (int16_t)(ans - 1);
    }
    b->qright = (uint8_t)rnd(b, 3);
    b->qopt[b->qright] = ans;
    uint8_t filled = 0;
    uint32_t start = rnd(b, 4);
    for (uint32_t k = 0; k < 4 && filled < 2; k++) {
        int16_t c = cand[(start + k) % 4];
        if (c <= 0 || c == (int16_t)ans) continue;
        uint8_t slot = (uint8_t)((b->qright + 1u + filled) % 3u);
        if (filled == 1 && b->qopt[(b->qright + 1u) % 3u] == (uint16_t)c) continue;
        b->qopt[slot] = (uint16_t)c;
        filled++;
    }
}

static uint32_t wiggle_start(rpg_battle_t *b) {
    b->taps = 0;
    b->last_key = NO_KEY;
    go(b, RB_WIGGLE);
    return BE_PHASE;
}

static uint32_t enemy_turn(rpg_battle_t *b) {
    uint32_t ev = 0;
    uint16_t f = b->info->flags;
    if (f & MONF_STAGED) {
        uint8_t stage = b->ehp * 5u <= b->ehp_max ? 3 : b->ehp * 2u <= b->ehp_max ? 2 : 1;
        if (stage > b->stage) {
            b->stage = stage;
            ev |= BE_STAGE;
        }
    }
    if (f & MONF_SKEWER) {  // 剩下几颗糖球 = ⌈5 × 血 / 满血⌉
        uint32_t balls = (b->ehp * (uint32_t)RB_SKEWER_BALLS + b->ehp_max - 1u) / (b->ehp_max ? b->ehp_max : 1u);
        if (balls < 1) balls = 1;
        if (balls < b->stage) {
            b->stage = (uint8_t)balls;
            ev |= BE_STAGE;
        }
    }
    b->hit = 0;
    b->hits = (f & MONF_SKEWER) ? b->stage : b->info->combo;
    if ((f & MONF_QUIZ) && rnd(b, 100) < RB_QUIZ_PCT) {
        make_quiz(b);
        go(b, RB_QUIZ);
        return ev | BE_PHASE | BE_QUIZ;
    }
    if ((f & MONF_KING) && b->stage == 3 && rnd(b, 100) < RB_REFUSE_PCT) {
        b->taps = 0;
        go(b, RB_REFUSE);
        return ev | BE_PHASE | BE_REFUSE;
    }
    if (((f & MONF_CANDY) && b->stage == 3 && rnd(b, 100) < RB_BRUSH_PCT) ||
        ((f & MONF_STICKY) && rnd(b, 100) < RB_STICKY_PCT)) {
        return ev | wiggle_start(b);
    }
    if ((f & MONF_BEAT) && rnd(b, 100) < RB_BEAT_PCT) {
        b->beat_hit = 0;
        b->beat_judged = 0;
        go(b, RB_BEAT);
        return ev | BE_PHASE;
    }
    if (raining(b)) b->hits = RB_RAIN_HITS;
    return ev | windup_start(b);
}

// ---------------------------------------------------------------------------
// 节拍

uint32_t rpg_beat_ms(int k) {
    return RB_BEAT_LEAD_MS + (uint32_t)k * RB_BEAT_GAP_MS;
}

static int beat_next(const rpg_battle_t *b) {  // 还没有结果的第一拍
    for (int k = 0; k < RB_BEAT_N; k++) {
        if (!(b->beat_judged & (1u << k))) return k;
    }
    return -1;
}

// 窗口已经过去的拍子记成错过
static uint32_t beat_expire(rpg_battle_t *b) {
    uint32_t ev = 0;
    int k;
    while ((k = beat_next(b)) >= 0 && b->t > rpg_beat_ms(k) + rule(b)->beat_win) {
        b->beat_judged |= (uint8_t)(1u << k);
        ev |= BE_BEAT_MISS;
    }
    return ev;
}

// 按了 ●:离得最近的、还没结果的那一拍。在窗口里 = 对;太早 = 这一拍错过(乱按不划算)。
static uint32_t beat_press(rpg_battle_t *b) {
    uint32_t ev = beat_expire(b);
    int k = beat_next(b);
    if (k < 0) return ev;
    uint32_t at = rpg_beat_ms(k), win = rule(b)->beat_win;
    b->beat_judged |= (uint8_t)(1u << k);
    if (b->t + win >= at) {  // 晚的已经被 beat_expire 处理过了
        b->beat_hit |= (uint8_t)(1u << k);
        return ev | BE_BEAT_OK;
    }
    return ev | BE_BEAT_MISS;
}

// 节拍结束:错一拍挨半下;全对 = 完美
static uint32_t beat_resolve(rpg_battle_t *b) {
    uint32_t ev = BE_PHASE;
    if (beat_next(b) >= 0) ev |= BE_BEAT_MISS;
    b->beat_judged = (uint8_t)((1u << RB_BEAT_N) - 1u);
    int hits = 0;
    for (int k = 0; k < RB_BEAT_N; k++) hits += (b->beat_hit >> k) & 1u;
    uint32_t miss = (uint32_t)(RB_BEAT_N - hits);
    if (miss == 0) {
        b->last_dmg = 0;
        b->outcome = OUT_DODGE;
        perfect(b);
        ev |= BE_DODGE;
    } else {
        hurt_kid(b, (uint16_t)((miss * enemy_base(b) + 1u) / 2u));
        b->outcome = OUT_HURT;
        ev |= BE_HURT;
    }
    b->hits = 0;
    go(b, RB_HIT);
    return ev;
}

// 来回按结束:够了不掉血、勇气 +1;不够挨两下
static uint32_t wiggle_resolve(rpg_battle_t *b) {
    b->hits = 0;
    if (b->taps >= rule(b)->wiggle_need) {
        b->outcome = OUT_DODGE;
        b->last_dmg = 0;
        sp_add(b, 1);
        go(b, RB_HIT);
        return BE_PHASE | BE_FREE;
    }
    hurt_kid(b, (uint16_t)(enemy_base(b) * 2u));
    b->outcome = OUT_HURT;
    go(b, RB_HIT);
    return BE_PHASE | BE_HURT;
}

// 敌人这一下的结果:dodge/block/hurt。
static uint32_t resolve_hit(rpg_battle_t *b, rpg_outcome_t out) {
    b->outcome = (uint8_t)out;
    b->answered = true;
    uint16_t base = enemy_base(b);
    uint32_t ev = BE_PHASE;
    if (out == OUT_DODGE) {
        b->last_dmg = 0;
        perfect(b);
        ev |= BE_DODGE;
        if (b->counter) {  // 爷爷的绝招:顺手打回去(1 倍攻击,无视防御)
            b->counter_dmg = kid_damage(b, 10, true);
            b->ehp = b->ehp > b->counter_dmg ? (uint16_t)(b->ehp - b->counter_dmg) : 0;
            ev |= BE_COUNTER;
        }
    } else if (out == OUT_BLOCK) {
        hurt_kid(b, (uint16_t)((base + 1u) / 2u));
        ev |= BE_BLOCK;
    } else {
        hurt_kid(b, base);
        ev |= BE_HURT;
    }
    go(b, RB_HIT);
    return ev;
}

static uint32_t end_enemy_turn(rpg_battle_t *b) {
    sp_add(b, 1);
    b->counter = false;
    // 用的是技能列表第 5 行(绝招):主菜单只有 4 行,光标放回「技能」
    if (b->sel >= 4) b->sel = 1;
    go(b, RB_MENU);
    return BE_PHASE;
}

static uint32_t after_act(rpg_battle_t *b) {
    if (b->ehp == 0) {
        go(b, RB_WON);
        return BE_PHASE | BE_WON;
    }
    uint32_t ev = 0;
    bool plus = (b->s->flags & RF_DOG_PLUS) != 0;
    if (rpg_pet_active(b->s) && b->act != ACT_FLEE && rnd(b, 100) < (plus ? RB_PET_PLUS_PCT : RB_PET_PCT)) {
        uint8_t atk = (uint8_t)(rpg_atk(b->s) * (plus ? 8u : 6u) / 10u);
        b->pet_dmg = atk > b->info->def ? (uint16_t)(atk - b->info->def) : 1;
        b->ehp = b->ehp > b->pet_dmg ? (uint16_t)(b->ehp - b->pet_dmg) : 0;
        ev |= BE_PET;
        if (b->ehp == 0) {
            go(b, RB_WON);
            return ev | BE_PHASE | BE_WON;
        }
    }
    return ev | enemy_turn(b);
}

static uint32_t after_hit(rpg_battle_t *b) {
    if (b->s->hp == 0) {
        go(b, RB_LOST);
        return BE_PHASE | BE_LOST;
    }
    if (b->ehp == 0) {  // 选择题答对把考试怪答趴下了;绝招打回去把怪打趴下了
        go(b, RB_WON);
        return BE_PHASE | BE_WON;
    }
    if (b->phase == RB_HIT && ++b->hit < b->hits) return windup_start(b);
    return end_enemy_turn(b);
}

static uint32_t quiz_answer(rpg_battle_t *b, int choice) {
    uint32_t ev = BE_PHASE;
    if (choice == b->qright) {
        uint16_t dmg = (uint16_t)(b->ehp_max / 8u ? b->ehp_max / 8u : 1u);
        hurt_enemy(b, dmg);
        ev |= BE_QUIZ_RIGHT | BE_ENEMY_HURT;
        b->outcome = OUT_DODGE;
    } else {
        uint8_t def = rpg_def(b->s);
        uint32_t atk = enemy_atk(b) * 3u / 2u;
        hurt_kid(b, (uint16_t)(atk > def ? atk - def : 1));
        ev |= BE_QUIZ_WRONG | BE_HURT;
        b->outcome = OUT_HURT;
    }
    b->hits = 0;  // 选择题就这一下
    go(b, RB_HIT);
    return ev;
}

// ---------------------------------------------------------------------------

uint32_t rpg_battle_input(rpg_battle_t *b, rpg_key_t key) {
    switch (b->phase) {
        case RB_MENU:
        case RB_SKILLS:
        case RB_ITEMS:
            return menu_input(b, key);
        case RB_BAR:
            return key == RB_KEY_OK ? resolve_bar(b, true) : 0;
        case RB_MASH:
        case RB_REFUSE:
            if (key != RB_KEY_OK) return 0;
            if (b->taps < 255) b->taps++;
            return BE_TAP;
        case RB_MEMO_INPUT: {
            if (b->taps >= RB_MEMO_LEN) return 0;
            if (key == rpg_lane_key((rpg_lane_t)b->memo[b->taps])) b->memo_ok++;
            b->taps++;
            return BE_TAP;
        }
        case RB_WINDUP:
            if (b->answered) return 0;
            b->answered = true;
            b->pend_key = (uint8_t)key;  // 出手那一刻再判:假动作骗的就是提前按的
            return 0;
        case RB_STRIKE: {
            if (b->answered) return 0;
            bool right = key == rpg_lane_key((rpg_lane_t)b->lane);
            if (!right) return resolve_hit(b, OUT_HURT);
            return resolve_hit(b, rpg_battle_progress(b) >= RB_DODGE_FROM ? OUT_DODGE : OUT_BLOCK);
        }
        case RB_QUIZ: {
            int choice = key == RB_KEY_UP ? 0 : key == RB_KEY_DOWN ? 1 : 2;
            return quiz_answer(b, choice);
        }
        case RB_BEAT:
            return key == RB_KEY_OK ? beat_press(b) : 0;
        case RB_WIGGLE:  // ▲▼ 交替才算,同一个键连按、● 都不算
            if (key == RB_KEY_OK || key == b->last_key) return 0;
            b->last_key = (uint8_t)key;
            if (b->taps < 255) b->taps++;
            return BE_TAP;
        default:
            return 0;
    }
}

static uint32_t phase_ms(const rpg_battle_t *b) {
    switch (b->phase) {
        case RB_BAR:        return RB_BAR_MS;
        case RB_MASH:       return RB_MASH_MS;
        case RB_MEMO_SHOW:  return RB_MEMO_SHOW_MS;
        case RB_MEMO_INPUT: return RB_MEMO_INPUT_MS;
        case RB_ACT:        return RB_ACT_MS;
        case RB_WINDUP:     return b->windup_ms;
        case RB_STRIKE:     return b->strike_ms;
        case RB_HIT:        return b->hits ? RB_HIT_MS : RB_HIT_MS + 420;  // 选择题、拒绝青椒多看一会儿
        case RB_QUIZ:       return RB_QUIZ_MS;
        case RB_REFUSE:     return RB_REFUSE_MS;
        case RB_BEAT:       return RB_BEAT_MS;
        case RB_WIGGLE:     return RB_WIGGLE_MS;
        default:            return 0;  // 等按键或已结束
    }
}

static uint32_t phase_end(rpg_battle_t *b) {
    switch (b->phase) {
        case RB_BAR:        return resolve_bar(b, false);
        case RB_MASH:       return resolve_mash(b);
        case RB_MEMO_SHOW:
            b->taps = 0;
            go(b, RB_MEMO_INPUT);
            return BE_PHASE;
        case RB_MEMO_INPUT: return resolve_beam(b);
        case RB_ACT:        return after_act(b);
        case RB_WINDUP: {
            uint8_t k = b->pend_key;
            go(b, RB_STRIKE);
            if (k == NO_KEY) return BE_PHASE | BE_STRIKE;
            bool right = k == rpg_lane_key((rpg_lane_t)b->lane);
            return BE_STRIKE | resolve_hit(b, right ? OUT_BLOCK : OUT_HURT);
        }
        case RB_STRIKE:     return resolve_hit(b, OUT_HURT);
        case RB_HIT:        return after_hit(b);
        case RB_QUIZ:       return quiz_answer(b, -1);
        case RB_BEAT:       return beat_resolve(b);
        case RB_WIGGLE:     return wiggle_resolve(b);
        case RB_REFUSE:
            if (b->taps >= rule(b)->refuse_need) {
                b->outcome = OUT_DODGE;
                b->last_dmg = 0;
                sp_add(b, 1);
                b->hits = 0;
                go(b, RB_HIT);
                return BE_PHASE | BE_REFUSE_OK;
            } else {
                hurt_kid(b, (uint16_t)(enemy_base(b) * 2u));
                b->outcome = OUT_HURT;
                b->hits = 0;
                go(b, RB_HIT);
                return BE_PHASE | BE_HURT;
            }
        default:            return 0;
    }
}

uint32_t rpg_battle_tick(rpg_battle_t *b, uint32_t dt_ms) {
    uint32_t ev = 0;
    for (int guard = 0; guard < 16; guard++) {
        if (b->phase == RB_MEMO_INPUT && b->taps >= RB_MEMO_LEN) {
            ev |= resolve_beam(b);
            continue;
        }
        uint32_t dur = phase_ms(b);
        if (!dur) {
            b->t += dt_ms;
            return ev;
        }
        if (b->phase == RB_WINDUP && b->feint_at && !b->feinted && b->t + dt_ms >= b->feint_at) {
            uint32_t step = b->feint_at > b->t ? b->feint_at - b->t : 0;
            dt_ms -= step;
            b->t += step;
            b->lane = b->feint_to;
            b->feinted = true;
            ev |= BE_FEINT;
            continue;
        }
        uint32_t left = dur > b->t ? dur - b->t : 0;
        if (dt_ms < left) {
            b->t += dt_ms;
            if (b->phase == RB_BEAT) ev |= beat_expire(b);
            return ev;
        }
        dt_ms -= left;
        b->t = dur;
        ev |= phase_end(b);
        if (rpg_battle_over(b)) return ev;
    }
    return ev;
}
