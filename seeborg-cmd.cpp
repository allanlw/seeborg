#include "seeborg.h"

#include <sstream>

using namespace std;

static string CMD_Help_f(class SeeBorg *self, const vector<string>& toks) {
    stringstream ss;
    ss << "SeeBorg commands:\n";
    for (auto &cmd : self->cmds) {
        ss << "!" << cmd.command << ": " << cmd.description << "\n";
    }
    return ss.str();
}

static string CMD_Version_f(class SeeBorg *self, const vector<string>& toks) {
    return "I am SeeBorg v" SEEBORGVERSIONSTRING
           " by Eugene 'HMage' Bujak";
}

static string CMD_Words_f(class SeeBorg *self, const vector<string>& toks) {
    stringstream ss;
    self->getIKnow(ss);
    return ss.str();
}

static string CMD_Known_f(class SeeBorg *self, const vector<string>& toks) {
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

static string CMD_Contexts_f(class SeeBorg *self, const vector<string>& toks) {
    return "Not implemented yet";
}

static string CMD_Unlearn_f(class SeeBorg *self, const vector<string>& toks) {
    return "Not implemented yet";
}

static string CMD_Replace_f(class SeeBorg *self, const vector<string>& toks) {
    return "Not implemented yet";
}

static string CMD_Quit_f(class SeeBorg *self, const vector<string>& toks) {
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
