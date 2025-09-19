#include "Entry/Entry.h"
#include "Entry/Register.h"
#include "Entry/DataManager.h"
#include "ll/api/mod/RegisterHelper.h"
#include <string>
#include <unordered_map>
#include <memory> 
#include "debug_shape/DebugText.h" 
#include "event.h" 
namespace HFloatingText {


Entry& Entry::getInstance() {
    static Entry instance;
    return instance;
}

std::unordered_map<std::string, std::unique_ptr<debug_shape::DebugText>>& Entry::getDebugTexts() {
    return mDebugTexts;
}

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
    registerPlayerConnectionListener();
    registerCommands();
    return true;
}

bool Entry::disable() {
    getSelf().getLogger().debug("Disabling...");
    // 清除所有 DebugText 对象
    mDebugTexts.clear();
    return true;
}

void Entry::reloadAllFloatingTexts() {
   mDebugTexts.clear(); // 清除所有现有的 DebugText 对象

   auto& data = DataManager::getInstance().getAllFloatingTexts();
   for (auto const& [name, val] : data) {
       auto debugText = std::make_unique<debug_shape::DebugText>(val.pos, val.text);
       debugText->draw(); // 绘制 DebugText
       mDebugTexts[name] = std::move(debugText); // 存储 DebugText 对象

       if (val.type == FloatingTextType::Dynamic) {
           getSelf().getLogger().warn("Dynamic floating text '{}' is not fully supported with DebugShape yet. Manual update logic needed.", name);
           // TODO: 实现动态文本的更新机制
       }
   }
}

} // namespace HFloatingText

LL_REGISTER_MOD(HFloatingText::Entry, HFloatingText::Entry::getInstance());
