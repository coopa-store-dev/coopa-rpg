// main/rpg_texts.h —— 「库巴大冒险」全部文案和剧本。
// ★ tools/gen_granny_fonts.sh 也从本文件提取字符生成字库;改文案后必须重新生成字库,
//   并运行 tests/test_granny_texts.py 检查字形覆盖。
// 剧本宏里的 RV(name) 由包含方定义(配音的台词,&LINE_<名字>),RV_NONE 表示不配音。
#pragma once

#define RT_TITLE          "库巴大冒险"
#define RT_SUBTITLE       "抢回奶奶的秘方！"
#define RT_CONTINUE       "继续冒险"
#define RT_NEW_GAME       "开始冒险"
#define RT_RESTART        "重新开始"
#define RT_RESTART_SURE   "再按 ● 确认重来"
#define RT_BESTIARY       "怪物图鉴"
#define RT_BACK_MENU      "回菜单"
#define RT_HEAT_FMT       "辣度：%s"        // 档名见 kit_texts.h
#define RT_CROWN          "已通关"
#define RT_SUBTITLE2      "第 2 章：抢回生日蛋糕！"   // 开始第 2 章以后的副标题
#define RT_CROWN2         "全部通关"

// 开机菜单上的卡片
#define RT_MENU_RPG       "库巴大冒险"
#define RT_MENU_RPG_SUB   "抢回奶奶的秘方"

#define RT_AREA_NAMES     { "小区", "学校", "菜市场", "公园", "游乐园" }
#define RT_NAMES_HOME     { "家", "小花园", "沙坑", "便利店", "楼道", "草丛", "车棚", "保安亭" }
#define RT_NAMES_SCHOOL   { "校门", "操场", "教室", "小卖部", "走廊", "小胖", "图书馆", "黑板报", \
                            "考场" }
#define RT_NAMES_MARKET   { "入口", "青椒摊", "洋葱堆", "菜摊", "臭豆腐", "苦瓜架", "爷爷", \
                            "冷库", "货车", "魔王厨房" }
#define RT_NAMES_PARK     { "门口", "草地", "池塘", "小卖亭", "广场", "钓鱼台", "爷爷", "花丛", \
                            "长廊", "后门" }
#define RT_NAMES_FAIR     { "大门", "小火车", "气球摊", "口香糖池", "冰淇淋车", "小胖", "舞台", \
                            "糖果仓库", "大王的家" }

#define RT_MON_NAMES      { "喵喵猫", "嗡嗡蚊", "扫把精", "大黄狗", "作业怪", "闹钟怪", "粉笔头", \
                            "跳绳蛇", "期末考试怪", "青椒兵", "洋葱头", "臭豆腐", "苦瓜侠", "挑食魔王", \
                            "气球怪", "鲤鱼精", "大音响", "糖葫芦侠", "跳跳糖", "口香糖", "冰淇淋", \
                            "糖果大王" }
#define RT_MON_NOTES      { "爪子挠头，很快", "嗡嗡两下", "专扫脚底板", "保安亭的看门狗", \
                            "一页一页砸过来", "叮铃铃三连", "从讲台飞过来", "专绊脚", \
                            "会出选择题", "辣！", "熏得你看不清", "臭到头晕", "又苦又硬", \
                            "偷走秘方的家伙", \
                            "飞来飞去，会骗人", "跳出水面，打头打脚", "要跟着节拍按 ●", \
                            "打掉一颗糖球，少打一下", "跳得飞快，三连", "缠住你就来回按 ▲▼", \
                            "怕辣！吃了辣条再打", "抢走生日蛋糕的家伙" }
#define RT_SPEAKERS       { "", "库巴", "奶奶", "爷爷", "大黄", "小胖", "卖菜阿姨", "考试怪", \
                            "挑食魔王", "糖葫芦侠", "糖果大王", "小黄" }

