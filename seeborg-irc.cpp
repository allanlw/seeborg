#ifdef __unix__
#include <unistd.h>
#include <sys/wait.h>
#else
#include <time.h>
#ifdef _MSC_VER
#pragma comment (lib, "wsock32.lib")
#endif				// _MSC_VER
#endif				// __unix__

#include <csignal>
#include <clocale>
#include <cctype>
#include <cstring>
#include <iostream>

#include "seeborg.h"
#include "seeborg-irc.h"
#include "seeutil.h"
#include "seeborg-irc-settings.h"

using namespace std;

// Variables// ---------------------------------------------------------------------------
BN_TInfo Info;
seeborg_t gSeeBorg;
bool connected = false;
bool cleanedup = false;

// Hacky mackro to define functions that do extraction of the params
#define DefExtract(X) \
static inline string Extract ## X(const char *who) { \
    char buf[4096]; \
    buf[0] = '\0'; \
    BN_Extract ## X(who, buf, sizeof(buf)); \
    return string(buf); \
}
DefExtract(Host)
DefExtract(Nick)
DefExtract(ExactUserName)

// Message processing
// ---------------------------------------------------------------------------
static void checkOwners(const string& hostname, const string& nickname)
{
    for (int i = 0, sz = botsettings.owners.size(); i < sz; i++) {
        if (botsettings.owners[i].hostname.empty() &&
              equalIString(nickname, botsettings.owners[i].nickname)) {
            botsettings.owners[i].hostname = hostname;
            cout << "Locked owner '" << nickname << "' to '" << hostname << "'\n";
            return;
        }
    }
}

static bool isOwner(const string& hostname, const string& nickname)
{
    for (int i = 0, sz = botsettings.owners.size(); i < sz; i++) {
        if (equalIString(hostname, botsettings.owners[i].hostname) &&
              equalIString(nickname, botsettings.owners[i].nickname)) {
            return true;
        }
    }
    return false;
}


static string ProcessMessage(BN_PInfo I, const string &hostname,
        const string &nickname, const char msg[], bool replying = false)
{
    string stdmessage = msg;
    lowerString(stdmessage);

    checkOwners(hostname, nickname);

    if (stdmessage[0] == '!') {
        string stdcmdreply;
        if (!isOwner(hostname, nickname)) return "";

        stdcmdreply = ircParseCommands(stdmessage);
        if (stdcmdreply != "") return stdcmdreply;

        stdcmdreply = gSeeBorg.ParseCommands(stdmessage);
        if (stdcmdreply != "") return stdcmdreply;

        return "";
    }

    trimString(stdmessage);

    // Ignore quotes
    if (isdigit(stdmessage[0]) || stdmessage[0] == '<' ||
        stdmessage[0] == '[') {
        return "";
    }

    // only both to see if we should reply if we're talking
    if (botsettings.speaking) {
        if (randFloat(0, 99) < botsettings.replyrate) {
            replying = true;
        }

        if ((!replying) && (botsettings.replyrate_magic > 0)) {
            int sz = botsettings.magicwords.size();
            for (int i = 0; i < sz; i++) {
                if (stdmessage.find(botsettings.magicwords[i]) == string::npos)
                    continue;

                if (randFloat(0, 99) < botsettings.replyrate_magic) {
                    replying = true;
                } else {
                    break;
                }
            }
        }
        if ((!replying) && (botsettings.replyrate_mynick > 0)) {
            string nickname = I->Nick;
            lowerString(nickname);
            if (stdmessage.find(nickname) != string::npos &&
                randFloat(0, 99) < botsettings.replyrate_mynick) {
                replying = true;
            }
        }
    }

    string replystring;

    if ((replying) && (botsettings.speaking)) {
        replystring = gSeeBorg.Reply(stdmessage);
    }

    if (botsettings.learning) {
        gSeeBorg.Learn(stdmessage);
    }

    return replystring;
}


