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
            // 获取所有悬浮字，先移除旧的，再为加入的玩家绘制新的
            auto& floatingTextManager = HFloatingText::FloatingTextManager::getInstance();
            auto& allFloatingTexts    = HFloatingText::DataManager::getInstance().getAllFloatingTexts();

            for (auto const& [name, data] : allFloatingTexts) {
                if (data.type == HFloatingText::FloatingTextType::Static) {
                    // 静态文本直接绘制
                    auto debugText = debug_shape::IDebugText::create(data.pos, data.text);
                    if (debugText) {
                        debug_shape::IDebugShapeDrawer::getInstance().drawShape(*debugText, player);
                    }
                } else {
                    // 动态文本由 FloatingTextManager 管理，这里只需要确保更新任务已启动
                    floatingTextManager.startDynamicTextUpdate(name, data);
                }
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
    auto& floatingTextManager = HFloatingText::FloatingTextManager::getInstance();
    auto& allFloatingTexts    = HFloatingText::DataManager::getInstance().getAllFloatingTexts();

    for (auto const& [name, data] : allFloatingTexts) {
        if (data.type == HFloatingText::FloatingTextType::Static) {
            // 静态文本直接绘制
            auto debugText = debug_shape::IDebugText::create(data.pos, data.text);
            if (debugText) {
                debug_shape::IDebugShapeDrawer::getInstance().drawShape(*debugText, player);
            }
        } else {
            // 动态文本由 FloatingTextManager 管理，这里只需要确保更新任务已启动
            floatingTextManager.startDynamicTextUpdate(name, data);
        }
    }
    return origin(player, std::move(changeRequest));
}
