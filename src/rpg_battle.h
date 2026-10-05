// main/rpg_battle.h —— 「库巴大冒险」战斗:回合制 + 按时机。纯逻辑,可主机测试。
//
// 我方回合:菜单(拳头 / 技能 / 道具 / 逃跑)。
//   拳头、韭菜盒子、奶奶的拖鞋:力量条,指针扫过,● 按在好区 ×1.5、完美区 ×2,不按 ×0.5。
//   连环拳:限时狂按 ●。学霸光波:先看 4 个箭头,藏起来后凭记忆按出。
// 敌方回合:每一下先蓄力(提示高度)再出手,▼ 蹲 / ● 挡 / ▲ 跳。
//   出手后段按对 = 闪开(不掉血,勇气 +1);更早按对 = 挡住(减半);按错或不按 = 全中。只认第一下。
// 特殊:考试怪的选择题、挑食魔王的三阶段和「硬塞青椒」(狂按 ● 拒绝)、洋葱头藏起提示。
// 第 2 章:大音响的节拍(跟着拍子按 ●)、口香糖缠人 / 糖果大王让你牙疼(来回按 ▲▼)、
//   糖葫芦侠的糖球(血越少打得越少)、冰淇淋怕辣、爷爷的绝招(这一回合闪开就打回去)。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rpg_world.h"

#define RB_BAR_MS        1000
#define RB_BAR_GOOD_FROM 600   // ‰
#define RB_BAR_PERF_FROM 780
#define RB_BAR_PERF_TO   880
#define RB_BAR_GOOD_TO   950
#define RB_MASH_MS       1500
#define RB_MASH_MAX_HITS 8
#define RB_MEMO_LEN      4
#define RB_MEMO_SHOW_MS  1300
#define RB_MEMO_INPUT_MS 4000
#define RB_ACT_MS        700   // 我方出招结果停留
#define RB_HIT_MS        380   // 敌人每一下的结果停留
#define RB_DODGE_FROM    600   // 出手进度 ‰ 之后按对 = 闪开
#define RB_FEINT_PCT     30
#define RB_FLEE_PCT      70
#define RB_PET_PCT       30
#define RB_QUIZ_PCT      35
#define RB_QUIZ_MS       3500
#define RB_REFUSE_PCT    50
#define RB_REFUSE_MS     2000
#define RB_REFUSE_NEED   10
#define RB_RAIN_HITS     4     // 魔王二阶段「青椒雨」连打几下(出手快 15%);糖果大王的「糖果雨」一样
#define RB_PET_PLUS_PCT  40    // 救回来的小黄
#define RB_BEAT_N        4     // 节拍:几拍
#define RB_BEAT_LEAD_MS  700   // 第一拍
#define RB_BEAT_GAP_MS   600   // 拍子间隔
#define RB_BEAT_MS       2800  // 整段(最后一拍 2500,后面留出窗口)
#define RB_BEAT_WIN      160   // 拍子前后多少 ms 内按 ● 算对(中辣)
#define RB_BEAT_PCT      50    // 大音响放歌的回合
#define RB_WIGGLE_MS     2500  // 来回按
#define RB_WIGGLE_NEED   8     // 要交替按几下(中辣)
#define RB_STICKY_PCT    40    // 口香糖缠人的回合
#define RB_BRUSH_PCT     50    // 糖果大王第三阶段「牙疼」的回合
#define RB_SKEWER_BALLS  5     // 糖葫芦侠的糖球
#define RB_MELT_X10      25    // 冰淇淋怕辣:辣条加成 ×2.5(平时 ×1.5)

// 辣度(存档里的 heat,见 kit_heat.h)管的数:战斗开始时按档取一份。中辣就是上面这些常量。
typedef struct {
    uint8_t hp_pct;          // 怪物血 ×%
    uint8_t atk_pct;         // 怪物攻击 ×%(先打折再减防)
    uint16_t perf_from;      // 力量条完美区 ‰
    uint16_t perf_to;
    uint8_t feint_pct;       // 假动作概率
    uint8_t flee_pct;        // 逃跑成功率(非 Boss)
    uint8_t refuse_need;     // 魔王「硬塞青椒」要按几下
    uint16_t beat_win;       // 节拍窗口 ±ms
    uint8_t wiggle_need;     // 来回按要几下
} rpg_heat_rule_t;

