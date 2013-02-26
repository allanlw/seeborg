#ifndef __SEEBORG_IRC_H__
#define __SEEBORG_IRC_H__

#include <string>

#include "seeborg.h"

// Bot commands
// ---------------------------------------------------------------------------
std::string CMD_Shutup_f(class SeeBorg *self, const std::string command);
std::string CMD_Wakeup_f(class SeeBorg *self, const std::string command);

std::string CMD_Replyrate_f(class SeeBorg *self, const std::string command);
std::string CMD_Replynick_f(class SeeBorg *self, const std::string command);
std::string CMD_Replyword_f(class SeeBorg *self, const std::string command);

std::string CMD_Join_f(class SeeBorg *self, const std::string command);
std::string CMD_Part_f(class SeeBorg *self, const std::string command);
std::string CMD_Quit_f(class SeeBorg *self, const std::string command);
std::string CMD_Save_f(class SeeBorg *self, const std::string command);
std::string CMD_Learning_f(class SeeBorg *self, const std::string command);
std::string CMD_ircHelp_f(class SeeBorg *self, const std::string command);

std::string ircParseCommands(const std::string command, const char *who);

static const botcommand_t ircbotcmds[] = {
    {"help", "Show this command list", CMD_ircHelp_f},
    {"shutup", "As the name says", CMD_Shutup_f},
    {"wakeup", "As the name says", CMD_Wakeup_f},
    {"join", "Join channel", CMD_Join_f},
    {"part", "Part channel", CMD_Part_f},

    {"replyrate", "Show/set reply rate", CMD_Replyrate_f},
    {"replynick", "Show/set nick reply rate", CMD_Replynick_f},
    {"replymagic", "Show/set magic word reply rate", CMD_Replyword_f},

    {"quit", "As the name implies", CMD_Quit_f},
    {"save", "Immedeately save dictionary and settings", CMD_Save_f},

    {
        "learning",
        "Enable/disable bot's learning ability, should be enabled",
        CMD_Learning_f
    },

    {NULL, NULL, NULL}
};

static const int numircbotcmds = sizeof(ircbotcmds) / sizeof(ircbotcmds[0]) - 1;


// Bot Settings
// ---------------------------------------------------------------------------
void LoadBotSettings();
void SaveBotSettings();


#endif
