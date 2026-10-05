// main/rpg_app.c —— 见 rpg_app.h。剧情、提示、战斗、换地区、结局排成一个步骤队列依次执行。
#include "rpg_app.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_random.h"
#include "kit_sfx.h"
#include "kit_art.h"
#include "kit_app.h"
#include "kit_sprite.h"
#include "kit_store.h"
#include "kit_texts.h"
#include "rpg_battle.h"
#include "rpg_texts.h"
#include "rpg_ui.h"
#include "rpg_world.h"

static const char *TAG = "rpg";

typedef enum {
    ST_TITLE = 0,
    ST_BEST,        // 怪物图鉴
    ST_MAP,
    ST_WALK,        // 在路上(动画)
    ST_STEPS,       // 正在执行步骤队列(对话 / 提示)
    ST_BATTLE,
    ST_BATTLE_END,  // 打完了,等按 ● 看结算
    ST_LIST,        // 菜单 / 商店 / 换地区
    ST_ENDING,
} st_t;

typedef enum { STEP_SCRIPT = 0, STEP_TOAST, STEP_BATTLE, STEP_AREA, STEP_ENDING, STEP_SHOP } step_kind_t;

typedef struct {
    uint8_t kind;
    uint8_t a;      // 剧本 / 怪物 / 地区
    uint8_t b;      // 战斗:是不是这个地点的怪
    char text[72];  // 提示框
} step_t;

typedef enum { LIST_PAUSE = 0, LIST_AREA, LIST_SHOP } list_t;

#define STEPS_MAX 12
#define TOAST_GUARD_MS 350   // 提示框 / 列表刚出来时手可能还按着(连按 ● 推对话)

static const char *const ITEM_NAMES[] = RT_ITEM_NAMES;
static const char *const SKILL_NAMES[] = RT_SKILL_NAMES;
static const char *const SKILL_NOTES[] = RT_SKILL_NOTES;
static const char *const ITEM_NOTES[] = RT_ITEM_NOTES;
static const char *const AREA_NAMES[] = RT_AREA_NAMES;
static const char *const PAUSE_ROWS[] = RT_PAUSE_ROWS;

static rpg_save_t s_save;
static bool s_has_save;
static uint32_t s_granny_best;
static st_t s_st;
static uint32_t s_st_ms;
static uint32_t s_rng;
static uint32_t s_play_ms;

typedef enum { TR_PLAY = 0, TR_BEST, TR_RESTART, TR_HEAT, TR_MENU } title_row_t;

static int s_title_sel;
static bool s_restart_armed;
static const char *s_title_rows[RPG_UI_TITLE_MAX];
static title_row_t s_title_ids[RPG_UI_TITLE_MAX];
static int s_title_n;
static kit_heat_t s_heat;      // 标题页的辣度:有存档时跟着存档(随时改、马上存),没有时「开始冒险」用它
static char s_heat_row[24];
static int s_best_sel;

static int s_target;       // 地图上选中的相邻地点下标;等于相邻个数 = 【菜单】

static step_t s_steps[STEPS_MAX];
static int s_step_n;
static int s_step_i;
static int s_line;

static rpg_battle_t s_battle;
static bool s_battle_at_node;
static uint32_t s_bev;     // 本帧按键产生的战斗事件,交给画面

static list_t s_list;
static int s_list_sel;
static char s_list_info[256];
static char s_list_rows[6][32];
static const char *s_list_ptrs[6];
static uint8_t s_area_ids[RPG_AREAS_ALL];  // 换地区列表:第几行是哪个地区
static int s_list_n;
static const char *s_list_note;
static rpg_ext_t s_ext;    // 第 2 章的进度(和 s_save 绑定,见 rpg_bind)

// ---------------------------------------------------------------------------
// 存档

static bool save_valid(const void *blob) {
    return rpg_save_valid(blob);
}

static bool ext_valid(const void *blob) {
    return rpg_ext_valid(blob);
}

// 直接读进 s_save(和以前一样:读坏了 s_save 可能已被部分覆盖,s_has_save 为 false)。
// 主存档有效再读第 2 章的 rpg/ch2,合并进内存(rpg_save_join:不是这份存档的、坏了的都不认)。
static void save_load(void) {
    s_has_save = kit_blob_load("rpg", "save", &s_save, sizeof(s_save), save_valid, &s_save);
    rpg_ext_new(&s_ext);
    if (!s_has_save) return;
    rpg_ext_t x;
    bool got = kit_blob_load("rpg", "ch2", &x, sizeof(x), ext_valid, &x);
    rpg_save_join(&s_save, &s_ext, got ? &x : NULL);
}

// 主存档只写第 1 章看得懂的样子(老固件回退也能读);开始了第 2 章才写 rpg/ch2(先写它,再写主存档)。
static void save_store(void) {
    s_save.play_s += s_play_ms / 1000u;
    s_play_ms %= 1000u;
    rpg_save_t m;
    rpg_ext_t x;
    rpg_save_split(&s_save, &s_ext, &m, &x);
    if (s_ext.story & RX_CH2) kit_blob_store("rpg", "ch2", &x, sizeof(x));
    kit_blob_store("rpg", "save", &m, sizeof(m));
    s_has_save = true;
}