#define RT_ITEM_NAMES     { "饺子", "辣条", "汽水" }
#define RT_ITEM_NOTES     { "回 40 血", "接下来 3 次攻击 ×1.5", "回 5 勇气" }
#define RT_SKILL_NAMES    { "连环拳", "韭菜盒子", "奶奶的拖鞋", "学霸光波", "爷爷的绝招" }
#define RT_SKILL_NOTES    { "狂按 ● 连打", "回血", "无视防御", "记住 4 个箭头", "闪开一下，打回去一下" }

// 地图
#define RT_HP             "血"
#define RT_SP             "勇气"
#define RT_LV_FMT         "Lv%u"
#define RT_GOLD_FMT       "¥%u"
#define RT_TARGET_FMT     "→ %s · %s"
#define RT_TARGET_DONE_FMT "→ %s · %s · %s"
#define RT_BOSS_LV_FMT    "→ %s · Boss · 建议 Lv%u"
#define RT_MENU_TARGET    "【菜单】状态 / 道具 / 换地区"
#define RT_AREA_PICK      "去哪个地区？"
#define RT_NEXT           "● 继续"
#define RT_DOGGO          "小黄"
#define RT_KIND_NAMES     { "休息点", "有怪物", "宝箱", "有人", "商店", "Boss" }
#define RT_CLEARED        "已打过"
#define RT_OPENED         "已打开"
#define RT_CHEST_FMT      "打开宝箱：获得%s ×%u！"
#define RT_CHEST_GOLD_FMT "打开宝箱：获得 %u 零花钱！"
#define RT_ENCOUNTER      "有怪物冲出来了！"
#define RT_RESTED         "血量回满了，已存档。"
#define RT_GOT_FMT        "获得%s ×%u！"

// 地图上的【菜单】
#define RT_PAUSE_TITLE    "菜单"
#define RT_PAUSE_ROWS     { "吃饺子", "换地区", "回首页", "关闭" }
#define RT_STAT_FMT       "Lv%u  经验 %u/%u\n血 %u/%u  勇气 %u/%u\n攻击 %u  防御 %u\n零花钱 %u"
#define RT_STAT_MAX_FMT   "Lv%u  满级\n血 %u/%u  勇气 %u/%u\n攻击 %u  防御 %u\n零花钱 %u"
#define RT_ITEMS_FMT      "饺子 %u  辣条 %u  汽水 %u"
#define RT_HAS_DOG        "伙伴：小黄"
#define RT_DOG_LOST       "伙伴：小黄（被口香糖拖走了）"
#define RT_TITLE_DODGER   "称号：闪避高手"
#define RT_NO_DUMPLING    "没有饺子了"
#define RT_HP_FULL        "血是满的"
#define RT_ATE            "吃了饺子，回了血！"

// 商店
#define RT_SHOP_TITLE     "欢迎光临！"
#define RT_SHOP_ROW_FMT   "%s  ¥%u  (有 %u)"
#define RT_SHOP_LEAVE     "走了"
#define RT_SHOP_POOR      "零花钱不够"
#define RT_SHOP_FULL      "拿不下了"
#define RT_SHOP_BOUGHT    "买好了！"
#define RT_SHOP_NOTE_FMT  "\n%s：%s"
#define RT_NOTE_FMT       "\n→ %s"

