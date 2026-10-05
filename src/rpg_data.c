// main/rpg_data.c —— 见 rpg_data.h。数值按「小区结束约 Lv4、学校约 Lv7、菜市场 Lv9~10」调
// (tests/test_rpg_sim.c 的全流程模拟会检查这个节奏和大致时长)。
#include "rpg_data.h"

#include <stddef.h>

#include "kit_lines.h"

#define RV(n)   (&LINE_##n)
#define RV_NONE NULL
#include "rpg_texts.h"

#define LEN(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))
#define H RPG_LANE_BIT(RPG_LANE_HIGH)
#define M RPG_LANE_BIT(RPG_LANE_MID)
#define L RPG_LANE_BIT(RPG_LANE_LOW)

static const rpg_mon_info_t MONS[MON_ALL] = {
    //                 hp  atk def exp gold windup strike lanes   combo area flags
    [MON_CAT]      = {  21,  6,  1,  5,  3,  800, 600, H | M,     1, 0, 0 },
    [MON_MOSQUITO] = {  14,  5,  0,  4,  2,  700, 550, H,         2, 0, 0 },
    [MON_BROOM]    = {  30,  7,  2,  7,  4,  900, 650, L,         1, 0, 0 },
    [MON_DOG]      = { 110, 12,  3, 30, 20,  750, 550, H | M | L, 2, 0, MONF_BOSS | MONF_FEINT },
    [MON_HOMEWORK] = {  60, 13,  5, 13,  8,  750, 550, H | M,     1, 1, 0 },
    [MON_CLOCK]    = {  51, 12,  4, 12,  7,  650, 450, H | M | L, 3, 1, 0 },
    [MON_CHALK]    = {  39, 15,  3, 12,  6,  600, 450, H,         1, 1, 0 },
    [MON_ROPE]     = {  66, 13,  6, 14,  9,  800, 550, L,         2, 1, 0 },
    [MON_EXAM]     = { 240, 19,  7, 60, 50,  700, 500, H | M | L, 2, 1,
                       MONF_BOSS | MONF_FEINT | MONF_QUIZ },
    [MON_PEPPER]   = { 105, 20,  9, 22, 12,  650, 480, H | M | L, 2, 2, 0 },
    [MON_ONION]    = {  90, 19,  8, 22, 12,  700, 500, H | M | L, 1, 2, MONF_TEARS },
    [MON_TOFU]     = { 120, 21, 10, 25, 14,  800, 520, M | L,     2, 2, 0 },
    [MON_BITTER]   = { 111, 22,  9, 24, 14,  700, 480, H | L,     2, 2, 0 },
    [MON_KING]     = { 360, 24, 12,  0,  0,  650, 450, H | M | L, 3, 2,
                       MONF_BOSS | MONF_FEINT | MONF_KING },
    // 第 2 章:按「公园结束约 Lv11、游乐园 Lv13」调(test_rpg_sim.c 的 test_ch2)
    [MON_BALLOON]  = { 120, 25, 10, 24, 16,  650, 480, H | M,     2, 3, MONF_FEINT },
    [MON_CARP]     = { 150, 26, 12, 27, 18,  700, 480, H | L,     1, 3, 0 },
    [MON_SPEAKER]  = { 140, 27, 11, 28, 20,  700, 500, M | L,     2, 3, MONF_BEAT },
    [MON_SKEWER]   = { 420, 26, 14, 80, 80,  650, 460, H | M | L, 5, 3,
                       MONF_BOSS | MONF_FEINT | MONF_SKEWER },
    [MON_POP]      = { 150, 30, 13, 34, 22,  550, 400, H | M | L, 3, 4, 0 },
    [MON_GUM]      = { 180, 29, 14, 36, 24,  750, 520, M | L,     1, 4, MONF_STICKY },
    [MON_ICE]      = { 200, 31, 18, 38, 26,  700, 500, H | M,     2, 4, MONF_MELT },
    [MON_CANDY]    = { 600, 31, 16,  0,  0,  600, 440, H | M | L, 3, 4,
                       MONF_BOSS | MONF_FEINT | MONF_CANDY },
};

const rpg_mon_info_t *rpg_mon(rpg_mon_t m) {
    return (unsigned)m < MON_ALL ? &MONS[m] : NULL;
}

uint16_t rpg_item_price(rpg_item_t item) {
    static const uint16_t PRICE[ITEM_COUNT] = { 10, 15, 12 };
    return (unsigned)item < ITEM_COUNT ? PRICE[item] : 0;
}

