// main/rpg_ui.c —— 见 rpg_ui.h。全部动画由调用方每帧驱动,不用 lv_anim。
#include "rpg_ui.h"

#include <stdio.h>
#include <string.h>

#include "kit_overlay.h"
#include "kit_sprite.h"
#include "kit_text.h"
#include "rpg_texts.h"
#include "ui_kit.h"

#define LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const char *const AREA_NAMES[] = RT_AREA_NAMES;
static const char *const MON_NAMES[] = RT_MON_NAMES;
static const char *const MON_NOTES[] = RT_MON_NOTES;
static const char *const SPEAKERS[] = RT_SPEAKERS;
static const char *const KIND_NAMES[] = RT_KIND_NAMES;
static const char *const ITEM_NAMES[] = RT_ITEM_NAMES;
static const char *const SKILL_NAMES[] = RT_SKILL_NAMES;
static const char *const CMD_NAMES[] = RT_CMD_NAMES;
static const char *const LANE_HINTS[] = RT_LANE_HINTS;
static const char *const KEY_NAMES[] = RT_KEY_NAMES;
_Static_assert(LEN(AREA_NAMES) == RPG_AREAS_ALL, "area names");
_Static_assert(LEN(MON_NAMES) == MON_ALL && LEN(MON_NOTES) == MON_ALL, "monster texts");
_Static_assert(LEN(SPEAKERS) == SPK_COUNT, "speaker names");
_Static_assert(LEN(KIND_NAMES) == NODE_BOSS + 1, "node kind names");
_Static_assert(LEN(ITEM_NAMES) == ITEM_COUNT && LEN(SKILL_NAMES) == SK_COUNT, "item / skill names");
_Static_assert(LEN(LANE_HINTS) == RPG_LANES && LEN(KEY_NAMES) == 3, "lane hints / key names");
_Static_assert(SPR_MON_KING - SPR_MON_CAT == MON_KING - MON_CAT, "monster sprites follow rpg_mon_t");

// ---------------------------------------------------------------------------
// 第 2 章的像素画(字符画 + kit_art 的调色板,用 spr_decode 解码进本 app 的精灵会话)

// 气球怪:红气球,白高光,瞪眼咧嘴,下面一根线
static const char *const ART_BALLOON[] = {
    "......kkkk......",
    "....kkrrrrkk....",
    "...krrrrrrwwk...",
    "..krrrrrrrrwwk..",
    "..krrkkrrkkrwk..",
    ".krrrkwrrkwrrrk.",
    ".krrrkkrrkkrrrk.",
    ".krrrrrrrrrrrrk.",
    ".krrkrrrrrrkrrk.",
    "..krrkkkkkkrrk..",
    "..krrrrrrrrrrk..",
    "...krrrrrrrrk...",
    "....kkrrrrkk....",
    "......kRRk......",
    ".......kk.......",
    "........k.......",
};
// 鲤鱼精:橙色鲤鱼跳出水面,下面水花
static const char *const ART_CARP[] = {
    "................",
    ".....kkk........",
    "....kyyyk.......",
    "..kkkoookkkk....",
    ".kwwkooooooookk.",
    ".kwkkoooyoooook.",
    "kooooooyyooookok",
    "krooooyyooooookk",
    "kooooooyooooooko",
    ".kfkoooooooook.k",
    "..kkooooooookok.",
    "...kkkkkkkkk.kk.",
    "................",
    "..B.....B....B..",
    ".BwB...BwB..BwB.",
    "BBBBB.BBBBBBBBBB",
};
// 大音响:黑音箱,怒眉,大喇叭,旁边飘着音符
static const char *const ART_SPEAKER[] = {
    "...........kk...",
    "..kkkkkkkk.kyk..",
    ".kddddddddkkyk..",
    ".kdkkddkkdk.k...",
    ".kdwkddkwdk.....",
    ".kdwwddwwdk..kk.",
    ".kddrrrrddk.kyk.",
    ".kkhhhhhhkk.kyk.",
    ".kkhgggghkk..k..",
    ".kkhgKKghkk.....",
    ".kkhgKKghkk.....",
    ".kkhgggghkk.....",
    ".kkhhhhhhkk.....",
    ".kddddddddk.....",
    ".kkkkkkkkkk.....",
    "..kk....kk......",
};
// 糖葫芦侠:竹签串五颗红山楂,最上面那颗戴草帽
static const char *const ART_SKEWER[] = {
    "....kkkkkk......",
    "...kyyyyyyk.....",
    "..kkkkkkkkkk....",
    "...krrrrrrk.....",
    "..krwkrrkwrk....",
    "..krkkrrkkrk....",
    "..krrrkkrrrk....",
    "...krrrrrrk.....",
    "....krrrrk......",
    "...krrrrrrk.....",
    "...krrwrrrk.....",
    "....krrrrk......",
    "...krrrrrrk.....",
    "...krrwrrrk.....",
    "....krrrrk......",
    "......bb........",
};
// 跳跳糖:蹦起来的粉色星星糖,四周炸开的小糖粒
static const char *const ART_POP[] = {
    ".y.....k......p.",
    "......kpk.......",
    "..B..kpppk...y..",
    ".kkkkkpwppkkkkk.",
    ".kpppppppppppk..",
    "..kppwkppkwppk..",
    "...kpkkppkkpk...",
    "...kpppppppk..B.",
    "..kpppkkkpppk...",
    "..kppk...kppk...",
    ".kpk.......kpk..",
    ".kk.........kk..",
    "....y...........",
    "...............p",
    "..B......y......",
    "................",
};
// 口香糖:粉红的一团,吹着泡泡
static const char *const ART_GUM[] = {
    "................",
    "..........kkk...",
    ".........kllllk.",
    "........kllwwllk",
    "........klwllllk",
    "...kkkk.kllllllk",
    "..kllllk.kllllk.",
    ".kllkllkkkkkk...",
    ".klwkllllkwlk...",
    ".klkkllllkklk...",
    "kllllllllllllk..",
    "kllllPPPPllllk..",
    "klllllllllllllk.",
    ".kllPlllllPllk..",
    "..kkllkkkllkk...",
    "....kk...kk.....",
};
// 冰淇淋:脆皮筒上一个奶油球一个草莓球,冒着冷汗
static const char *const ART_ICE[] = {
    ".....kkkkk......",
    "....kpppppk.....",
    "...kpwpppppk....",
    "...kpppppppk....",
    "..kkkkkkkkkkk...",
    ".kcccccccccccck.",
    ".kckkcccccckkck.",
    ".kccccccccccck..",
    ".kcccckkkkccck..",
    "..kkkkkkkkkkk...",
    "...knbnbnbnk.hB.",
    "...kbnbnbnbk..B.",
    "....knbnbnk.....",
    "....kbnbnbk.....",
    ".....knbnk......",
    "......kk........",
};
// 糖果大王:紫色糖纸包的大糖果,两头拧着,戴金王冠
static const char *const ART_CANDY[] = {
    "....y..y..y.....",
    "....kykykyk.....",
    "....kyyyyyk.....",
    "..kkkkkkkkkkk...",
    "kkkvvvvvvvvvkkkk",
    "kVkvwwvvvwwvkVVk",
    "kVVkkwvvvkwvkVk.",
    "kVkvvvvvvvvvkVVk",
    "kkkvvkkkkkvvkkkk",
    "..kvvkwkwkvvk...",
    "..kvvvvvvvvvk...",
    "..kkkkkkkkkkk...",
    "...kpk...kpk....",
    "...kpk...kpk....",
    "..kkkk...kkkk...",
    "................",
};
// 生日蛋糕(结局 2):奶油蛋糕插三根蜡烛
static const char *const ART_CAKE[] = {
    "................",
    "...y...y...y....",
    "..yoy.yoy.yoy...",
    "...B...p...G....",
    "...B...p...G....",
    ".kkkkkkkkkkkkkk.",
    ".kccccccccccccck",
    ".kcpcccpcccpccck",
    ".kppppppppppppk.",
    ".kccccccccccccck",
    ".kcccrccccrccck.",
    ".kccccccccccccck",
    ".kppppppppppppk.",
    "kkkkkkkkkkkkkkkk",
    "khhhhhhhhhhhhhhk",
    ".kkkkkkkkkkkkkk.",
};

#define ART16(a) { 16, 16, a }
static const spr_art_t CH2_ART[MON_ALL - MON_CH1_COUNT] = {
    ART16(ART_BALLOON), ART16(ART_CARP), ART16(ART_SPEAKER), ART16(ART_SKEWER),
    ART16(ART_POP),     ART16(ART_GUM),  ART16(ART_ICE),     ART16(ART_CANDY),
};
static const spr_art_t CAKE_ART = ART16(ART_CAKE);
_Static_assert(MON_CANDY - MON_CH1_COUNT + 1 == LEN(CH2_ART), "chapter 2 monster art follows rpg_mon_t");

