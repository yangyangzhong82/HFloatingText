#include "Entry/FloatingTextManager.h"
#include "Entry/Entry.h"
#include "PA/PlaceholderAPI.h" // 引入 PlaceholderAPI
#include "ll/api/coro/CoroTask.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ServerThreadExecutor.h"
#include "logger.h"
#include "mc/world/level/Level.h"
#include "mc/world/actor/player/Player.h" // 引入 Player 头文件
#include "debug_shape/api/shape/IDebugText.h"
#include "debug_shape/api/IDebugShapeDrawer.h"


#include <chrono>
#include <iomanip> // For std::put_time
#include <sstream> // For std::ostringstream

namespace HFloatingText {

FloatingTextManager::FloatingTextManager() : mRunning(false) {}

FloatingTextManager::~FloatingTextManager() {
    stopAllDynamicTextUpdates();
    // 清除所有 DebugText 实例
    mDebugTexts.clear();
}

FloatingTextManager& FloatingTextManager::getInstance() {
    static FloatingTextManager instance;
    return instance;
}

std::string
FloatingTextManager::getDynamicTextContent(const std::string& name, const FloatingTextData& data, Player* player) {
    std::string baseText = data.text;

    if (name == "time_text") {
        auto    now       = std::chrono::system_clock::now();
        auto    in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf;
        localtime_s(&tm_buf, &in_time_t); // Use localtime_s for thread safety on Windows

        std::ostringstream oss;
        oss << "当前时间: " << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        baseText = oss.str();
    }

    // 使用 PlaceholderAPI 替换占位符
 auto paService = PA::PA_GetPlaceholderService();
if (paService) {
    if (player) {
        auto ctx = PA::PlayerContext::factory(player);
        return paService->replace(baseText, ctx.get());
    } else {
        return paService->replaceServer(baseText);
    }
}
return baseText;
}

ll::coro::CoroTask<> FloatingTextManager::updateDynamicTextTask(
    std::string        name,
    FloatingTextData   data, // 复制数据，因为协程可能在数据管理器之外运行
    std::atomic<bool>& runningFlag
) {
    logger.debug("Dynamic text update task started for: {}", name);
    while (runningFlag) {
        // 获取 DebugText 对象
        if (!mDebugTexts.contains(name)) {
            // 如果不存在，则创建
            mDebugTexts[name] = debug_shape::IDebugText::create(data.pos, data.text);
            logger.debug("Created new IDebugText for: {}", name);
        }
        auto& debugText = mDebugTexts[name];

        if (!debugText) {
            logger.warn("Dynamic text {} is null, stopping update task.", name);
            break; // 如果 DebugText 不存在，则停止此任务
        }

        // 获取所有在线玩家
        auto level = ll::service::getLevel();
        if (level) {
            level->forEachPlayer([&](Player& player) {
                // 获取最新的文本内容，针对每个玩家
                std::string newText = getDynamicTextContent(name, data, &player);

                if (debugText->getText() != newText) { // 避免不必要的更新
                    debugText->setText(newText);
                    // 重新绘制以使更改生效，针对特定玩家
                    debug_shape::IDebugShapeDrawer::getInstance().drawShape(*debugText, player);
                }
                return true; // 继续遍历
            });
        } else {
            // 如果没有玩家，仍然更新服务器级文本
            std::string newText = getDynamicTextContent(name, data, nullptr);
            if (debugText->getText() != newText) { // 避免不必要的更新
                debugText->setText(newText);
                // 重新绘制以使更改生效
                debug_shape::IDebugShapeDrawer::getInstance().drawShape(*debugText);
                logger.debug("Updated dynamic text {} to: {}", name, newText);
            }
        }

        // 等待指定间隔
        if (data.interval.has_value() && data.interval.value() > 0) {
            co_await std::chrono::milliseconds(data.interval.value());
        } else {
            // 如果没有设置间隔或间隔为0，则默认等待1秒，防止无限循环
            co_await std::chrono::seconds(1);
        }
    }
    logger.debug("Dynamic text update task stopped for: {}", name);
    co_return;
}

void FloatingTextManager::startDynamicTextUpdate(const std::string& name, const FloatingTextData& data) {
    if (data.type != FloatingTextType::Dynamic) {
        logger.warn("Attempted to start dynamic update for static text: {}", name);
        return;
    }
    if (mDynamicTextTasks.contains(name)) {
        logger.warn("Dynamic text update task for {} is already running. Stopping existing task.", name);
        stopDynamicTextUpdate(name);
    }

    logger.debug("Starting dynamic text update for: {}", name);
    // 启动协程并存储其句柄
    auto task = ll::coro::keepThis(
        [this](std::string n, FloatingTextData d, std::atomic<bool>& flag) {
            return updateDynamicTextTask(std::move(n), std::move(d), flag);
        },
        name,
        data,
        std::ref(mRunning)
    );
    task.launch(ll::thread::ServerThreadExecutor::getDefault());
    mDynamicTextTasks.emplace(name, std::move(task));
}

void FloatingTextManager::stopDynamicTextUpdate(const std::string& name) {
    if (mDynamicTextTasks.contains(name)) {
        logger.debug("Stopping dynamic text update for: {}", name);
        // 协程的析构函数会自动销毁句柄，从而停止任务
        mDynamicTextTasks.erase(name);
        // 同时删除 DebugText 实例
        mDebugTexts.erase(name);
    } else {
        logger.debug("No dynamic text update task found for: {}", name);
    }
}

void FloatingTextManager::startAllDynamicTextUpdates() {
    if (mRunning) {
        logger.warn("All dynamic text updates are already running.");
        return;
    }
    mRunning = true;
    logger.debug("Starting all dynamic text updates...");
    auto& allFloatingTexts = DataManager::getInstance().getAllFloatingTexts();
    for (auto const& [name, data] : allFloatingTexts) {
        if (data.type == FloatingTextType::Dynamic) {
            startDynamicTextUpdate(name, data);
        }
    }
}

void FloatingTextManager::stopAllDynamicTextUpdates() {
    if (!mRunning) {
        logger.warn("All dynamic text updates are already stopped.");
        return;
    }
    mRunning = false; // 设置标志位，通知所有协程停止
    logger.debug("Stopping all dynamic text updates...");
    mDynamicTextTasks.clear(); // 清除所有任务，这将导致协程句柄被销毁
    mDebugTexts.clear();       // 清除所有 DebugText 实例
}

} // namespace HFloatingText