void rpg_app_erase(void) {
    if (!kit_key_erase("rpg", "save")) return;
    kit_key_erase("rpg", "ch2");
    s_has_save = false;
    memset(&s_save, 0, sizeof(s_save));
    rpg_ext_new(&s_ext);
}

void rpg_app_init(void) {
    rpg_bind(&s_save, &s_ext);
    save_load();
    s_rng = esp_random() | 1u;
    ESP_LOGI(TAG, "存档 %s", s_has_save ? "有" : "无");
}

bool rpg_app_won(void) {
    return s_has_save && (s_save.flags & RF_WON);
}

uint16_t rpg_app_seen(void) {
    return s_has_save ? (uint16_t)(s_save.seen | s_save.beaten) : 0;
}

// 只有在计时的时候算忙;等孩子按键(对话、菜单、结算)时照样可以熄屏,熄屏后第一下只用来唤醒。
bool rpg_app_busy(void) {
    rpg_bphase_t ph = s_battle.phase;
    bool waiting = ph == RB_MENU || ph == RB_SKILLS || ph == RB_ITEMS;
    return s_st == ST_WALK || (s_st == ST_BATTLE && !waiting) || (s_st == ST_BATTLE_END && ph != RB_WON);
}

// ---------------------------------------------------------------------------
// 标题

static void go(st_t st) {
    s_st = st;
    s_st_ms = 0;
}

static void heat_row_fill(void) {
    static const char *const NAMES[KIT_HEATS] = KIT_HEAT_NAMES;
    snprintf(s_heat_row, sizeof(s_heat_row), RT_HEAT_FMT, NAMES[s_heat]);
}

static void title_draw(void) {
    const char *crown = !rpg_app_won() ? NULL : (s_save.flags & RF_WON2) ? RT_CROWN2 : RT_CROWN;
    const char *sub = s_has_save && (s_ext.story & RX_CH2) ? RT_SUBTITLE2 : RT_SUBTITLE;
    rpg_ui_show_title(crown, sub, s_title_rows, s_title_n);
}

static void show_title(void) {
    rpg_ui_hide_overlays();
    if (s_has_save) s_heat = kit_heat_from(s_save.heat);
    heat_row_fill();
    s_title_n = 0;
    s_title_ids[s_title_n] = TR_PLAY;
    s_title_rows[s_title_n++] = s_has_save ? RT_CONTINUE : RT_NEW_GAME;
    s_title_ids[s_title_n] = TR_BEST;
    s_title_rows[s_title_n++] = RT_BESTIARY;
    if (s_has_save) {
        s_title_ids[s_title_n] = TR_RESTART;
        s_title_rows[s_title_n++] = RT_RESTART;
    }
    // 放在「回菜单」上面:「▲ 一下回菜单」「▼ 一下到图鉴」都不变。有存档时这就是第 5 行(上限)。
    s_title_ids[s_title_n] = TR_HEAT;
    s_title_rows[s_title_n++] = s_heat_row;
    s_title_ids[s_title_n] = TR_MENU;
    s_title_rows[s_title_n++] = RT_BACK_MENU;
    s_title_sel = 0;
    s_restart_armed = false;
    title_draw();
    go(ST_TITLE);
}

void rpg_app_enter(uint32_t granny_best) {
    s_granny_best = granny_best;
    show_title();
}

// ---------------------------------------------------------------------------
// 步骤队列

static void steps_clear(void) {
    s_step_n = 0;
    s_step_i = 0;
    s_line = 0;
}

static step_t *step_add(step_kind_t kind, uint8_t a, uint8_t b) {
    if (s_step_n >= STEPS_MAX) return NULL;
    step_t *st = &s_steps[s_step_n++];
    memset(st, 0, sizeof(*st));
    st->kind = (uint8_t)kind;
    st->a = a;
    st->b = b;
    return st;
}

static void toast_add(const char *fmt_text) {
    step_t *st = step_add(STEP_TOAST, 0, 0);
    if (st) snprintf(st->text, sizeof(st->text), "%s", fmt_text);
}

static void show_map(void) {
    rpg_ui_hide_overlays();
    rpg_ui_show_map(&s_save);
    s_target = 0;
    go(ST_MAP);
}

static void open_shop(void);
static void step_run(void);

static void steps_next(void) {
    s_step_i++;
    s_line = 0;
    step_run();
}

static void say_line(void) {
    const rpg_script_info_t *sc = rpg_script((rpg_script_t)s_steps[s_step_i].a);
    const rpg_line_t *l = &sc->lines[s_line];
    rpg_ui_dialog((rpg_speaker_t)l->speaker, l->text);
    if (l->voice) sfx_play_line(l->voice, SFX_NONE, false);
}

