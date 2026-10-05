// main/rpg_world.c —— 见 rpg_world.h。
#include "rpg_world.h"

#include <stddef.h>
#include <string.h>

#define AREA_PARK RPG_AREAS        // 3
#define AREA_FAIR (RPG_AREAS + 1)  // 4

// rpg_bind 绑定的一对:只有这份存档能看到第 2 章的进度
static rpg_save_t *s_bound;
static rpg_ext_t *s_bound_x;

static rpg_ext_t *ext_of(const rpg_save_t *s) {
    return s && s == s_bound ? s_bound_x : NULL;
}

void rpg_bind(rpg_save_t *s, rpg_ext_t *x) {
    s_bound = s;
    s_bound_x = x;
}

static uint32_t rng_next(uint32_t *r) {
    uint32_t x = *r ? *r : 0x9E3779B9u;  // xorshift32
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *r = x;
    return x;
}

void rpg_new_game(rpg_save_t *s, bool gift) {
    memset(s, 0, sizeof(*s));
    s->version = RPG_SAVE_VERSION;
    s->level = 1;
    s->gold = RPG_START_GOLD;
    s->items[ITEM_DUMPLING] = 1;
    s->areas_open = 1;
    if (gift) {
        s->flags |= RF_GIFT;
        s->items[ITEM_DUMPLING] += 3;
        s->items[ITEM_SODA] += 1;
    }
    s->hp = rpg_hp_max(s);
    s->sp = rpg_sp_max(s);
}

uint16_t rpg_hp_max(const rpg_save_t *s) {
    return rpg_max_hp(s->level);
}

uint8_t rpg_sp_max(const rpg_save_t *s) {
    return rpg_max_sp(s->level);
}

uint8_t rpg_atk(const rpg_save_t *s) {
    return rpg_base_atk(s->level);
}

uint8_t rpg_def(const rpg_save_t *s) {
    return (uint8_t)(rpg_base_def(s->level) + ((s->flags & RF_GIFT) ? RPG_GIFT_DEF : 0));
}

bool rpg_has_skill(const rpg_save_t *s, rpg_skill_t sk) {
    if (sk == SK_MOVE) return (s->flags & RF_MOVE) != 0;  // 爷爷教的,不看等级
    return s->level >= rpg_skill_level(sk);
}

static uint16_t done_in(const rpg_save_t *s, const rpg_ext_t *x, uint8_t area) {
    if (area < RPG_AREAS) return s->done[area];
    return x && area < RPG_AREAS_ALL ? x->done[area - RPG_AREAS] : 0;
}

uint16_t rpg_done_bits(const rpg_save_t *s, uint8_t area) {
    return done_in(s, ext_of(s), area);
}

bool rpg_done(const rpg_save_t *s, uint8_t area, uint8_t node) {
    return (rpg_done_bits(s, area) >> node) & 1u;
}

static void mark_done(rpg_save_t *s) {
    if (s->area < RPG_AREAS) {
        s->done[s->area] |= (uint16_t)(1u << s->node);
        return;
    }
    rpg_ext_t *x = ext_of(s);
    if (x && s->area < RPG_AREAS_ALL) x->done[s->area - RPG_AREAS] |= (uint16_t)(1u << s->node);
}

static bool shown_in(const rpg_ext_t *x, uint8_t area, uint8_t node) {
    uint8_t gate = rpg_area(area)->nodes[node].gate;
    return !gate || (x && (x->story & (1u << (gate - 1u))));
}

bool rpg_node_shown(const rpg_save_t *s, uint8_t area, uint8_t node) {
    return shown_in(ext_of(s), area, node);
}

static bool open_in(const rpg_save_t *s, const rpg_ext_t *x, uint8_t area) {
    if (area < RPG_AREAS) return area < s->areas_open;
    if (!x) return false;
    if (area == AREA_PARK) return (x->story & RX_OPEN_PARK) != 0;
    if (area == AREA_FAIR) return (x->story & RX_OPEN_FAIR) != 0;
    return false;
}

bool rpg_area_open(const rpg_save_t *s, uint8_t area) {
    return open_in(s, ext_of(s), area);
}