// 战斗
#define RT_CMD_NAMES      { "拳头", "技能", "道具", "逃跑" }
#define RT_CMD_BACK       "返回"
#define RT_SKILL_ROW_FMT  "%s  勇气%u"
#define RT_ITEM_ROW_FMT   "%s ×%u"
#define RT_NO_SP          "勇气不够！"
#define RT_BAR_HINT       "看准了按 ●"
#define RT_MASH_HINT      "狂按 ●！"
#define RT_MEMORY_HINT    "记住顺序！"
#define RT_MEMORY_GO      "按出来！"
#define RT_PERFECT        "完美！"
#define RT_PERFECT_SHORT  "完美"
#define RT_DODGE_SHORT    "闪开了"
#define RT_STREAK_FMT     "%s ×%u！"   // 连续完美:「完美 ×3！」「闪开了 ×2！」
#define RT_GOOD           "不错！"
#define RT_WEAK           "没力气…"
#define RT_DODGE          "闪开了！"
#define RT_BLOCK          "挡住了！"
#define RT_FLED           "溜了溜了！"
#define RT_FLEE_FAIL      "跑不掉！"
#define RT_NO_FLEE        "Boss 战跑不掉！"
#define RT_SPICY_ON       "辣得冒火！攻击 UP"
#define RT_WIN            "胜利！"
#define RT_LOST           "累趴了…"
#define RT_REWARD_FMT     "经验 +%u  零花钱 +%u"
#define RT_LEVEL_UP_FMT   "升到 Lv%u 了！\n血 %u  攻击 %u  防御 %u"
#define RT_NEW_SKILL_FMT  "学会了「%s」！"
#define RT_QUIZ_TITLE     "选择题！"
#define RT_QUIZ_FMT       "%u %s %u = ?"
#define RT_QUIZ_RIGHT     "答对啦！"
#define RT_QUIZ_WRONG     "答错了！"
#define RT_REFUSE_TITLE   "不吃青椒！"
#define RT_REFUSE_OK      "拒绝成功！"
#define RT_PHASE2         "青椒雨！"
#define RT_PHASE3         "硬塞青椒！"
#define RT_HIDDEN_HINT    "看不清…"
#define RT_LANE_HINTS     { "▼ 蹲下", "● 挡住", "▲ 跳起" }   // 按 rpg_lane_t:打头、打身子、打脚
#define RT_KEY_NAMES      { "▲", "▼", "●" }                  // 按 rpg_key_t
#define RT_KEY_OK         "●"                                 // 对话框右下角:按 ● 下一句
#define RT_MEMO_DONE      "★"
#define RT_MEMO_TODO      "?"
#define RT_MUL            "×"
#define RT_ADD            "+"
#define RT_SPICY_TAG      "辣"
#define RT_SODA_POP       "+5 勇气"
// 第 2 章的战斗
#define RT_BEAT_HINT      "跟着节拍按 ●！"
#define RT_BEAT_DOT       "●"
#define RT_BEAT_OK        "★"
#define RT_BEAT_MISS      "×"
#define RT_STUCK_TITLE    "被缠住了！"
#define RT_BRUSH_TITLE    "牙疼！快刷牙！"
#define RT_WIGGLE_FMT     "▲▼ %u/%u"        // 来回按:按了几下 / 要几下
#define RT_FREE_OK        "甩开了！"
#define RT_BRUSH_OK       "刷干净了！"
#define RT_CANDY_PHASE2   "糖果雨！"
#define RT_CANDY_PHASE3   "牙疼了！"
#define RT_BALLS_FMT      "还剩 %u 颗糖球！"
#define RT_MOVE_GO        "爷爷的绝招！"
#define RT_COUNTER_FMT    "-%u"             // 绝招打回去(橙色,和普通出拳的红字分开)
#define RT_MELT           "冰淇淋怕辣！"

// 结局和图鉴
#define RT_END_TITLE      "第 1 章 完"
#define RT_END_STATS_FMT  "冒险用时 %u 分钟\n战斗 %u 场  完美 %u 次\n最终等级 Lv%u"
#define RT_END_THANKS     "谢谢你，勇敢的库巴！"
#define RT_END_HELP       "● 回首页，开始第 2 章"
#define RT_END2_TITLE     "生日快乐！"
#define RT_END2_THANKS    "奶奶的生日蛋糕抢回来啦！"
#define RT_END2_HELP      "● 回首页"
#define RT_BEST_TITLE     "怪物图鉴"
#define RT_BEST_LOCKED    "？？？"
#define RT_BEST_SEEN      "遇到过，还没打赢"
#define RT_BEST_FMT       "血 %u  攻 %u  防 %u"
#define RT_BEST_HELP      "▲▼ 翻看   ● 回去"
#define RT_BEST_INDEX_FMT "%d / %d   ★%d"

