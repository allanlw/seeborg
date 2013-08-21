#include "seeborg-irc-cmd.h"

#include <cstdlib>
#include <csignal>
#include <clocale>
#include <cctype>
#include <fstream>
#include <cstring>
#include <iostream>

#include "seeutil.h"

using namespace std;

static string CMD_Shutup_f(class SeeBorg *self, const string& command)
{
    if (!botsettings.speaking) {
        return "I'm already shut up *<:'-(";
    }

    botsettings.speaking = false;
    return "I'll shut up... :o";
}

static string CMD_Wakeup_f(class SeeBorg *self, const string& command)
{
    if (botsettings.speaking) {
        return "I'm already awake! :-D";
    }

    botsettings.speaking = true;
    return "Woohoo!";
}

static string CMD_Save_f(class SeeBorg *self, const string& command)
{
    cout << "Saving settings...\n";
    SaveBotSettings();
    gSeeBorg.SaveSettings();
    return "Settings saved.";
}

static string CMD_Join_f(class SeeBorg *self, const string& command)
{
    if (CMA_Argc() < 2) {
        return "Specify channels to join.";
    }

    for (int i = 1, sz = CMA_Argc(); i < sz; i++) {
        string channel = CMA_Argv(i);
        lowerString(channel);
        cout << "Joining " << channel << "...\n";
        BN_SendJoinMessage(&Info, CMA_Argv(i), NULL);
        botsettings.channels.insert(channel);
    }

    return "Joined specified channels.";
}


static string CMD_Part_f(class SeeBorg *self, const string& command)
{
    if (CMA_Argc() < 2) {
        return "Specify channels to leave.";
    }

    for (int i = 1, sz = CMA_Argc(); i < sz; i++) {
        string channel = CMA_Argv(i);
        cout << "Leaving " << channel << "...\n";
        BN_SendPartMessage(&Info, CMA_Argv(i), NULL);

        if (botsettings.channels.find(channel) !=
                botsettings.channels.end()) {
            botsettings.channels.erase(botsettings.channels.find(channel));
        }
    }

    return "Left specified channels.";
}


static string CMD_Replyrate_f(class SeeBorg *self, const string& command)
{
    char retstr[4096];
    if (CMA_Argc() >= 2) {
        botsettings.replyrate = atof(CMA_Argv(1));
    }

    snprintf(retstr, 4096, "Reply rate is set to %.1f%%",
             botsettings.replyrate);
    return retstr;
}

static string CMD_Replynick_f(class SeeBorg *self, const string& command)
{
    char retstr[4096];
    if (CMA_Argc() >= 2) {
        botsettings.replyrate_mynick = atof(CMA_Argv(1));
    }

    snprintf(retstr, 4096, "Reply rate to nickname is set to %.1f%%",
             botsettings.replyrate_mynick);
    return retstr;
}

static string CMD_Replyword_f(class SeeBorg *self, const string& command)
{
    char retstr[4096];
    if (CMA_Argc() >= 2) {
        botsettings.replyrate_magic = atof(CMA_Argv(1));
    }

    snprintf(retstr, 4096, "Reply rate to magic words is set to %.1f%%",
             botsettings.replyrate_magic);
    return retstr;
}

static string CMD_Learning_f(class SeeBorg *self, const string& command)
{
    if (CMA_Argc() >= 2) {
        botsettings.learning = atoi(CMA_Argv(1));
    }
    string retstr = "Learning is ";
    retstr += (botsettings.learning) ? "enabled" : "disabled";
    return retstr;
}

static string CMD_ircHelp_f(class SeeBorg *self, const string& command);

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

static string CMD_ircHelp_f(class SeeBorg *self, const string& command)
{
    string retstr;
    retstr = "IRC SeeBorg commands:\n";
    for (int i = 0; i < numircbotcmds; i++) {
        retstr += "!";
        retstr += ircbotcmds[i].command;
        retstr += ": ";
        retstr += ircbotcmds[i].description;
        retstr += "\n";
    }
    retstr += CMD_Help_f(self, command);

    return retstr;
}

string ircParseCommands(const string& cmd)
{
    if (cmd[0] != '!') {
        return "";
    }

    string command = cmd;
    lowerString(command);
    CMA_TokenizeString(command.c_str());
    for (int i = 0; i < numircbotcmds; i++) {
        if (!strncmp(CMA_Argv(0) + 1, ircbotcmds[i].command,
                 strlen(ircbotcmds[i].command))) {
            return ircbotcmds[i].func(&gSeeBorg, command);
        }
    }
    return "";
}