static void mon_bits(rpg_save_t *s, rpg_mon_t mon, bool beaten) {
    if (mon < MON_CH1_COUNT) {
        s->seen |= (uint16_t)(1u << mon);
        if (beaten) s->beaten |= (uint16_t)(1u << mon);
        return;
    }
    rpg_ext_t *x = ext_of(s);
    if (!x || mon >= MON_ALL) return;
    x->seen |= (uint16_t)(1u << (mon - MON_CH1_COUNT));
    if (beaten) x->beaten |= (uint16_t)(1u << (mon - MON_CH1_COUNT));
}

bool rpg_seen_mon(const rpg_save_t *s, rpg_mon_t mon) {
    if (mon < MON_CH1_COUNT) return (s->seen >> mon) & 1u;
    const rpg_ext_t *x = ext_of(s);
    return x && mon < MON_ALL && ((x->seen >> (mon - MON_CH1_COUNT)) & 1u);
}

bool rpg_beaten_mon(const rpg_save_t *s, rpg_mon_t mon) {
    if (mon < MON_CH1_COUNT) return (s->beaten >> mon) & 1u;
    const rpg_ext_t *x = ext_of(s);
    return x && mon < MON_ALL && ((x->beaten >> (mon - MON_CH1_COUNT)) & 1u);
}

bool rpg_pet_active(const rpg_save_t *s) {
    return (s->flags & RF_DOG) && !(s->flags & RF_DOG_LOST);
}

uint8_t rpg_level_cap(const rpg_save_t *s) {
    return (s->flags & RF_WON) ? RPG_MAX_LEVEL2 : RPG_MAX_LEVEL;
}

uint16_t rpg_exp_next(const rpg_save_t *s) {
    return s->level < rpg_level_cap(s) ? rpg_exp_need(s->level) : 0;
}

bool rpg_save_valid(const rpg_save_t *s) {
    if (s->version != RPG_SAVE_VERSION) return false;
    if (s->level < 1 || s->level > RPG_MAX_LEVEL) return false;
    if (s->areas_open < 1 || s->areas_open > RPG_AREAS || s->area >= s->areas_open) return false;
    if (s->node >= rpg_area(s->area)->n) return false;
    if (s->hp > rpg_hp_max(s) || s->sp > rpg_sp_max(s)) return false;
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (s->items[i] > RPG_ITEM_HOLD) return false;
    }
    return true;
}

uint8_t rpg_links(const rpg_save_t *s, uint8_t out[RPG_MAX_LINKS]) {
    const rpg_node_t *n = &rpg_area(s->area)->nodes[s->node];
    uint8_t k = 0;
    for (int i = 0; i < RPG_MAX_LINKS && n->links[i] != RPG_NO_NODE; i++) {
        if (rpg_node_shown(s, s->area, n->links[i])) out[k++] = n->links[i];
    }
    return k;
}

void rpg_give(rpg_save_t *s, const rpg_loot_t *loot) {
    if (!loot) return;
    if (loot->item < ITEM_COUNT) {
        unsigned n = s->items[loot->item] + loot->count;
        s->items[loot->item] = (uint8_t)(n > RPG_ITEM_HOLD ? RPG_ITEM_HOLD : n);
    }
    uint32_t g = (uint32_t)s->gold + loot->gold;
    s->gold = (uint16_t)(g > 9999u ? 9999u : g);
}

static const rpg_loot_t *node_loot(const rpg_node_t *n) {
    return n->loot == 0xFF ? NULL : rpg_loot(n->loot);
}

static void heal_full(rpg_save_t *s) {
    s->hp = rpg_hp_max(s);
    s->sp = rpg_sp_max(s);
}

// 这个地区里打 mon 的那个战斗地点打过没有
static bool mon_place_done(const rpg_save_t *s, uint8_t area, rpg_mon_t mon) {
    const rpg_area_t *a = rpg_area(area);
    for (uint8_t i = 0; i < a->n; i++) {
        if (a->nodes[i].kind == NODE_BATTLE && a->nodes[i].arg == mon && rpg_done(s, area, i)) return true;
    }
    return false;
}

// 公园的爷爷:没打大音响 → 求助;打倒了 → 讲往事、教绝招(只一次);以后 → 回满血
static uint8_t grandpa_park(rpg_save_t *s) {
    if (s->flags & RF_MOVE) {
        heal_full(s);
        return SCR_GP_AFTER;
    }
    if (!mon_place_done(s, s->area, MON_SPEAKER)) return SCR_GP_ASK;
    s->flags |= RF_MOVE;
    mark_done(s);
    return SCR_GP_TEACH;
}