static void DoChanTalk(BN_PInfo I, const string& hostname, const string &nickname,
        const char *Msg, const char *Chan) {
    string reply = ProcessMessage(I, hostname, nickname, Msg);

    if (reply.empty()) return;

    vector<string> curlines;
    splitString(reply, curlines, "\n");
    for (int i = 0, sz = curlines.size(); i < sz; i++) {
        cout << "(" << Chan << ") <" << I->Nick << "> " << reply << "\n";
        BN_SendChannelMessage(I, Chan, curlines[i].c_str());
    }
}

// BotNet callback functions
// ---------------------------------------------------------------------------
static void ProcOnConnected(BN_PInfo I, const char HostName[])
{
    cout << "Connected to " << HostName << "%s...\n";
    BN_EnableFloodProtection(I, 1000, 1000, 60);
    connected = true;
    BN_Register(I, botsettings.nickname.c_str(),
                botsettings.username.c_str(),
                botsettings.realname.c_str());
}


static void ProcOnRegistered(BN_PInfo I)
{
    cout << "Registered...\n";
    set<string>::iterator it = botsettings.channels.begin();
    for (; it != botsettings.channels.end(); ++it) {
        const string &cname = *it;
        cout << "Joining " << cname << "...\n";
        BN_SendJoinMessage(I, cname.c_str(), NULL);
    }
}

// returned string is freed then, so we malloc() the return string each call
static char *ProcOnCTCP(BN_PInfo I, const char Who[], const char Whom[],
                 const char Type[])
{
    string nickname = ExtractNick(Who);
    cout << "CTCP " << Type << " query by " << nickname
         << " for " << Whom << "\n";

    if (!strcasecmp(Type, "VERSION")) {
        return strdup("mIRC32 v5.7 K.Mardam-Bey");
    } else {
        return strdup("Forget about it");
    }
}


static void ProcOnPingPong(BN_PInfo I)
{
    static time_t oldtime = time(NULL);
    if (oldtime + botsettings.autosaveperiod < time(NULL)) {
        oldtime = time(NULL);
        SaveBotSettings();
        gSeeBorg.SaveSettings();
    }
}

// ---

static void ProcOnInvite(BN_PInfo I, const char Chan[], const char Who[],
                  const char Whom[])
{
    string nickname = ExtractNick(Who);
    string hostname = ExtractHost(Who);
    cout << "Received invitation to " << Chan << " by " << nickname << "\n";

    if (botsettings.joininvites) {
        if (botsettings.joininvites != 1) {
            // Check if the invite is sent by owner
            if (!isOwner(hostname, nickname)) {
                return;
            }
        }
        BN_SendJoinMessage(I, Chan, NULL);
    }
}


static void ProcOnKick(BN_PInfo I, const char Chan[], const char Who[],
                const char Whom[], const char Msg[])
{
    string nickname = ExtractNick(Who);

    cout << "(" << Chan << ") * " << Whom << " has been kicked from " << Chan
         << " by " << nickname << " [" << Msg << "]\n";

    if (strstr(Whom, I->Nick) != NULL) {
        BN_SendJoinMessage(I, Chan, NULL);
    }
}


static void ProcOnPrivateTalk(BN_PInfo I, const char Who[], const char Whom[],
                       const char Msg[])
{
    string nickname = ExtractNick(Who);
    string hostname = ExtractHost(Who);
    cout << nickname << ": " << Msg << "\n";

    string reply = ProcessMessage(I, hostname, nickname, Msg, true);

    if (!reply.empty()) {
        vector<string> curlines;
        splitString(reply, curlines, "\n");
        for (int i = 0, sz = curlines.size(); i < sz; i++) {
            cout << Whom << " -> " << nickname << ": " << reply << "\n";
            BN_SendPrivateMessage(I, nickname.c_str(), curlines[i].c_str());
        }
    }
}


static void ProcOnChannelTalk(BN_PInfo I, const char Chan[], const char Who[],
                       const char Msg[])
{
    string nickname = ExtractNick(Who);
    string hostname = ExtractHost(Who);
    cout << "(" << Chan << ") <" << nickname << "> " << Msg << "\n";

    DoChanTalk(I, hostname, nickname, Msg, Chan);
}