static void battle_begin(rpg_mon_t mon, bool at_node) {
    rpg_ui_hide_overlays();
    rpg_battle_start(&s_battle, &s_save, mon, s_rng ^ esp_random());
    s_battle_at_node = at_node;
    s_bev = 0;
    rpg_ui_show_battle(&s_battle);
    sfx_play((rpg_mon(mon)->flags & MONF_BOSS) ? SFX_CHAIN : SFX_START);
    go(ST_BATTLE);
}

// 执行当前步骤;队列走完回到地图。
static void step_run(void) {
    if (s_step_i >= s_step_n) {  // 走完了:剧情和提示都是盖在地图上的,收起浮层就回到地图
        steps_clear();
        rpg_ui_hide_overlays();
        go(ST_MAP);
        return;
    }
    step_t *st = &s_steps[s_step_i];
    switch (st->kind) {
        case STEP_SCRIPT:
            go(ST_STEPS);
            say_line();
            break;
        case STEP_TOAST:
            go(ST_STEPS);
            rpg_ui_dialog(SPK_NONE, NULL);
            rpg_ui_toast(st->text);
            break;
        case STEP_BATTLE:
            battle_begin((rpg_mon_t)st->a, st->b != 0);
            break;
        case STEP_AREA:
            rpg_change_area(&s_save, st->a);
            save_store();
            rpg_ui_show_map(&s_save);
            s_target = 0;
            go(ST_STEPS);
            steps_next();
            break;
        case STEP_SHOP:
            rpg_ui_hide_overlays();
            open_shop();
            break;
        default: {  // STEP_ENDING:结局是最后一步(a = 1:第 2 章的生日会)
            bool ch2 = st->a != 0;
            steps_clear();
            rpg_ui_hide_overlays();
            rpg_ui_show_ending(&s_save, ch2);
            sfx_play(SFX_LEVEL_UP);
            go(ST_ENDING);
            break;
        }
    }
}

static void steps_start(void) {
    s_step_i = 0;
    s_line = 0;
    if (s_step_n == 0) {
        go(ST_MAP);
        return;
    }
    go(ST_STEPS);
    step_run();
}

static void steps_input(int key) {
    if (key != 2) return;
    step_t *st = &s_steps[s_step_i];
    if (st->kind == STEP_SCRIPT) {
        if (rpg_ui_dialog_typing()) {
            rpg_ui_dialog_finish();
            return;
        }
        const rpg_script_info_t *sc = rpg_script((rpg_script_t)st->a);
        if (++s_line < sc->n) {
            say_line();
            return;
        }
        rpg_ui_dialog(SPK_NONE, NULL);
        steps_next();
    } else if (st->kind == STEP_TOAST) {
        if (s_st_ms < TOAST_GUARD_MS) return;
        rpg_ui_toast(NULL);
        steps_next();
    }
}

// ---------------------------------------------------------------------------
// 列表浮层:菜单、换地区、商店

static void list_show(void) {
    const char *title = s_list == LIST_SHOP ? RT_SHOP_TITLE : s_list == LIST_AREA ? RT_AREA_PICK : RT_PAUSE_TITLE;
    rpg_ui_list(title, s_list_info, s_list_ptrs, s_list_n, s_list_sel);
}

static void pause_fill(void) {
    const rpg_save_t *s = &s_save;
    int n;
    if (rpg_exp_next(s)) {
        n = snprintf(s_list_info, sizeof(s_list_info), RT_STAT_FMT, (unsigned)s->level, (unsigned)s->exp,
                     (unsigned)rpg_exp_next(s), (unsigned)s->hp, (unsigned)rpg_hp_max(s),
                     (unsigned)s->sp, (unsigned)rpg_sp_max(s), (unsigned)rpg_atk(s), (unsigned)rpg_def(s),
                     (unsigned)s->gold);
    } else {
        n = snprintf(s_list_info, sizeof(s_list_info), RT_STAT_MAX_FMT, (unsigned)s->level, (unsigned)s->hp,
                     (unsigned)rpg_hp_max(s), (unsigned)s->sp, (unsigned)rpg_sp_max(s), (unsigned)rpg_atk(s),
                     (unsigned)rpg_def(s), (unsigned)s->gold);
    }
    if (n < 0) n = 0;
    n += snprintf(s_list_info + n, sizeof(s_list_info) - (size_t)n, "\n" RT_ITEMS_FMT,
                  (unsigned)s->items[ITEM_DUMPLING], (unsigned)s->items[ITEM_SPICY], (unsigned)s->items[ITEM_SODA]);
    if (s->flags & RF_DOG) {
        n += snprintf(s_list_info + n, sizeof(s_list_info) - (size_t)n, "\n%s",
                      (s->flags & RF_DOG_LOST) ? RT_DOG_LOST : RT_HAS_DOG);
    }
    if (s->flags & RF_GIFT) n += snprintf(s_list_info + n, sizeof(s_list_info) - (size_t)n, "\n" RT_TITLE_DODGER);
    if (s_list_note) snprintf(s_list_info + n, sizeof(s_list_info) - (size_t)n, RT_NOTE_FMT, s_list_note);
    s_list_n = 4;
    for (int i = 0; i < s_list_n; i++) s_list_ptrs[i] = PAUSE_ROWS[i];
}