static uint8_t boss_intro(uint8_t mon) {
    switch (mon) {
        case MON_DOG:    return SCR_DOG_IN;
        case MON_EXAM:   return SCR_EXAM_IN;
        case MON_SKEWER: return SCR_SKEWER_IN;
        case MON_CANDY:  return SCR_CANDY_IN;
        default:         return SCR_KING_IN;
    }
}

rpg_event_t rpg_arrive(rpg_save_t *s, uint8_t node, uint32_t *rng) {
    rpg_event_t e = { EV_NONE, 0, SCR_COUNT, NULL };
    uint8_t links[RPG_MAX_LINKS];
    uint8_t k = rpg_links(s, links);
    bool adjacent = false;
    for (uint8_t i = 0; i < k; i++) adjacent |= links[i] == node;
    if (!adjacent) return e;

    const rpg_area_t *a = rpg_area(s->area);
    const rpg_node_t *n = &a->nodes[node];
    s->node = node;
    bool done = rpg_done(s, s->area, node);
    switch (n->kind) {
        case NODE_REST:
            e.kind = EV_REST;
            e.script = n->arg;
            s->hp = rpg_hp_max(s);
            s->sp = rpg_sp_max(s);
            break;
        case NODE_BATTLE:
            if (!done) {
                e.kind = EV_BATTLE;
                e.mon = n->arg;
            }
            break;
        case NODE_BOSS:
            if (!done) {
                e.kind = EV_BOSS;
                e.mon = n->arg;
                e.script = boss_intro(n->arg);
            }
            break;
        case NODE_CHEST:
            if (done) {
                e.script = SCR_NOTHING;
                break;
            }
            e.kind = EV_CHEST;
            e.loot = node_loot(n);
            rpg_give(s, e.loot);
            mark_done(s);
            break;
        case NODE_NPC:
            e.kind = EV_NPC;
            if (n->arg == SCR_GP_ASK) {
                e.script = grandpa_park(s);
                break;
            }
            if (n->arg == SCR_GRANDPA_MARKET) {  // 菜市场的爷爷每次都给你补满
                s->hp = rpg_hp_max(s);
                s->sp = rpg_sp_max(s);
            } else if (done) {
                e.script = SCR_NOTHING;
                break;
            }
            e.script = n->arg;
            if (!done) {
                e.loot = node_loot(n);
                rpg_give(s, e.loot);
                mark_done(s);
            }
            break;
        default:
            e.kind = EV_SHOP;
            e.script = n->arg ? n->arg : SCR_SHOP;  // 公园小卖亭是挑食魔王开的
            break;
    }
    // 已经清空的地点(打过的战斗点、开过的宝箱、说过话的人):路上可能冲出小怪
    bool quiet = e.kind == EV_NONE || (e.kind == EV_NPC && e.script == SCR_NOTHING);
    unsigned pct = s->level > a->boss_lv ? RPG_ENCOUNTER_EASY_PCT : RPG_ENCOUNTER_PCT;
    if (quiet && n->kind != NODE_BOSS && rng_next(rng) % 100u < pct) {
        e.kind = EV_ENCOUNTER;
        e.mon = a->mons[rng_next(rng) % a->mon_n];
        e.script = SCR_COUNT;
    }
    return e;
}

void rpg_meet(rpg_save_t *s, rpg_mon_t mon) {
    mon_bits(s, mon, false);
}

uint8_t rpg_gain_exp(rpg_save_t *s, uint16_t exp) {
    uint32_t e = (uint32_t)s->exp + exp;
    s->exp = (uint16_t)(e > 60000u ? 60000u : e);
    uint8_t up = 0, cap = rpg_level_cap(s);
    while (s->level < cap && s->exp >= rpg_exp_need(s->level)) {
        s->level++;
        up++;
    }
    if (up) {
        s->hp = rpg_hp_max(s);
        s->sp = rpg_sp_max(s);
    }
    return up;
}

uint16_t rpg_win_exp(const rpg_save_t *s, rpg_mon_t mon) {
    const rpg_mon_info_t *m = rpg_mon(mon);
    uint8_t entry = m->area ? rpg_area((uint8_t)(m->area - 1))->boss_lv : 0;
    uint32_t exp = s->level < entry ? m->exp * 3u / 2u : m->exp;
    return (uint16_t)(exp * kit_heat_pct(kit_heat_from(s->heat), 100, RPG_HOT_REWARD_PCT) / 100u);
}