static const lv_image_dsc_t *mon_img(int m) {
    if (m < MON_CH1_COUNT) return uk_img((spr_id_t)(SPR_MON_CAT + m));
    return kit_img(&CH2_ART[m - MON_CH1_COUNT], spr_decode);
}

// 和 uk_sprite / uk_sprite_set 一样,只是图从参数来(第 2 章的图不在 spr_id_t 里)
static lv_obj_t *img_new(lv_obj_t *parent, const lv_image_dsc_t *d, int scale) {
    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, d);
    lv_obj_set_size(img, d->header.w, d->header.h);
    lv_image_set_scale(img, (uint32_t)(256 * scale));
    lv_image_set_antialias(img, false);
    return img;
}

static void img_set(lv_obj_t *img, const lv_image_dsc_t *d) {
    if (lv_image_get_src(img) == d) return;
    lv_image_set_src(img, d);
    lv_obj_set_size(img, d->header.w, d->header.h);
}

static spr_id_t speaker_spr(rpg_speaker_t s) {
    switch (s) {
        case SPK_KUBA:    return SPR_KID;
        case SPK_GRANNY:  return SPR_GRANNY;
        case SPK_GRANDPA: return SPR_GRANDPA;
        case SPK_DOG:
        case SPK_PUP:     return SPR_MON_DOG;
        case SPK_FATTY:   return SPR_NPC_FATTY;
        case SPK_AUNT:    return SPR_NPC_AUNT;
        case SPK_EXAM:    return SPR_MON_EXAM;
        case SPK_KING:    return SPR_MON_KING;
        default:          return SPR_COUNT;
    }
}

static spr_id_t node_icon(uint8_t kind) {
    switch (kind) {
        case NODE_REST:   return SPR_ICON_REST;
        case NODE_BATTLE: return SPR_ICON_FIGHT;
        case NODE_CHEST:  return SPR_ICON_CHEST;
        case NODE_NPC:    return SPR_ICON_TALK;
        case NODE_SHOP:   return SPR_ICON_SHOP;
        default:          return SPR_ICON_BOSS;
    }
}

static const lv_image_dsc_t *speaker_img(rpg_speaker_t s) {
    if (s == SPK_SKEWER) return mon_img(MON_SKEWER);
    if (s == SPK_CANDY) return mon_img(MON_CANDY);
    spr_id_t id = speaker_spr(s);
    return id != SPR_COUNT ? uk_img(id) : NULL;
}

// 地图和战斗画面都很大:先换上空屏、删掉旧屏,再建新屏。不这样的话打完仗回地图那一刻战斗画面、新地图、
// 对话浮层同时在 LVGL 池里(第 2 章实测峰值 39244 / 39612)。中间不出帧(一直持有 LVGL 锁),画面一样。
static void screen_drop(void) {
    uk_screen_swap(uk_screen_new(UK_WALL));
}

static void bar_set(lv_obj_t *fill, int full_w, uint32_t v, uint32_t max) {
    int w = max ? (int)(v * (uint32_t)full_w / max) : 0;
    if (lv_obj_get_width(fill) != w) lv_obj_set_width(fill, w);
}

// ---------------------------------------------------------------------------
// 标题页

#define TROWS_MAX RPG_UI_TITLE_MAX

static struct {
    lv_obj_t *rows[TROWS_MAX];
    lv_obj_t *kid;
    lv_obj_t *cat;
    lv_obj_t *king;
    int n;
    int shown;
    uint32_t t;
} T;

void rpg_ui_show_title(const char *crown_text, const char *sub_text, const char *const *rows, int n) {
    memset(&T, 0, sizeof(T));
    T.n = n > TROWS_MAX ? TROWS_MAX : n;
    T.shown = -1;
    lv_obj_t *scr = uk_screen_new(UK_WALL);
    uk_rect(scr, 0, 176, UK_W, UK_H - 176, UK_WAINSCOT);

    lv_obj_t *title = uk_text(scr, UK_F36, UK_RED, RT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_t *sub = uk_text(scr, UK_F12, UK_INK, sub_text);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 66);
    if (crown_text) {
        lv_obj_t *crown = uk_pill(scr, UK_F12, UK_GOLD, UK_WHITE, crown_text);
        lv_obj_align(crown, LV_ALIGN_TOP_MID, 0, 84);
    }
    T.cat = uk_sprite(scr, SPR_MON_CAT, 3);
    T.kid = uk_sprite(scr, SPR_KID_WAVE, 4);
    T.king = uk_sprite(scr, SPR_MON_KING, 3);

    // 4 行:从 186 起每行 30(药丸正好 30 高)。5 行放不下,往上提一点、行距压到 27(最后一行底边 317)。
    int y0 = T.n > 4 ? 179 : 186, dy = T.n > 4 ? 27 : 30;
    for (int i = 0; i < T.n; i++) {
        T.rows[i] = uk_pill(scr, UK_F24, UK_WAINSCOT, UK_INK, rows[i]);
        lv_obj_align(T.rows[i], LV_ALIGN_TOP_MID, 0, y0 + i * dy);
    }
    uk_screen_swap(scr);
    rpg_ui_title_frame(0, 0);
}

void rpg_ui_title_frame(uint32_t dt_ms, int sel) {
    T.t += dt_ms;
    if (sel != T.shown) {
        T.shown = sel;
        for (int i = 0; i < T.n; i++) {
            bool on = i == sel;
            lv_obj_set_style_bg_color(T.rows[i], lv_color_hex(on ? UK_RED : UK_WAINSCOT), 0);
            lv_obj_set_style_text_color(T.rows[i], lv_color_hex(on ? UK_WHITE : UK_INK), 0);
        }
    }
    uk_spos(T.cat, 18, 112 - uk_tri(T.t, 500, 4));
    uk_spos(T.kid, 88, 104 - uk_tri(T.t + 250, 700, 6));
    uk_spos(T.king, 174, 112 - uk_tri(T.t + 120, 600, 4));
}

// ---------------------------------------------------------------------------
// 地图

#define MAP_Y   52   // 地图区域的顶;y 坐标再压缩到 9/10,底下留给信息条
#define MAX_LINES 16

static struct {
    lv_obj_t *icons[RPG_MAX_NODES];
    lv_obj_t *checks[RPG_MAX_NODES];
    lv_obj_t *kid;
    lv_obj_t *dog;
    lv_obj_t *ring;
    lv_obj_t *info;
    lv_obj_t *gold;
    lv_obj_t *lv;
    lv_obj_t *hp_fill;
    lv_obj_t *hp_text;
    lv_obj_t *sp;
    lv_point_precise_t pts[MAX_LINES][2];
    uint8_t area;
    uint8_t n;
    int32_t done_shown;
    int32_t hud_key;
    uint32_t t;
    char buf[64];
} M;

#define HP_BAR_W 56   // 满级时「102/102」和「勇气10」都要放得下

static void node_xy(uint8_t area, uint8_t node, int *x, int *y) {
    const rpg_node_t *n = &rpg_area(area)->nodes[node];
    *x = n->x;
    *y = MAP_Y + n->y * 9 / 10;
}

