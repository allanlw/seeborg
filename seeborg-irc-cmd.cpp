#include "seeborg-irc-cmd.h"

#include <cstdlib>
#include <csignal>
#include <clocale>
#include <cctype>
#include <fstream>
#include <cstring>
#include <iostream>

#include "seeborg-irc.h"
#include "seeutil.h"

using namespace std;

static string CMD_Shutup_f(SeeBorg *self, const vector<string>& toks)
{
    if (!botsettings.speaking) {
        return "I'm already shut up *<:'-(";
    }

    botsettings.speaking = false;
    return "I'll shut up... :o";
}

static string CMD_Wakeup_f(SeeBorg *self, const vector<string>& toks)
{
    if (botsettings.speaking) {
        return "I'm already awake! :-D";
    }

    botsettings.speaking = true;
    return "Woohoo!";
}

static string CMD_Join_f(SeeBorg *self, const vector<string>& toks)
{
    if (toks.size() < 2) {
        return "Specify channels to join.";
    }

    for (int i = 1, sz = toks.size(); i < sz; i++) {
        cout << "Joining " << toks[i] << "...\n";
        irc_cmd_join(Session, toks[i].c_str(), NULL);
        botsettings.channels.insert(toks[i]);
    }

    return "Joined specified channels.";
}

static string CMD_Part_f(SeeBorg *self, const vector<string>& toks)
{
    if (toks.size() < 2) {
        return "Specify channels to leave.";
    }

    for (int i = 1, sz = toks.size(); i < sz; i++) {
        cout << "Leaving " << toks[i] << "...\n";
        irc_cmd_part(Session, toks[i].c_str());

        if (botsettings.channels.find(toks[i]) !=
                botsettings.channels.end()) {
            botsettings.channels.erase(botsettings.channels.find(toks[i]));
        }
    }

    return "Left specified channels.";
}

static string CMD_Replyrate_f(SeeBorg *self, const vector<string>& toks)
{
    char retstr[4096];
    if (toks.size() >= 2) {
        botsettings.replyrate = atof(toks[1].c_str());
    }

    snprintf(retstr, 4096, "Reply rate is set to %.1f%%",
             botsettings.replyrate);
    return retstr;
}

static string CMD_Replynick_f(SeeBorg *self, const vector<string>& toks)
{
    char retstr[4096];
    if (toks.size() >= 2) {
        botsettings.replyrate_mynick = atof(toks[1].c_str());
    }

    snprintf(retstr, 4096, "Reply rate to nickname is set to %.1f%%",
             botsettings.replyrate_mynick);
    return retstr;
}

static string CMD_Replyword_f(SeeBorg *self, const vector<string>& toks)
{
    char retstr[4096];
    if (toks.size() >= 2) {
        botsettings.replyrate_magic = atof(toks[1].c_str());
    }

    snprintf(retstr, 4096, "Reply rate to magic words is set to %.1f%%",
             botsettings.replyrate_magic);
    return retstr;
}

static string CMD_Learning_f(SeeBorg *self, const vector<string>& toks)
{
    if (toks.size() >= 2) {
        botsettings.learning = atoi(toks[1].c_str());
    }
    string retstr = "Learning is ";
    retstr += (botsettings.learning ? "enabled" : "disabled");
    return retstr;
}

static string CMD_Save_f(SeeBorg *self, const vector<string>& toks)
{
    cout << "Saving settings...\n";
    SaveBotSettings();
    self->SaveSettings();
    return "Settings saved.";
}


static const BotCommand ircbotcmds[] = {
    {"shutup", "As the name says (IRC)", CMD_Shutup_f},
    {"wakeup", "As the name says (IRC)", CMD_Wakeup_f},
    {"join", "Join channel (IRC)", CMD_Join_f},
    {"part", "Part channel (IRC)", CMD_Part_f},

    {"replyrate", "Show/set reply rate (IRC)", CMD_Replyrate_f},
    {"replynick", "Show/set nick reply rate (IRC)", CMD_Replynick_f},
    {"replymagic", "Show/set magic word reply rate (IRC)", CMD_Replyword_f},

    {"save", "Immedeately save dictionary and settings (IRC)", CMD_Save_f},

    {
        "learning",
        "Enable/disable bot's learning ability, should be enabled (IRC)",
        CMD_Learning_f
    },

    {NULL, NULL, NULL}
};

void InstallIrcCommands(SeeBorg *self) {
    self->AddCommands(ircbotcmds);
}