uint16_t rpg_win_gold(const rpg_save_t *s, rpg_mon_t mon) {
    return (uint16_t)(rpg_mon(mon)->gold * kit_heat_pct(kit_heat_from(s->heat), 100, RPG_HOT_REWARD_PCT) / 100u);
}

rpg_win_t rpg_win(rpg_save_t *s, rpg_mon_t mon, bool at_node) {
    rpg_win_t w = { 0, 0, -1, false, false, false, NULL };
    uint8_t before = s->level;
    s->battles++;
    mon_bits(s, mon, true);
    rpg_give(s, &(rpg_loot_t){ ITEM_COUNT, 0, rpg_win_gold(s, mon) });
    w.levels = rpg_gain_exp(s, rpg_win_exp(s, mon));
    for (int k = 0; k < SK_COUNT; k++) {
        uint8_t lv = rpg_skill_level((rpg_skill_t)k);
        if (lv > before && lv <= s->level && rpg_has_skill(s, (rpg_skill_t)k)) w.skills |= (uint8_t)(1u << k);
    }
    if (at_node) {
        const rpg_node_t *n = &rpg_area(s->area)->nodes[s->node];
        if (!rpg_done(s, s->area, s->node)) {
            w.loot = node_loot(n);
            rpg_give(s, w.loot);
        }
        mark_done(s);
        // 口香糖池:救回小黄。小黄变强,还闻出了隐藏的糖果仓库
        rpg_ext_t *x = ext_of(s);
        if (mon == MON_GUM && n->kind == NODE_BATTLE && n->arg == MON_GUM && (s->flags & RF_DOG_LOST) && x) {
            s->flags = (uint8_t)((s->flags & ~RF_DOG_LOST) | RF_DOG_PLUS);
            x->story |= RX_SECRET;
            w.dog_free = true;
        }
    }
    if (mon == MON_DOG) s->flags |= RF_DOG;
    if (mon == MON_DOG || mon == MON_EXAM) {
        uint8_t open = mon == MON_DOG ? 2 : 3;
        if (s->areas_open < open) {
            s->areas_open = open;
            w.area = (int8_t)(open - 1);
        }
    }
    if (mon == MON_KING) {
        s->flags |= RF_WON;
        w.won_game = true;
    }
    rpg_ext_t *x = ext_of(s);
    if (mon == MON_SKEWER && x && !(x->story & RX_OPEN_FAIR)) {  // 游乐园开放;一进去小黄就被拖走
        x->story |= RX_OPEN_FAIR;
        w.area = AREA_FAIR;
        if (s->flags & RF_DOG) s->flags |= RF_DOG_LOST;
    }
    if (mon == MON_CANDY) {
        s->flags |= RF_WON2;
        w.won_ch2 = true;
    }
    return w;
}

void rpg_lose(rpg_save_t *s) {
    s->node = rpg_area(s->area)->start;
    s->hp = rpg_hp_max(s);
    s->sp = rpg_sp_max(s);
}

rpg_buy_t rpg_buy(rpg_save_t *s, rpg_item_t item) {
    uint16_t price = rpg_item_price(item);
    if (s->items[item] >= RPG_ITEM_MAX) return BUY_FULL;
    if (s->gold < price) return BUY_POOR;
    s->gold -= price;
    s->items[item]++;
    return BUY_OK;
}

bool rpg_eat(rpg_save_t *s) {
    uint16_t max = rpg_hp_max(s);
    if (!s->items[ITEM_DUMPLING] || s->hp >= max) return false;
    s->items[ITEM_DUMPLING]--;
    s->hp = (uint16_t)(s->hp + RPG_DUMPLING_HEAL > max ? max : s->hp + RPG_DUMPLING_HEAL);
    return true;
}

bool rpg_change_area(rpg_save_t *s, uint8_t area) {
    if (!rpg_area_open(s, area)) return false;
    s->area = area;
    s->node = rpg_area(area)->start;
    return true;
}

// ---------------------------------------------------------------------------
// 第 2 章:开场、ext 的校验、存档拆分与合并

bool rpg_ch2_ready(const rpg_save_t *s) {
    const rpg_ext_t *x = ext_of(s);
    return (s->flags & RF_WON) && x && !(x->story & RX_CH2);
}