static void area_fill(void) {
    s_list_info[0] = '\0';
    s_list_n = 0;
    for (uint8_t a = 0; a < RPG_AREAS_ALL; a++) {
        if (!rpg_area_open(&s_save, a)) continue;
        s_area_ids[s_list_n] = a;
        s_list_ptrs[s_list_n++] = AREA_NAMES[a];
    }
    s_list_ptrs[s_list_n++] = RT_CMD_BACK;
}

static void shop_fill(void) {
    int n = snprintf(s_list_info, sizeof(s_list_info), RT_GOLD_FMT, (unsigned)s_save.gold);
    if (s_list_note && n > 0) n += snprintf(s_list_info + n, sizeof(s_list_info) - (size_t)n, "  %s", s_list_note);
    if (s_list_sel < ITEM_COUNT && n > 0) {
        snprintf(s_list_info + n, sizeof(s_list_info) - (size_t)n, RT_SHOP_NOTE_FMT, ITEM_NAMES[s_list_sel],
                 ITEM_NOTES[s_list_sel]);
    }
    s_list_n = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        snprintf(s_list_rows[i], sizeof(s_list_rows[i]), RT_SHOP_ROW_FMT, ITEM_NAMES[i],
                 (unsigned)rpg_item_price((rpg_item_t)i), (unsigned)s_save.items[i]);
        s_list_ptrs[s_list_n++] = s_list_rows[i];
    }
    s_list_ptrs[s_list_n++] = RT_SHOP_LEAVE;
}

static void list_open(list_t which) {
    s_list = which;
    s_list_sel = 0;
    s_list_note = NULL;
    if (which == LIST_PAUSE) pause_fill();
    else if (which == LIST_AREA) area_fill();
    else shop_fill();
    list_show();
    go(ST_LIST);
}

static void open_shop(void) {
    list_open(LIST_SHOP);
}

static void list_close(void) {
    rpg_ui_list(NULL, NULL, NULL, 0, 0);
    // 商店可能是步骤队列的一步:关了就接着走队列
    if (s_step_n && s_step_i < s_step_n && s_steps[s_step_i].kind == STEP_SHOP) {
        go(ST_STEPS);
        steps_next();
        return;
    }
    go(ST_MAP);
}

static void list_input(int key) {
    if (key != 2) {
        s_list_sel = (s_list_sel + (key == 0 ? s_list_n - 1 : 1)) % s_list_n;
        sfx_play(SFX_TAP);
        if (s_list == LIST_SHOP) shop_fill();  // 说明跟着光标换
        list_show();
        return;
    }
    if (s_st_ms < TOAST_GUARD_MS) return;
    if (s_list == LIST_SHOP) {
        if (s_list_sel == ITEM_COUNT) {
            list_close();
            return;
        }
        rpg_buy_t r = rpg_buy(&s_save, (rpg_item_t)s_list_sel);
        s_list_note = r == BUY_OK ? RT_SHOP_BOUGHT : r == BUY_POOR ? RT_SHOP_POOR : RT_SHOP_FULL;
        if (r == BUY_OK) {
            sfx_play(SFX_DODGE);
            save_store();
        }
        shop_fill();
        list_show();
        return;
    }
    if (s_list == LIST_AREA) {
        if (s_list_sel >= s_list_n - 1) {  // 最后一行「返回」
            list_open(LIST_PAUSE);
            return;
        }
        rpg_ui_list(NULL, NULL, NULL, 0, 0);
        rpg_change_area(&s_save, s_area_ids[s_list_sel]);
        save_store();
        show_map();
        return;
    }
    switch (s_list_sel) {
        case 0:  // 吃饺子
            if (rpg_eat(&s_save)) {
                s_list_note = RT_ATE;
                sfx_play(SFX_DODGE);
                save_store();
            } else {
                s_list_note = s_save.items[ITEM_DUMPLING] ? RT_HP_FULL : RT_NO_DUMPLING;
            }
            pause_fill();
            list_show();
            break;
        case 1:
            list_open(LIST_AREA);
            break;
        case 2:  // 回首页
            save_store();
            show_title();
            break;
        default:
            list_close();
            break;
    }
}

// ---------------------------------------------------------------------------
// 地图

static uint8_t links(uint8_t out[RPG_MAX_LINKS]) {
    return rpg_links(&s_save, out);
}

