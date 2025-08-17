#pragma once

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/level/dimension/Dimension.h"
#include <string>
#include <unordered_map>
#include <optional>

namespace HFloatingText {

enum class FloatingTextType { Static, Dynamic };

struct FloatingTextData {
    std::string        text;
    Vec3               pos;
    DimensionType      dimid;
    FloatingTextType   type;
    std::optional<int> interval; // Only for dynamic text
};

class DataManager {
public:
    static DataManager& getInstance();

    DataManager(const DataManager&)            = delete;
    DataManager(DataManager&&)                 = delete;
    DataManager& operator=(const DataManager&) = delete;
    DataManager& operator=(DataManager&&)      = delete;

    bool load();
    bool save();

    void addOrUpdateFloatingText(const std::string& name, FloatingTextData data);
    void removeFloatingText(const std::string& name);
    std::unordered_map<std::string, FloatingTextData>& getAllFloatingTexts();

private:
    DataManager();
    ~DataManager() = default;

    std::string                                        mFilePath;
    std::unordered_map<std::string, FloatingTextData> mFloatingTexts;
};

} // namespace HFloatingText