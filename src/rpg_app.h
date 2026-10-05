// main/rpg_app.h —— 「库巴大冒险」的总控:标题、地图、剧情、战斗、商店、菜单、结局,
// 以及存档(NVS 命名空间 "rpg")和声音。壳(main.c)只通过描述表 rpg_app(kit_app.h)调用,
// 都在游戏任务里、持有 LVGL 锁时。
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// 开机时调用一次:读存档。
void rpg_app_init(void);
// 从开机菜单进入:显示标题页。granny_best 为奶奶游戏最高分(决定开局有没有奶奶的包裹)。
void rpg_app_enter(uint32_t granny_best);
// 按键(PRESS):0 = ▲,1 = ▼,2 = ●。返回 true 表示玩家选了「回菜单」。
bool rpg_app_input(int key);
void rpg_app_frame(uint32_t dt_ms);
// 正在计时(走路、敌人出招、力量条等):别熄屏。等按键的时候不算。
bool rpg_app_busy(void);
// 快没电了:现在就把能存的存好(之后不会再调别的 rpg_app_*)。
// 打完一仗等着看结算:照按了 ● 结算存档;正在打或者马上要打:不存,开机回到走过去之前(和断电一样);
// 别的时候进度早就存过了,只补上游戏时间。
void rpg_app_save_now(void);
// 打通过(开机菜单上的卡片戴皇冠)。
bool rpg_app_won(void);
// 怪物图鉴里遇到过或打赢过的怪(位 = rpg_mon_t;拖鞋传说的怪物图鉴借来点亮,只读)。
uint16_t rpg_app_seen(void);
// 调试通道的状态行("RP ..." 含换行),返回长度。
size_t rpg_app_status(char *buf, size_t len);
// 调试:删掉大冒险存档(只给测试用)。
void rpg_app_erase(void);
// 调试:在地图上直接和第 mon 种怪打一场(先把存档拉到满级、配齐道具和小黄;只给测试用)。
void rpg_app_debug_fight(int mon);