static void arrive_steps(rpg_event_t e) {
    char buf[72];
    steps_clear();
    switch (e.kind) {
        case EV_REST:
            step_add(STEP_SCRIPT, e.script, 0);
            toast_add(RT_RESTED);
            sfx_play(SFX_START);
            break;
        case EV_BATTLE:
            step_add(STEP_BATTLE, e.mon, 1);
            break;
        case EV_ENCOUNTER:
            toast_add(RT_ENCOUNTER);
            step_add(STEP_BATTLE, e.mon, 0);
            sfx_play(SFX_FEINT);
            break;
        case EV_BOSS:
            step_add(STEP_SCRIPT, e.script, 0);
            step_add(STEP_BATTLE, e.mon, 1);
            break;
        case EV_CHEST:
            if (e.loot && e.loot->item < ITEM_COUNT) {
                snprintf(buf, sizeof(buf), RT_CHEST_FMT, ITEM_NAMES[e.loot->item], (unsigned)e.loot->count);
            } else {
                snprintf(buf, sizeof(buf), RT_CHEST_GOLD_FMT, (unsigned)(e.loot ? e.loot->gold : 0));
            }
            toast_add(buf);
            sfx_play(SFX_PERFECT);
            break;
        case EV_NPC:
            step_add(STEP_SCRIPT, e.script, 0);
            if (e.loot && e.loot->item < ITEM_COUNT) {
                snprintf(buf, sizeof(buf), RT_GOT_FMT, ITEM_NAMES[e.loot->item], (unsigned)e.loot->count);
                toast_add(buf);
            }
            break;
        case EV_SHOP:
            step_add(STEP_SCRIPT, e.script, 0);
            step_add(STEP_SHOP, 0, 0);
            break;
        default:
            if (e.script < SCR_COUNT) step_add(STEP_SCRIPT, e.script, 0);
            break;
    }
    // 要打仗的地方先不存:断电重开不能站在 Boss 门口直接走掉;打完由 battle_finish 存
    if (e.kind != EV_BATTLE && e.kind != EV_BOSS && e.kind != EV_ENCOUNTER) save_store();
    steps_start();
}

static void map_input(int key) {
    uint8_t l[RPG_MAX_LINKS];
    int n = links(l);
    if (key == 0 || key == 1) {
        s_target = (s_target + (key == 0 ? n : 1)) % (n + 1);
        sfx_play(SFX_TAP);
        return;
    }
    if (s_target >= n) {  // ≡ 菜单
        list_open(LIST_PAUSE);
        return;
    }
    go(ST_WALK);
}

static void walk_done(void) {
    uint8_t l[RPG_MAX_LINKS];
    int n = links(l);
    uint8_t to = l[s_target < n ? s_target : 0];
    // rpg_arrive 要求从相邻地点出发:动画期间 node 还没变
    rpg_event_t e = rpg_arrive(&s_save, to, &s_rng);
    s_target = 0;
    go(ST_MAP);
    arrive_steps(e);
}

// ---------------------------------------------------------------------------
// 战斗

static void battle_sounds(uint32_t ev) {
    const rpg_battle_t *b = &s_battle;
    if (ev & BE_LOST) sfx_play(SFX_GAME_OVER);
    else if (ev & BE_WON) sfx_play(SFX_LEVEL_UP);
    else if ((ev & BE_STAGE) && (b->info->flags & MONF_KING))
        sfx_play_line(b->stage == 2 ? &LINE_RPG_KING_RAIN : &LINE_RPG_KING_EAT, SFX_CHAIN, false);
    else if (ev & BE_STAGE) sfx_play(SFX_CHAIN);  // 糖果大王换阶段、糖葫芦少了一颗
    else if (ev & BE_QUIZ) sfx_play_line(&LINE_RPG_QUIZ, SFX_NONE, false);
    else if (ev & BE_SLIPPER) sfx_play_line(&LINE_RPG_SLIPPER, SFX_NONE, false);
    else if (ev & (BE_HURT | BE_QUIZ_WRONG)) sfx_play(SFX_HIT);
    else if (ev & (BE_DODGE | BE_REFUSE_OK | BE_QUIZ_RIGHT | BE_FREE | BE_BEAT_OK)) sfx_play(SFX_PERFECT);
    else if ((ev & BE_GRADE) && b->grade == GRADE_PERFECT) sfx_play(SFX_PERFECT);
    else if (ev & (BE_ENEMY_HURT | BE_PET)) sfx_play(SFX_HIT);
    else if (ev & (BE_BLOCK | BE_HEAL | BE_ITEM)) sfx_play(SFX_DODGE);
    else if (ev & BE_FEINT) sfx_play(SFX_FEINT);
    else if (ev & BE_STRIKE) sfx_play(SFX_WHOOSH);
    else if (ev & BE_WINDUP) sfx_play(SFX_WINDUP);
    else if (ev & (BE_TAP | BE_CURSOR)) sfx_play(SFX_TAP);
    else if (ev & (BE_NO_SP | BE_NO_FLEE | BE_FLEE_FAIL | BE_BEAT_MISS)) sfx_play(SFX_FEINT);
    else if (ev & BE_FLED) sfx_play(SFX_START);
}