// 剧本:{ 说话的人, 配音, 台词 }
#define RT_SCR_OPENING { \
    { SPK_GRANNY, RV(RPG_OPEN), "库巴！不好了！奶奶的韭菜盒子秘方被偷走了！" }, \
    { SPK_KUBA, RV_NONE, "谁偷的？" }, \
    { SPK_GRANNY, RV_NONE, "挑食魔王！就是那个什么菜都不爱吃的家伙！" }, \
    { SPK_GRANNY, RV_NONE, "没有秘方，奶奶再也做不出韭菜盒子了……" }, \
    { SPK_KUBA, RV_NONE, "包在我身上！我这就去把秘方抢回来！" }, \
    { SPK_GRANNY, RV_NONE, "路上累了就回家，奶奶给你热饺子。" }, \
    { SPK_NONE, RV_NONE, "▲▼ 选要去的地方，● 走过去。选到【菜单】可以看状态。" }, \
}
#define RT_SCR_GIFT { \
    { SPK_GRANNY, RV(RPG_GIFT), "等等！你在奶奶那儿躲拖鞋，练得不错嘛！" }, \
    { SPK_GRANNY, RV_NONE, "这个包裹拿着：三个饺子，一瓶汽水。" }, \
    { SPK_NONE, RV_NONE, "获得称号「闪避高手」，防御 +2！" }, \
}
#define RT_SCR_REST_HOME { \
    { SPK_GRANNY, RV(RPG_REST), "回来啦？吃口饺子，歇一歇。" }, \
}
#define RT_SCR_REST { \
    { SPK_NONE, RV_NONE, "找了个地方坐下歇了一会儿。" }, \
}
#define RT_SCR_GRANDPA { \
    { SPK_GRANDPA, RV(RPG_GRANDPA), "库巴，听说你要去找挑食魔王？" }, \
    { SPK_GRANDPA, RV_NONE, "记住：看准了再躲。最后一刻躲开，一点都不疼。" }, \
    { SPK_GRANDPA, RV_NONE, "拿着，2 个饺子，路上吃。" }, \
}
#define RT_SCR_DOG_IN { \
    { SPK_DOG, RV(RPG_DOG), "汪！汪汪！" }, \
    { SPK_KUBA, RV_NONE, "大黄让一让，我要去学校！" }, \
    { SPK_DOG, RV_NONE, "汪呜——！（不让！）" }, \
}
#define RT_SCR_DOG_DOWN { \
    { SPK_DOG, RV_NONE, "呜……汪？" }, \
    { SPK_KUBA, RV_NONE, "原来你只是饿了啊。给你半个饺子。" }, \
    { SPK_DOG, RV(RPG_DOG), "汪汪！（使劲摇尾巴）" }, \
    { SPK_NONE, RV_NONE, "大黄成了你的伙伴「小黄」！战斗时它会帮你咬敌人。" }, \
    { SPK_NONE, RV_NONE, "学校开放了！" }, \
}
#define RT_SCR_SCHOOL { \
    { SPK_KUBA, RV_NONE, "挑食魔王的脚印……一直通到学校里。" }, \
}
#define RT_SCR_FATTY { \
    { SPK_FATTY, RV_NONE, "库巴！考场里有个期末考试怪，专出选择题！" }, \
    { SPK_FATTY, RV_NONE, "答对了它自己掉血，答错了就扣你的血。" }, \
    { SPK_FATTY, RV_NONE, "这瓶汽水给你，加油！" }, \
}
#define RT_SCR_EXAM_IN { \
    { SPK_EXAM, RV(RPG_EXAM), "哈哈哈！先把这张卷子做完再说！" }, \
}
#define RT_SCR_EXAM_DOWN { \
    { SPK_EXAM, RV_NONE, "满……满分？！不可能！" }, \
    { SPK_EXAM, RV_NONE, "魔王……在菜市场后面的厨房里……" }, \
    { SPK_NONE, RV_NONE, "菜市场开放了！" }, \
}
#define RT_SCR_MARKET { \
    { SPK_KUBA, RV_NONE, "好重的青椒味……魔王就在这里！" }, \
}
#define RT_SCR_GRANDPA_MARKET { \
    { SPK_GRANDPA, RV(RPG_GRANDPA2), "我就知道你会来！奶奶让我给你送吃的。" }, \
    { SPK_NONE, RV_NONE, "血量和勇气都回满了！" }, \
}
#define RT_SCR_KING_IN { \
    { SPK_KING, RV(RPG_KING), "韭菜？青椒？统统都难吃！" }, \
    { SPK_KING, RV_NONE, "秘方就在我这儿，有本事来拿！" }, \
}
#define RT_SCR_KING_DOWN { \
    { SPK_KING, RV(RPG_KING_DOWN), "呜……其实……我只是没吃过好吃的……" }, \
    { SPK_KUBA, RV_NONE, "那你跟我回家，尝尝奶奶的韭菜盒子！" }, \
}
#define RT_SCR_ENDING { \
    { SPK_GRANNY, RV(RPG_END), "秘方回来啦！我大孙子真棒！" }, \
    { SPK_GRANDPA, RV(RPG_END2), "来来来，一家人吃饭！" }, \
    { SPK_KING, RV(RPG_KING_END), "……真香！" }, \
    { SPK_DOG, RV_NONE, "汪！" }, \
}
#define RT_SCR_LOSE { \
    { SPK_NONE, RV_NONE, "库巴累趴了，被送回了休息点。东西一样没丢，血也回满了，再试一次！" }, \
}
#define RT_SCR_SHOP { \
    { SPK_AUNT, RV_NONE, "欢迎光临！要点什么？" }, \
}
#define RT_SCR_NOTHING { \
    { SPK_NONE, RV_NONE, "这里已经没什么了。" }, \
}

