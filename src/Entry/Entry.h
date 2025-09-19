#pragma once

#include "ll/api/mod/NativeMod.h"
#include "debug_shape/DebugText.h" // 引入 DebugText 头文件
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
    std::unordered_map<std::string, std::unique_ptr<debug_shape::DebugText>>& getDebugTexts();

    // TODO: Implement this method if you need to unload the mod.
    // /// @return True if the mod is unloaded successfully.
    // bool unload();

private:
    ll::mod::NativeMod& mSelf;
    std::unordered_map<std::string, std::unique_ptr<debug_shape::DebugText>> mDebugTexts; // 管理 DebugText 对象
};

} // namespace HFloatingText
