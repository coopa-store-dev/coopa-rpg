// main/rpg_ui.h —— 「库巴大冒险」的画面。所有函数都必须在持有 bsp_lvgl_lock() 时调用。
// 屏幕:标题、地图、战斗、图鉴、结局;浮层(在 lv_layer_top 上,跨屏幕保留,引擎见 kit_overlay.h):
// 对话框、提示框、列表。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "rpg_battle.h"
#include "rpg_world.h"

// 标题页。crown:标题下的金色小牌(NULL = 不显示);sub:副标题。
// rows 为各行文字(最多 RPG_UI_TITLE_MAX 行,多的不显示),sel 为光标。
// 有存档时正好 5 行:继续 / 图鉴 / 重新开始 / 辣度 / 回菜单。再加一行就得改布局(行距已经压到 27)。
#define RPG_UI_TITLE_MAX 5
void rpg_ui_show_title(const char *crown, const char *sub, const char *const *rows, int n);
void rpg_ui_title_frame(uint32_t dt_ms, int sel);

// 地图。target:选中的相邻地点;RPG_UI_TARGET_MENU = 选中【菜单】;RPG_UI_TARGET_NONE = 不选
// (走路、对话、列表时)。walk_from / walk_ms:从哪里走过来、已经走了多久(0 = 没在走)。
#define RPG_UI_TARGET_MENU (-1)
#define RPG_UI_TARGET_NONE (-2)
void rpg_ui_show_map(const rpg_save_t *s);
void rpg_ui_map_frame(const rpg_save_t *s, uint32_t dt_ms, int target, int walk_from, uint32_t walk_ms);
#define RPG_UI_WALK_MS 450

// 战斗。rpg_ui_battle_pose:玩家刚在敌方回合按的键(rpg_key_t),用来摆姿势。
void rpg_ui_show_battle(const rpg_battle_t *b);
void rpg_ui_battle_frame(const rpg_battle_t *b, uint32_t ev, uint32_t dt_ms);
void rpg_ui_battle_pose(int key);
// 打赢后的结算条。
void rpg_ui_battle_result(const char *line1, const char *line2);

void rpg_ui_show_bestiary(const rpg_save_t *s);
void rpg_ui_bestiary_frame(uint32_t dt_ms, int sel);
#define RPG_UI_BEST_N MON_ALL

// 结局:ch2 = 第 2 章的生日会(否则第 1 章)。
void rpg_ui_show_ending(const rpg_save_t *s, bool ch2);
void rpg_ui_ending_frame(uint32_t dt_ms);

// 浮层
void rpg_ui_dialog(rpg_speaker_t spk, const char *text);  // 逐字显示
bool rpg_ui_dialog_typing(void);                         // 还在逐字显示
void rpg_ui_dialog_finish(void);                         // 一下子显示完
void rpg_ui_toast(const char *text);                     // 居中提示框(「● 继续」)
// 列表浮层:标题、说明(可多行)、各行、光标。n = 0 收起。
void rpg_ui_list(const char *title, const char *info, const char *const *rows, int n, int sel);
void rpg_ui_hide_overlays(void);
void rpg_ui_overlay_frame(uint32_t dt_ms);
// 离开大冒险:删掉浮层控件,还给 LVGL 池(下次用到再建)。
void rpg_ui_release(void);
