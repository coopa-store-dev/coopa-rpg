// main/rpg_world.h —— 「库巴大冒险」的世界:角色属性、背包、地图移动、地点事件、剧情进度。
// 纯逻辑(不依赖 ESP-IDF / LVGL),可主机测试。
//
// 存档:rpg_save_t(NVS rpg/save,36 B,孩子卡上就是这个,结构不能变)+ 第 2 章的 rpg_ext_t(rpg/ch2)。
// 内存里的状态是这一对:rpg_save_t 的 area 可以是第 2 章的 3/4、level 可以到 RPG_MAX_LEVEL2;
// 存进 NVS 前用 rpg_save_split 拆成「第 1 章看得懂的主存档」+ ext(老固件回退也能读主存档),
// 读出来用 rpg_save_join 合回去。要用到 ext 的函数(地区、地点完成位、图鉴、隐藏地点、第 2 章剧情)
// 通过 rpg_bind 绑定的那一对去找;没绑定的存档当作没有第 2 章(第 1 章的逻辑和以前一模一样)。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "kit_heat.h"
#include "rpg_data.h"

#define RPG_SAVE_VERSION 1
#define RPG_START_GOLD   20
#define RPG_GIFT_BEST    500   // 奶奶游戏最高分到这里,开局有奶奶的包裹
#define RPG_GIFT_DEF     2
#define RPG_ENCOUNTER_PCT 35   // 已经清空的地点(Boss 除外),路过遇到小怪的概率
#define RPG_ENCOUNTER_EASY_PCT 15  // 等级已经超过这个地区的建议等级(回头路过):少被打断
#define RPG_HOT_REWARD_PCT 120     // 特辣:打赢的经验和零花钱 ×1.2(怪又凶又厚,不补偿就追不上等级)

#define RF_GIFT (1u << 0)   // 收到奶奶的包裹(称号「闪避高手」)
#define RF_DOG  (1u << 1)   // 小黄是伙伴
#define RF_WON  (1u << 2)   // 打通了(第 1 章)
// 第 2 章(老固件不认识这几位,原样保留)。战斗要看的放在这里,战斗代码只用 rpg_save_t。
#define RF_MOVE     (1u << 3)   // 学会了爷爷的绝招
#define RF_DOG_LOST (1u << 4)   // 小黄被口香糖拖走了(不帮忙)
#define RF_DOG_PLUS (1u << 5)   // 救回来的小黄:咬得更常、更重
#define RF_WON2     (1u << 6)   // 第 2 章通关

// 第 2 章的剧情位(rpg_ext_t.story)
#define RX_CH2       (1u << RXB_CH2)        // 开始了(开场播过)
#define RX_OPEN_PARK (1u << RXB_OPEN_PARK)  // 公园开放
#define RX_OPEN_FAIR (1u << RXB_OPEN_FAIR)  // 游乐园开放
#define RX_SECRET    (1u << RXB_SECRET)     // 小黄闻出了糖果仓库(隐藏地点出现)

typedef struct {
    uint8_t version;
    uint8_t level;
    uint16_t exp;
    uint16_t hp;
    uint8_t sp;
    uint8_t flags;
    uint16_t gold;
    uint8_t items[ITEM_COUNT];
    uint8_t area;
    uint8_t node;
    uint8_t areas_open;          // 1..RPG_AREAS
    uint16_t done[RPG_AREAS];    // 每个地点一位:打过 / 开过 / 说过话
    uint16_t seen;               // 怪物图鉴:遇到过
    uint16_t beaten;             // 怪物图鉴:打赢过
    uint8_t heat;                // 辣度 kit_heat_t(原来的对齐填充,旧存档是 0 = 中辣;读时用 kit_heat_from 夹值)
    uint8_t pad;                 // 原来的对齐填充。新游戏是 0;开始第 2 章时取 1..255,和 rpg_ext_t.tag 配对
    uint32_t play_s;
    uint16_t battles;
    uint16_t perfects;
} rpg_save_t;
// 孩子卡上的存档就是这 36 字节:不能变大,也不能挪字段
_Static_assert(sizeof(rpg_save_t) == 36, "rpg save blob is 36 bytes");

