#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"

#include "Entry/Entry.h" // 引入 Entry.h
#include "Entry/FloatingTextManager.h" // 引入 FloatingTextManager.h
#include "debug_shape/api/IDebugShapeDrawer.h" // 引入 IDebugShapeDrawer.h
#include "debug_shape/api/shape/IDebugText.h" // 引入 IDebugText.h
void registerPlayerConnectionListener() {
    ll::event::EventBus::getInstance().emplaceListener<ll::event::player::PlayerJoinEvent>(
        [](ll::event::player::PlayerJoinEvent& event) {
            auto& player = event.self();
            // When a player joins, show all existing floating texts to them.
            HFloatingText::FloatingTextManager::getInstance().showAllTextsToPlayer(player);
        }
    );
}

LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerChangeDimensionHook2,       // Hook 名称
    ll::memory::HookPriority::Normal, // Hook 优先级
    Level,
    &Level::$requestPlayerChangeDimension,
    void,
    Player&                  player,
    ChangeDimensionRequest&& changeRequest
) {
    // When a player changes dimension, re-show all floating texts to them.
    HFloatingText::FloatingTextManager::getInstance().showAllTextsToPlayer(player);
    return origin(player, std::move(changeRequest));
}
