#include <fstream>
#include <cstring>
#include <deque>

#include "seeborg.h"
#include "seeutil.h"

#define LINE_SEP "."
#define LINES_TXT "lines.txt"

using namespace std;

int SeeBorg::LoadSettings(void)
{
    // TODO: WIP
    string str;
    ifstream ifs(LINES_TXT);
    if (ifs.bad()) {
        printf("Not found, creating dictionary.\n");
        return false;
    }

    while (getline(ifs, str)) {
        this->Learn(str);
    }

    ifs.close();

    printf("Parsed %i lines.\n", lines.size());
    printf("I know %i words (%i contexts, %.2f per word), %i lines.\n",
           words.size(), num_contexts,
           (float) num_contexts / (float) words.size(), lines.size());

    return true;
}

int SeeBorg::SaveSettings(void)
{
    ofstream ofs(LINES_TXT);
    set<string>::iterator it;
    for (it = lines.begin(); it != lines.end(); ++it) {
        ofs << *it << endl;
    }
    ofs.close();
    return true;
}

string SeeBorg::Reply(string message)
{
    FilterMessage(message);
    string replystring;

    vector<string> curlines;
    splitString(message, curlines, LINE_SEP);
    vector<string> curwords;

    int sz, i;
    for (sz = curlines.size(), i = 0; i < sz; i++) {
        tokenizeString(curlines[i], curwords);
    }

    if (curwords.empty()) {
        return replystring;
    }
    // Filter out the words we don't know about
    int known = -1;
    vector<string> index;
    for (sz = curwords.size(), i = 0; i < sz; i++) {
        string &x = curwords[i];
        if (words.find(x) == words.end()) {
            continue;
        }
        int k = words[x].size();
        if ((known == -1) || (k < known)) {
            index.clear();
            index.push_back(x);
            known = k;
        } else if (k == known) {
            index.push_back(x);
        }
    }

    if (index.empty()) {
        return replystring;
    }

    deque<string> sentence;

    // pick a random word to start building the reply
    int x = rand() % (index.size());
    sentence.push_back(index[x]);

    // Build on the left edge
    bool done = false;
    while (!done) {
        // cline = line, w = word number
        int c = this->words[sentence[0]].size();
        context_t l = this->words[sentence[0]][rand() % (c)];
        int w = l.second;
        vector<string> cwords;
        tokenizeString(*(l.first), cwords);

        int depth =
            rand() % (max_context_depth - min_context_depth) +
            min_context_depth;
        for (int i = 1; i <= depth; i++) {
            if ((w - i) < 0) {
                done = true;
                break;
            } else {
                sentence.push_front(cwords[w - i]);
            }
            if ((w - i) == 0) {
                done = true;
                break;
            }
        }
    }

    // Build on the right edge
    done = false;
    while (!done) {
        if (words.find(sentence.back()) == words.end()) {
            printf("%s:%i: words.find(sentence.back()) == words.end()\n",
                   __FILE__, __LINE__);
        }

        int c = this->words[sentence.back()].size();
        context_t l = this->words[sentence.back()][rand() % (c)];
        int w = l.second;
        vector<string> cwords;
        tokenizeString(*(l.first), cwords);

        int depth =
            rand() % (max_context_depth - min_context_depth) +
            min_context_depth;
        for (int i = 1; i <= depth; i++) {
            if ((w + i) >= cwords.size()) {
                done = true;
                break;
            } else {
                sentence.push_back(cwords[w + i]);
            }
        }
    }

    for (i = 0, sz = sentence.size() - 1; i < sz; i++) {
        replystring += sentence[i];
        replystring += ' ';
    }
    replystring += sentence.back();

    return replystring;
}


int SeeBorg::Learn(string &body)
{
    FilterMessage(body);
    vector<string> curlines;
    splitString(body, curlines, LINE_SEP);

    int sz = curlines.size();
    for (int i = 0; i < sz; i++) {
        LearnLine(curlines[i]);
    }

    return true;
}

int SeeBorg::LearnLine(string &line)
{
    // Ignore quotes
    if (isdigit(line[0])) {
        return false;
    }
    if (line[0] == '<') {
        return false;
    }
    if (line[0] == '[') {
        return false;
    }

    vector<string> curwords;
    tokenizeString(line, curwords);
    string cleanline = joinString(curwords);
    if (lines.find(cleanline) != lines.end()) {
        return false;
    }

    set<string>::iterator lineit = lines.insert(cleanline).first;

    int sz = curwords.size();
    for (int i = 0; i < sz; i++) {
        map<string, word_t>::iterator wit = words.find(curwords[i]);
        if (wit == words.end()) {
            word_t cword;
            context_t cxt(lineit, i);
            cword.push_back(cxt);
            words[curwords[i]] = cword;
        } else {
            context_t cxt(lineit, i);
            ((*wit).second).push_back(cxt);
        }
        num_contexts++;
    }

    return true;
}


