#ifndef __SEEBORG_IRC_SETTINGS_H__
#define __SEEBORG_IRC_SETTINGS_H__

#include <string>

#include "seeborg.h"

// Bot Settings
// ---------------------------------------------------------------------------
typedef struct ircbotowner_s {
    std::string nickname;
    std::string hostname;
} ircbotowner_t;

typedef struct botsettings_s {
    botsettings_s() {
        nickname = "SeeBorg";
        username = "SeeBorg";
        realname = "I am SeeBorg v" SEEBORGVERSIONSTRING;
        quitmessage = "Byebye...";
        replyrate = 1;
        replyrate_magic = 33;
        replyrate_mynick = 33;
        learning = true;
        speaking = true;
        joininvites = 2;	// 2: Only react to owners, 1: react to anyone

        // These are default channels
        channels.insert("#seeborg");
        channels.insert("#test");

        serverport = 6667;
        autosaveperiod = 600;
    }
    // IRC-specific
    std::string server;
    int serverport;
    std::string nickname;
    std::string username;
    std::string realname;
    std::set<std::string> channels;
    std::vector<ircbotowner_t> owners;
    std::string quitmessage;

    // Other settings
    float replyrate;
    int learning;


    int speaking;
    int stealth;		// TODO
    std::vector<std::string> censored;	// TODO
    std::vector<std::string> ignorelist;	// TODO
    int reply2ignored;		// TODO

    int joininvites;		// TODO
    float replyrate_mynick;
    float replyrate_magic;
    std::vector<std::string> magicwords;

    int autosaveperiod;

} botsettings_t;

extern botsettings_t botsettings;

void LoadBotSettings();
void SaveBotSettings();

#endif