uint8_t rpg_skill_level(rpg_skill_t sk) {
    static const uint8_t LV[SK_COUNT] = { 2, 3, 5, 7, 1 };  // 绝招不看等级,看有没有学会(RF_MOVE)
    return (unsigned)sk < SK_COUNT ? LV[sk] : 0xFF;
}

uint8_t rpg_skill_cost(rpg_skill_t sk) {
    static const uint8_t COST[SK_COUNT] = { 3, 3, 5, 6, 4 };
    return (unsigned)sk < SK_COUNT ? COST[sk] : 0xFF;
}

// ---------------------------------------------------------------------------
// 剧本

static const rpg_line_t S_OPENING[] = RT_SCR_OPENING;
static const rpg_line_t S_GIFT[] = RT_SCR_GIFT;
static const rpg_line_t S_REST_HOME[] = RT_SCR_REST_HOME;
static const rpg_line_t S_REST[] = RT_SCR_REST;
static const rpg_line_t S_GRANDPA[] = RT_SCR_GRANDPA;
static const rpg_line_t S_DOG_IN[] = RT_SCR_DOG_IN;
static const rpg_line_t S_DOG_DOWN[] = RT_SCR_DOG_DOWN;
static const rpg_line_t S_SCHOOL[] = RT_SCR_SCHOOL;
static const rpg_line_t S_FATTY[] = RT_SCR_FATTY;
static const rpg_line_t S_EXAM_IN[] = RT_SCR_EXAM_IN;
static const rpg_line_t S_EXAM_DOWN[] = RT_SCR_EXAM_DOWN;
static const rpg_line_t S_MARKET[] = RT_SCR_MARKET;
static const rpg_line_t S_GRANDPA_MARKET[] = RT_SCR_GRANDPA_MARKET;
static const rpg_line_t S_KING_IN[] = RT_SCR_KING_IN;
static const rpg_line_t S_KING_DOWN[] = RT_SCR_KING_DOWN;
static const rpg_line_t S_ENDING[] = RT_SCR_ENDING;
static const rpg_line_t S_LOSE[] = RT_SCR_LOSE;
static const rpg_line_t S_SHOP[] = RT_SCR_SHOP;
static const rpg_line_t S_NOTHING[] = RT_SCR_NOTHING;
static const rpg_line_t S_CH2_OPEN[] = RT_SCR_CH2_OPEN;
static const rpg_line_t S_PARK[] = RT_SCR_PARK;
static const rpg_line_t S_SHOP_KING[] = RT_SCR_SHOP_KING;
static const rpg_line_t S_GP_ASK[] = RT_SCR_GP_ASK;
static const rpg_line_t S_GP_TEACH[] = RT_SCR_GP_TEACH;
static const rpg_line_t S_GP_AFTER[] = RT_SCR_GP_AFTER;
static const rpg_line_t S_SKEWER_IN[] = RT_SCR_SKEWER_IN;
static const rpg_line_t S_SKEWER_DOWN[] = RT_SCR_SKEWER_DOWN;
static const rpg_line_t S_FAIR[] = RT_SCR_FAIR;
static const rpg_line_t S_DOG_FREE[] = RT_SCR_DOG_FREE;
static const rpg_line_t S_FATTY2[] = RT_SCR_FATTY2;
static const rpg_line_t S_CANDY_IN[] = RT_SCR_CANDY_IN;
static const rpg_line_t S_CANDY_DOWN[] = RT_SCR_CANDY_DOWN;
static const rpg_line_t S_ENDING2[] = RT_SCR_ENDING2;

