#include "Entry/Entry.h"
#include "Entry/Register.h"
#include "Entry/DataManager.h"
#include "ll/api/mod/RegisterHelper.h"
#include <string>
#include <unordered_map>
#include <memory> 
#include "debug_shape/api/shape/IDebugText.h" 
#include "debug_shape/api/IDebugShapeDrawer.h" 
#include "../event.h"
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
    if (!DataManager::getInstance().load()) {
        getSelf().getLogger().error("Failed to load floating text data!");
    }
    FloatingTextManager::getInstance().loadAndShowAllTexts(); // 加载并显示所有文本
    registerPlayerConnectionListener();
    registerCommands();
    return true;
}

bool Entry::disable() {
    getSelf().getLogger().debug("Disabling...");
    FloatingTextManager::getInstance().unloadAllTexts(); // 卸载所有文本
    return true;
}

void Entry::reloadAllFloatingTexts() {
    getSelf().getLogger().debug("Reloading all floating texts...");
    FloatingTextManager::getInstance().unloadAllTexts();
    // Re-load data from file
    if (DataManager::getInstance().load()) {
        FloatingTextManager::getInstance().loadAndShowAllTexts();
    } else {
        getSelf().getLogger().error("Failed to reload floating text data!");
    }
}

} // namespace HFloatingText

LL_REGISTER_MOD(HFloatingText::Entry, HFloatingText::Entry::getInstance());
