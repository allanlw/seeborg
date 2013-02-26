#ifndef __SEEBORG_H__
#define __SEEBORG_H__

#ifdef _MSC_VER
#pragma warning (disable: 4786)
#pragma warning (disable: 4503)
#endif

#ifdef _WIN32
#define strcasecmp stricmp
#define snprintf _snprintf
#else
#include <ctype.h>
#endif

#include <map>
#include <vector>
#include <string>
#include <set>
#include <deque>

#define SEEBORGVERSIONMINOR 0
#define SEEBORGVERSIONMAJOR 51
#define SEEBORGVERSIONSTRING "0.51 beta"

using namespace std;

typedef struct botcommand_s {
    const char *command;
    const char *description;
    string(*func) (class SeeBorg *self, const string cmd);
} botcommand_t;

typedef pair < set < string >::iterator, int >context_t;
typedef vector < context_t > word_t;

class SeeBorg
{
public:
    SeeBorg() {
        num_contexts = 0;
        min_context_depth = 1;
        max_context_depth = 4;
    } int Learn(string &body);
    string Reply(string message);

    int LoadSettings(void);
    int SaveSettings(void);

    string ParseCommands(const string command);

// private:
    int LearnLine(string &line);
    int FilterMessage(string &message);

    int num_contexts;

    int min_context_depth;
    int max_context_depth;

    set < string > lines;
    map < string, word_t > words;

};

typedef class SeeBorg seeborg_t;

extern seeborg_t gSeeBorg;

// Bot commands
string CMD_Help_f(class SeeBorg *self, const string command);
string CMD_Version_f(class SeeBorg *self, const string command);
string CMD_Words_f(class SeeBorg *self, const string command);
string CMD_Known_f(class SeeBorg *self, const string command);
string CMD_Contexts_f(class SeeBorg *self, const string command);
string CMD_Unlearn_f(class SeeBorg *self, const string command);
string CMD_Replace_f(class SeeBorg *self, const string command);
string CMD_Quit_f(class SeeBorg *self, const string command);

static botcommand_t botcmds[] = {
    {"help", "Show this command list", CMD_Help_f},
    {"version", "Show SeeBorg version", CMD_Version_f},
    {"words", "Show how many words the borg knows", CMD_Words_f},
    {"known", "Query the bot if the word is known", CMD_Known_f},

    {
        "contexts", "Show contexts containing the command argument",
        CMD_Contexts_f
    },
    {"unlearn", "Unlearn the command argument", CMD_Unlearn_f},
    {
        "replace", "Replace all occurences of old word with new one",
        CMD_Replace_f
    },

    {"quit", "As the name implies", CMD_Quit_f},

    {NULL, NULL, NULL}
};

static int numbotcmds = sizeof(botcmds) / sizeof(botcmds[0]) - 1;

// ---------------------------------------------------------------------------

#endif