#define SCR(a) { a, LEN(a) }
static const rpg_script_info_t SCRIPTS[SCR_COUNT] = {
    [SCR_OPENING] = SCR(S_OPENING),       [SCR_GIFT] = SCR(S_GIFT),
    [SCR_REST_HOME] = SCR(S_REST_HOME),   [SCR_REST] = SCR(S_REST),
    [SCR_GRANDPA] = SCR(S_GRANDPA),       [SCR_DOG_IN] = SCR(S_DOG_IN),
    [SCR_DOG_DOWN] = SCR(S_DOG_DOWN),     [SCR_SCHOOL] = SCR(S_SCHOOL),
    [SCR_FATTY] = SCR(S_FATTY),           [SCR_EXAM_IN] = SCR(S_EXAM_IN),
    [SCR_EXAM_DOWN] = SCR(S_EXAM_DOWN),   [SCR_MARKET] = SCR(S_MARKET),
    [SCR_GRANDPA_MARKET] = SCR(S_GRANDPA_MARKET),
    [SCR_KING_IN] = SCR(S_KING_IN),       [SCR_KING_DOWN] = SCR(S_KING_DOWN),
    [SCR_ENDING] = SCR(S_ENDING),         [SCR_LOSE] = SCR(S_LOSE),
    [SCR_SHOP] = SCR(S_SHOP),             [SCR_NOTHING] = SCR(S_NOTHING),
    [SCR_CH2_OPEN] = SCR(S_CH2_OPEN),     [SCR_PARK] = SCR(S_PARK),
    [SCR_SHOP_KING] = SCR(S_SHOP_KING),   [SCR_GP_ASK] = SCR(S_GP_ASK),
    [SCR_GP_TEACH] = SCR(S_GP_TEACH),     [SCR_GP_AFTER] = SCR(S_GP_AFTER),
    [SCR_SKEWER_IN] = SCR(S_SKEWER_IN),   [SCR_SKEWER_DOWN] = SCR(S_SKEWER_DOWN),
    [SCR_FAIR] = SCR(S_FAIR),             [SCR_DOG_FREE] = SCR(S_DOG_FREE),
    [SCR_FATTY2] = SCR(S_FATTY2),         [SCR_CANDY_IN] = SCR(S_CANDY_IN),
    [SCR_CANDY_DOWN] = SCR(S_CANDY_DOWN), [SCR_ENDING2] = SCR(S_ENDING2),
};

const rpg_script_info_t *rpg_script(rpg_script_t s) {
    return (unsigned)s < SCR_COUNT ? &SCRIPTS[s] : NULL;
}

// ---------------------------------------------------------------------------
// 宝箱和 NPC 给的东西

static const rpg_loot_t LOOTS[] = {
    { ITEM_SPICY, 1, 0 },     // 0 小区草丛
    { ITEM_SODA, 2, 0 },      // 1 学校图书馆
    { ITEM_DUMPLING, 3, 0 },  // 2 菜市场冷库
    { ITEM_DUMPLING, 2, 0 },  // 3 爷爷(小花园)
    { ITEM_SODA, 1, 0 },      // 4 小胖
    { ITEM_DUMPLING, 2, 30 }, // 5 爷爷(菜市场)
    { ITEM_COUNT, 0, 25 },    // 6 菜市场货车下面捡到零花钱(打完怪掉的)
    { ITEM_DUMPLING, 3, 0 },  // 7 公园钓鱼台
    { ITEM_SODA, 2, 0 },      // 8 公园长廊
    { ITEM_COUNT, 0, 40 },    // 9 公园花丛(打完气球怪捡的)
    { ITEM_SPICY, 2, 0 },     // 10 游乐园的小胖
    { ITEM_SODA, 3, 60 },     // 11 游乐园糖果仓库(小黄闻出来的隐藏地点)
    { ITEM_COUNT, 0, 50 },    // 12 游乐园舞台(打完跳跳糖捡的)
};

const rpg_loot_t *rpg_loot(uint8_t id) {
    return id < LEN(LOOTS) ? &LOOTS[id] : NULL;
}

// ---------------------------------------------------------------------------
// 地图。坐标是地图区域里的位置(240 × 236)。Boss 地点都是路的尽头,不会顺路撞上。

#define X RPG_NO_NODE
#define NONE 0xFF

static const rpg_node_t HOME[] = {
    { NODE_REST,    40,  20, SCR_REST_HOME, NONE, { 1, 3, X, X }, 0 },  // 0 家
    { NODE_NPC,     40,  88, SCR_GRANDPA, 3,      { 0, 2, X, X }, 0 },  // 1 小花园
    { NODE_BATTLE,  40, 156, MON_CAT, NONE,       { 1, 6, X, X }, 0 },  // 2 沙坑
    { NODE_SHOP,   160,  20, 0, NONE,             { 0, 4, X, X }, 0 },  // 3 便利店
    { NODE_BATTLE, 160,  92, MON_MOSQUITO, NONE,  { 3, 5, 6, X }, 0 },  // 4 楼道
    { NODE_CHEST,  210, 160, 0, 0,                { 4, X, X, X }, 0 },  // 5 草丛
    { NODE_BATTLE, 124, 168, MON_BROOM, NONE,     { 2, 4, 7, X }, 0 },  // 6 车棚
    { NODE_BOSS,   124, 224, MON_DOG, NONE,       { 6, X, X, X }, 0 },  // 7 保安亭
};