const rpg_heat_rule_t *rpg_heat_rule(kit_heat_t h);

typedef enum {
    RB_MENU = 0,     // 主菜单
    RB_SKILLS,       // 技能列表(最后一项「返回」)
    RB_ITEMS,        // 道具列表(最后一项「返回」)
    RB_BAR,          // 力量条
    RB_MASH,         // 连环拳
    RB_MEMO_SHOW,    // 学霸光波:看
    RB_MEMO_INPUT,   // 学霸光波:按
    RB_ACT,          // 我方出招结果
    RB_WINDUP,       // 敌人蓄力
    RB_STRIKE,       // 敌人出手
    RB_HIT,          // 敌人这一下的结果
    RB_QUIZ,         // 选择题
    RB_REFUSE,       // 硬塞青椒
    RB_WON,
    RB_LOST,
    RB_FLED,
    RB_BEAT,         // 跟着节拍按 ●(大音响)。放在最后:状态行里的阶段编号不变
    RB_WIGGLE,       // 来回按 ▲▼(口香糖缠住 / 糖果大王:刷牙)
} rpg_bphase_t;

typedef enum {
    ACT_PUNCH = 0,
    ACT_COMBO,
    ACT_HEAL,
    ACT_SLIPPER,
    ACT_BEAM,
    ACT_ITEM,
    ACT_FLEE,
    ACT_MOVE,        // 爷爷的绝招:摆好架势,不出拳
} rpg_act_t;

typedef enum { GRADE_WEAK = 0, GRADE_GOOD, GRADE_PERFECT } rpg_grade_t;
typedef enum { OUT_HURT = 0, OUT_BLOCK, OUT_DODGE } rpg_outcome_t;
typedef enum { RB_KEY_UP = 0, RB_KEY_DOWN, RB_KEY_OK } rpg_key_t;

// 事件位(rpg_battle_tick / rpg_battle_input 返回)
#define BE_PHASE      (1u << 0)   // 换了阶段(界面整体刷新)
#define BE_CURSOR     (1u << 1)   // 菜单光标动了
#define BE_ENEMY_HURT (1u << 2)   // last_dmg
#define BE_HEAL       (1u << 3)   // last_heal
#define BE_WINDUP     (1u << 4)   // 敌人开始蓄力(lane)
#define BE_FEINT      (1u << 5)
#define BE_STRIKE     (1u << 6)
#define BE_DODGE      (1u << 7)
#define BE_BLOCK      (1u << 8)
#define BE_HURT       (1u << 9)   // 库巴掉血 last_dmg
#define BE_PET        (1u << 10)  // 小黄咬了一口(pet_dmg)
#define BE_WON        (1u << 11)
#define BE_LOST       (1u << 12)
#define BE_FLED       (1u << 13)
#define BE_FLEE_FAIL  (1u << 14)
#define BE_NO_FLEE    (1u << 15)  // Boss 战不能逃
#define BE_NO_SP      (1u << 16)
#define BE_TAP        (1u << 17)  // 狂按 / 光波按了一下
#define BE_QUIZ       (1u << 18)
#define BE_QUIZ_RIGHT (1u << 19)
#define BE_QUIZ_WRONG (1u << 20)
#define BE_STAGE      (1u << 21)  // 魔王进入下一阶段(stage)
#define BE_REFUSE     (1u << 22)
#define BE_REFUSE_OK  (1u << 23)
#define BE_SLIPPER    (1u << 24)  // 奶奶出场扔拖鞋
#define BE_GRADE      (1u << 25)  // 力量条 / 连打 / 光波出了结果(grade)
#define BE_SPICY      (1u << 26)  // 吃了辣条
#define BE_ITEM       (1u << 27)  // 用了道具(item)
#define BE_BEAT_OK    (1u << 28)  // 节拍:这一拍对了
#define BE_FREE       (1u << 29)  // 来回按够了:甩开了 / 刷干净了
#define BE_COUNTER    (1u << 30)  // 爷爷的绝招:闪开顺手打回去(counter_dmg)
#define BE_BEAT_MISS  (1u << 31)  // 节拍:这一拍错过了

