#include "seeborg.h"

#include <sstream>

using namespace std;

SEEBORG_CMD(Help) {
    stringstream ss;
    ss << "SeeBorg commands:\n";
    for (auto &cmd : self->cmds) {
        ss << "!" << cmd.command << ": " << cmd.description << "\n";
    }
    return ss.str();
}

SEEBORG_CMD(Version) {
    return "I am SeeBorg v" SEEBORGVERSIONSTRING
           " by Eugene 'HMage' Bujak";
}

SEEBORG_CMD(Words) {
    stringstream ss;
    self->getIKnow(ss);
    return ss.str();
}

SEEBORG_CMD(Known) {
    if (toks.size() < 2) {
        return "Not enough parameters, usage: !known <word>";
    }

    const string& lookup = toks[1];

    stringstream ss;

    map<string, word_t>::iterator wit = self->words.find(lookup);
    if (wit != self->words.end()) {
        ss << lookup << " is known in " << wit->second.size() << " contexts.";
    } else {
        ss << lookup << " is not known.";
    }
    return ss.str();
}

SEEBORG_CMD(Contexts) {
    return "Not implemented yet";
}

SEEBORG_CMD(Unlearn) {
    return "Not implemented yet";
}

SEEBORG_CMD(Replace) {
    return "Not implemented yet";
}

SEEBORG_CMD(Quit) {
    exit(0);
}

static const BotCommand botcmds[] = {
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

void SeeBorg::AddDefaultCommands() {
    AddCommands(botcmds);
}