static const rpg_node_t SCHOOL[] = {
    { NODE_REST,   120,  16, SCR_REST, NONE,      { 1, 2, 4, X }, 0 },  // 0 校门
    { NODE_BATTLE,  36,  64, MON_ROPE, NONE,      { 0, 3, X, X }, 0 },  // 1 操场
    { NODE_BATTLE, 204,  64, MON_HOMEWORK, NONE,  { 0, 5, X, X }, 0 },  // 2 教室
    { NODE_SHOP,    36, 136, 0, NONE,             { 1, 6, X, X }, 0 },  // 3 小卖部
    { NODE_BATTLE, 120, 104, MON_CLOCK, NONE,     { 0, 7, X, X }, 0 },  // 4 走廊
    { NODE_NPC,    204, 140, SCR_FATTY, 4,        { 2, X, X, X }, 0 },  // 5 小胖
    { NODE_CHEST,   36, 206, 0, 1,                { 3, 7, X, X }, 0 },  // 6 图书馆
    { NODE_BATTLE, 120, 170, MON_CHALK, NONE,     { 4, 6, 8, X }, 0 },  // 7 黑板报
    { NODE_BOSS,   204, 222, MON_EXAM, NONE,      { 7, X, X, X }, 0 },  // 8 考场
};

static const rpg_node_t MARKET[] = {
    { NODE_REST,    30,  16, SCR_REST, NONE,      { 1, 3, X, X }, 0 },  // 0 入口
    { NODE_BATTLE, 116,  22, MON_PEPPER, NONE,    { 0, 2, 4, X }, 0 },  // 1 青椒摊
    { NODE_BATTLE, 208,  34, MON_ONION, NONE,     { 1, 5, X, X }, 0 },  // 2 洋葱堆
    { NODE_SHOP,    30,  92, 0, NONE,             { 0, 4, 6, X }, 0 },  // 3 菜摊
    { NODE_BATTLE, 120,  88, MON_TOFU, NONE,      { 1, 3, 7, X }, 0 },  // 4 臭豆腐
    { NODE_BATTLE, 208, 110, MON_BITTER, NONE,    { 2, 8, X, X }, 0 },  // 5 苦瓜架
    { NODE_NPC,     30, 168, SCR_GRANDPA_MARKET, 5, { 3, 7, X, X }, 0 }, // 6 爷爷
    { NODE_CHEST,  120, 160, 0, 2,                { 4, 6, 8, 9 }, 0 },  // 7 冷库
    { NODE_BATTLE, 208, 184, MON_PEPPER, 6,       { 5, 7, X, X }, 0 },  // 8 货车
    { NODE_BOSS,   120, 224, MON_KING, NONE,      { 7, X, X, X }, 0 },  // 9 魔王厨房
};

// 第 2 章。公园的商店 arg 是剧本(挑食魔王的小摊),别的商店 arg 0 = 卖菜阿姨。
static const rpg_node_t PARK[] = {
    { NODE_REST,    30,  16, SCR_REST, NONE,      { 1, 3, X, X }, 0 },  // 0 门口
    { NODE_BATTLE, 120,  24, MON_BALLOON, NONE,   { 0, 2, 4, X }, 0 },  // 1 草地
    { NODE_BATTLE, 208,  40, MON_CARP, NONE,      { 1, 5, X, X }, 0 },  // 2 池塘
    { NODE_SHOP,    30,  96, SCR_SHOP_KING, NONE, { 0, 4, 6, X }, 0 },  // 3 小卖亭
    { NODE_BATTLE, 120, 100, MON_SPEAKER, NONE,   { 1, 3, 7, X }, 0 },  // 4 广场(爷爷支线)
    { NODE_CHEST,  208, 116, 0, 7,                { 2, 8, X, X }, 0 },  // 5 钓鱼台
    { NODE_NPC,     30, 176, SCR_GP_ASK, NONE,    { 3, 7, X, X }, 0 },  // 6 爷爷
    { NODE_BATTLE, 120, 170, MON_BALLOON, 9,      { 4, 6, 8, 9 }, 0 },  // 7 花丛
    { NODE_CHEST,  208, 188, 0, 8,                { 5, 7, X, X }, 0 },  // 8 长廊
    { NODE_BOSS,   120, 224, MON_SKEWER, NONE,    { 7, X, X, X }, 0 },  // 9 后门
};

