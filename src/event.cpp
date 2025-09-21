#include "ll/api/event/EventBus.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/Dimension.h"

#include "Entry/Entry.h" // 引入 Entry.h
#include "debug_shape/DebugShapeDrawer.h" // 引入 DebugShapeDrawer.h
void registerPlayerConnectionListener() {
    ll::event::EventBus::getInstance().emplaceListener<ll::event::player::PlayerJoinEvent>(
        [](ll::event::player::PlayerJoinEvent& event) {
            auto& player = event.self();
            // 获取所有悬浮字，先移除旧的，再为加入的玩家绘制新的
            auto& debugTexts = HFloatingText::Entry::getInstance().getDebugTexts();
            for (auto const& [name, debugTextPtr] : debugTexts) {
                debug_shape::DebugShapeDrawer::removeShape(debugTextPtr.get(), player); // 移除旧的
                debug_shape::DebugShapeDrawer::drawShape(debugTextPtr.get(), player);   // 绘制新的
            }
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
    // 在玩家切换维度时重新绘制所有悬浮字
    auto& debugTexts = HFloatingText::Entry::getInstance().getDebugTexts();
    for (auto const& [name, debugTextPtr] : debugTexts) {
        debug_shape::DebugShapeDrawer::removeShape(debugTextPtr.get(), player); // 移除旧的
        debug_shape::DebugShapeDrawer::drawShape(debugTextPtr.get(), player);   // 绘制新的
    }
    return origin(player, std::move(changeRequest));
}