// 第 2 章的进度(NVS rpg/ch2)。主存档里放不下的都在这里;只有开始了第 2 章才存。
typedef struct {
    uint8_t version;    // RPG_EXT_VERSION
    uint8_t tag;        // 等于主存档的 pad:老固件上「重新开始」会把 pad 清零,这份 ext 就不再认
    uint8_t area;       // 站在第 2 章的地区时 = 3/4(主存档里写的是小区的家);否则 0
    uint8_t node;
    uint16_t hp;        // 真实血量和勇气(Lv11 起主存档里夹到 Lv10 的上限)
    uint8_t sp;
    uint8_t pad;
    uint16_t story;     // RX_*
    uint16_t done[RPG_AREAS_ALL - RPG_AREAS];  // 公园、游乐园:每个地点一位
    uint16_t seen;      // 图鉴:第 2 章的怪(位 = 怪 - MON_CH1_COUNT)
    uint16_t beaten;
    uint8_t rsv[2];
    uint32_t sum;       // FNV-1a(前面所有字节)
} rpg_ext_t;
_Static_assert(sizeof(rpg_ext_t) == 24, "rpg ext blob is 24 bytes");
#define RPG_EXT_VERSION 1

typedef enum {
    EV_NONE = 0,     // 什么都没发生(已经清空的地点)
    EV_REST,         // 回满血;script 为台词
    EV_BATTLE,       // 这个地点的怪(第一次)
    EV_ENCOUNTER,    // 路过遇到的小怪
    EV_BOSS,         // script 为 Boss 开场白
    EV_CHEST,        // loot 已经给了
    EV_NPC,          // script;loot 已经给了(可能没有)
    EV_SHOP,
} rpg_event_kind_t;

typedef struct {
    uint8_t kind;    // rpg_event_kind_t
    uint8_t mon;     // EV_BATTLE / EV_ENCOUNTER / EV_BOSS
    uint8_t script;  // rpg_script_t,SCR_COUNT = 没有
    const rpg_loot_t *loot;  // 给了的东西,NULL = 没有
} rpg_event_t;

typedef struct {
    uint8_t levels;    // 升了几级
    uint8_t skills;    // 新学会的技能(位 = rpg_skill_t;一次连升几级可能学会好几个)
    int8_t area;       // 新开放的地区,-1 = 没有
    bool won_game;     // 打倒了挑食魔王
    bool won_ch2;      // 打倒了糖果大王(第 2 章通关)
    bool dog_free;     // 口香糖池打赢:小黄救回来了(隐藏地点也出现了)
    const rpg_loot_t *loot;  // 这个地点第一次打赢给的东西
} rpg_win_t;

typedef enum { BUY_OK = 0, BUY_POOR, BUY_FULL } rpg_buy_t;

// 新游戏。gift:奶奶游戏最高分够了,开局送包裹(防御 +2、3 饺子 1 汽水)。
void rpg_new_game(rpg_save_t *s, bool gift);
// 读出来的存档是否可用(版本、范围)。不看辣度:认不得的辣度当中辣,不判存档坏。
bool rpg_save_valid(const rpg_save_t *s);

uint16_t rpg_hp_max(const rpg_save_t *s);
uint8_t rpg_sp_max(const rpg_save_t *s);
uint8_t rpg_atk(const rpg_save_t *s);
uint8_t rpg_def(const rpg_save_t *s);
bool rpg_has_skill(const rpg_save_t *s, rpg_skill_t sk);
bool rpg_done(const rpg_save_t *s, uint8_t area, uint8_t node);

