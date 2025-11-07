#include "Entry/Entry.h"
#include "Entry/Register.h"
#include "Entry/DataManager.h"
#include "ll/api/mod/RegisterHelper.h"
#include <string>
#include <unordered_map>
#include <memory> 
#include "debug_shape/api/shape/IDebugText.h" // 引入 DebugText 头文件
#include "debug_shape/api/IDebugShapeDrawer.h" // 引入 IDebugShapeDrawer 头文件
#include "event.h"
namespace HFloatingText {


Entry& Entry::getInstance() {
    static Entry instance;
    return instance;
}

// 移除 getDebugTexts 方法，因为 FloatingTextManager 现在直接管理 DebugText 实例

bool Entry::load() {
    getSelf().getLogger().debug("Loading...");
    return true;
}

bool Entry::enable() {
    getSelf().getLogger().debug("Enabling...");
    if (DataManager::getInstance().load()) {
        reloadAllFloatingTexts();
    } else {
        getSelf().getLogger().error("Failed to load floating text data!");
    }
    FloatingTextManager::getInstance().startAllDynamicTextUpdates(); // 启动所有动态文本更新
    registerPlayerConnectionListener();
    registerCommands();
    return true;
}

bool Entry::disable() {
    getSelf().getLogger().debug("Disabling...");
    FloatingTextManager::getInstance().stopAllDynamicTextUpdates(); // 停止所有动态文本更新
    return true;
}

void Entry::reloadAllFloatingTexts() {
   // 清除所有现有的 DebugText 对象 (由 FloatingTextManager 管理)
   FloatingTextManager::getInstance().stopAllDynamicTextUpdates();

   auto& data = DataManager::getInstance().getAllFloatingTexts();
   for (auto const& [name, val] : data) {
       // 静态文本直接绘制
       if (val.type == FloatingTextType::Static) {
           auto debugText = debug_shape::IDebugText::create(val.pos, val.text);
           if (debugText) {
               debug_shape::IDebugShapeDrawer::getInstance().drawShape(*debugText);
           }
       }
       // 动态文本的更新由 FloatingTextManager 管理
       FloatingTextManager::getInstance().startDynamicTextUpdate(name, val);
   }
}

} // namespace HFloatingText

LL_REGISTER_MOD(HFloatingText::Entry, HFloatingText::Entry::getInstance());