void rpg_ch2_begin(rpg_save_t *s, uint32_t *rng) {
    rpg_ext_t *x = ext_of(s);
    if (!x) return;
    x->story |= RX_CH2 | RX_OPEN_PARK;
    if (!s->pad) s->pad = (uint8_t)(1u + rng_next(rng) % 255u);
    x->tag = s->pad;
}

void rpg_ext_new(rpg_ext_t *x) {
    memset(x, 0, sizeof(*x));
    x->version = RPG_EXT_VERSION;
}

static uint32_t fnv1a(const void *p, size_t n) {
    const uint8_t *b = p;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) {
        h ^= b[i];
        h *= 16777619u;
    }
    return h;
}

void rpg_ext_seal(rpg_ext_t *x) {
    x->sum = fnv1a(x, offsetof(rpg_ext_t, sum));
}

bool rpg_ext_valid(const rpg_ext_t *x) {
    if (x->version != RPG_EXT_VERSION || x->sum != fnv1a(x, offsetof(rpg_ext_t, sum))) return false;
    if (x->area && (x->area < RPG_AREAS || x->area >= RPG_AREAS_ALL)) return false;
    return x->hp <= rpg_max_hp(RPG_MAX_LEVEL2) && x->sp <= rpg_max_sp(RPG_MAX_LEVEL2);
}

void rpg_save_split(const rpg_save_t *s, const rpg_ext_t *x, rpg_save_t *main, rpg_ext_t *ext) {
    *main = *s;
    *ext = *x;
    ext->version = RPG_EXT_VERSION;
    ext->tag = s->pad;
    ext->area = 0;
    ext->node = 0;
    ext->hp = s->hp;
    ext->sp = s->sp;
    ext->pad = 0;
    memset(ext->rsv, 0, sizeof(ext->rsv));
    if (s->level > RPG_MAX_LEVEL) {  // 老固件只认到 Lv10
        main->level = RPG_MAX_LEVEL;
        if (main->hp > rpg_max_hp(RPG_MAX_LEVEL)) main->hp = rpg_max_hp(RPG_MAX_LEVEL);
        if (main->sp > rpg_max_sp(RPG_MAX_LEVEL)) main->sp = rpg_max_sp(RPG_MAX_LEVEL);
    }
    if (s->area >= RPG_AREAS) {  // 在第 2 章的地区:主存档里站在小区的家
        ext->area = s->area;
        ext->node = s->node;
        main->area = 0;
        main->node = rpg_area(0)->start;
    }
    rpg_ext_seal(ext);
}

void rpg_save_join(rpg_save_t *s, rpg_ext_t *x, const rpg_ext_t *loaded) {
    rpg_ext_new(x);
    if (!(s->flags & RF_WON)) return;
    // 通关以后上限是 15:第 1 章在 Lv10 攒下的经验按新上限补升级
    while (s->level < RPG_MAX_LEVEL2 && s->exp >= rpg_exp_need(s->level)) s->level++;
    if (!loaded || !s->pad || !rpg_ext_valid(loaded) || loaded->tag != s->pad) return;
    *x = *loaded;
    x->area = 0;
    x->node = 0;
    if (s->level > RPG_MAX_LEVEL) {
        uint16_t hp = rpg_hp_max(s);
        uint8_t sp = rpg_sp_max(s);
        s->hp = loaded->hp < hp ? loaded->hp : hp;
        s->sp = loaded->sp < sp ? loaded->sp : sp;
    }
    // 主存档还站在家里(没被老固件挪过):回到第 2 章那个地点
    uint8_t a = loaded->area, n = loaded->node;
    if (a >= RPG_AREAS && s->area == 0 && s->node == rpg_area(0)->start && open_in(s, x, a) &&
        n < rpg_area(a)->n && shown_in(x, a, n)) {
        s->area = a;
        s->node = n;
    }
}

// 里程碑掩码(Coopa Store):位见 games/rpg/coopa.toml 的 milestones。
uint8_t rpg_milestones_of(const rpg_save_t *s, bool has_save) {
    if (!has_save) return 0;
    return (uint8_t)((s->level >= 5 ? 1u : 0) | (s->flags & RF_WON ? 2u : 0) | (s->level >= 10 ? 4u : 0) |
                     (s->flags & RF_WON2 ? 8u : 0));
}