static void ProcOnAction(BN_PInfo I, const char Chan[], const char Who[],
                  const char Msg[])
{
    string nickname = ExtractNick(Who);
    cout << "(" << Chan << ") * " << nickname << " " << Msg << "\n";

    if (botsettings.learning) {
        string line = nickname + " " + Msg;
        gSeeBorg.Learn(line);
    }
}


static void ProcOnJoin(BN_PInfo I, const char Chan[], const char Who[])
{
    string nickname = ExtractNick(Who);
    string hostname = ExtractHost(Who);
    string username = ExtractExactUserName(Who);

    cout << "(" << Chan << ") " << nickname << " (" << username << "@" << hostname
         << ") has joined the channel\n";

    DoChanTalk(I, hostname, nickname, nickname.c_str(), Chan);
}


static void ProcOnPart(BN_PInfo I, const char Chan[], const char Who[],
                const char Msg[])
{
    string nickname = ExtractNick(Who);
    string hostname = ExtractHost(Who);
    string username = ExtractExactUserName(Who);

    cout << "(" << Chan << ") " << nickname << " (" << username << "@" << hostname
         << ") has left the channel the channel (" << Msg << ")\n";

    DoChanTalk(I, hostname, nickname, Msg, Chan);
}


static void ProcOnQuit(BN_PInfo I, const char Who[], const char Msg[])
{
    string nickname = ExtractNick(Who);
    string hostname = ExtractHost(Who);
    string username = ExtractExactUserName(Who);

    cout << "" << nickname << " (" << username << "@" << hostname
         << ") has quit IRC (" << Msg << ")\n";
}

// Main Body
// ---------------------------------------------------------------------------

void cleanup(void)
{
    if (cleanedup) return;
    cleanedup = true;
    if (connected) {
        cout << "Disconnecting from server...\n";
        BN_SendQuitMessage(&Info, botsettings.quitmessage.c_str());
    }
    cout << "Saving dictionary...\n";
    gSeeBorg.SaveSettings();
    cout << "Saving settings...\n";
    SaveBotSettings();
}

static void sig_term(int i)
{
    if (!cleanedup) {
        // Save the settings before returning back to default signal handler
        cleanup();
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
#ifndef _WIN32
        // Windows doesn't define these signals
        signal(SIGQUIT, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
#endif
        _exit(0);
    }
}


int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    cout << "SeeBorg v" SEEBORGVERSIONSTRING
           ", copyright (C) 2003 Eugene Bujak.\n" "Uses " << BN_GetCopyright() << "\n";

    LoadBotSettings();
    if (argc < 2) {
        if (botsettings.server.empty()) {
            SaveBotSettings();
            cout << "No server to connect to (check seeborg-irc.cfg)";
            return 1;
        }
    } else {
        botsettings.server = argv[1];
    }

    memset(&Info, 0, sizeof(Info));

    Info.CB.OnConnected = ProcOnConnected;
    Info.CB.OnRegistered = ProcOnRegistered;
    Info.CB.OnCTCP = ProcOnCTCP;
    Info.CB.OnInvite = ProcOnInvite;
    Info.CB.OnKick = ProcOnKick;
    Info.CB.OnPrivateTalk = ProcOnPrivateTalk;
    Info.CB.OnAction = ProcOnAction;
    Info.CB.OnJoin = ProcOnJoin;
    Info.CB.OnPart = ProcOnPart;
    Info.CB.OnQuit = ProcOnQuit;
    Info.CB.OnChannelTalk = ProcOnChannelTalk;
    Info.CB.OnPingPong = ProcOnPingPong;


    srand(time(NULL));
    cout << "Loading dictionary...\n";
    gSeeBorg.LoadSettings();
    signal(SIGINT, sig_term);
    signal(SIGTERM, sig_term);
#ifndef _WIN32
    // Windows doesn't define these signals
    signal(SIGQUIT, sig_term);
    signal(SIGHUP, sig_term);
#endif
    cleanedup = false;
    atexit(cleanup);

    while (BN_Connect(&Info, botsettings.server.c_str(), 6667, 0) != true) {
        cout << "Disconnected.\n";
#ifdef __unix__
        sleep(10);
#elif defined(_WIN32)
        Sleep(10 * 1000);
#endif
        cout << "Reconnecting...\n";
    }
    return 0;
}
