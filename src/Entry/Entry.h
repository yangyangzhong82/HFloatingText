#pragma once

#include "ll/api/mod/NativeMod.h"
#include "debug_shape/api/shape/IDebugText.h" // 引入 DebugText 头文件
#include "Entry/FloatingTextManager.h"
#include <string>
#include <unordered_map>
#include <memory> // For std::unique_ptr

namespace HFloatingText {

class Entry {

public:
    static Entry& getInstance();

    Entry() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    void reloadAllFloatingTexts();

    // 获取 DebugText 对象的管理器
    // 移除 getDebugTexts 方法，因为 FloatingTextManager 现在直接管理 DebugText 实例

    // TODO: Implement this method if you need to unload the mod.
    // /// @return True if the mod is unloaded successfully.
    // bool unload();

private:
    ll::mod::NativeMod& mSelf;
    // 移除 mDebugTexts，因为 FloatingTextManager 现在直接管理 DebugText 实例
};

} // namespace HFloatingText
