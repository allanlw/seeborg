#include <fstream>
#include <cstring>

#include "seeborg-irc-settings.h"
#include "seeutil.h"

using namespace std;

BotSettings botsettings;

// Bot Config File
// ---------------------------------------------------------------------------

struct ConfigSetting {
    const char *configline;
    const char *description;

    string *stringptr;
    float *floatptr;
    int *intptr;
};

static const ConfigSetting configsettings[] = {
  {"server", "Address of IRC server", &botsettings.server, NULL, NULL},
  {"serverport", "Server port", NULL, NULL, &botsettings.serverport},

  {"nickname", "Bot's nickname", &botsettings.nickname, NULL, NULL},
  {"username", "Bot's username (will show as ~<username>@some.host.com)", &botsettings.username, NULL, NULL},
  {"realname", "Bot's realname (will show in whois)", &botsettings.realname, NULL, NULL},
  {"quitmessage", "Bot's quit message", &botsettings.quitmessage, NULL, NULL},

  {NULL, NULL, NULL, NULL, NULL},      // Newline in cfg

  {"replyrate", "Reply rate to all messages (in percent)", NULL, &botsettings.replyrate, NULL},
  {"replynick", "Reply rate to messages containing bot's nickname (in percent)", NULL, &botsettings.replyrate_mynick, NULL},
  {"replymagic", "Reply rate to messages containing magic words (in percent)", NULL, &botsettings.replyrate_magic, NULL},

  {NULL, NULL, NULL, NULL, NULL},      // Newline in cfg

  {"speaking", "Controls whether the bot speaks at all (boolean)", NULL, NULL, &botsettings.speaking},
  {"learning", "Does the bot learn, or just replies (boolean)", NULL, NULL, &botsettings.learning},
  {"stealth", "Try to emulate a popular IRC client's behaviour (TODO, boolean)", NULL, NULL, &botsettings.stealth},
  {"joininvites", "Join the channels the bot was invited to (0 - no, 1 - yes, 2 - only by owner)", NULL, NULL, &botsettings.joininvites},

  {NULL, NULL, NULL, NULL, NULL},      // Newline in cfg

  {"autosaveperiod", "Autosave period (in seconds)", NULL, NULL, &botsettings.autosaveperiod},

  {NULL, NULL, NULL, NULL, NULL}
};

static const int numconfigsettings =
    sizeof(configsettings) / sizeof(configsettings[0]) - 1;

void LoadBotSettings()
{
    string str;
    ifstream ifs("seeborg-irc.cfg");
    if (ifs.bad()) {
        return;
    }

    while (getline(ifs, str)) {
        trimString(str);
        if (str[0] == ';') {
            continue;
        }
        if (str[0] == '#') {
            continue;
        }
        if (str.empty()) {
            continue;
        }

        vector<string> cursetting;

        if (splitString(str, cursetting, "=") < 2) {
            continue;
        }

        trimString(cursetting[0]);
        trimString(cursetting[1]);
        if (!strcasecmp(cursetting[0].c_str(), "channels")) {
            vector<string> cursplit;
            if (!splitString(cursetting[1], cursplit, " ")) {
                continue;
            }
            botsettings.channels.clear();
            for (int i = 0, sz = cursplit.size(); i < sz; i++) {
                lowerString(cursplit[i]);
                botsettings.channels.insert(cursplit[i]);
            }
        }

        if (!strcasecmp(cursetting[0].c_str(), "owners")) {
            vector<string> cursplit;
            if (!splitString(cursetting[1], cursplit, " ")) {
                continue;
            }
            botsettings.owners.clear();
            for (int i = 0, sz = cursplit.size(); i < sz; i++) {
                IrcBotOwner ircbotowner;
                ircbotowner.nickname = cursplit[i];
                botsettings.owners.push_back(ircbotowner);
            }
        }

        if (!strcasecmp(cursetting[0].c_str(), "magicwords")) {
            vector<string> cursplit;
            if (!splitString(cursetting[1], cursplit, " ")) {
                continue;
            }
            botsettings.magicwords.clear();
            for (int i = 0, sz = cursplit.size(); i < sz; i++) {
                botsettings.magicwords.push_back(cursplit[i]);
            }
        }

        for (int i = 0; i < numconfigsettings; i++) {
            const ConfigSetting *s = &configsettings[i];
            if (s->configline == NULL) {
                continue;
            }
            if (!strcasecmp(s->configline, cursetting[0].c_str())) {
                if (s->stringptr != NULL) {
                    *s->stringptr = cursetting[1];
                } else if (s->floatptr != NULL) {
                    *s->floatptr = atof(cursetting[1].c_str());
                } else if (s->intptr != NULL) {
                    *s->intptr = atoi(cursetting[1].c_str());
                }
                break;
            }
        }
    }
    ifs.close();
}

void SaveBotSettings()
{
    FILE *f = fopen("seeborg-irc.cfg", "w");
    //  if (f == NULL) return;

    fprintf(f,
            "; SeeBorg " SEEBORGVERSIONSTRING
            " settings file\n; Lines beginning with ; or # are treated as comments\n\n\n");
    int i, sz;

    for (i = 0; i < numconfigsettings; i++) {
        const ConfigSetting *s = &configsettings[i];
        if (s->configline == NULL) {
            fprintf(f, "\n\n");
            continue;
        }

        fprintf(f, "; %s\n", s->description);
        if (s->stringptr != NULL) {
            fprintf(f, "%s = %s\n", s->configline,
                    (*s->stringptr).c_str());
        } else if (s->floatptr != NULL) {
            fprintf(f, "%s = %.2f\n", s->configline, *s->floatptr);
        } else if (s->intptr != NULL) {
            fprintf(f, "%s = %i\n", s->configline, *s->intptr);
        }
    }

    fprintf(f, "; Channel list to join to\n");
    fprintf(f, "channels =");

    set<string>::iterator it = botsettings.channels.begin();
    for (; it != botsettings.channels.end(); ++it) {
        fprintf(f, " %s", (*it).c_str());
    }
    fprintf(f, "\n");

    fprintf(f, "; Magic word list\n");
    fprintf(f, "magicwords =");
    for (i = 0, sz = botsettings.magicwords.size(); i < sz; i++) {
        fprintf(f, " %s", botsettings.magicwords[i].c_str());
    }
    fprintf(f, "\n");

    fprintf(f, "; Owner list (nicknames)\n");
    fprintf(f, "owners =");
    for (i = 0, sz = botsettings.owners.size(); i < sz; i++) {
        fprintf(f, " %s", botsettings.owners[i].nickname.c_str());
    }
    fprintf(f, "\n");


    fclose(f);
}