string SeeBorg::ParseCommands(const string& cmd)
{
    if (cmd[0] != '!') {
        return "";
    }
    string command = cmd;
    lowerString(command);
    vector<string> tokens = CMA_TokenizeString(command.c_str());
    if (tokens.size() < 1) return ""; // this shouldn't happen...

    string cmd_name = tokens[0].substr(1);

    for (int i = 0; i < cmds.size(); i++) {
        if (cmd_name == cmds[i].command) {
            return cmds[i].func(this, tokens);
        }
    }
    return "";
}

// Utility
// ---
int SeeBorg::FilterMessage(string &message)
{
    int n;			// MSVC doesn't like 'for' locality
    for (n = message.find('\n'); n != message.npos;
            n = message.find('\n', n)) {
        message.erase(n, 1);
    }

    for (n = message.find('\r'); n != message.npos;
            n = message.find('\r', n)) {
        message.erase(n, 1);
    }

    for (n = message.find('\"'); n != message.npos;
            n = message.find('\"', n)) {
        message.erase(n, 1);
    }

    for (n = message.find("?"); n != message.npos;
            n = message.find("?", n)) {
        message.replace(n, 1, "?.");
        n++;
    }

    for (n = message.find("!"); n != message.npos;
            n = message.find("!", n)) {
        message.replace(n, 1, "!.");
        n++;
    }

    // Remove message start text in the form NICK: from the start
    if ((n = message.find(':')) != string::npos) {
        bool fail = false;
        for (int i = 0; i < n; i++) {
            if (!isalnum(message[i]) && message[i] != '_' &&
                message[i] != '-') {
                fail = true;
                break;
            }
        }
        if (!fail) {
            message.erase(0, n+1);
        }
    }

    lowerString(message);

    return true;
}

// ---------------------------------------------------------------------------

string CMD_Help_f(class SeeBorg *self, const vector<string>& toks)
{
    string retstr;
    retstr = "SeeBorg commands:\n";
    for (int i = 0; i < self->cmds.size(); i++) {
        retstr += "!";
        retstr += self->cmds[i].command;
        retstr += ": ";
        retstr += self->cmds[i].description;
        retstr += "\n";
    }
    return retstr;
}

string CMD_Version_f(class SeeBorg *self, const vector<string>& toks)
{
    return "I am SeeBorg v" SEEBORGVERSIONSTRING
           " by Eugene 'HMage' Bujak";
}

string CMD_Words_f(class SeeBorg *self, const vector<string>& toks)
{
    char retstr[4096];

    snprintf(retstr, 4096,
             "I know %i words (%i contexts, %.2f per word), %i lines.",
             self->words.size(), self->num_contexts,
             self->num_contexts / (float) self->words.size(),
             self->lines.size());
    return retstr;
}

string CMD_Known_f(class SeeBorg *self, const vector<string>& toks)
{
    if (toks.size() < 2) {
        return "Not enough parameters, usage: !known <word>";
    }

    const string& lookup = toks[1];

    map<string, word_t>::iterator wit = self->words.find(lookup);
    if (wit != self->words.end()) {
        char retstr[4096];
        int wordcontexts = ((*wit).second).size();
        snprintf(retstr, 4096, "%s is known (%i contexts)", lookup.c_str(),
                 wordcontexts);
        return retstr;
    } else {
        return lookup + " is unknown";
    }
}

string CMD_Contexts_f(class SeeBorg *self, const vector<string>& toks)
{
    return "Not implemented yet";
}

string CMD_Unlearn_f(class SeeBorg *self, const vector<string>& toks)
{
    return "Not implemented yet";
}

string CMD_Replace_f(class SeeBorg *self, const vector<string>& toks)
{
    return "Not implemented yet";
}

string CMD_Quit_f(class SeeBorg *self, const vector<string>& toks)
{
    exit(0);

    return "Wow!";
}

static const botcommand_t botcmds[] = {
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

SeeBorg::SeeBorg() {
    num_contexts = 0;
    min_context_depth = 1;
    max_context_depth = 4;

    AddCommands(botcmds);
}

void SeeBorg::AddCommands(const botcommand_t *cmds) {
  for (int i = 0; cmds[i].command != NULL; i++) {
    this->cmds.push_back(cmds[i]);
  }
}