void rpg_ui_show_map(const rpg_save_t *s) {
    screen_drop();
    memset(&M, 0, sizeof(M));
    M.area = s->area;
    const rpg_area_t *a = rpg_area(s->area);
    M.n = a->n;
    M.done_shown = -1;
    M.hud_key = -1;
    lv_obj_t *scr = uk_screen_new(UK_WALL);

    // HUD
    lv_obj_t *area = uk_text(scr, UK_F12, UK_RED, AREA_NAMES[s->area]);
    lv_obj_set_pos(area, 26, 4);
    M.gold = uk_text(scr, UK_F12, UK_ORANGE, "");
    lv_obj_align(M.gold, LV_ALIGN_TOP_RIGHT, -26, 4);
    uk_battery(scr, 150, 6, 1);  // 地区名和零花钱中间
    M.lv = uk_text(scr, UK_F12, UK_GREEN, "");
    lv_obj_set_pos(M.lv, 26, 20);
    lv_obj_t *bar = uk_rect(scr, 58, 24, HP_BAR_W, 7, UK_LINE);
    lv_obj_set_style_radius(bar, 2, 0);
    M.hp_fill = uk_rect(bar, 0, 0, HP_BAR_W, 7, UK_RED);
    lv_obj_set_style_radius(M.hp_fill, 2, 0);
    M.hp_text = uk_text(scr, UK_F12, UK_INK, "");
    lv_obj_set_pos(M.hp_text, 120, 20);
    M.sp = uk_text(scr, UK_F12, UK_BLUE, "");
    lv_obj_align(M.sp, LV_ALIGN_TOP_RIGHT, -22, 20);

    // 路(隐藏地点还没出现:连它的路也不画)
    int nl = 0;
    for (uint8_t i = 0; i < a->n; i++) {
        if (!rpg_node_shown(s, s->area, i)) continue;
        for (int k = 0; k < RPG_MAX_LINKS && a->nodes[i].links[k] != RPG_NO_NODE; k++) {
            uint8_t j = a->nodes[i].links[k];
            if (j < i || nl >= MAX_LINES || !rpg_node_shown(s, s->area, j)) continue;
            int x0, y0, x1, y1;
            node_xy(s->area, i, &x0, &y0);
            node_xy(s->area, j, &x1, &y1);
            M.pts[nl][0] = (lv_point_precise_t){ x0, y0 };
            M.pts[nl][1] = (lv_point_precise_t){ x1, y1 };
            lv_obj_t *line = lv_line_create(scr);
            lv_line_set_points(line, M.pts[nl], 2);
            lv_obj_set_style_line_width(line, 4, 0);
            lv_obj_set_style_line_color(line, lv_color_hex(UK_LINE), 0);
            lv_obj_set_style_line_rounded(line, true, 0);
            nl++;
        }
    }

    // 地点
    for (uint8_t i = 0; i < a->n; i++) {
        if (!rpg_node_shown(s, s->area, i)) continue;
        int x, y;
        node_xy(s->area, i, &x, &y);
        lv_obj_t *dot = uk_rect(scr, x - 15, y - 15, 30, 30, UK_WHITE);
        lv_obj_set_style_radius(dot, 15, 0);
        lv_obj_set_style_border_color(dot, lv_color_hex(UK_LINE), 0);
        lv_obj_set_style_border_width(dot, 2, 0);
        M.icons[i] = uk_sprite(scr, node_icon(a->nodes[i].kind), 2);
        uk_spos(M.icons[i], x - 12, y - 12);
        M.checks[i] = uk_sprite(scr, SPR_ICON_DONE, 1);
        uk_spos(M.checks[i], x + 6, y - 18);
        lv_obj_t *name = uk_text(scr, UK_F12, UK_INK, a->names[i]);
        lv_obj_set_width(name, 72);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(name, x - 36, y + 15);
    }

    M.ring = lv_obj_create(scr);
    lv_obj_remove_style_all(M.ring);
    lv_obj_remove_flag(M.ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(M.ring, 38, 38);
    lv_obj_set_style_radius(M.ring, 19, 0);
    lv_obj_set_style_border_color(M.ring, lv_color_hex(UK_RED), 0);
    lv_obj_set_style_border_width(M.ring, 3, 0);

    M.dog = uk_sprite(scr, SPR_MON_DOG, 1);
    M.kid = uk_sprite(scr, SPR_KID, 2);
    uk_show(M.dog, rpg_pet_active(s));

    lv_obj_t *strip = uk_rect(scr, 0, 294, UK_W, 26, UK_WAINSCOT);
    (void)strip;
    M.info = uk_text(scr, UK_F12, UK_INK, "");
    lv_obj_align(M.info, LV_ALIGN_TOP_MID, 0, 298);

    uk_screen_swap(scr);
}

static void map_hud(const rpg_save_t *s) {
    int32_t key = (int32_t)(s->hp | (s->sp << 10) | (s->level << 16) | ((s->gold & 0x3F) << 20)) ^ s->gold;
    if (key == M.hud_key) return;
    M.hud_key = key;
    snprintf(M.buf, sizeof(M.buf), RT_GOLD_FMT, (unsigned)s->gold);
    uk_set_text(M.gold, M.buf);
    snprintf(M.buf, sizeof(M.buf), RT_LV_FMT, (unsigned)s->level);
    uk_set_text(M.lv, M.buf);
    bar_set(M.hp_fill, HP_BAR_W, s->hp, rpg_hp_max(s));
    snprintf(M.buf, sizeof(M.buf), "%u/%u", (unsigned)s->hp, (unsigned)rpg_hp_max(s));
    uk_set_text(M.hp_text, M.buf);
    snprintf(M.buf, sizeof(M.buf), RT_SP "%u", (unsigned)s->sp);
    uk_set_text(M.sp, M.buf);
}

static void map_info(const rpg_save_t *s, int target) {
    const rpg_area_t *a = rpg_area(s->area);
    if (target == RPG_UI_TARGET_NONE) {
        uk_set_text(M.info, "");
        return;
    }
    if (target < 0) {
        uk_set_text(M.info, RT_MENU_TARGET);
        return;
    }
    const rpg_node_t *n = &a->nodes[target];
    bool done = rpg_done(s, s->area, (uint8_t)target);
    if (n->kind == NODE_BOSS && !done) {
        snprintf(M.buf, sizeof(M.buf), RT_BOSS_LV_FMT, a->names[target], (unsigned)a->boss_lv);
    } else if (done && (n->kind == NODE_BATTLE || n->kind == NODE_BOSS || n->kind == NODE_CHEST)) {
        snprintf(M.buf, sizeof(M.buf), RT_TARGET_DONE_FMT, a->names[target], KIND_NAMES[n->kind],
                 n->kind == NODE_CHEST ? RT_OPENED : RT_CLEARED);
    } else {
        snprintf(M.buf, sizeof(M.buf), RT_TARGET_FMT, a->names[target], KIND_NAMES[n->kind]);
    }
    uk_set_text(M.info, M.buf);
}

void rpg_ui_map_frame(const rpg_save_t *s, uint32_t dt_ms, int target, int walk_from, uint32_t walk_ms) {
    M.t += dt_ms;
    map_hud(s);
    int32_t done = rpg_done_bits(s, s->area);
    if (done != M.done_shown) {
        M.done_shown = done;
        const rpg_area_t *a = rpg_area(s->area);
        for (uint8_t i = 0; i < M.n; i++) {
            if (!M.icons[i]) continue;  // 隐藏地点(画地图时还没出现)
            bool d = (done >> i) & 1;
            bool show_check = d && a->nodes[i].kind != NODE_REST && a->nodes[i].kind != NODE_SHOP;
            uk_show(M.checks[i], show_check);
            lv_obj_set_style_image_recolor(M.icons[i], lv_color_hex(UK_GRAY), 0);
            lv_obj_set_style_image_recolor_opa(M.icons[i], show_check ? LV_OPA_50 : LV_OPA_TRANSP, 0);
        }
    }
    uk_show(M.dog, rpg_pet_active(s));

    int x, y;
    node_xy(s->area, s->node, &x, &y);
    if (walk_ms && walk_from >= 0 && walk_ms < RPG_UI_WALK_MS) {
        int fx, fy;
        node_xy(s->area, (uint8_t)walk_from, &fx, &fy);
        x = fx + (x - fx) * (int)walk_ms / RPG_UI_WALK_MS;
        y = fy + (y - fy) * (int)walk_ms / RPG_UI_WALK_MS - uk_tri(walk_ms, 150, 4);
    } else {
        y -= uk_tri(M.t, 800, 3);
    }
    uk_spos(M.kid, x - 16, y - 28);  // 站在地点图标上(当前地点的图标不重要)
    uk_spos(M.dog, x + 12, y - 12 + uk_tri(M.t + 200, 500, 2));

    uk_show(M.ring, target >= 0 && (M.t / 300) % 2 == 0);
    if (target >= 0) {
        int tx, ty;
        node_xy(s->area, (uint8_t)target, &tx, &ty);
        lv_obj_set_pos(M.ring, tx - 19, ty - 19);
    }
    map_info(s, target);
}

// ---------------------------------------------------------------------------
// 战斗

#define EN_X     80     // 敌人(放大 5 倍 = 80px)的家
#define EN_Y     40
#define KID_X    16
#define KID_Y    196
#define KID_SIZE 64
#define MENU_X   108
#define MENU_Y   140
#define MENU_W   128   // 「奶奶的拖鞋  勇气5」要放得下
#define MENU_H   120
#define BAR_X    20
#define BAR_Y    142
#define BAR_W    200
#define TIME_X   40
#define TIME_Y   188
#define TIME_W   160
#define KHP_W    64
#define EHP_W    120
#define ROWS_MAX 6

static struct {
    lv_obj_t *enemy;
    lv_obj_t *ename;
    lv_obj_t *efill;
    lv_obj_t *kid;
    lv_obj_t *dog;
    lv_obj_t *granny;
    lv_obj_t *grandpa;   // 爷爷的绝招
    lv_obj_t *slipper;
    lv_obj_t *hint;
    lv_obj_t *bar;
    lv_obj_t *marker;
    lv_obj_t *big;
    lv_obj_t *count;
    lv_obj_t *time;
    lv_obj_t *time_fill;
    lv_obj_t *keys[RB_MEMO_LEN];
    lv_obj_t *opts[3];
    lv_obj_t *menu;
    lv_obj_t *rows[ROWS_MAX];
    lv_obj_t *banner;
    lv_obj_t *pop_e;     // 敌人头上的伤害数字
    lv_obj_t *pop_p;     // 小黄咬的
    lv_obj_t *pop_k;     // 库巴这边:闪开 / 挡住 / 掉血 / 回血(色块,压在谁身上都看得清)
    lv_obj_t *klv;
    lv_obj_t *khp_fill;
    lv_obj_t *khp_text;
    lv_obj_t *ksp;
    lv_obj_t *spicy;
    lv_obj_t *flash;
    lv_obj_t *result;
    lv_obj_t *result2;
    int pose;
    uint32_t t;
    uint32_t pose_ms;
    uint32_t pop_e_ms;
    uint32_t pop_p_ms;
    uint32_t pop_k_ms;
    uint32_t sp_ms;      // 勇气涨了:数字闪一下
    uint32_t feint_ms;   // 假动作:提示晃一下
    uint32_t banner_ms;
    uint32_t shake_ms;
    uint32_t flash_ms;
    uint32_t ehit_ms;
    uint32_t pet_ms;
    uint32_t slip_ms;
    int32_t status_key;
    int menu_key;
    uint8_t streak;      // 连续完美(出拳完美 / 闪开 / 光波全对)
    uint8_t last_sp;
    bool hint_now;       // 提示正亮着「就是现在」
    bool sp_lit;
    int ex;
    int ey;
    char buf[48];
    char rowbuf[ROWS_MAX][28];
} B;

static const char *key_label(rpg_key_t k) {
    return KEY_NAMES[k];
}

static const char *lane_hint(rpg_lane_t l) {
    return LANE_HINTS[l];
}

static uint32_t lane_color(rpg_lane_t l) {
    return l == RPG_LANE_HIGH ? UK_BLUE : l == RPG_LANE_LOW ? 0x8B5A2B : UK_ORANGE;
}

static int lane_y(rpg_lane_t l) {  // 敌人扑过来的高度(敌人图像中心)
    return l == RPG_LANE_HIGH ? KID_Y + 10 : l == RPG_LANE_LOW ? KID_Y + 54 : KID_Y + 32;
}

void rpg_ui_show_battle(const rpg_battle_t *b) {
    screen_drop();
    memset(&B, 0, sizeof(B));
    B.pose = -1;
    B.status_key = -1;
    B.menu_key = -1;
    lv_obj_t *scr = uk_screen_new(UK_WALL);
    uk_rect(scr, 0, 262, UK_W, UK_H - 262, UK_FLOOR);
    uk_rect(scr, 0, 262, UK_W, 3, UK_FLOOR_D);

    // 敌人
    B.ename = uk_text(scr, UK_F12, UK_INK, MON_NAMES[b->mon]);
    lv_obj_align(B.ename, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_t *ebar = uk_rect(scr, (UK_W - EHP_W) / 2, 24, EHP_W, 6, UK_LINE);
    lv_obj_set_style_radius(ebar, 2, 0);
    B.efill = uk_rect(ebar, 0, 0, EHP_W, 6, UK_RED);
    uk_battery(scr, 184, 7, 1);  // 右上角,敌人名字右边
    B.enemy = img_new(scr, mon_img(b->mon), 5);
    lv_obj_set_style_image_recolor(B.enemy, lv_color_hex(UK_WHITE), 0);
    B.ex = EN_X;
    B.ey = EN_Y;

    // 库巴、小黄、奶奶
    B.kid = uk_sprite(scr, SPR_KID, 4);
    lv_obj_set_style_image_recolor(B.kid, lv_color_hex(UK_RED), 0);
    B.dog = uk_sprite(scr, SPR_MON_DOG, 2);
    uk_show(B.dog, rpg_pet_active(b->s));
    B.granny = uk_sprite(scr, SPR_GRANNY, 3);
    B.grandpa = uk_sprite(scr, SPR_GRANDPA, 3);
    uk_show(B.grandpa, false);
    B.slipper = uk_sprite(scr, SPR_SLIPPER, 3);
    uk_show(B.granny, false);
    uk_show(B.slipper, false);

    // 中间的提示区
    B.hint = uk_pill(scr, UK_F24, UK_BLUE, UK_WHITE, "");
    B.bar = uk_rect(scr, BAR_X, BAR_Y, BAR_W, 16, UK_LINE);
    lv_obj_set_style_radius(B.bar, 3, 0);
    lv_obj_t *good = uk_rect(B.bar, BAR_W * RB_BAR_GOOD_FROM / 1000, 0,
                             BAR_W * (RB_BAR_GOOD_TO - RB_BAR_GOOD_FROM) / 1000, 16, 0x9CCB86);
    (void)good;
    const rpg_heat_rule_t *rule = rpg_heat_rule((kit_heat_t)b->heat);  // 完美区按辣度
    uk_rect(B.bar, BAR_W * rule->perf_from / 1000, 0, BAR_W * (rule->perf_to - rule->perf_from) / 1000, 16, UK_GOLD);
    B.marker = uk_rect(B.bar, 0, 0, 4, 16, UK_INK);
    B.big = uk_text(scr, UK_F24, UK_RED, "");
    B.count = uk_text(scr, UK_F36, UK_INK, "");
    B.time = uk_rect(scr, TIME_X, TIME_Y, TIME_W, 6, UK_LINE);
    B.time_fill = uk_rect(B.time, 0, 0, TIME_W, 6, UK_ORANGE);
    for (int i = 0; i < RB_MEMO_LEN; i++) B.keys[i] = uk_text(scr, UK_F36, UK_INK, "");
    for (int i = 0; i < 3; i++) B.opts[i] = uk_pill(scr, UK_F24, UK_INK, UK_WHITE, "");

    // 菜单
    B.menu = uk_panel(scr, MENU_X, MENU_Y, MENU_W, MENU_H);
    for (int i = 0; i < ROWS_MAX; i++) B.rows[i] = uk_pill(B.menu, UK_F12, UK_WHITE, UK_INK, "");

    // 状态栏(地板上)
    // 两行:等级 + 血条 + 血量;勇气 + 辣条标记(满级时数字变宽,一行放不下)
    B.klv = uk_text(scr, UK_F12, UK_WHITE, "");
    lv_obj_set_pos(B.klv, 26, 270);
    lv_obj_t *kbar = uk_rect(scr, 66, 274, KHP_W, 7, UK_FLOOR_D);
    B.khp_fill = uk_rect(kbar, 0, 0, KHP_W, 7, UK_RED);
    B.khp_text = uk_text(scr, UK_F12, UK_WHITE, "");
    lv_obj_set_pos(B.khp_text, 138, 270);
    B.ksp = uk_text(scr, UK_F12, 0xBFDCFF, "");
    lv_obj_set_pos(B.ksp, 26, 288);
    B.spicy = uk_pill(scr, UK_F12, UK_RED, UK_WHITE, RT_SPICY_TAG);
    lv_obj_set_pos(B.spicy, 112, 286);

    B.pop_e = uk_text(scr, UK_F24, UK_RED, "");
    B.pop_p = uk_text(scr, UK_F24, UK_ORANGE, "");
    B.pop_k = uk_pill(scr, UK_F24, UK_RED, UK_WHITE, "");
    B.banner = uk_pill(scr, UK_F24, UK_RED, UK_WHITE, "");
    B.result = uk_pill(scr, UK_F24, UK_GOLD, UK_WHITE, "");
    B.result2 = uk_text(scr, UK_F12, UK_INK, "");
    B.flash = uk_rect(scr, 0, 0, UK_W, UK_H, UK_RED);
    lv_obj_set_style_bg_opa(B.flash, LV_OPA_40, 0);

    lv_obj_t *hide[] = { B.hint, B.bar, B.big, B.count, B.time, B.pop_e, B.pop_p, B.pop_k, B.banner, B.result,
                         B.result2, B.flash, B.spicy };
    for (int i = 0; i < LEN(hide); i++) uk_show(hide[i], false);
    for (int i = 0; i < RB_MEMO_LEN; i++) uk_show(B.keys[i], false);
    for (int i = 0; i < 3; i++) uk_show(B.opts[i], false);

    B.last_sp = b->s->sp;
    uk_screen_swap(scr);
    rpg_ui_battle_frame(b, BE_PHASE, 0);
}

void rpg_ui_battle_pose(int key) {
    B.pose = key;
    B.pose_ms = 0;
}

void rpg_ui_battle_result(const char *line1, const char *line2) {
    lv_label_set_text(B.result, line1);
    lv_obj_align(B.result, LV_ALIGN_TOP_MID, 0, 132);
    uk_show(B.result, true);
    lv_label_set_text(B.result2, line2 ? line2 : "");
    lv_obj_set_style_text_align(B.result2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(B.result2, LV_ALIGN_TOP_MID, 0, 170);
    uk_show(B.result2, line2 != NULL);
    uk_show(B.menu, false);
    uk_show(B.banner, false);
}

#define POP_MS 800

static void pop(lv_obj_t *l, uint32_t *ms, uint32_t color, const char *s) {
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_label_set_text(l, s);
    *ms = POP_MS;
    uk_show(l, true);
}

// 库巴这边的结果:白字色块
static void pop_kid(uint32_t bg, const char *s) {
    lv_obj_set_style_bg_color(B.pop_k, lv_color_hex(bg), 0);
    lv_label_set_text(B.pop_k, s);
    B.pop_k_ms = POP_MS;
    uk_show(B.pop_k, true);
}

static void banner(uint32_t bg, const char *s, uint32_t ms) {
    lv_label_set_text(B.banner, s);
    lv_obj_set_style_bg_color(B.banner, lv_color_hex(bg), 0);
    lv_obj_align(B.banner, LV_ALIGN_TOP_MID, 0, 100);
    B.banner_ms = ms;
    uk_show(B.banner, true);
}

static void menu_refresh(const rpg_battle_t *b) {
    bool on = b->phase == RB_MENU || b->phase == RB_SKILLS || b->phase == RB_ITEMS;
    uk_show(B.menu, on);
    if (!on) return;
    int n = rpg_battle_rows(b);
    if (n > ROWS_MAX) n = ROWS_MAX;
    int key = (int)b->phase * 64 + b->sel * 8 + n;
    for (int i = 0; i < n; i++) {
        int r = rpg_battle_row(b, (uint8_t)i);
        if (b->phase == RB_MENU) {
            snprintf(B.rowbuf[i], sizeof(B.rowbuf[i]), "%s", CMD_NAMES[r]);
        } else if (r < 0) {
            snprintf(B.rowbuf[i], sizeof(B.rowbuf[i]), "%s", RT_CMD_BACK);
        } else if (b->phase == RB_SKILLS) {
            snprintf(B.rowbuf[i], sizeof(B.rowbuf[i]), RT_SKILL_ROW_FMT, SKILL_NAMES[r],
                     (unsigned)rpg_skill_cost((rpg_skill_t)r));
        } else {
            snprintf(B.rowbuf[i], sizeof(B.rowbuf[i]), RT_ITEM_ROW_FMT, ITEM_NAMES[r],
                     (unsigned)b->s->items[r]);
        }
    }
    if (key == B.menu_key) return;
    B.menu_key = key;
    bool big = b->phase == RB_MENU;
    int dy = big ? 28 : 19;
    for (int i = 0; i < ROWS_MAX; i++) {
        uk_show(B.rows[i], i < n);
        if (i >= n) continue;
        lv_obj_set_style_text_font(B.rows[i], big ? UK_F24 : UK_F12, 0);
        lv_label_set_text(B.rows[i], B.rowbuf[i]);
        bool sel = i == b->sel;
        lv_obj_set_style_bg_color(B.rows[i], lv_color_hex(sel ? UK_RED : UK_WHITE), 0);
        lv_obj_set_style_text_color(B.rows[i], lv_color_hex(sel ? UK_WHITE : UK_INK), 0);
        lv_obj_set_pos(B.rows[i], 4, 2 + i * dy);
    }
}

static void status_refresh(const rpg_battle_t *b) {
    const rpg_save_t *s = b->s;
    int32_t key = (int32_t)(s->hp | (s->sp << 10) | (s->level << 16) | (b->spicy << 20)) + b->ehp * 7919;
    if (key == B.status_key) return;
    B.status_key = key;
    snprintf(B.buf, sizeof(B.buf), RT_LV_FMT, (unsigned)s->level);
    uk_set_text(B.klv, B.buf);
    bar_set(B.khp_fill, KHP_W, s->hp, rpg_hp_max(s));
    snprintf(B.buf, sizeof(B.buf), "%u/%u", (unsigned)s->hp, (unsigned)rpg_hp_max(s));
    uk_set_text(B.khp_text, B.buf);
    snprintf(B.buf, sizeof(B.buf), RT_SP "%u/%u", (unsigned)s->sp, (unsigned)rpg_sp_max(s));
    uk_set_text(B.ksp, B.buf);
    if (s->sp > B.last_sp) B.sp_ms = 600;  // 勇气涨了:闪一下金色
    B.last_sp = s->sp;
    uk_show(B.spicy, b->spicy > 0);
    bar_set(B.efill, EHP_W, b->ehp, b->ehp_max);
}

static bool middle_phase(rpg_bphase_t ph) {  // 用到屏幕中间那块的阶段
    return ph == RB_BAR || ph == RB_MASH || ph == RB_MEMO_SHOW || ph == RB_QUIZ || ph == RB_REFUSE || ph == RB_BEAT ||
           ph == RB_WIGGLE;
}

static void on_events(const rpg_battle_t *b, uint32_t ev) {
    const char *grades[3] = { RT_WEAK, RT_GOOD, RT_PERFECT };
    if (ev & (BE_WINDUP | BE_PHASE)) {
        if (b->phase == RB_WINDUP || b->phase == RB_MENU) B.pose = -1;
    }
    // 上一条横幅(「答错了！」「不错！」)还挂着就进了力量条 / 选择题:先收起来,别压住中间的字
    if ((ev & BE_PHASE) && middle_phase(b->phase)) {
        B.banner_ms = 0;
        uk_show(B.banner, false);
    }
    if (ev & (BE_WINDUP | BE_FEINT)) {
        bool tears = (b->info->flags & MONF_TEARS) != 0;
        lv_label_set_text(B.hint, tears ? RT_HIDDEN_HINT : lane_hint((rpg_lane_t)b->lane));
        lv_obj_set_style_bg_color(B.hint, lv_color_hex(tears ? UK_GRAY : lane_color((rpg_lane_t)b->lane)), 0);
        B.hint_now = false;
    }
    if (ev & BE_FEINT) B.feint_ms = 300;
    if ((ev & BE_PHASE) && b->phase == RB_ACT && b->act == ACT_MOVE) banner(UK_GOLD, RT_MOVE_GO, RB_ACT_MS);
    // 连续完美:出拳完美、闪开、光波全对都算;挨打、挡住、没打好就断
    if ((ev & BE_DODGE) || ((ev & BE_GRADE) && b->grade == GRADE_PERFECT)) {
        if (B.streak < 99) B.streak++;
    } else if (ev & (BE_HURT | BE_BLOCK | BE_GRADE)) {
        B.streak = 0;
    }
    if (ev & BE_ENEMY_HURT) {
        bool perfect = (ev & BE_GRADE) && b->grade == GRADE_PERFECT;
        snprintf(B.buf, sizeof(B.buf), "-%u", (unsigned)b->last_dmg);
        lv_obj_set_style_text_font(B.pop_e, perfect ? UK_F36 : UK_F24, 0);  // 完美一击:数字更大
        pop(B.pop_e, &B.pop_e_ms, perfect ? UK_ORANGE : UK_RED, B.buf);
        B.ehit_ms = perfect ? 450 : 300;
    }
    if (ev & BE_COUNTER) {  // 绝招打回去:敌人头上的数字
        snprintf(B.buf, sizeof(B.buf), RT_COUNTER_FMT, (unsigned)b->counter_dmg);
        lv_obj_set_style_text_font(B.pop_e, UK_F24, 0);
        pop(B.pop_e, &B.pop_e_ms, UK_ORANGE, B.buf);
        B.ehit_ms = 300;
    }
    if (ev & BE_PET) {
        snprintf(B.buf, sizeof(B.buf), "%s-%u", RT_DOGGO, (unsigned)b->pet_dmg);
        pop(B.pop_p, &B.pop_p_ms, UK_ORANGE, B.buf);
        B.pet_ms = 500;
        B.ehit_ms = 300;
    }
    if (ev & BE_GRADE) {
        const char *g = grades[b->grade];
        if (b->grade == GRADE_PERFECT && B.streak >= 2) {
            snprintf(B.buf, sizeof(B.buf), RT_STREAK_FMT, RT_PERFECT_SHORT, (unsigned)B.streak);
            g = B.buf;
        }
        banner(b->grade == GRADE_PERFECT ? UK_GOLD : b->grade == GRADE_GOOD ? UK_GREEN : UK_GRAY, g, 600);
    }
    if (ev & BE_HEAL) {
        snprintf(B.buf, sizeof(B.buf), "+%u", (unsigned)b->last_heal);
        pop_kid(UK_GREEN, B.buf);
    }
    if (ev & BE_DODGE) {
        if (B.streak >= 2) snprintf(B.buf, sizeof(B.buf), RT_STREAK_FMT, RT_DODGE_SHORT, (unsigned)B.streak);
        pop_kid(UK_GOLD, B.streak >= 2 ? B.buf : RT_DODGE);
    }
    if (ev & BE_BLOCK) {
        snprintf(B.buf, sizeof(B.buf), "%s-%u", RT_BLOCK, (unsigned)b->last_dmg);
        pop_kid(UK_BLUE, B.buf);
    }
    if ((ev & BE_HURT) && !(ev & BE_QUIZ_WRONG)) {
        snprintf(B.buf, sizeof(B.buf), "-%u", (unsigned)b->last_dmg);
        pop_kid(UK_RED, B.buf);
        B.shake_ms = 300;
        B.flash_ms = 120;
    }
    if (ev & BE_SLIPPER) B.slip_ms = RB_BAR_MS + RB_ACT_MS;
    // 第三阶段一来就硬塞青椒 / 要刷牙时,下面已经说清楚了,别再压一条横幅
    if ((ev & BE_STAGE) && !(ev & BE_REFUSE) && b->phase != RB_WIGGLE) {
        if (b->info->flags & MONF_SKEWER) {
            snprintf(B.buf, sizeof(B.buf), RT_BALLS_FMT, (unsigned)b->stage);
            banner(UK_RED, B.buf, 1200);
        } else if (b->info->flags & MONF_CANDY) {
            banner(UK_RED, b->stage == 2 ? RT_CANDY_PHASE2 : RT_CANDY_PHASE3, 1200);
        } else {
            banner(UK_RED, b->stage == 2 ? RT_PHASE2 : RT_PHASE3, 1200);
        }
    }
    if (ev & BE_QUIZ_RIGHT) banner(UK_GREEN, RT_QUIZ_RIGHT, 1000);
    if (ev & BE_QUIZ_WRONG) {
        banner(UK_RED, RT_QUIZ_WRONG, 1000);
        B.shake_ms = 300;
        B.flash_ms = 120;
    }
    if (ev & BE_REFUSE_OK) banner(UK_GREEN, RT_REFUSE_OK, 1000);
    if (ev & BE_FREE) banner(UK_GREEN, (b->info->flags & MONF_CANDY) ? RT_BRUSH_OK : RT_FREE_OK, 1000);
    if (ev & BE_SPICY) banner(UK_RED, (b->info->flags & MONF_MELT) ? RT_MELT : RT_SPICY_ON, 900);
    if (ev & BE_FLEE_FAIL) banner(UK_GRAY, RT_FLEE_FAIL, 900);
    if (ev & BE_NO_FLEE) banner(UK_GRAY, RT_NO_FLEE, 900);
    if (ev & BE_NO_SP) banner(UK_GRAY, RT_NO_SP, 900);
    if (ev & BE_FLED) banner(UK_GREEN, RT_FLED, 2000);
    if (ev & BE_LOST) {
        banner(UK_GRAY, RT_LOST, 2000);
        lv_obj_set_style_image_recolor(B.kid, lv_color_hex(UK_GRAY), 0);  // 趴下变灰(见 actors)
    }
    if ((ev & BE_ITEM) && b->item == ITEM_SODA) pop_kid(UK_BLUE, RT_SODA_POP);
}

// 力量条、狂按、光波、选择题、拒绝青椒:按阶段显示中间那块。
static void middle(const rpg_battle_t *b) {
    rpg_bphase_t ph = b->phase;
    uint32_t p = rpg_battle_progress(b);
    bool bar = ph == RB_BAR;
    uk_show(B.bar, bar);
    if (bar) lv_obj_set_x(B.marker, (int)(p * (BAR_W - 4) / 1000));

    bool timed = ph == RB_MASH || ph == RB_REFUSE || ph == RB_MEMO_INPUT || ph == RB_QUIZ || ph == RB_BEAT ||
                 ph == RB_WIGGLE;
    uk_show(B.time, timed);
    if (timed) {
        bar_set(B.time_fill, TIME_W, 1000 - p, 1000);
        lv_obj_set_y(B.time, ph == RB_QUIZ ? 118 : TIME_Y);  // 选择题的三个答案会挡住下面那条
    }

    const char *big = NULL;
    uint32_t big_color = UK_RED;
    if (ph == RB_BAR) big = b->act == ACT_HEAL ? SKILL_NAMES[SK_HEAL] : b->act == ACT_SLIPPER ?
                                SKILL_NAMES[SK_SLIPPER] : RT_BAR_HINT;
    else if (ph == RB_MASH) big = RT_MASH_HINT;
    else if (ph == RB_REFUSE) big = RT_REFUSE_TITLE;
    else if (ph == RB_MEMO_SHOW) big = RT_MEMORY_HINT;
    else if (ph == RB_MEMO_INPUT) big = RT_MEMORY_GO;
    else if (ph == RB_BEAT) big = RT_BEAT_HINT;
    else if (ph == RB_WIGGLE) big = (b->info->flags & MONF_CANDY) ? RT_BRUSH_TITLE : RT_STUCK_TITLE;
    if (ph == RB_QUIZ) {
        snprintf(B.buf, sizeof(B.buf), RT_QUIZ_FMT, (unsigned)b->qa, b->qmul ? RT_MUL : RT_ADD, (unsigned)b->qb);
        big = B.buf;
        big_color = UK_INK;
    }
    uk_show(B.big, big != NULL);
    if (big) {
        uk_set_text(B.big, big);
        lv_obj_set_style_text_color(B.big, lv_color_hex(big_color), 0);
        lv_obj_align(B.big, LV_ALIGN_TOP_MID, 0, ph == RB_BAR ? 116 : 128);
    }

    bool count = ph == RB_MASH || ph == RB_REFUSE || ph == RB_WIGGLE;
    uk_show(B.count, count);
    if (count) {
        if (ph == RB_REFUSE) snprintf(B.buf, sizeof(B.buf), "%u/%u", (unsigned)b->taps,
                                     (unsigned)rpg_heat_rule((kit_heat_t)b->heat)->refuse_need);
        else if (ph == RB_WIGGLE) snprintf(B.buf, sizeof(B.buf), RT_WIGGLE_FMT, (unsigned)b->taps,
                                          (unsigned)rpg_heat_rule((kit_heat_t)b->heat)->wiggle_need);
        else snprintf(B.buf, sizeof(B.buf), "%u", (unsigned)b->taps);
        uk_set_text(B.count, B.buf);
        lv_obj_align(B.count, LV_ALIGN_TOP_MID, 0, 150 - uk_tri(B.t, 160, 3));
    }

    bool memo = ph == RB_MEMO_SHOW || ph == RB_MEMO_INPUT;
    _Static_assert(RB_BEAT_N == RB_MEMO_LEN, "beat dots reuse the memo labels");
    for (int i = 0; i < RB_MEMO_LEN; i++) {
        if (ph == RB_BEAT) {  // 节拍:四个圆点,快到这一拍时变金色跳一下;对了 ★,错过 ×
            uk_show(B.keys[i], true);
            bool judged = (b->beat_judged >> i) & 1, hit = (b->beat_hit >> i) & 1;
            int32_t d = (int32_t)b->t - (int32_t)rpg_beat_ms(i);
            bool now = !judged && d > -250 && d < 120;
            uk_set_text(B.keys[i], judged ? (hit ? RT_BEAT_OK : RT_BEAT_MISS) : RT_BEAT_DOT);
            lv_obj_set_style_text_color(B.keys[i], lv_color_hex(judged ? (hit ? UK_GREEN : UK_GRAY)
                                                                        : now ? UK_GOLD : UK_INK), 0);
            lv_obj_set_pos(B.keys[i], 44 + i * 42, 152 - (now ? 6 : 0));
            continue;
        }
        uk_show(B.keys[i], memo);
        if (!memo) continue;
        const char *k = ph == RB_MEMO_SHOW ? key_label(rpg_lane_key((rpg_lane_t)b->memo[i]))
                                           : i < b->taps ? RT_MEMO_DONE : RT_MEMO_TODO;
        uk_set_text(B.keys[i], k);
        lv_obj_set_style_text_color(B.keys[i], lv_color_hex(ph == RB_MEMO_SHOW ? UK_INK : i < b->taps ?
                                                              UK_GREEN : UK_GRAY), 0);
        lv_obj_set_pos(B.keys[i], 44 + i * 42, 152);
    }

    for (int i = 0; i < 3; i++) {
        uk_show(B.opts[i], ph == RB_QUIZ);
        if (ph != RB_QUIZ) continue;
        static const rpg_key_t K[3] = { RB_KEY_UP, RB_KEY_DOWN, RB_KEY_OK };
        snprintf(B.buf, sizeof(B.buf), "%s%u", key_label(K[i]), (unsigned)b->qopt[i]);
        uk_set_text(B.opts[i], B.buf);
        lv_obj_set_pos(B.opts[i], 8 + i * 78, 166);
    }

    bool hint = ph == RB_WINDUP || ph == RB_STRIKE;
    uk_show(B.hint, hint);
    if (hint) {
        // 出手后段(闪开的时机)提示变金色:就是现在!洋葱头熏得看不清,不给
        bool now = ph == RB_STRIKE && p >= RB_DODGE_FROM && !b->answered && !(b->info->flags & MONF_TEARS);
        if (now != B.hint_now) {
            B.hint_now = now;
            lv_obj_set_style_bg_color(B.hint, lv_color_hex(now ? UK_GOLD : lane_color((rpg_lane_t)b->lane)), 0);
        }
        int dx = uk_countdown(&B.feint_ms, 0) ? ((B.t / 40) % 2 ? 4 : -4) : 0;
        lv_obj_align(B.hint, LV_ALIGN_TOP_MID, 40 + dx, 132);
    }
}

static void actors(const rpg_battle_t *b, uint32_t dt) {
    // 敌人:蓄力时抖,出手时扑向库巴(高度 = 招式),结果出来后退回去
    int hx = EN_X, hy = EN_Y;
    int x = hx, y = hy;
    uint32_t p = rpg_battle_progress(b);
    if (b->phase == RB_WINDUP) {
        int shake = 1 + (int)(p * 4 / 1000);
        x += (B.t / 40) % 2 ? shake : -shake;
    } else if (b->phase == RB_STRIKE) {
        int tx = KID_X + KID_SIZE / 2 + 10 - 40, ty = lane_y((rpg_lane_t)b->lane) - 40;
        x = hx + (tx - hx) * (int)p / 1000;
        y = hy + (ty - hy) * (int)p / 1000;
    } else if (b->phase == RB_HIT && b->hits) {
        uint32_t q = b->t * 1000u / RB_HIT_MS;
        if (q > 1000) q = 1000;
        int tx = KID_X + KID_SIZE / 2 + 10 - 40, ty = lane_y((rpg_lane_t)b->lane) - 40;
        x = tx + (hx - tx) * (int)q / 1000;
        y = ty + (hy - ty) * (int)q / 1000;
    } else {
        y -= uk_tri(B.t, 900, 3);
    }
    if ((b->info->flags & (MONF_KING | MONF_CANDY)) && (b->stage == 2 || b->stage == 3)) x += (B.t / 60) % 2 ? 1 : -1;
    if (B.ehit_ms) x += (int)B.ehit_ms / 30 + ((B.t / 40) % 2 ? 2 : -2);  // 挨打:往后一退、抖一抖
    if (b->phase == RB_WON) y += (int)(b->t / 8 > 60 ? 60 : b->t / 8);
    uk_spos(B.enemy, x, y);
    lv_obj_set_style_image_recolor_opa(B.enemy, uk_countdown(&B.ehit_ms, dt) && (B.t / 60) % 2 ?
                                       LV_OPA_70 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_opa(B.enemy, b->phase == RB_WON ? (lv_opa_t)(255 - (b->t / 4 > 200 ? 200 : b->t / 4))
                                                    : LV_OPA_COVER, 0);

    // 库巴
    B.pose_ms += dt;
    spr_id_t ks = B.pose == RB_KEY_OK ? SPR_KID_GUARD : B.pose == RB_KEY_UP ? SPR_KID_WAVE : SPR_KID;
    if (b->phase == RB_WON || b->phase == RB_FLED) ks = SPR_KID_WAVE;
    uk_sprite_set(B.kid, ks);
    bool lost = b->phase == RB_LOST;  // 累趴了:趴下、变灰
    int kh = lost ? 28 : B.pose == RB_KEY_DOWN ? 40 : KID_SIZE;
    lv_image_set_scale_y(B.kid, (uint32_t)(256 * 4 * kh / KID_SIZE));
    int ky = KID_Y + KID_SIZE - kh;
    if (B.pose == RB_KEY_UP) ky -= B.pose_ms < 120 ? (int)B.pose_ms * 40 / 120 : 40;
    if (b->phase == RB_FLED) B.pose = -1;
    int kx = KID_X + (b->phase == RB_FLED ? -(int)(b->t / 6) : 0);
    if (uk_countdown(&B.shake_ms, dt)) kx += (B.t / 30) % 2 ? 3 : -3;
    uk_spos(B.kid, kx, ky - (B.pose < 0 && !lost ? uk_tri(B.t, 700, 2) : 0));
    lv_obj_set_style_image_recolor_opa(B.kid, lost ? LV_OPA_60 : B.shake_ms ? LV_OPA_50 : LV_OPA_TRANSP, 0);

    // 小黄:咬的时候扑上去
    if (rpg_pet_active(b->s)) {
        uk_show(B.dog, true);
        int dx = 84, dy = 228 - uk_tri(B.t + 300, 500, 3);
        if (uk_countdown(&B.pet_ms, dt)) {
            int q = 500 - (int)B.pet_ms;
            int k = q < 250 ? q : 500 - q;
            dx += k * 4 / 10;
            dy -= k * 5 / 10;
        }
        uk_spos(B.dog, dx, dy);
    }

    // 爷爷的绝招:爷爷从左上探出来摆个架势
    bool move = b->phase == RB_ACT && b->act == ACT_MOVE;
    uk_show(B.grandpa, move);
    if (move) uk_spos(B.grandpa, 2, 60 - uk_tri(B.t, 300, 4));

    // 奶奶的拖鞋:奶奶从左上探出来,拖鞋飞向敌人
    bool slip = uk_countdown(&B.slip_ms, dt) > 0;
    uk_show(B.granny, slip);
    uk_show(B.slipper, slip && b->phase == RB_ACT);
    if (slip) {
        uk_spos(B.granny, 2, 60 - uk_tri(B.t, 300, 4));
        if (b->phase == RB_ACT) {
            int q = (int)(b->t > 400 ? 400 : b->t);
            uk_spos(B.slipper, 30 + q * 80 / 400, 90 - q * 30 / 400);
            lv_image_set_rotation(B.slipper, (int32_t)((B.t * 4) % 3600));
        }
    }
}

void rpg_ui_battle_frame(const rpg_battle_t *b, uint32_t ev, uint32_t dt_ms) {
    B.t += dt_ms;
    if (ev) on_events(b, ev);
    if (ev & (BE_PHASE | BE_CURSOR | BE_ITEM)) B.menu_key = -1;
    menu_refresh(b);
    status_refresh(b);
    middle(b);
    actors(b, dt_ms);

    // 飘字:敌人右边(血条下面,不压名字)、小黄的在敌人左边、库巴的在他右上方(敌人扑过来的地方之外)
    if (B.pop_e_ms) {
        uk_countdown(&B.pop_e_ms, dt_ms);
        lv_obj_align(B.pop_e, LV_ALIGN_TOP_RIGHT, -8, 70 - (int)(POP_MS - B.pop_e_ms) / 32);
        uk_show(B.pop_e, B.pop_e_ms > 0);
    }
    if (B.pop_p_ms) {
        uk_countdown(&B.pop_p_ms, dt_ms);
        lv_obj_set_pos(B.pop_p, 8, 70 - (int)(POP_MS - B.pop_p_ms) / 32);
        uk_show(B.pop_p, B.pop_p_ms > 0);
    }
    if (B.pop_k_ms) {
        uk_countdown(&B.pop_k_ms, dt_ms);
        lv_obj_update_layout(B.pop_k);
        int w = lv_obj_get_width(B.pop_k);
        int x = 168 - w / 2;
        if (x > UK_W - 8 - w) x = UK_W - 8 - w;
        lv_obj_set_pos(B.pop_k, x < 88 ? 88 : x, 204 - (int)(POP_MS - B.pop_k_ms) / 32);
        uk_show(B.pop_k, B.pop_k_ms > 0);
    }
    if (B.banner_ms) {
        uk_countdown(&B.banner_ms, dt_ms);
        uk_show(B.banner, B.banner_ms > 0);
    }
    uk_show(B.flash, uk_countdown(&B.flash_ms, dt_ms) > 0);
    uk_countdown(&B.feint_ms, dt_ms);
    bool sp_on = uk_countdown(&B.sp_ms, dt_ms) && (B.t / 100) % 2 == 0;
    if (sp_on != B.sp_lit) {
        B.sp_lit = sp_on;
        lv_obj_set_style_text_color(B.ksp, lv_color_hex(sp_on ? UK_GOLD : 0xBFDCFF), 0);
    }
}

// ---------------------------------------------------------------------------
// 怪物图鉴

static struct {
    const rpg_save_t *s;
    lv_obj_t *icon;
    lv_obj_t *name;
    lv_obj_t *note;
    lv_obj_t *stats;
    lv_obj_t *index;
    int shown;
    uint32_t t;
    char buf[48];
} D;

void rpg_ui_show_bestiary(const rpg_save_t *s) {
    memset(&D, 0, sizeof(D));
    D.s = s;
    D.shown = -1;
    lv_obj_t *scr = uk_screen_new(UK_WALL);
    lv_obj_t *title = uk_text(scr, UK_F24, UK_RED, RT_BEST_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);
    D.index = uk_text(scr, UK_F12, UK_INK, "");
    lv_obj_align(D.index, LV_ALIGN_TOP_MID, 0, 44);
    lv_obj_t *card = uk_panel(scr, 18, 62, 204, 200);
    D.icon = uk_sprite(card, SPR_MON_CAT, 6);
    lv_obj_set_style_image_recolor(D.icon, lv_color_hex(UK_INK), 0);
    D.name = uk_text(card, UK_F24, UK_INK, "");
    D.note = uk_text(card, UK_F12, UK_GRAY, "");
    D.stats = uk_pill(card, UK_F12, UK_GREEN, UK_WHITE, "");
    lv_obj_t *help = uk_text(scr, UK_F12, UK_INK, RT_BEST_HELP);
    lv_obj_align(help, LV_ALIGN_BOTTOM_MID, 0, -14);
    uk_screen_swap(scr);
}

void rpg_ui_bestiary_frame(uint32_t dt_ms, int sel) {
    D.t += dt_ms;
    bool beaten = rpg_beaten_mon(D.s, (rpg_mon_t)sel), seen = rpg_seen_mon(D.s, (rpg_mon_t)sel);
    if (sel != D.shown) {
        D.shown = sel;
        img_set(D.icon, mon_img(sel));
        lv_obj_set_style_image_recolor_opa(D.icon, beaten ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
        lv_label_set_text(D.name, seen || beaten ? MON_NAMES[sel] : RT_BEST_LOCKED);
        lv_obj_align(D.name, LV_ALIGN_TOP_MID, 0, 108);
        lv_label_set_text(D.note, beaten ? MON_NOTES[sel] : seen ? RT_BEST_SEEN : "");
        lv_obj_align(D.note, LV_ALIGN_TOP_MID, 0, 140);
        const rpg_mon_info_t *m = rpg_mon((rpg_mon_t)sel);
        snprintf(D.buf, sizeof(D.buf), RT_BEST_FMT, (unsigned)m->hp, (unsigned)m->atk, (unsigned)m->def);
        lv_label_set_text(D.stats, D.buf);
        lv_obj_align(D.stats, LV_ALIGN_TOP_MID, 0, 162);
        uk_show(D.stats, beaten);
        int n = 0;
        for (int i = 0; i < MON_ALL; i++) n += rpg_beaten_mon(D.s, (rpg_mon_t)i);
        snprintf(D.buf, sizeof(D.buf), RT_BEST_INDEX_FMT, sel + 1, MON_ALL, n);
        lv_label_set_text(D.index, D.buf);
    }
    uk_spos(D.icon, 54, 6 - (beaten ? uk_tri(D.t, 800, 4) : 0));
}

// ---------------------------------------------------------------------------
// 结局

static struct {
    lv_obj_t *who[5];
    uint32_t t;
} E;

void rpg_ui_show_ending(const rpg_save_t *s, bool ch2) {
    memset(&E, 0, sizeof(E));
    lv_obj_t *scr = uk_screen_new(UK_WALL);
    uk_rect(scr, 0, 176, UK_W, UK_H - 176, UK_WAINSCOT);
    lv_obj_t *t = uk_text(scr, UK_F36, UK_GOLD, ch2 ? RT_END2_TITLE : RT_END_TITLE);
    lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_t *th = uk_text(scr, UK_F12, UK_RED, ch2 ? RT_END2_THANKS : RT_END_THANKS);
    lv_obj_align(th, LV_ALIGN_TOP_MID, 0, 66);
    static const spr_id_t WHO[5] = { SPR_GRANDPA, SPR_GRANNY, SPR_KID_WAVE, SPR_MON_DOG, SPR_MON_KING };
    for (int i = 0; i < 5; i++) E.who[i] = uk_sprite(scr, WHO[i], 3);
    if (ch2) img_set(E.who[4], mon_img(MON_CANDY));  // 生日会上多了糖果大王
    // 桌上一盘韭菜盒子(第 2 章:生日蛋糕)
    lv_obj_t *dish = ch2 ? img_new(scr, kit_img(&CAKE_ART, spr_decode), 3) : uk_sprite(scr, SPR_DUMPLING, 3);
    uk_spos(dish, 102, ch2 ? 146 : 150);  // 蛋糕比韭菜盒子高一点:往上挪,别压住下面的字
    char buf[96];
    unsigned minutes = (unsigned)(s->play_s / 60u);
    snprintf(buf, sizeof(buf), RT_END_STATS_FMT, minutes, (unsigned)s->battles, (unsigned)s->perfects,
             (unsigned)s->level);
    lv_obj_t *st = uk_text(scr, UK_F12, UK_INK, buf);
    lv_obj_set_style_text_align(st, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(st, LV_ALIGN_TOP_MID, 0, 196);
    lv_obj_t *help = uk_text(scr, UK_F12, UK_GREEN, ch2 ? RT_END2_HELP : RT_END_HELP);
    lv_obj_align(help, LV_ALIGN_BOTTOM_MID, 0, -16);
    uk_screen_swap(scr);
    rpg_ui_ending_frame(0);
}

void rpg_ui_ending_frame(uint32_t dt_ms) {
    E.t += dt_ms;
    for (int i = 0; i < 5; i++) uk_spos(E.who[i], 2 + i * 46, 98 - uk_tri(E.t + (uint32_t)i * 140, 600, 6));
}

// ---------------------------------------------------------------------------
// 浮层(lv_layer_top,引擎见 kit_overlay.h):第一次用到时建,离开大冒险时 rpg_ui_release 删掉。

static const kit_overlay_style_t OV = {
    .parent = KIT_OV_LAYER_TOP,
    .dlg = {
        .box = { 6, 198, 228, 108 },  // 底边 306:下面两个角躲开屏幕 30px 圆角
        .face_x = 6, .face_y = 10, .face_scale = 3,
        .x_face = 62, .x_bare = 8, .who_y = 4,
        .say_y_face = 22, .say_y_bare = 10, .say_w_face = 152, .say_w_bare = 206,
        .wrap = KIT_WRAP_RPG, .cap = 160,
        .more = RT_KEY_OK,  // 按 ● 下一句
    },
    .toast = {
        .box = { 18, 104, 204, 112 },
        .text_y = 12, .text_w = 180, .text_flags = KIT_OV_TEXT_CENTER | KIT_OV_TEXT_MID,
        .wrap = KIT_WRAP_RPG, .cap = 96,
        .size = KIT_SIZE_FIXED,
        .more = RT_NEXT, .more_dy = -6,
    },
    .list = {
        .box = { 16, 36, 208, 252 },
        .size = KIT_SIZE_FIXED, .place = KIT_LIST_PLACE_FIXED,
        .title_y = 6,
        .info_x = 7, .info_y = 38, .info_w = 188,
        .rows_y = 42, .info_gap = 8,
        .row_x = 10, .row_dy = 24, .row_pad_ver = -1, .rows_max = 6,
    },
};

static void overlays_make(void) {
    if (!kit_ov_bound(&OV)) kit_ov_bind(&OV, NULL, KIT_OV_ALL, uk_img(SPR_GRANNY));
}

void rpg_ui_dialog(rpg_speaker_t spk, const char *text) {
    overlays_make();
    if (!text) {
        kit_ov_dialog(NULL, NULL, NULL);
        return;
    }
    kit_ov_dialog(speaker_img(spk), SPEAKERS[spk], text);
}

bool rpg_ui_dialog_typing(void) {
    return kit_ov_bound(&OV) && kit_ov_typing();
}

void rpg_ui_dialog_finish(void) {
    if (kit_ov_bound(&OV)) kit_ov_finish();
}

void rpg_ui_toast(const char *text) {
    overlays_make();
    kit_ov_toast(text);
}

void rpg_ui_list(const char *title, const char *info, const char *const *rows, int n, int sel) {
    overlays_make();
    kit_ov_list(title, info, rows, n, sel, -1);
}

void rpg_ui_hide_overlays(void) {
    if (kit_ov_bound(&OV)) kit_ov_hide();
}

void rpg_ui_overlay_frame(uint32_t dt_ms) {
    if (kit_ov_bound(&OV)) kit_ov_frame(dt_ms);
}

void rpg_ui_release(void) {
    if (kit_ov_bound(&OV)) kit_ov_release();
}
