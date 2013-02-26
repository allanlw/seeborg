#ifdef __unix__
#include <unistd.h>
#include <sys/wait.h>
#else
#include <time.h>
#ifdef _MSC_VER
#pragma comment (lib, "wsock32.lib")
#endif // _MSC_VER
#endif // __unix__

#include <stdlib.h>
#include <signal.h>
#include <locale.h>
#include <ctype.h>

#include "seeborg.h"
#include "seeborg-irc.h"
#include "seeutil.h"


extern "C" {
#include "botnet/botnet.h"
}


// Variables
// ---------------------------------------------------------------------------

BN_TInfo  Info;
seeborg_t gSeeBorg;
bool	  initialized = false;
bool	  connected = false;

int		  g_argc;
char**	  g_argv;

#define ABORT_HACK "#abort#"


// Bot Settings
// ---------------------------------------------------------------------------
typedef struct ircbotowner_s {
  string nickname;
  string hostname;
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
  string				server;
  int					serverport;
  string				nickname;
  string				username;
  string				realname;
  set<string>			channels;
  vector<ircbotowner_t>	owners;
  string				quitmessage;

  // Other settings
  float					replyrate;
  int					learning;
  

  int					speaking;
  int					stealth;		// TODO
  vector<string>		censored;		// TODO
  vector<string>		ignorelist;		// TODO
  int					reply2ignored;	// TODO

  int					joininvites;	// TODO
  float					replyrate_mynick;
  float					replyrate_magic;
  vector<string>		magicwords;

  int					autosaveperiod;

} botsettings_t;

botsettings_t botsettings;

// Bot Config File
// ---------------------------------------------------------------------------

typedef struct configsetting_s {
  const char*		configline;
  const char* 	description;

  string*	stringptr;
  float*	floatptr;
  int*		intptr;
} configsetting_t;

static configsetting_t configsettings[] = {
  {"server", "Address of IRC server", &botsettings.server, NULL, NULL},
  {"serverport", "Server port", NULL, NULL, &botsettings.serverport},

  {"nickname", "Bot's nickname", &botsettings.nickname, NULL, NULL},
  {"username", "Bot's username (will show as ~<username>@some.host.com)", &botsettings.username, NULL, NULL},
  {"realname", "Bot's realname (will show in whois)", &botsettings.realname, NULL, NULL},
  {"quitmessage", "Bot's quit message", &botsettings.quitmessage, NULL, NULL},

  {NULL, NULL, NULL, NULL, NULL},	// Newline in cfg

  {"replyrate", "Reply rate to all messages (in percent)", NULL, &botsettings.replyrate, NULL},
  {"replynick", "Reply rate to messages containing bot's nickname (in percent)", NULL, &botsettings.replyrate_mynick, NULL},
  {"replymagic", "Reply rate to messages containing magic words (in percent)", NULL, &botsettings.replyrate_magic, NULL},

  {NULL, NULL, NULL, NULL, NULL},	// Newline in cfg

  {"speaking", "Controls whether the bot speaks at all (boolean)", NULL, NULL, &botsettings.speaking},
  {"learning", "Does the bot learn, or just replies (boolean)", NULL, NULL, &botsettings.learning},
  {"stealth", "Try to emulate a popular IRC client's behaviour (TODO, boolean)", NULL, NULL, &botsettings.stealth},
  {"joininvites", "Join the channels the bot was invited to (0 - no, 1 - yes, 2 - only by owner)", NULL, NULL, &botsettings.joininvites},

  {NULL, NULL, NULL, NULL, NULL},	// Newline in cfg

  {"autosaveperiod", "Autosave period (in seconds)", NULL, NULL, &botsettings.autosaveperiod},

  {NULL, NULL, NULL, NULL, NULL}
};

static int numconfigsettings = sizeof(configsettings) / sizeof(configsettings[0]) - 1;

