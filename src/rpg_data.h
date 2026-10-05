// main/rpg_data.h —— 「库巴大冒险」的数据表:怪物、技能、道具、地图、剧情。纯数据,可主机测试。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "kit_lines.h"

#define RPG_MAX_LEVEL  10    // 第 1 章的等级上限(主存档里的等级永远不超过它,老固件也能读)
#define RPG_MAX_LEVEL2 15    // 打通第 1 章以后的上限(第 2 章)
#define RPG_AREAS      3     // 第 1 章的地区数(rpg_save_t.done 的大小,存档结构不能变)
#define RPG_AREAS_ALL  5     // 加上第 2 章的公园、游乐园(进度在 rpg_ext_t)
#define RPG_MAX_NODES  10
#define RPG_MAX_LINKS  4     // 每个地点最多连几条路
#define RPG_NO_NODE    0xFF

// 敌人招式打的高度,和奶奶游戏一样:打头 ▼ 蹲,打身子 ● 挡,打脚 ▲ 跳。
typedef enum {
    RPG_LANE_HIGH = 0,
    RPG_LANE_MID,
    RPG_LANE_LOW,
    RPG_LANES,
} rpg_lane_t;

#define RPG_LANE_BIT(l) (1u << (l))

typedef enum {
    MON_CAT = 0,     // 小区
    MON_MOSQUITO,
    MON_BROOM,
    MON_DOG,         // 小区 Boss:大黄狗
    MON_HOMEWORK,    // 学校
    MON_CLOCK,
    MON_CHALK,
    MON_ROPE,
    MON_EXAM,        // 学校 Boss:期末考试怪
    MON_PEPPER,      // 菜市场
    MON_ONION,
    MON_TOFU,
    MON_BITTER,
    MON_KING,        // 第 1 章最终 Boss:挑食魔王
    MON_COUNT,       // 第 1 章的怪数。拖鞋传说的怪物图鉴按这个顺序对齐(card_app.c 的断言),不能变
    MON_BALLOON = MON_COUNT,  // 第 2 章 公园
    MON_CARP,
    MON_SPEAKER,
    MON_SKEWER,      // 公园 Boss:糖葫芦侠
    MON_POP,         // 游乐园:跳跳糖
    MON_GUM,
    MON_ICE,
    MON_CANDY,       // 第 2 章最终 Boss:糖果大王
    MON_ALL,         // 全部的怪(图鉴、数据表)
} rpg_mon_t;

#define MON_CH1_COUNT MON_COUNT  // 第 1 章的怪:图鉴位在主存档(seen / beaten),之后的在 rpg_ext_t

#define MONF_BOSS  (1u << 0)
#define MONF_FEINT (1u << 1)  // 蓄力中途可能换招
#define MONF_TEARS (1u << 2)  // 洋葱:不显示按键提示
#define MONF_QUIZ  (1u << 3)  // 考试怪:会出选择题
#define MONF_KING  (1u << 4)  // 魔王:三阶段
#define MONF_BEAT   (1u << 5)  // 大音响:有的回合「跟着节拍按 ●」
#define MONF_STICKY (1u << 6)  // 口香糖:有的回合缠住你,来回按 ▲▼ 甩开
#define MONF_SKEWER (1u << 7)  // 糖葫芦侠:每回合打的下数 = 剩下的糖球(血越少越少)
#define MONF_CANDY  (1u << 8)  // 糖果大王:三阶段,第三阶段「牙疼！快刷牙！」(来回按 ▲▼)
#define MONF_MELT   (1u << 9)  // 冰淇淋:怕辣,辣条加成更大

typedef struct {
    uint16_t hp;
    uint8_t atk;
    uint8_t def;
    uint8_t exp;
    uint8_t gold;
    uint16_t windup_ms;
    uint16_t strike_ms;
    uint8_t lanes;   // RPG_LANE_BIT 的组合
    uint8_t combo;   // 每回合连打几下
    uint8_t area;
    uint16_t flags;
} rpg_mon_info_t;

const rpg_mon_info_t *rpg_mon(rpg_mon_t m);

typedef enum {
    ITEM_DUMPLING = 0,  // 饺子:回血
    ITEM_SPICY,         // 辣条:3 回合攻击 ×1.5
    ITEM_SODA,          // 汽水:回勇气
    ITEM_COUNT,
} rpg_item_t;

#define RPG_ITEM_MAX      9    // 商店最多卖到这么多
#define RPG_ITEM_HOLD     99   // 背包最多装这么多:爷爷奶奶给的、宝箱里的可以超过商店上限,不会被吞
#define RPG_DUMPLING_HEAL 40
#define RPG_SODA_SP       5
#define RPG_SPICY_TURNS   3   // 接下来 3 次攻击 ×1.5

uint16_t rpg_item_price(rpg_item_t item);

typedef enum {
    SK_COMBO = 0,  // 连环拳:狂按 ●
    SK_HEAL,       // 韭菜盒子:回血
    SK_SLIPPER,    // 奶奶的拖鞋:无视防御
    SK_BEAM,       // 学霸光波:记住四个箭头
    SK_MOVE,       // 爷爷的绝招:这一回合闪开一下就打回去一下(第 2 章爷爷支线学会,不看等级)
    SK_COUNT,
} rpg_skill_t;