// 打完了:记账,排好后面的步骤(升级、学技能、东西、Boss 剧情)。
// 战斗总是步骤队列的最后一步(见 arrive_steps),所以这里直接重排队列。
static void battle_finish(void) {
    rpg_bphase_t ph = s_battle.phase;
    steps_clear();
    if (ph == RB_WON) {
        rpg_mon_t mon = s_battle.mon;
        rpg_win_t w = rpg_win(&s_save, mon, s_battle_at_node);
        char buf[72];
        if (w.levels) {
            snprintf(buf, sizeof(buf), RT_LEVEL_UP_FMT, (unsigned)s_save.level, (unsigned)rpg_hp_max(&s_save),
                     (unsigned)rpg_atk(&s_save), (unsigned)rpg_def(&s_save));
            toast_add(buf);
        }
        for (int sk = 0; sk < SK_COUNT; sk++) {  // 连升几级:每个新技能一页
            if (!(w.skills & (1u << sk))) continue;
            int k = snprintf(buf, sizeof(buf), RT_NEW_SKILL_FMT, SKILL_NAMES[sk]);
            if (k > 0 && (size_t)k < sizeof(buf)) snprintf(buf + k, sizeof(buf) - (size_t)k, "\n%s", SKILL_NOTES[sk]);
            toast_add(buf);
        }
        if (w.loot) {
            if (w.loot->item < ITEM_COUNT) {
                snprintf(buf, sizeof(buf), RT_GOT_FMT, ITEM_NAMES[w.loot->item], (unsigned)w.loot->count);
            } else {
                snprintf(buf, sizeof(buf), RT_CHEST_GOLD_FMT, (unsigned)w.loot->gold);
            }
            toast_add(buf);
        }
        if (mon == MON_DOG) step_add(STEP_SCRIPT, SCR_DOG_DOWN, 0);
        if (mon == MON_EXAM) step_add(STEP_SCRIPT, SCR_EXAM_DOWN, 0);
        if (mon == MON_SKEWER) step_add(STEP_SCRIPT, SCR_SKEWER_DOWN, 0);
        if (w.dog_free) step_add(STEP_SCRIPT, SCR_DOG_FREE, 0);  // 地图已经重画:糖果仓库出现了
        if (w.area >= 0) {
            static const uint8_t ARRIVE[RPG_AREAS_ALL] = { SCR_COUNT, SCR_SCHOOL, SCR_MARKET, SCR_PARK, SCR_FAIR };
            step_add(STEP_AREA, (uint8_t)w.area, 0);
            step_add(STEP_SCRIPT, ARRIVE[w.area], 0);
        }
        if (w.won_game) {
            step_add(STEP_SCRIPT, SCR_KING_DOWN, 0);
            step_add(STEP_SCRIPT, SCR_ENDING, 0);
            step_add(STEP_ENDING, 0, 0);
        }
        if (w.won_ch2) {
            step_add(STEP_SCRIPT, SCR_CANDY_DOWN, 0);
            step_add(STEP_SCRIPT, SCR_ENDING2, 0);
            step_add(STEP_ENDING, 1, 0);
        }
    } else if (ph == RB_LOST) {
        rpg_lose(&s_save);
        step_add(STEP_SCRIPT, SCR_LOSE, 0);
    }
    save_store();
    show_map();
    steps_start();
}

static void battle_input(int key) {
    rpg_battle_t *b = &s_battle;
    if ((b->phase == RB_WINDUP || b->phase == RB_STRIKE) && !b->answered) rpg_ui_battle_pose(key);
    uint32_t ev = rpg_battle_input(b, (rpg_key_t)key);
    s_bev |= ev;
    battle_sounds(ev);
}

static void battle_frame(uint32_t dt) {
    rpg_battle_t *b = &s_battle;
    uint32_t ev = rpg_battle_tick(b, dt);
    battle_sounds(ev);
    ev |= s_bev;
    s_bev = 0;
    rpg_ui_battle_frame(b, ev, dt);
    if (ev & BE_WON) {
        char buf[64];
        if (b->info->exp || b->info->gold) {  // 魔王不给经验:别显示「+0」
            snprintf(buf, sizeof(buf), RT_REWARD_FMT "\n" RT_NEXT, (unsigned)rpg_win_exp(&s_save, b->mon),
                     (unsigned)rpg_win_gold(&s_save, b->mon));
        } else {
            snprintf(buf, sizeof(buf), "%s", RT_NEXT);
        }
        rpg_ui_battle_result(RT_WIN, buf);
        go(ST_BATTLE_END);
    } else if (ev & (BE_LOST | BE_FLED)) {
        go(ST_BATTLE_END);
    }
}

void rpg_app_debug_fight(int mon) {
    if (s_st != ST_MAP || mon < 0 || mon >= MON_ALL) return;
    s_save.level = RPG_MAX_LEVEL;
    s_save.hp = rpg_hp_max(&s_save);
    s_save.sp = rpg_sp_max(&s_save);
    s_save.flags |= RF_DOG;
    for (int i = 0; i < ITEM_COUNT; i++) s_save.items[i] = 3;
    steps_clear();
    battle_begin((rpg_mon_t)mon, false);
}

// 步骤队列里还有没打的仗(到了要打仗的地方,剧情还没演完)。
static bool battle_pending(void) {
    for (int i = s_step_i; i < s_step_n; i++) {
        if (s_steps[i].kind == STEP_BATTLE) return true;
    }
    return false;
}