// 当前地点的相邻地点,返回个数。
uint8_t rpg_links(const rpg_save_t *s, uint8_t out[RPG_MAX_LINKS]);
// 走到相邻地点 node,返回那里发生的事(宝箱、NPC 的东西当场给)。node 不相邻时什么都不做。
rpg_event_t rpg_arrive(rpg_save_t *s, uint8_t node, uint32_t *rng);
// 记一笔「遇到过」(怪物图鉴)。
void rpg_meet(rpg_save_t *s, rpg_mon_t mon);
// 打赢 mon 能拿多少经验。等级还没到这个地区的入场等级(上一个地区 Boss 的建议等级)时 ×1.5,
// 帮落后的孩子追上设计的成长曲线(正常节奏的人拿不到)。特辣再 ×RPG_HOT_REWARD_PCT%。
uint16_t rpg_win_exp(const rpg_save_t *s, rpg_mon_t mon);
// 打赢 mon 能拿多少零花钱(特辣 ×RPG_HOT_REWARD_PCT%)。
uint16_t rpg_win_gold(const rpg_save_t *s, rpg_mon_t mon);
// 打赢了 mon。at_node:打的是当前地点的怪(第一次),不是路过遇到的。
rpg_win_t rpg_win(rpg_save_t *s, rpg_mon_t mon, bool at_node);
// 输了:回到本地区休息点,血和勇气回满,什么都不丢。
void rpg_lose(rpg_save_t *s);
// 加经验,返回升了几级(升级回满血)。
uint8_t rpg_gain_exp(rpg_save_t *s, uint16_t exp);
void rpg_give(rpg_save_t *s, const rpg_loot_t *loot);
rpg_buy_t rpg_buy(rpg_save_t *s, rpg_item_t item);
// 地图上吃饺子。血满或没饺子返回 false。
bool rpg_eat(rpg_save_t *s);
// 换到已开放的地区(站在休息点)。
bool rpg_change_area(rpg_save_t *s, uint8_t area);

// ---------------------------------------------------------------------------
// 第 2 章

// 绑定内存里的一对存档(s, x):下面这些函数遇到 s 时用 x。传 NULL 解绑。
void rpg_bind(rpg_save_t *s, rpg_ext_t *x);
// 空的 ext(没开始第 2 章)。
void rpg_ext_new(rpg_ext_t *x);
void rpg_ext_seal(rpg_ext_t *x);         // 算校验
bool rpg_ext_valid(const rpg_ext_t *x);  // 版本、校验、范围
// 存进 NVS 前:main 是第 1 章看得懂的主存档(等级 ≤ 10、血 / 勇气夹到 Lv10 上限、在第 2 章地区时写成小区的家),
// ext 是第 2 章进度(已经算好校验)。还没开始第 2 章时 main 和 s 逐字节相同。
void rpg_save_split(const rpg_save_t *s, const rpg_ext_t *x, rpg_save_t *main, rpg_ext_t *ext);
// 读出来以后:s 进来是(有效的)主存档,出去是合并好的内存状态;x 出去是第 2 章进度。
// loaded = 读到的 ext(NULL = 没有);校验 / 版本 / tag 不对、主存档没通关都当没有。
// 通关的存档按经验补升级(上限 RPG_MAX_LEVEL2)。
void rpg_save_join(rpg_save_t *s, rpg_ext_t *x, const rpg_ext_t *loaded);
// 打通了第 1 章、还没开始第 2 章(该播开场了)。要先 rpg_bind。
bool rpg_ch2_ready(const rpg_save_t *s);
// 开始第 2 章:公园开放,给这份存档配一个 tag。
void rpg_ch2_begin(rpg_save_t *s, uint32_t *rng);
uint8_t rpg_level_cap(const rpg_save_t *s);   // RPG_MAX_LEVEL,通关第 1 章以后 RPG_MAX_LEVEL2
uint16_t rpg_exp_next(const rpg_save_t *s);   // 升下一级要的累计经验;到上限返回 0
bool rpg_area_open(const rpg_save_t *s, uint8_t area);
uint16_t rpg_done_bits(const rpg_save_t *s, uint8_t area);
// 地点看不看得见(隐藏地点要剧情位;看不见的地点不在 rpg_links 里,也走不过去)。
bool rpg_node_shown(const rpg_save_t *s, uint8_t area, uint8_t node);
bool rpg_seen_mon(const rpg_save_t *s, rpg_mon_t mon);
bool rpg_beaten_mon(const rpg_save_t *s, rpg_mon_t mon);
// 小黄在不在身边帮忙(有伙伴、没被口香糖拖走)。
bool rpg_pet_active(const rpg_save_t *s);

// 已达成的里程碑(位 i = games/rpg/coopa.toml 的 milestones 第 i 条;没有存档时为 0)。纯函数。
uint8_t rpg_milestones_of(const rpg_save_t *s, bool has_save);