void LoadBotSettings() {
  string str;
  FILE* f = fopen ("seeborg-irc.cfg", "r");
  if (f == NULL) return;

  while (fReadStringLine (f, str)) {
	trimString(str);
	if (str[0] == ';') continue;
	if (str[0] == '#') continue;
	if (str.empty()) continue;

	vector<string> cursetting;

	if (splitString (str, cursetting, "=") < 2) continue;

	trimString(cursetting[0]);
	trimString(cursetting[1]);
	if (!strcasecmp(cursetting[0].c_str(), "channels"))	{
	  vector<string> cursplit;
	  if (!splitString (cursetting[1], cursplit, " ")) continue;
	  botsettings.channels.clear();
	  for (int i = 0, sz = cursplit.size(); i < sz; i++) {
		lowerString(cursplit[i]);
		botsettings.channels.insert(cursplit[i]);
	  }
	}

	if (!strcasecmp(cursetting[0].c_str(), "owners"))	{
	  vector<string> cursplit;
	  if (!splitString (cursetting[1], cursplit, " ")) continue;
	  botsettings.owners.clear();
	  for (int i = 0, sz = cursplit.size(); i < sz; i++) {
		ircbotowner_t	ircbotowner;
		ircbotowner.nickname = cursplit[i];
		botsettings.owners.push_back(ircbotowner);
	  }
	}

	if (!strcasecmp(cursetting[0].c_str(), "magicwords"))	{
	  vector<string> cursplit;
	  if (!splitString (cursetting[1], cursplit, " ")) continue;
	  botsettings.magicwords.clear();
	  for (int i = 0, sz = cursplit.size(); i < sz; i++) {
		botsettings.magicwords.push_back(cursplit[i]);
	  }
	}

	for (int i = 0; i < numconfigsettings; i++) {
	  configsetting_t* s = &configsettings[i];
	  if (s->configline == NULL) continue;
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
  fclose(f);
}

void SaveBotSettings() {
  FILE* f = fopen ("seeborg-irc.cfg", "w");
  //  if (f == NULL) return;

  fprintf (f, "; SeeBorg " SEEBORGVERSIONSTRING " settings file\n; Lines beginning with ; or # are treated as comments\n\n\n");
  int i, sz;

  for (i = 0; i < numconfigsettings; i++) {
	configsetting_t* s = &configsettings[i];
	if (s->configline == NULL) {
	  fprintf (f, "\n\n");
	  continue;
	}
	
	fprintf (f, "; %s\n", s->description);
	if (s->stringptr != NULL) {
	  fprintf (f, "%s = %s\n", s->configline, (*s->stringptr).c_str());
	} else if (s->floatptr != NULL) {
	  fprintf (f, "%s = %.2f\n", s->configline, *s->floatptr);
	} else if (s->intptr != NULL) {
	  fprintf (f, "%s = %i\n", s->configline, *s->intptr);
	}
  }

  fprintf (f, "; Channel list to join to\n");
  fprintf (f, "channels =");

  set<string>::iterator it = botsettings.channels.begin();
  for (; it != botsettings.channels.end(); ++it) {
	fprintf (f, " %s", (*it).c_str());
  }
  fprintf (f, "\n");

  fprintf (f, "; Magic word list\n");
  fprintf (f, "magicwords =");
  for (i = 0, sz = botsettings.magicwords.size(); i < sz; i++) {
	fprintf (f, " %s", botsettings.magicwords[i].c_str());
  }
  fprintf (f, "\n");

  fprintf (f, "; Owner list (nicknames)\n");
  fprintf (f, "owners =");
  for (i = 0, sz = botsettings.owners.size(); i < sz; i++) {
	fprintf (f, " %s", botsettings.owners[i].nickname.c_str());
  }
  fprintf (f, "\n");


  fclose(f);
}

// Message processing
// ---------------------------------------------------------------------------
void checkOwners(const char who[]) {
  char	hostname[4096];
  char  nickname[4096];
  BN_ExtractHost(who, hostname, sizeof(hostname));
  BN_ExtractNick(who, nickname, sizeof(nickname));

  for (int i = 0, sz = botsettings.owners.size(); i < sz; i++) {
	if (botsettings.owners[i].hostname.empty()) {
	  if (!strcasecmp(nickname, botsettings.owners[i].nickname.c_str())) {
		botsettings.owners[i].hostname = hostname;
		printf ("Locked owner '%s' to '%s'\n", nickname, hostname);
		return;
	  }
	}
  }
}

bool isOwner(const char who[]) {
  if (botsettings.owners.empty()) return false;
  char	hostname[4096];
  char  nickname[4096];

  BN_ExtractHost(who, hostname, sizeof(hostname));
  BN_ExtractNick(who, nickname, sizeof(nickname));

  for (int i = 0, sz = botsettings.owners.size(); i < sz; i++) {
	  if (!strcasecmp(hostname, botsettings.owners[i].hostname.c_str())) {
		  if (!strcasecmp(nickname, botsettings.owners[i].nickname.c_str())) {
			  return true;
		  }
	  }
  }
  return false;
}


string ProcessMessage(BN_PInfo I, const char who[], const char msg[], bool replying = false) {
  char* message = strdup(msg);
  lowerString(message);
  string stdmessage = message;
  free (message);

  checkOwners(who);

  if (stdmessage[0] == '!') {
	string stdcmdreply;
	stdcmdreply = ircParseCommands(stdmessage, who);
	if (stdcmdreply == ABORT_HACK) {
	  return "";
	}

	if (stdcmdreply != "") {
	  return stdcmdreply;
	} else {
	  stdcmdreply = gSeeBorg.ParseCommands(stdmessage);
	  if (stdcmdreply == ABORT_HACK) {
		return "";
	  }
	  if (stdcmdreply != "") {
		return stdcmdreply;
	  }
	}

	return "";
  }


  trimString(stdmessage);

  // Ignore quotes
  if (isdigit(stdmessage[0])) return "";
  if (stdmessage[0] == '<') return "";
  if (stdmessage[0] == '[') return "";

  if (randFloat(0, 99) < botsettings.replyrate) {
	replying = true;
  }

  if ((!replying) && (botsettings.replyrate_magic > 0)) {
	int sz = botsettings.magicwords.size();
	for (int i = 0; i < sz; i++) {
	  if (strstr(stdmessage.c_str(), botsettings.magicwords[i].c_str()) != NULL) {
		if (randFloat(0, 99) < botsettings.replyrate_magic) replying = true;
		else break;
	  }
	}
  }
  if ((!replying) && (botsettings.replyrate_mynick > 0)) {
	char* nickname = strdup(I->Nick);
	lowerString(nickname);
	if (strstr(stdmessage.c_str(), nickname) != NULL) {
	  if (randFloat(0, 99) < botsettings.replyrate_mynick) replying = true;
	}
	free(nickname);
  }

  string replystring;

  if ((replying) && (botsettings.speaking)) {
    replystring = gSeeBorg.Reply(stdmessage);
  }

  if (botsettings.learning) gSeeBorg.Learn(stdmessage);

  return replystring;
}


// BotNet callback functions
// ---------------------------------------------------------------------------
void ProcOnConnected(BN_PInfo I, const char HostName[]) {
  printf("Connected to %s...\n", HostName);
  BN_EnableFloodProtection(I, 1000, 1000, 60);
  connected = true;
  BN_Register(I, botsettings.nickname.c_str(), botsettings.username.c_str(), botsettings.realname.c_str());
}


void ProcOnRegistered(BN_PInfo I) {
  printf ("Registered...\n");
  set<string>::iterator it = botsettings.channels.begin();
  for (; it != botsettings.channels.end(); ++it) {
	printf ("Joining %s...\n", (*it).c_str());
	BN_SendJoinMessage (I, (*it).c_str(), NULL);
  }
}

// returned string is freed then, so we malloc() the return string each call
char *ProcOnCTCP(BN_PInfo I,const char Who[],const char Whom[],const char Type[]) {
  char  nickname[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  printf ("CTCP %s query by %s for %s\n", Type, nickname, Whom);

  char  replystring[4096];
  if (!strcasecmp(Type, "VERSION")) {
	  sprintf (replystring, "mIRC32 v5.7 K.Mardam-Bey");
  } else sprintf (replystring, "Forget about it");

  return strdup(replystring);
}


void ProcOnPingPong(BN_PInfo I) {
  static time_t oldtime = time(NULL);
  if (oldtime + botsettings.autosaveperiod < time(NULL)) {
	oldtime = time(NULL);
	SaveBotSettings();
    gSeeBorg.SaveSettings();
  }
}

// ---

void ProcOnInvite(BN_PInfo I,const char Chan[],const char Who[],const char Whom[]) {
  char  nickname[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  printf ("Received invitation to %s by %s\n", Chan, nickname);

  if (botsettings.joininvites) {
	if (botsettings.joininvites != 1) {
	  // Check if the invite is sent by owner
	  if (!isOwner(Who)) return;
	}
	BN_SendJoinMessage (I, Chan, NULL);
  }
}


void ProcOnKick(BN_PInfo I,const char Chan[],const char Who[],const char Whom[],const char Msg[]) {
  char  nickname[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  printf ("(%s) * %s has been kicked from %s by %s [%s]\n", Chan, Whom, Chan, nickname, Msg);

  if (strstr(Whom, I->Nick) != NULL) {
    BN_SendJoinMessage (I, Chan, NULL);
  }
}


void ProcOnPrivateTalk(BN_PInfo I,const char Who[],const char Whom[],const char Msg[]) {
  char  nickname[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  //  printf("%s -> %s: %s\n", nickname, Whom, Msg);
  printf ("%s: %s\n", nickname, Msg);

  string reply = ProcessMessage(I, Who, Msg, true);

  if (!reply.empty()) {
	  vector<string> curlines;
	  splitString(reply, curlines, "\n");
	  for (int i = 0, sz = curlines.size(); i < sz; i++) {
		  printf("%s -> %s: %s\n", Whom, nickname, reply.c_str());
		  BN_SendPrivateMessage(I, nickname, curlines[i].c_str());
	  }
  }
}


void ProcOnChannelTalk(BN_PInfo I,const char Chan[],const char Who[],const char Msg[]) {
  char  nickname[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  printf ("(%s) <%s> %s\n", Chan, nickname, Msg);

  string reply = ProcessMessage(I, Who, Msg);

  if (!reply.empty()) {
	  vector<string> curlines;
	  splitString(reply, curlines, "\n");
	  for (int i = 0, sz = curlines.size(); i < sz; i++) {
		  printf ("(%s) <%s> %s\n", Chan, I->Nick, reply.c_str());
		  BN_SendChannelMessage(I, Chan, curlines[i].c_str());
	  }
  }
}


void ProcOnAction(BN_PInfo I,const char Chan[],const char Who[],const char Msg[]) {
  char  nickname[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  printf ("(%s) * %s %s\n", Chan, nickname, Msg);

  if (botsettings.learning) {
	  string stdstr;
	  stdstr = nickname;
	  stdstr += " ";
	  stdstr += Msg;
	  gSeeBorg.Learn(stdstr);
  }

}


void ProcOnJoin (BN_PInfo I, const char Chan[],const char Who[]) {
  char  nickname[4096];
  char  hostname[4096];
  char  username[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  BN_ExtractHost(Who, hostname, sizeof(hostname));
  BN_ExtractExactUserName(Who, username, sizeof(username));

  printf ("(%s) %s (%s@%s) has joined the channel\n", Chan, nickname, username, hostname);

  string reply = ProcessMessage(I, Who, nickname);
  if (!reply.empty()) {
	  vector<string> curlines;
	  splitString(reply, curlines, "\n");
	  for (int i = 0, sz = curlines.size(); i < sz; i++) {
		  printf ("(%s) %s: %s\n", Chan, I->Nick, reply.c_str());
		  BN_SendChannelMessage(I, Chan, curlines[i].c_str());
	  }
  }
}


void ProcOnPart (BN_PInfo I, const char Chan[],const char Who[], const char Msg[]) {
  char  nickname[4096];
  char  hostname[4096];
  char  username[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  BN_ExtractHost(Who, hostname, sizeof(hostname));
  BN_ExtractExactUserName(Who, username, sizeof(username));

  printf ("(%s) %s (%s@%s) has left the channel (%s)\n", Chan, nickname, username, hostname, Msg);
  string reply = ProcessMessage(I, Who, nickname);
  if (!reply.empty()) {
	  vector<string> curlines;
	  splitString(reply, curlines, "\n");
	  for (int i = 0, sz = curlines.size(); i < sz; i++) {
		  printf ("(%s) %s: %s\n", Chan, I->Nick, reply.c_str());
		  BN_SendChannelMessage(I, Chan, curlines[i].c_str());
	  }
  }
}


void ProcOnQuit (BN_PInfo I, const char Who[],const char Msg[]) {
  char  nickname[4096];
  char  hostname[4096];
  char  username[4096];
  BN_ExtractNick(Who, nickname, sizeof(nickname));
  BN_ExtractHost(Who, hostname, sizeof(hostname));
  BN_ExtractExactUserName(Who, username, sizeof(username));

  printf ("%s (%s@%s) has quit IRC (%s)\n", nickname, username, hostname, Msg);
}

// Bot Commands body
// ---------------------------------------------------------------------------
string ircParseCommands(const string cmd, const char* who) {
  if (cmd[0] != '!') return "";

  if (!isOwner(who)) return ABORT_HACK;

  string command = cmd;
  lowerString(command);
  CMA_TokenizeString(command.c_str());
  for (int i = 0; i < numircbotcmds; i++) {
	if (!strncmp(CMA_Argv(0) + 1, ircbotcmds[i].command, strlen(ircbotcmds[i].command))) {
	  return ircbotcmds[i].func(&gSeeBorg, command);
	}
  }
  return "";
}

string CMD_Shutup_f(class SeeBorg* self, const string command) {
  if (!botsettings.speaking) return "";

  botsettings.speaking = false;
  return "I'll shut up... :o";
}

string CMD_Wakeup_f(class SeeBorg* self, const string command) {
  if (botsettings.speaking) return "";

  botsettings.speaking = true;
  return "Woohoo!";
}

string CMD_Save_f(class SeeBorg* self, const string command) {
  printf ("Saving settings...\n");
  SaveBotSettings();
  gSeeBorg.SaveSettings();
  return "okay";
}                                                             

string CMD_Join_f (class SeeBorg* self, const string command) {
  if (CMA_Argc() < 2) return "";

  for (int i = 1, sz = CMA_Argc(); i < sz; i++) {
	string channel = CMA_Argv(i);
	lowerString(channel);
	printf ("Joining %s...\n", CMA_Argv(i));
    BN_SendJoinMessage (&Info, CMA_Argv(i), NULL);
	botsettings.channels.insert(channel);
  }

  return "okay";
}


string CMD_Part_f (class SeeBorg* self, const string command) {
  if (CMA_Argc() < 2) return "";

  for (int i = 1, sz = CMA_Argc(); i < sz; i++) {
	string channel = CMA_Argv(i);
	printf ("Leaving %s...\n", CMA_Argv(i));
    BN_SendPartMessage (&Info, CMA_Argv(i), NULL);

	if (botsettings.channels.find(channel) != botsettings.channels.end()) {
	  botsettings.channels.erase(botsettings.channels.find(channel));
	}
  }

  return "okay";
}


string CMD_Replyrate_f(class SeeBorg* self, const string command) {
  static char retstr[4096];
  if (CMA_Argc() < 2) {
	snprintf (retstr, 4096, "Reply rate is %.1f%%", botsettings.replyrate);
	return retstr;
  }

  botsettings.replyrate = atof(CMA_Argv(1));
  snprintf (retstr, 4096, "Reply rate is set to %.1f%%", botsettings.replyrate);
  return retstr;
}

string CMD_Replynick_f(class SeeBorg* self, const string command) {
  static char retstr[4096];
  if (CMA_Argc() < 2) {
	snprintf (retstr, 4096, "Reply rate to nickname is %.1f%%", botsettings.replyrate_mynick);
	return retstr;
  }

  botsettings.replyrate_mynick = atof(CMA_Argv(1));
  snprintf (retstr, 4096, "Reply rate to nickname is set to %.1f%%", botsettings.replyrate_mynick);
  return retstr;
}

string CMD_Replyword_f(class SeeBorg* self, const string command) {
  static char retstr[4096];
  if (CMA_Argc() < 2) {
	snprintf (retstr, 4096, "Reply rate to magic words is %.1f%%", botsettings.replyrate_magic);
	return retstr;
  }

  botsettings.replyrate_magic = atof(CMA_Argv(1));
  snprintf (retstr, 4096, "Reply rate to magic words is set to %.1f%%", botsettings.replyrate_magic);
  return retstr;
}

string CMD_ircHelp_f(class SeeBorg* self, const string command) {
  static string retstr;
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


string CMD_Learning_f(class SeeBorg* self, const string command) {
  string retstr;
  if (CMA_Argc() < 2) {
	retstr = "Learning is ";
	retstr += (botsettings.learning) ? "enabled" : "disabled";
	return retstr;
  }

  botsettings.learning = atoi(CMA_Argv(1));
  retstr = "Learning is set to ";
  retstr += (botsettings.learning) ? "enabled" : "disabled";
  return retstr;
}


// Main Body
// ---------------------------------------------------------------------------

void cleanup(void) {
  // handle this only when we are initialized
  if (initialized) {
	if (connected) {
	  printf ("Disconnecting from server...\n");
	  BN_SendQuitMessage(&Info, botsettings.quitmessage.c_str());
	}
	printf ("Saving dictionary...\n");
	gSeeBorg.SaveSettings();
	printf ("Saving settings...\n");
	SaveBotSettings();
  }
}


typedef void (*sighandler_t)(int);

void sig_term(int i) {
  static int si = 0;
  if (!si) {
	si++;
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


int main (int argc, char* argv[]) {
  setlocale(LC_ALL, "");

  printf ("SeeBorg v" SEEBORGVERSIONSTRING ", copyright (C) 2003 Eugene Bujak.\n"
		  "Uses %s\n", BN_GetCopyright());

  LoadBotSettings();
  if (argc < 2) {
    if (botsettings.server.empty()) {
	  SaveBotSettings();
	  printf ("No server to connect to (check seeborg-irc.cfg)");
	  return 1;
	}
  } else {
	botsettings.server = argv[1];
  }


  g_argv = argv;
  g_argc = argc;

  memset (&Info, 0, sizeof(Info));

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
  printf ("Loading dictionary...\n");
  gSeeBorg.LoadSettings();
  signal(SIGINT, sig_term);
  signal(SIGTERM, sig_term);
#ifndef _WIN32
  // Windows doesn't define these signals
  signal(SIGQUIT, sig_term);
  signal(SIGHUP, sig_term);
#endif
  atexit(cleanup);

  initialized = true;
  while(BN_Connect(&Info,botsettings.server.c_str(), 6667, 0) != true) {
	printf ("Disconnected.\n");
#ifdef __unix__
    sleep(10);
#elif defined(_WIN32)
    Sleep(10*1000);
#endif
	printf ("Reconnecting...\n");
  }
  return 0;
}

// Emacs editing variables
// ---------------------------------------------------------------------------

/*
 * Local variables:
 *  tab-width: 4 
*/
