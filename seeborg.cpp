#include <fstream>
#include <deque>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "seeborg.h"
#include "seeutil.h"

#define LINE_SEP "."
#define LINES_TXT "lines.txt"

using namespace std;

void SeeBorg::getIKnow(std::ostream& out) const {
    float ctxperword = (float)num_contexts / words.size();

    out << "I know " << words.size() << " words (" << num_contexts;
    out << " contexts, " << fixed << setprecision(2) << ctxperword;
    out << " per word), " << lines.size() << " lines.";
}

bool SeeBorg::LoadSettings() {
    // TODO: WIP
    ifstream ifs(LINES_TXT);
    if (ifs.bad()) {
        cout << "Not found, creating dictionary.\n";
        return false;
    }

    string str;
    while (getline(ifs, str)) {
        Learn(str);
    }

    ifs.close();

    cout << "Parsed " << lines.size() << " lines.\n";
    getIKnow(cout);

    return true;
}

bool SeeBorg::SaveSettings() {
    ofstream ofs(LINES_TXT);
    for (auto &line : lines) {
        ofs << line << endl;
    }
    ofs.close();
    return true;
}

string SeeBorg::choosePivot(vector<string>& curwords) {
    // Filter out the words we don't know about
    // EDIT: this actually chooses the words in the index that
    // have contexts, but the least number of contexts
    int known = -1;
    vector<string> index;
    for (auto &x : curwords) {
        auto it = words.find(x);
        if (it == words.end()) {
            continue;
        }
        int k = it->second.size();
        if (known == -1 || k < known) {
            index.clear();
            index.push_back(x);
            known = k;
        } else if (k == known) {
            index.push_back(x);
        }
    }

    if (index.empty()) {
        return "";
    }

    return getRandom(index);
}

int SeeBorg::getRandDepth() {
    return rand() % (max_context_depth - min_context_depth) +
        min_context_depth;
}

static void buildLeftRight(SeeBorg &self, deque<string> &sentence, bool left) {
    auto &words = self.words;
    while (true) {
        auto it = words.find(left ? sentence.front() : sentence.back());
        if (it == words.end()) {
            cout << "!! words.find(" << (left ? "first" : "last")
                 << " == words.end()\n";
            return;
        }

        vector<string> cwords;
        context_t l = getRandom(it->second);
        tokenizeString(*l.first, cwords);
        int w = l.second;

        for (int i = 1, depth = self.getRandDepth(); i <= depth; i++) {
            if (left) {
                if (w - i < 0) return;
                sentence.push_front(cwords[w - i]);
            } else {
                if (w + i >= cwords.size()) return;
                sentence.push_back(cwords[w + i]);
            }
        }
    }
}

string SeeBorg::Reply(string message) {
    FilterMessage(message);

    vector<string> curlines;
    splitString(message, curlines, LINE_SEP);
    vector<string> curwords;

    for (auto &line : lines) {
        tokenizeString(line, curwords);
    }

    if (curwords.empty()) {
        return "";
    }

    deque<string> sentence;
    sentence.push_back(choosePivot(curwords));

    buildLeftRight(*this, sentence, true);
    buildLeftRight(*this, sentence, false);

    return joinString(sentence);
}


int SeeBorg::Learn(string &body) {
    FilterMessage(body);
    vector<string> curlines;
    splitString(body, curlines, LINE_SEP);

    for (auto &l : curlines) {
        LearnLine(l);
    }

    return true;
}

int SeeBorg::LearnLine(string &line) {
    // Ignore quotes
    if (isdigit(line[0]) || line[0] == '<' || line[0] == '[') {
        return false;
    }

    vector<string> curwords;
    tokenizeString(line, curwords);
    string cleanline = joinString(curwords);
    if (lines.find(cleanline) != lines.end()) {
        return false;
    }

    set<string>::iterator lineit = lines.insert(cleanline).first;

    for (int i = 0, sz = curwords.size(); i < sz; i++) {
        auto &word = curwords[i];
        auto wit = words.find(word);
        if (wit == words.end()) {
            word_t cword;
            cword.emplace_back(lineit, i);
            words[word] = cword;
        } else {
            wit->second.emplace_back(lineit, i);
        }
        num_contexts++;
    }

    return true;
}


string SeeBorg::ParseCommands(const string& cmd) {
    if (cmd[0] != '!') {
        return "";
    }
    string command = cmd;
    lowerString(command);
    vector<string> tokens = CMA_TokenizeString(command.c_str());
    if (tokens.size() < 1) return ""; // this shouldn't happen...

    string cmd_name = tokens[0].substr(1);

    for (auto &cmd : cmds) {
        if (cmd_name == cmd.command) {
            return cmd.func(this, tokens);
        }
    }
    return "";
}

struct replacement {
  const char *needle;
  const char *repl;
};

static const replacement msg_filters[] = {
    {"\n", ""},
    {"\r", ""},
    {"\"", ""},
    {"?", "?."},
    {"!", "!."},
};

// Utility
// ---
int SeeBorg::FilterMessage(string &message) {
    for (auto &r : msg_filters) {
        for (int n = message.find(r.needle); n != message.npos;
                n = message.find(r.needle, n+strlen(r.repl))) {
            message.replace(n, strlen(r.needle), r.repl);
        }
    }

    // Remove message start text in the form NICK: from the start
    for (int i = 0, sz = message.size(); i != sz; i++) {
        if (isalnum(message[i]) || message[i] == '_' || message[i] == '-') {
            continue;
        }
        if (message[i] == ':') {
            message.erase(0, i+1);
        }
        break;
    }
    lowerString(message);

    return true;
}

SeeBorg::SeeBorg() : num_contexts(0), min_context_depth(1),
      max_context_depth(4) {
    AddDefaultCommands();
}

void SeeBorg::AddCommands(const BotCommand *cmds) {
  for (int i = 0; cmds[i].command != NULL; i++) {
    this->cmds.push_back(cmds[i]);
  }
}
