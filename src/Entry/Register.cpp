#include "Entry/Register.h"
#include "gmlib/gm/floating_text/FloatingTextManager.h"
#include "gmlib/gm/floating_text/base/StaticFloatingText.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/Overload.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/legacy/ActorRuntimeID.h"
#include "mc/server/commands/Command.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPositionFloat.h"
#include "mc/world/level/dimension/Dimension.h"
#include <string>
#include <unordered_map>
#include "logger.h"
#include "gmlib/gm/floating_text/base/DynamicFloatingText.h"
#include "ll/api/io/FileUtils.h"
#include "mc/server/commands/CommandPosition.h" // 命令位置参数类型
#include "Entry/DataManager.h"
#include "Entry/Entry.h"

#include "mc/legacy/ActorRuntimeID.h"
template <>
struct fmt::formatter<ActorRuntimeID> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ActorRuntimeID& id, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}", id.rawID);
    }
};

namespace HFloatingText {

struct CreateCommand {
    std::string          name;
    std::string          text;
    int                  dimid;
    CommandPositionFloat pos;
};
struct CreateDynamicCommand {
    std::string          name;
    std::string          text;
    int                  dimid;
    CommandPositionFloat pos;
    int                  interval;
};
struct EditCommand {
    std::string name;
    std::string text;
};
struct DeleteCommand {
    std::string name;
};

std::unordered_map<std::string, ActorRuntimeID> mFloatingTexts;

void editFloatingText(const CommandOrigin& origin, CommandOutput& output, const EditCommand& param) {
    logger.debug("Editing floating text: name={}, text={}", param.name, param.text);
    if (!mFloatingTexts.contains(param.name)) {
        output.error("Floating text with this name does not exist.");
        logger.debug("Floating text with name {} does not exist.", param.name);
        return;
    }

    auto runtimeId = mFloatingTexts[param.name];
    logger.debug("Found runtimeId {} for name {}.", runtimeId, param.name);
    auto floatingText = gmlib::FloatingTextManager::getInstance().get(runtimeId);
    if (auto ptr = floatingText.lock()) {
        ptr->setText(param.text);

        auto& data = DataManager::getInstance().getAllFloatingTexts();
        if (data.contains(param.name)) {
            data[param.name].text = param.text;
            DataManager::getInstance().save();
        }

        output.success("Floating text updated.");
        logger.debug("Successfully updated floating text with name {}.", param.name);
    } else {
        output.error("Failed to find floating text to update.");
        logger.warn("Could not find floating text with runtimeId {}, removing from map.", runtimeId);
        mFloatingTexts.erase(param.name);
        DataManager::getInstance().removeFloatingText(param.name);
    }
}

void deleteFloatingText(const CommandOrigin& origin, CommandOutput& output, const DeleteCommand& param) {
    logger.debug("Deleting floating text: name={}", param.name);
    if (!mFloatingTexts.contains(param.name)) {
        output.error("Floating text with this name does not exist.");
        logger.debug("Floating text with name {} does not exist.", param.name);
        return;
    }

    auto runtimeId = mFloatingTexts[param.name];
    logger.debug("Found runtimeId {} for name {}.", runtimeId, param.name);
    if (gmlib::FloatingTextManager::getInstance().remove(runtimeId)) {
        mFloatingTexts.erase(param.name);
        DataManager::getInstance().removeFloatingText(param.name);
        output.success("Floating text deleted.");
        logger.debug("Successfully deleted floating text with name {}.", param.name);
    } else {
        output.error("Failed to delete floating text.");
        logger.warn("Failed to delete floating text with runtimeId {}.", runtimeId);
    }
}


void registerCommands() {
    logger.debug("Registering HFloatingText commands...");
    auto& registrar = ll::command::CommandRegistrar::getInstance();
    auto& command = registrar.getOrCreateCommand("hft", "HFloatingText command");

    command.overload<CreateCommand>()
        .text("create")
        .required("name")
        .required("text")
        .required("dimid")
        .required("pos")
        .execute([&command](
                     const CommandOrigin& origin,
                     CommandOutput&       output,
                     const CreateCommand& param,
                     ::Command const&     cmd
                 ) {
            logger.debug(
                "Command 'create' executed: name={}, text={}, dimid={}",
                param.name,
                param.text,
                param.dimid
            );
            if (mFloatingTexts.contains(param.name)) {
                output.error("Floating text with this name already exists.");
                logger.debug("Floating text with name {} already exists.", param.name);
                return;
            }
            auto pos = param.pos.getPosition(cmd.mVersion, origin, Vec3::ZERO());
            logger.debug("Creating static floating text at position: ({}, {}, {})", pos.x, pos.y, pos.z);
            auto floatingText =
                gmlib::FloatingTextManager::getInstance().addStatic(param.text, pos, (DimensionType)param.dimid);
            if (auto ptr = floatingText.lock()) {
                auto runtimeId = ptr->getRuntimeID();
                mFloatingTexts[param.name] = runtimeId;
                DataManager::getInstance().addOrUpdateFloatingText(
                    param.name,
                    {param.text, pos, (DimensionType)param.dimid, FloatingTextType::Static, std::nullopt}
                );
                output.success("Floating text created.");
                logger.debug("Successfully created static floating text with name {} and runtimeId {}.", param.name, runtimeId);
            } else {
                output.error("Failed to create floating text.");
                logger.error("Failed to create static floating text with name {}.", param.name);
            }
        });

    command.overload<CreateDynamicCommand>()
        .text("createdynamic")
        .required("name")
        .required("text")
        .required("dimid")
        .required("pos")
        .required("interval")
        .execute([&command](
                     const CommandOrigin&         origin,
                     CommandOutput&               output,
                     const CreateDynamicCommand&  param,
                     ::Command const&             cmd
                 ) {
            logger.debug(
                "Command 'createdynamic' executed: name={}, text={}, dimid={}, interval={}",
                param.name,
                param.text,
                param.dimid,
                param.interval
            );
            if (mFloatingTexts.contains(param.name)) {
                output.error("Floating text with this name already exists.");
                logger.debug("Floating text with name {} already exists.", param.name);
                return;
            }
            auto pos = param.pos.getPosition(cmd.mVersion, origin, Vec3::ZERO());
            logger.debug("Creating dynamic floating text at position: ({}, {}, {})", pos.x, pos.y, pos.z);
            auto floatingText =
                gmlib::FloatingTextManager::getInstance().addDynamic(param.text, pos, (DimensionType)param.dimid, (uint)param.interval);
            if (auto ptr = floatingText.lock()) {
                auto runtimeId = ptr->getRuntimeID();
                mFloatingTexts[param.name] = runtimeId;
                DataManager::getInstance().addOrUpdateFloatingText(
                    param.name,
                    {param.text, pos, (DimensionType)param.dimid, FloatingTextType::Dynamic, param.interval}
                );
                if (auto dynamicPtr = std::dynamic_pointer_cast<gmlib::DynamicFloatingText>(ptr)) {
                    dynamicPtr->startUpdate();
                    logger.debug("Started update for dynamic floating text with name {}.", param.name);
                }
                output.success("Dynamic floating text created.");
                logger.debug("Successfully created dynamic floating text with name {} and runtimeId {}.", param.name, runtimeId);
            } else {
                output.error("Failed to create dynamic floating text.");
                logger.error("Failed to create dynamic floating text with name {}.", param.name);
            }
        });

    command.overload<EditCommand>()
        .text("edit")
        .required("name")
        .required("text")
        .execute([](const CommandOrigin& origin, CommandOutput& output, const EditCommand& param) {
            logger.debug("Command 'edit' executed, forwarding to editFloatingText.");
            editFloatingText(origin, output, param);
        });

    command.overload<DeleteCommand>()
        .text("delete")
        .required("name")
        .execute([](const CommandOrigin& origin, CommandOutput& output, const DeleteCommand& param) {
            logger.debug("Command 'delete' executed, forwarding to deleteFloatingText.");
            deleteFloatingText(origin, output, param);
        });

    command.overload()
        .text("reload")
        .execute([](const CommandOrigin& origin, CommandOutput& output) {
            Entry::getInstance().reloadAllFloatingTexts();
            output.success("All floating texts have been reloaded.");
        });
    logger.debug("HFloatingText commands registered.");
}

} // namespace HFloatingText
