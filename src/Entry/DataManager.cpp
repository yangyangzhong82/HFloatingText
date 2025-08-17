#include "Entry/DataManager.h"
#include "Entry/Entry.h"
#include "ll/api/io/FileUtils.h"
#include "mc/deps/core/math/Vec3.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>

namespace HFloatingText {

using json = nlohmann::json;

void to_json(json& j, const FloatingTextData& p) {
    j = json{
        {"text",     p.text},
        {"pos",      {{"x", p.pos.x}, {"y", p.pos.y}, {"z", p.pos.z}}},
        {"dimid",    (int)p.dimid},
        {"type",     p.type == FloatingTextType::Static ? "static" : "dynamic"}
    };
    if (p.type == FloatingTextType::Dynamic && p.interval.has_value()) {
        j["interval"] = p.interval.value();
    }
}

void from_json(const json& j, FloatingTextData& p) {
    j.at("text").get_to(p.text);
    j.at("pos").at("x").get_to(p.pos.x);
    j.at("pos").at("y").get_to(p.pos.y);
    j.at("pos").at("z").get_to(p.pos.z);
    p.dimid = (DimensionType)j.at("dimid").get<int>();
    std::string typeStr;
    j.at("type").get_to(typeStr);
    p.type = (typeStr == "static") ? FloatingTextType::Static : FloatingTextType::Dynamic;
    if (p.type == FloatingTextType::Dynamic && j.contains("interval")) {
        p.interval = j.at("interval").get<int>();
    }
}

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

DataManager::DataManager() {
    mFilePath = (Entry::getInstance().getSelf().getDataDir() / "floating_texts.json").string();
}

bool DataManager::load() {
    if (!std::filesystem::exists(mFilePath)) {
        return save(); // Create an empty file if it doesn't exist
    }

    std::ifstream file(mFilePath);
    if (!file.is_open()) {
        return false;
    }

    try {
        json j;
        file >> j;
        mFloatingTexts = j.get<std::unordered_map<std::string, FloatingTextData>>();
    } catch (const json::exception&) {
        return false;
    }

    return true;
}

bool DataManager::save() {
    try {
        std::filesystem::path filePath(mFilePath);
        if (!std::filesystem::exists(filePath.parent_path())) {
            std::filesystem::create_directories(filePath.parent_path());
        }
    } catch (const std::filesystem::filesystem_error&) {
        return false; // Failed to create directory
    }

    std::ofstream file(mFilePath);
    if (!file.is_open()) {
        return false;
    }

    try {
        json j = mFloatingTexts;
        file << j.dump(4);
    } catch (const json::exception&) {
        return false;
    }

    return true;
}

void DataManager::addOrUpdateFloatingText(const std::string& name, FloatingTextData data) {
    mFloatingTexts[name] = data;
    save();
}

void DataManager::removeFloatingText(const std::string& name) {
    if (mFloatingTexts.erase(name) > 0) {
        save();
    }
}

std::unordered_map<std::string, FloatingTextData>& DataManager::getAllFloatingTexts() { return mFloatingTexts; }

} // namespace HFloatingText