static const rpg_node_t FAIR[] = {
    { NODE_REST,    30,  16, SCR_REST, NONE,      { 1, 2, X, X }, 0 },  // 0 大门
    { NODE_BATTLE, 120,  24, MON_POP, NONE,       { 0, 3, 4, X }, 0 },  // 1 小火车
    { NODE_SHOP,    30, 100, 0, NONE,             { 0, 5, X, X }, 0 },  // 2 气球摊
    { NODE_BATTLE, 208,  44, MON_GUM, NONE,       { 1, 7, X, X }, 0 },  // 3 口香糖池(救小黄)
    { NODE_BATTLE, 120, 104, MON_ICE, NONE,       { 1, 5, 6, X }, 0 },  // 4 冰淇淋车
    { NODE_NPC,     30, 180, SCR_FATTY2, 10,      { 2, 4, X, X }, 0 },  // 5 小胖
    { NODE_BATTLE, 140, 172, MON_POP, 12,         { 4, 8, X, X }, 0 },  // 6 舞台
    { NODE_CHEST,  208, 130, 0, 11,               { 3, X, X, X }, 1 + RXB_SECRET },  // 7 糖果仓库(隐藏)
    { NODE_BOSS,   140, 226, MON_CANDY, NONE,     { 6, X, X, X }, 0 },  // 8 大王的家
};

static const char *const N_HOME[] = RT_NAMES_HOME;
static const char *const N_SCHOOL[] = RT_NAMES_SCHOOL;
static const char *const N_MARKET[] = RT_NAMES_MARKET;
_Static_assert(sizeof(N_HOME) / sizeof(N_HOME[0]) == sizeof(HOME) / sizeof(HOME[0]), "home names");
_Static_assert(sizeof(N_SCHOOL) / sizeof(N_SCHOOL[0]) == sizeof(SCHOOL) / sizeof(SCHOOL[0]),
               "school names");
_Static_assert(sizeof(N_MARKET) / sizeof(N_MARKET[0]) == sizeof(MARKET) / sizeof(MARKET[0]),
               "market names");
static const char *const N_PARK[] = RT_NAMES_PARK;
static const char *const N_FAIR[] = RT_NAMES_FAIR;
_Static_assert(sizeof(N_PARK) / sizeof(N_PARK[0]) == sizeof(PARK) / sizeof(PARK[0]), "park names");
_Static_assert(sizeof(N_FAIR) / sizeof(N_FAIR[0]) == sizeof(FAIR) / sizeof(FAIR[0]), "fair names");

static const rpg_area_t AREAS[RPG_AREAS_ALL] = {
    { HOME, N_HOME, LEN(HOME), 0, 7, { MON_CAT, MON_MOSQUITO, MON_BROOM }, 3, 4 },
    { SCHOOL, N_SCHOOL, LEN(SCHOOL), 0, 8, { MON_HOMEWORK, MON_CLOCK, MON_CHALK, MON_ROPE }, 4, 7 },
    { MARKET, N_MARKET, LEN(MARKET), 0, 9, { MON_PEPPER, MON_ONION, MON_TOFU, MON_BITTER }, 4, 9 },
    { PARK, N_PARK, LEN(PARK), 0, 9, { MON_BALLOON, MON_CARP, MON_SPEAKER }, 3, 11 },
    { FAIR, N_FAIR, LEN(FAIR), 0, 8, { MON_POP, MON_GUM, MON_ICE }, 3, 13 },
};

const rpg_area_t *rpg_area(uint8_t area) {
    return area < RPG_AREAS_ALL ? &AREAS[area] : NULL;
}

// ---------------------------------------------------------------------------
// 成长

// Lv1..Lv14 → 下一级要的累计经验;前 9 个是第 1 章(到 Lv10),之后是第 2 章
static const uint16_t NEED[RPG_MAX_LEVEL2 - 1] = { 20, 50, 90, 150, 230, 330, 460, 620, 800,
                                                   1000, 1250, 1550, 1900, 2300 };

uint16_t rpg_exp_for(uint8_t level) {
    return level >= 1 && level < RPG_MAX_LEVEL ? NEED[level - 1] : 0;
}

uint16_t rpg_exp_need(uint8_t level) {
    return level >= 1 && level < RPG_MAX_LEVEL2 ? NEED[level - 1] : 0;
}

uint16_t rpg_max_hp(uint8_t level) {
    return (uint16_t)(30u + 8u * (level - 1u));
}

uint8_t rpg_base_atk(uint8_t level) {
    return (uint8_t)(6u + 2u * (level - 1u));
}

uint8_t rpg_base_def(uint8_t level) {
    return (uint8_t)(2u + (level - 1u));
}

uint8_t rpg_max_sp(uint8_t level) {
    return (uint8_t)(5u + level / 2u);
}