// ---------------------------------------------------------------------------
// 第 2 章「奶奶的生日」
#define RT_SCR_CH2_OPEN { \
    { SPK_GRANNY, RV_NONE, "库巴，明天是奶奶的生日！爷爷去公园给我取蛋糕了。" }, \
    { SPK_GRANDPA, RV_NONE, "不好了不好了！蛋糕被抢走了！" }, \
    { SPK_GRANDPA, RV_NONE, "一个全身都是糖的家伙，自称「糖果大王」！" }, \
    { SPK_KING, RV_NONE, "糖果大王？他只吃糖，不吃饭，比我以前还挑食！" }, \
    { SPK_KING, RV_NONE, "他的老窝就在公园后面的游乐园里。" }, \
    { SPK_KUBA, RV_NONE, "奶奶的生日蛋糕，包在我身上！" }, \
    { SPK_GRANNY, RV_NONE, "带上小黄，路上小心！" }, \
    { SPK_NONE, RV_NONE, "第 2 章「奶奶的生日」开始了！公园开放了！" }, \
}
#define RT_SCR_PARK { \
    { SPK_KUBA, RV_NONE, "公园里怎么这么热闹？到处都是糖果变的怪！" }, \
}
#define RT_SCR_SHOP_KING { \
    { SPK_KING, RV_NONE, "欢迎光临！我现在什么菜都吃，还帮奶奶卖饺子！" }, \
}
#define RT_SCR_GP_ASK { \
    { SPK_GRANDPA, RV_NONE, "库巴，广场上的大音响被糖果大王变成了怪物！" }, \
    { SPK_GRANDPA, RV_NONE, "奶奶生日那天，我还想和奶奶在广场上跳舞呢……" }, \
    { SPK_GRANDPA, RV_NONE, "你去把大音响打趴下，爷爷教你一个绝招！" }, \
}
#define RT_SCR_GP_TEACH { \
    { SPK_GRANDPA, RV_NONE, "音响好了！很多年前，我就是在这个广场上第一次和奶奶跳舞。" }, \
    { SPK_GRANDPA, RV_NONE, "那时候你奶奶跳得最好看，我老是跳错，被笑了好半天！" }, \
    { SPK_GRANDPA, RV_NONE, "看好了，这是爷爷的绝招：闪开的时候，顺手打回去！" }, \
    { SPK_NONE, RV_NONE, "学会了「爷爷的绝招」！用了以后，这一回合每闪开一下，就打回去一下。" }, \
}
#define RT_SCR_GP_AFTER { \
    { SPK_GRANDPA, RV_NONE, "绝招练得怎么样？来，喝口水，歇一歇。" }, \
    { SPK_NONE, RV_NONE, "血量和勇气都回满了！" }, \
}
#define RT_SCR_SKEWER_IN { \
    { SPK_SKEWER, RV_NONE, "要去游乐园？先吃我一根糖葫芦！" }, \
    { SPK_SKEWER, RV_NONE, "我有五颗糖球，一颗打你一下！" }, \
}
#define RT_SCR_SKEWER_DOWN { \
    { SPK_SKEWER, RV_NONE, "糖球……一颗都不剩了……" }, \
    { SPK_SKEWER, RV_NONE, "大王带着蛋糕，躲在游乐园最里面……" }, \
    { SPK_NONE, RV_NONE, "游乐园开放了！" }, \
}
#define RT_SCR_FAIR { \
    { SPK_KUBA, RV_NONE, "到处都是糖！小黄，别乱跑——" }, \
    { SPK_PUP, RV_NONE, "汪呜……！" }, \
    { SPK_NONE, RV_NONE, "一团口香糖把小黄缠住，拖走了！" }, \
    { SPK_KUBA, RV_NONE, "小黄！我这就去口香糖池找你！" }, \
    { SPK_NONE, RV_NONE, "没有小黄，打仗的时候就没人帮你咬了。" }, \
}
#define RT_SCR_DOG_FREE { \
    { SPK_PUP, RV(RPG_DOG), "汪汪汪！（使劲甩掉身上的口香糖）" }, \
    { SPK_KUBA, RV_NONE, "小黄，你没事吧？" }, \
    { SPK_NONE, RV_NONE, "小黄更勇敢了：咬得更重，也更常帮忙！" }, \
    { SPK_PUP, RV_NONE, "汪！（闻来闻去，往一边跑）" }, \
    { SPK_NONE, RV_NONE, "小黄闻到了好东西，地图上多了一个地方！" }, \
}
#define RT_SCR_FATTY2 { \
    { SPK_FATTY, RV_NONE, "库巴！我吃了好多糖，现在牙好疼……" }, \
    { SPK_FATTY, RV_NONE, "听说冰淇淋怪最怕辣，吃根辣条再打它！" }, \
    { SPK_FATTY, RV_NONE, "糖果大王快输的时候会让你牙疼，赶快来回按 ▲▼ 刷牙！" }, \
    { SPK_FATTY, RV_NONE, "这包辣条给你，我再也不吃那么多糖了！" }, \
}
#define RT_SCR_CANDY_IN { \
    { SPK_CANDY, RV_NONE, "嘿嘿，蛋糕上的奶油最好吃了！" }, \
    { SPK_CANDY, RV_NONE, "吃饭？吃菜？我只吃糖！你也一起来吃糖吧！" }, \
}
#define RT_SCR_CANDY_DOWN { \
    { SPK_CANDY, RV_NONE, "哎哟……牙好疼……原来天天吃糖，牙会坏掉……" }, \
    { SPK_KUBA, RV_NONE, "以后少吃糖，多吃饭，每天刷牙！" }, \
    { SPK_CANDY, RV_NONE, "蛋糕还给你……我能去奶奶的生日会吗？" }, \
}
#define RT_SCR_ENDING2 { \
    { SPK_GRANNY, RV(RPG_END), "蛋糕回来啦！还多了两个客人？" }, \
    { SPK_KING, RV_NONE, "糖果大王，快尝尝奶奶的韭菜盒子！" }, \
    { SPK_CANDY, RV_NONE, "……真香！比糖还好吃！" }, \
    { SPK_GRANDPA, RV(RPG_END2), "老伴，咱们去广场跳舞吧！" }, \
    { SPK_PUP, RV_NONE, "汪汪！" }, \
    { SPK_NONE, RV_NONE, "奶奶，生日快乐！" }, \
}
