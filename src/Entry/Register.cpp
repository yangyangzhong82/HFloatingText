#include "Entry/Register.h"
#include "Entry/DataManager.h"
#include "Entry/Entry.h"
#include "debug_shape/api/shape/IDebugText.h"
#include "debug_shape/api/IDebugShapeDrawer.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/Overload.h"
#include "ll/api/io/FileUtils.h"
#include "logger.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/server/commands/Command.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandPosition.h"
#include "mc/server/commands/CommandPositionFloat.h"
#include "mc/world/level/dimension/Dimension.h"
#include <memory>
#include <string>
#include <unordered_map>


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


void editFloatingText(const CommandOrigin& origin, CommandOutput& output, const EditCommand& param) {
    logger.debug("Editing floating text: name={}, text={}", param.name, param.text);

    auto& allTexts = DataManager::getInstance().getAllFloatingTexts();
    if (!allTexts.contains(param.name)) {
        output.error("Floating text with this name does not exist.");
        logger.debug("Floating text with name {} does not exist.", param.name);
        return;
    }

    auto& data = allTexts.at(param.name);
    data.text  = param.text;
    DataManager::getInstance().save();

    // Reload the text to apply changes
    FloatingTextManager::getInstance().removeText(param.name);
    if (data.type == FloatingTextType::Dynamic) {
        FloatingTextManager::getInstance().startDynamicTextUpdate(param.name, data);
    } else {
        FloatingTextManager::getInstance().addStaticText(param.name, data);
    }

    output.success("Floating text updated.");
    logger.debug("Successfully updated floating text with name {}.", param.name);
}

void deleteFloatingText(const CommandOrigin& origin, CommandOutput& output, const DeleteCommand& param) {
    logger.debug("Deleting floating text: name={}", param.name);

    auto& allTexts = DataManager::getInstance().getAllFloatingTexts();
    if (!allTexts.contains(param.name)) {
        output.error("Floating text with this name does not exist.");
        logger.debug("Floating text with name {} does not exist.", param.name);
        return;
    }

    DataManager::getInstance().removeFloatingText(param.name);
    FloatingTextManager::getInstance().removeText(param.name);
    output.success("Floating text deleted.");
    logger.debug("Successfully deleted floating text with name {}.", param.name);
}


void registerCommands() {
    logger.debug("Registering HFloatingText commands...");
    auto& registrar = ll::command::CommandRegistrar::getInstance();
    auto& command = registrar.getOrCreateCommand("hft", "HFloatingText command",CommandPermissionLevel::GameDirectors);

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
            auto& allTexts = DataManager::getInstance().getAllFloatingTexts();
            if (allTexts.contains(param.name)) {
                output.error("Floating text with this name already exists.");
                logger.debug("Floating text with name {} already exists.", param.name);
                return;
            }
            auto pos = param.pos.getPosition(cmd.mVersion, origin, Vec3::ZERO());
            logger.debug("Creating static floating text at position: ({}, {}, {})", pos.x, pos.y, pos.z);

            FloatingTextData newData{
                param.text,
                pos,
                (DimensionType)param.dimid,
                FloatingTextType::Static,
                std::nullopt};
            DataManager::getInstance().addOrUpdateFloatingText(param.name, newData);
            FloatingTextManager::getInstance().addStaticText(param.name, newData);

            output.success("Floating text created.");
            logger.debug("Successfully created static floating text with name {}.", param.name);
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
                "Command \'createdynamic\' executed: name={}, text={}, dimid={}, interval={}",
                param.name,
                param.text,
                param.dimid,
                param.interval
            );
            auto& allTexts = DataManager::getInstance().getAllFloatingTexts();
            if (allTexts.contains(param.name)) {
                output.error("Floating text with this name already exists.");
                logger.debug("Floating text with name {} already exists.", param.name);
                return;
            }
            auto pos = param.pos.getPosition(cmd.mVersion, origin, Vec3::ZERO());
            logger.debug("Creating dynamic floating text at position: ({}, {}, {})", pos.x, pos.y, pos.z);

            FloatingTextData newData{
                param.text,
                pos,
                (DimensionType)param.dimid,
                FloatingTextType::Dynamic,
                param.interval};
            DataManager::getInstance().addOrUpdateFloatingText(param.name, newData);
            FloatingTextManager::getInstance().startDynamicTextUpdate(param.name, newData);

            output.success("Dynamic floating text created.");
            logger.debug("Successfully created dynamic floating text with name {}.", param.name);
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