void rpg_app_save_now(void) {
    if (!s_has_save) return;
    if (s_st == ST_BATTLE_END) {
        battle_finish();  // 赢了的经验、东西,输了的处罚,都和按 ● 一样记下来
        return;
    }
    // 要打仗的地方先不存(见 arrive_steps):断电重开不能站在怪面前直接走掉
    if (s_st == ST_BATTLE || battle_pending()) return;
    if (s_st == ST_TITLE || s_st == ST_BEST || s_st == ST_ENDING) return;  // 存档没动过
    save_store();
}

// ---------------------------------------------------------------------------

static void new_game(void) {
    bool gift = s_granny_best >= RPG_GIFT_BEST;
    rpg_new_game(&s_save, gift);
    rpg_ext_new(&s_ext);
    kit_key_erase("rpg", "ch2");  // 上一份存档的第 2 章进度不要了
    s_save.heat = (uint8_t)s_heat;  // 重新开始也沿用标题页上的辣度
    s_play_ms = 0;
    save_store();
    show_map();
    steps_clear();
    step_add(STEP_SCRIPT, SCR_OPENING, 0);
    if (gift) step_add(STEP_SCRIPT, SCR_GIFT, 0);
    steps_start();
}

// 打通了第 1 章,第一次「继续冒险」:回家听奶奶和爷爷说蛋糕被抢了,然后去公园
static void ch2_start(void) {
    rpg_ch2_begin(&s_save, &s_rng);
    show_map();
    steps_clear();
    step_add(STEP_AREA, 0, 0);
    step_add(STEP_SCRIPT, SCR_CH2_OPEN, 0);
    step_add(STEP_AREA, RPG_AREAS, 0);
    step_add(STEP_SCRIPT, SCR_PARK, 0);
    steps_start();
}

static bool title_input(int key) {
    if (key != 2) {
        s_title_sel = (s_title_sel + (key == 0 ? s_title_n - 1 : 1)) % s_title_n;
        if (s_restart_armed) {
            s_restart_armed = false;
            for (int i = 0; i < s_title_n; i++) {
                if (s_title_ids[i] == TR_RESTART) s_title_rows[i] = RT_RESTART;
            }
            title_draw();
        }
        sfx_play(SFX_TAP);
        return false;
    }
    title_row_t row = s_title_ids[s_title_sel];
    if (row == TR_MENU) return true;
    if (row == TR_HEAT) {  // ● 循环 微 → 中 → 特;有存档马上存,下一仗就按新的辣度打
        s_heat = kit_heat_next(s_heat);
        heat_row_fill();
        if (s_has_save) {
            s_save.heat = (uint8_t)s_heat;
            save_store();
        }
        title_draw();
        sfx_play(SFX_TAP);
        return false;
    }
    if (row == TR_BEST) {
        s_best_sel = 0;
        rpg_ui_show_bestiary(&s_save);
        go(ST_BEST);
    } else if (row == TR_RESTART) {
        if (!s_restart_armed) {  // 再按一次才重来
            s_restart_armed = true;
            s_title_rows[s_title_sel] = RT_RESTART_SURE;
            title_draw();
            return false;
        }
        new_game();
    } else if (s_has_save && rpg_ch2_ready(&s_save)) {
        ch2_start();
    } else if (s_has_save) {
        show_map();
    } else {
        new_game();
    }
    return false;
}

bool rpg_app_input(int key) {
    switch (s_st) {
        case ST_TITLE:
            return title_input(key);
        case ST_BEST:
            if (key == 2) show_title();
            else s_best_sel = (s_best_sel + (key == 0 ? RPG_UI_BEST_N - 1 : 1)) % RPG_UI_BEST_N;
            break;
        case ST_MAP:
            map_input(key);
            break;
        case ST_STEPS:
            steps_input(key);
            break;
        case ST_BATTLE:
            battle_input(key);
            break;
        case ST_BATTLE_END:
            if (key == 2 && s_st_ms > 600) battle_finish();
            break;
        case ST_LIST:
            list_input(key);
            break;
        case ST_ENDING:
            if (key == 2 && s_st_ms > 1500) show_title();
            break;
        default:
            break;
    }
    return false;
}

