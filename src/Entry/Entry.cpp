#include "Entry/Entry.h"
#include "Entry/Register.h"
#include "Entry/DataManager.h"
#include "gmlib/gm/floating_text/FloatingTextManager.h"
#include "gmlib/gm/floating_text/base/DynamicFloatingText.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mc/legacy/ActorRuntimeID.h"
#include <string>
#include <unordered_map>

namespace HFloatingText {

extern std::unordered_map<std::string, ActorRuntimeID> mFloatingTexts;

Entry& Entry::getInstance() {
    static Entry instance;
    return instance;
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
    registerCommands();
    return true;
}

bool Entry::disable() {
    getSelf().getLogger().debug("Disabling...");
    return true;
}

void Entry::reloadAllFloatingTexts() {
   mFloatingTexts.clear();
   gmlib::FloatingTextManager::getInstance().removeAll();

   auto& data = DataManager::getInstance().getAllFloatingTexts();
   for (auto const& [name, val] : data) {
       if (val.type == FloatingTextType::Static) {
           auto floatingText = gmlib::FloatingTextManager::getInstance().addStatic(val.text, val.pos, val.dimid);
           if (auto ptr = floatingText.lock()) {
               mFloatingTexts[name] = ptr->getRuntimeID();
           }
       } else {
           auto floatingText = gmlib::FloatingTextManager::getInstance().addDynamic(
               val.text,
               val.pos,
               val.dimid,
               val.interval.value_or(1000)
           );
           if (auto ptr = floatingText.lock()) {
               mFloatingTexts[name] = ptr->getRuntimeID();
               if (auto dynamicPtr = std::dynamic_pointer_cast<gmlib::DynamicFloatingText>(ptr)) {
                   dynamicPtr->startUpdate();
               }
           }
       }
   }
}

} // namespace HFloatingText

LL_REGISTER_MOD(HFloatingText::Entry, HFloatingText::Entry::getInstance());