uint8_t rpg_skill_level(rpg_skill_t sk);  // 几级学会
uint8_t rpg_skill_cost(rpg_skill_t sk);

typedef enum {
    NODE_REST = 0,  // 休息点:回满血、存档
    NODE_BATTLE,
    NODE_CHEST,
    NODE_NPC,
    NODE_SHOP,
    NODE_BOSS,
} rpg_node_kind_t;

typedef enum {
    SPK_NONE = 0,  // 旁白
    SPK_KUBA,
    SPK_GRANNY,
    SPK_GRANDPA,
    SPK_DOG,
    SPK_FATTY,     // 同学小胖
    SPK_AUNT,      // 卖菜阿姨
    SPK_EXAM,
    SPK_KING,
    SPK_SKEWER,    // 糖葫芦侠
    SPK_CANDY,     // 糖果大王
    SPK_PUP,       // 小黄(成了伙伴以后;第 1 章的台词还是「大黄」)
    SPK_COUNT,
} rpg_speaker_t;

typedef enum {
    SCR_OPENING = 0,
    SCR_GIFT,
    SCR_REST_HOME,
    SCR_REST,
    SCR_GRANDPA,
    SCR_DOG_IN,
    SCR_DOG_DOWN,
    SCR_SCHOOL,
    SCR_FATTY,
    SCR_EXAM_IN,
    SCR_EXAM_DOWN,
    SCR_MARKET,
    SCR_GRANDPA_MARKET,
    SCR_KING_IN,
    SCR_KING_DOWN,
    SCR_ENDING,
    SCR_LOSE,
    SCR_SHOP,
    SCR_NOTHING,     // 已经来过的 NPC / 空宝箱
    // 第 2 章「奶奶的生日」
    SCR_CH2_OPEN,    // 开场(在家)
    SCR_PARK,        // 到公园
    SCR_SHOP_KING,   // 公园小卖亭:挑食魔王帮奶奶卖饺子
    SCR_GP_ASK,      // 公园的爷爷:求你打倒大音响(地图上的 NPC 用这个编号)
    SCR_GP_TEACH,    // 打倒大音响以后:讲和奶奶跳舞的往事,教绝招
    SCR_GP_AFTER,    // 以后每次:回满血
    SCR_SKEWER_IN,
    SCR_SKEWER_DOWN,
    SCR_FAIR,        // 到游乐园:小黄被口香糖拖走
    SCR_DOG_FREE,    // 口香糖池打赢:救回小黄
    SCR_FATTY2,      // 游乐园的小胖
    SCR_CANDY_IN,
    SCR_CANDY_DOWN,
    SCR_ENDING2,     // 奶奶的生日会
    SCR_COUNT,
} rpg_script_t;

typedef struct {
    uint8_t speaker;  // rpg_speaker_t
    const kit_line_t *voice;  // NULL = 不配音
    const char *text;
} rpg_line_t;

typedef struct {
    const rpg_line_t *lines;
    uint8_t n;
} rpg_script_info_t;

const rpg_script_info_t *rpg_script(rpg_script_t s);

// 宝箱 / NPC 给的东西
typedef struct {
    uint8_t item;   // rpg_item_t,ITEM_COUNT = 没有道具
    uint8_t count;
    uint16_t gold;
} rpg_loot_t;

typedef struct {
    uint8_t kind;   // rpg_node_kind_t
    uint8_t x;      // 地图上的位置(地图区域内坐标)
    uint8_t y;
    uint8_t arg;    // 战斗/Boss:怪物;NPC/休息点:剧本;宝箱/商店:不用
    uint8_t loot;   // 第一次完成这个地点给的东西(rpg_loot 编号),0xFF = 没有
    uint8_t links[RPG_MAX_LINKS];  // 相邻地点,RPG_NO_NODE 结尾
    uint8_t gate;   // 隐藏地点:0 = 一直看得见;否则 1 + 剧情位编号(RXB_*),那一位有了才出现
} rpg_node_t;

// 第 2 章的剧情位编号(rpg_ext_t.story 的第几位;rpg_world.h 的 RX_* 是对应的掩码)
enum { RXB_CH2 = 0, RXB_OPEN_PARK, RXB_OPEN_FAIR, RXB_SECRET };

typedef struct {
    const rpg_node_t *nodes;
    const char *const *names;  // 地点名
    uint8_t n;
    uint8_t start;    // 进入地区时站的地方(休息点)
    uint8_t boss;     // Boss 地点
    uint8_t mons[4];  // 刷级时随机遇到的小怪
    uint8_t mon_n;
    uint8_t boss_lv;  // Boss 地点显示的「建议等级」
} rpg_area_t;

const rpg_area_t *rpg_area(uint8_t area);
const rpg_loot_t *rpg_loot(uint8_t id);

// 升到 level+1 需要的累计经验;第 1 章的满级(RPG_MAX_LEVEL)返回 0。
uint16_t rpg_exp_for(uint8_t level);
// 同上,但一直排到 RPG_MAX_LEVEL2(通关第 1 章以后用);RPG_MAX_LEVEL2 返回 0。
uint16_t rpg_exp_need(uint8_t level);
uint16_t rpg_max_hp(uint8_t level);
uint8_t rpg_base_atk(uint8_t level);
uint8_t rpg_base_def(uint8_t level);
uint8_t rpg_max_sp(uint8_t level);