void rpg_app_frame(uint32_t dt_ms) {
    s_st_ms += dt_ms;
    if (s_st != ST_TITLE && s_st != ST_BEST && s_st != ST_ENDING) s_play_ms += dt_ms;
    switch (s_st) {
        case ST_TITLE:
            rpg_ui_title_frame(dt_ms, s_title_sel);
            break;
        case ST_BEST:
            rpg_ui_bestiary_frame(dt_ms, s_best_sel);
            break;
        case ST_MAP:
        case ST_LIST: {
            uint8_t l[RPG_MAX_LINKS];
            int n = links(l);
            int target = s_st == ST_LIST ? RPG_UI_TARGET_NONE : s_target >= n ? RPG_UI_TARGET_MENU : l[s_target];
            rpg_ui_map_frame(&s_save, dt_ms, target, -1, 0);
            break;
        }
        case ST_WALK: {
            uint8_t l[RPG_MAX_LINKS];
            int n = links(l);
            uint8_t to = l[s_target < n ? s_target : 0];
            // 画面上先把库巴摆到目的地方向:借 node 画,结束时再真正到达
            uint8_t from = s_save.node;
            s_save.node = to;
            rpg_ui_map_frame(&s_save, dt_ms, RPG_UI_TARGET_NONE, from, s_st_ms ? s_st_ms : 1);
            s_save.node = from;
            if (s_st_ms >= RPG_UI_WALK_MS) walk_done();
            break;
        }
        case ST_STEPS:  // 对话、提示、商店都盖在地图上
            rpg_ui_map_frame(&s_save, dt_ms, RPG_UI_TARGET_NONE, -1, 0);
            break;
        case ST_BATTLE:
            battle_frame(dt_ms);
            break;
        case ST_BATTLE_END:
            rpg_ui_battle_frame(&s_battle, 0, dt_ms);
            // 输了或跑了不用结算条,停一会儿自动回地图
            if (s_battle.phase != RB_WON && s_st_ms > 1500) battle_finish();
            break;
        case ST_ENDING:
            rpg_ui_ending_frame(dt_ms);
            break;
    }
    rpg_ui_overlay_frame(dt_ms);
}

size_t rpg_app_status(char *buf, size_t len) {
    const rpg_battle_t *b = &s_battle;
    uint8_t l[RPG_MAX_LINKS];
    int n = (s_has_save || s_st != ST_TITLE) ? links(l) : 0;
    int target = s_target < n ? l[s_target] : -1;
    int r = snprintf(buf, len, "RP %d %d %d %d %lu %u %u %u %u %d %d %u %u %u %u %d %d %u %u\n", (int)s_st,
                     (int)b->phase, (int)(s_st == ST_BATTLE ? b->sel : s_st == ST_LIST ? s_list_sel : s_title_sel),
                     (int)b->lane, (unsigned long)rpg_battle_progress(b), (unsigned)s_save.hp, (unsigned)b->ehp,
                     (unsigned)s_save.area, (unsigned)s_save.node, target, (int)b->qright,
                     (unsigned)b->memo[0], (unsigned)b->memo[1], (unsigned)b->memo[2], (unsigned)b->memo[3],
                     (int)rpg_ui_dialog_typing(), (int)b->answered, (unsigned)s_save.level,
                     (unsigned)b->strike_ms);
    return r > 0 && (size_t)r < len ? (size_t)r : 0;
}

// ---------------------------------------------------------------------------
// 壳的描述表(kit_app.h)

static void kit_enter(const kit_links_t *links) {
    rpg_app_enter(links->granny_best);
}

static bool kit_input(const kit_input_t *in) {
    if (in->long_press) return false;  // 只用 PRESS
    return rpg_app_input(in->key);
}

static unsigned kit_busy(void) {
    return rpg_app_busy() ? KIT_BUSY_TIMING : 0;
}

static void kit_publish(kit_links_t *links) {
    links->rpg_won = rpg_app_won();
    links->rpg_seen = rpg_app_seen();
}

// R 删存档(正在玩就重进标题页);e..r = RM00..RM13 直接打某种怪(只在大冒险里)。都只给测试用。
static kit_admin_t kit_admin(int cmd, bool active) {
    if (cmd >= 'e' && cmd < 'e' + 14) {
        if (active) rpg_app_debug_fight(cmd - 'e');
        return KIT_ADMIN_DONE;
    }
    if (cmd == 'R') {
        rpg_app_erase();
        if (active) kit_go(&rpg_app);
        return KIT_ADMIN_DONE;
    }
    return KIT_ADMIN_NOT_MINE;
}

static const kit_menu_card_t MENU_ENTRY = { RT_MENU_RPG, RT_MENU_RPG_SUB, { SPR_KID_WAVE, SPR_MON_KING } };

// Coopa Store 的里程碑(位见 games/rpg/coopa.toml)。不取随机数:外壳随时会调。
static uint8_t rpg_ms(void) {
    return rpg_milestones_of(&s_save, s_has_save);
}

const kit_app_t rpg_app = {
    .id = "rpg",
    // 会用到的全部 39 张:第 1 章 30 张(sim 实测峰值 17696)+ 第 2 章 8 只怪和生日蛋糕(各 16×16)。卡上 37464 B
    .sprite_budget = KIT_SPR_BUDGET(26688 + 9 * 16 * 16 * 4, 39),
    .init = rpg_app_init,
    .enter = kit_enter,
    .input = kit_input,
    .frame = rpg_app_frame,
    .leave = rpg_ui_release,  // 浮层在 lv_layer_top 上,不删会一直占着 LVGL 池
    .busy = kit_busy,
    .won = rpg_app_won,
    .milestones = rpg_ms,
    .publish = kit_publish,
    .status = rpg_app_status,
    .save_now = rpg_app_save_now,
    .admin = kit_admin,
    .card = &MENU_ENTRY,
};
