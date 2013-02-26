#include "seeborg.h"

extern const botcommand_t botcmds[] = {
    {"help", "Show this command list", CMD_Help_f},
    {"version", "Show SeeBorg version", CMD_Version_f},
    {"words", "Show how many words the borg knows", CMD_Words_f},
    {"known", "Query the bot if the word is known", CMD_Known_f},
    {"contexts", "Show contexts containing the command argument", CMD_Contexts_f},
    {"unlearn", "Unlearn the command argument", CMD_Unlearn_f},
    {"replace", "Replace all occurences of old word with new one", CMD_Replace_f},

    {"quit", "As the name implies", CMD_Quit_f},

    {NULL, NULL, NULL}
};

extern const int numbotcmds = sizeof(botcmds) / sizeof(botcmds[0]) - 1;


