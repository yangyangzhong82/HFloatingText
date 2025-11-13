#pragma once

#include "Entry/DataManager.h"
#include "ll/api/coro/CoroTask.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "mc/deps/core/math/Vec3.h"
#include "debug_shape/api/shape/IDebugText.h"
#include "debug_shape/api/IDebugShapeDrawer.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>

namespace HFloatingText {

class FloatingTextManager {
private:
    std::unordered_map<std::string, ll::coro::CoroTask<>> mDynamicTextTasks;
    std::atomic<bool>                                      mRunning;

    // 存储 IDebugText 实例的映射
    std::unordered_map<std::string, std::unique_ptr<debug_shape::IDebugText>> mDebugTexts;

    FloatingTextManager();
    ~FloatingTextManager();

    // 协程任务函数，用于更新单个动态文本
    ll::coro::CoroTask<> updateDynamicTextTask(
        std::string name,
        FloatingTextData data,
        std::atomic<bool>& runningFlag
    );

public:
    static FloatingTextManager& getInstance();

    // 添加静态文本
    void addStaticText(const std::string& name, const FloatingTextData& data);

    // 移除文本（静态或动态）
    void removeText(const std::string& name);

    // 启动单个动态文本的更新
    void startDynamicTextUpdate(const std::string& name, const FloatingTextData& data);

    // 停止单个动态文本的更新
    void stopDynamicTextUpdate(const std::string& name);

    // 向指定玩家显示所有悬浮字
    void showAllTextsToPlayer(Player& player);

    // 加载并显示所有悬浮字
    void loadAndShowAllTexts();

    // 卸载所有悬浮字
    void unloadAllTexts();

    // 获取动态文本的当前内容 (示例，后续可扩展)
    std::string getDynamicTextContent(const std::string& name, const FloatingTextData& data, Player* player);
};

} // namespace HFloatingText
