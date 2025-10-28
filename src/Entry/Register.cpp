#include "Entry/Register.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/Overload.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/server/commands/Command.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPositionFloat.h"
#include "mc/world/level/dimension/Dimension.h"
#include <string>
#include <unordered_map>
#include <memory> 
#include "logger.h"
#include "ll/api/io/FileUtils.h"
#include "mc/server/commands/CommandPosition.h" 
#include "Entry/DataManager.h"
#include "Entry/Entry.h"

#include "debug_shape/DebugText.h"

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
    auto& debugTexts = Entry::getInstance().getDebugTexts();
    if (!debugTexts.contains(param.name)) {
        output.error("Floating text with this name does not exist.");
        logger.debug("Floating text with name {} does not exist.", param.name);
        return;
    }

    auto& debugText = debugTexts[param.name];
    debugText->setText(param.text);
    debug_shape::DebugShapeDrawer::drawShape(debugText.get()); // 立即更新显示

    auto& data = DataManager::getInstance().getAllFloatingTexts();
    if (data.contains(param.name)) {
        data[param.name].text = param.text;
        DataManager::getInstance().save();

        // 如果是动态文本，需要重新启动其更新任务以反映新文本
        if (data[param.name].type == FloatingTextType::Dynamic) {
            FloatingTextManager::getInstance().startDynamicTextUpdate(param.name, data[param.name]);
        }
    }

    output.success("Floating text updated.");
    logger.debug("Successfully updated floating text with name {}.", param.name);
}

void deleteFloatingText(const CommandOrigin& origin, CommandOutput& output, const DeleteCommand& param) {
    logger.debug("Deleting floating text: name={}", param.name);
    auto& debugTexts = Entry::getInstance().getDebugTexts();
    if (!debugTexts.contains(param.name)) {
        output.error("Floating text with this name does not exist.");
        logger.debug("Floating text with name {} does not exist.", param.name);
        return;
    }

    debugTexts.erase(param.name);
    DataManager::getInstance().removeFloatingText(param.name);
    FloatingTextManager::getInstance().stopDynamicTextUpdate(param.name); // 停止动态文本的更新任务
    output.success("Floating text deleted.");
    logger.debug("Successfully deleted floating text with name {}.", param.name);
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
            auto& debugTexts = Entry::getInstance().getDebugTexts();
            if (debugTexts.contains(param.name)) {
                output.error("Floating text with this name already exists.");
                logger.debug("Floating text with name {} already exists.", param.name);
                return;
            }
            auto pos = param.pos.getPosition(cmd.mVersion, origin, Vec3::ZERO());
            logger.debug("Creating static floating text at position: ({}, {}, {})", pos.x, pos.y, pos.z);
            
            auto debugText = std::make_unique<debug_shape::DebugText>(pos, param.text);
            debugText->draw(); // 绘制 DebugText
            debugTexts[param.name] = std::move(debugText); // 存储 DebugText 对象

            DataManager::getInstance().addOrUpdateFloatingText(
                param.name,
                {param.text, pos, (DimensionType)param.dimid, FloatingTextType::Static, std::nullopt}
            );
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
                "Command 'createdynamic' executed: name={}, text={}, dimid={}, interval={}",
                param.name,
                param.text,
                param.dimid,
                param.interval
            );
            auto& debugTexts = Entry::getInstance().getDebugTexts();
            if (debugTexts.contains(param.name)) {
                output.error("Floating text with this name already exists.");
                logger.debug("Floating text with name {} already exists.", param.name);
                return;
            }
            auto pos = param.pos.getPosition(cmd.mVersion, origin, Vec3::ZERO());
            logger.debug("Creating dynamic floating text at position: ({}, {}, {})", pos.x, pos.y, pos.z);
            
            // 对于动态文本，暂时只创建 DebugText，不处理动态更新逻辑
            auto debugText = std::make_unique<debug_shape::DebugText>(pos, param.text);
            debugText->draw(); // 绘制 DebugText
            debugTexts[param.name] = std::move(debugText); // 存储 DebugText 对象

            DataManager::getInstance().addOrUpdateFloatingText(
                param.name,
                {param.text, pos, (DimensionType)param.dimid, FloatingTextType::Dynamic, param.interval}
            );
            // 启动动态文本的更新任务
            FloatingTextManager::getInstance().startDynamicTextUpdate(
                param.name,
                {param.text, pos, (DimensionType)param.dimid, FloatingTextType::Dynamic, param.interval}
            );
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