typedef struct {
    rpg_save_t *s;           // 血、勇气、道具直接改存档里的
    rpg_mon_t mon;
    const rpg_mon_info_t *info;
    uint16_t ehp;
    uint16_t ehp_max;
    rpg_bphase_t phase;
    uint32_t t;              // 本阶段已过毫秒
    uint8_t sel;             // 菜单光标
    uint8_t act;             // rpg_act_t
    uint8_t item;            // ACT_ITEM 用的道具
    uint8_t grade;           // rpg_grade_t
    uint16_t last_dmg;
    uint16_t last_heal;
    uint16_t pet_dmg;
    uint8_t taps;            // 狂按次数 / 光波已按几个
    uint8_t memo[RB_MEMO_LEN];   // 光波:rpg_lane_t
    uint8_t memo_ok;
    uint8_t spicy;           // 辣条还剩几回合
    // 敌方回合
    uint8_t hit;             // 第几下
    uint8_t hits;            // 这回合一共几下
    uint8_t lane;            // 当前这一下(rpg_lane_t)
    uint8_t feint_to;
    uint16_t feint_at;       // 0 = 不假动作
    bool feinted;
    uint16_t windup_ms;
    uint16_t strike_ms;
    bool answered;           // 这一下已经按过
    uint8_t pend_key;        // 蓄力时就按了(rpg_key_t,0xFF = 没有):出手那一刻按最终招式判
    uint8_t outcome;         // rpg_outcome_t
    uint8_t stage;           // 魔王 / 糖果大王阶段 1..3;糖葫芦侠:剩下几颗糖球
    // 选择题
    uint8_t qa, qb;
    bool qmul;               // 乘法(否则加法)
    uint16_t qopt[3];        // ▲ ▼ ● 三个选项
    uint8_t qright;          // 正确的是第几个
    uint32_t rng;
    uint16_t perfects;       // 本场完美次数(出拳完美 + 闪开)
    uint8_t heat;            // kit_heat_t:开打时从存档取(夹过值),打完之前不变
    // 第 2 章
    uint8_t beat_hit;        // 节拍:哪几拍按对了(位)
    uint8_t beat_judged;     // 哪几拍已经有结果(对了或错过)
    uint8_t last_key;        // 来回按:上一下(rpg_key_t,0xFF = 还没按)
    bool counter;            // 爷爷的绝招:这一回合闪开就打回去
    uint16_t counter_dmg;
} rpg_battle_t;

void rpg_battle_start(rpg_battle_t *b, rpg_save_t *s, rpg_mon_t mon, uint32_t seed);
uint32_t rpg_battle_input(rpg_battle_t *b, rpg_key_t key);
uint32_t rpg_battle_tick(rpg_battle_t *b, uint32_t dt_ms);

// 菜单的项数和第 i 项是什么(技能 / 道具菜单的最后一项是「返回」,返回 -1)。
uint8_t rpg_battle_rows(const rpg_battle_t *b);
int rpg_battle_row(const rpg_battle_t *b, uint8_t i);
// 力量条 / 出手进度 0..1000。
uint32_t rpg_battle_progress(const rpg_battle_t *b);
// 节拍:第 k 拍在本阶段的第几毫秒。
uint32_t rpg_beat_ms(int k);
// 这一下敌人招式对应的正确按键。
rpg_key_t rpg_lane_key(rpg_lane_t lane);
bool rpg_battle_over(const rpg_battle_t *b);
