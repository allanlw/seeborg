#include <cctype>

#include "seeutil.h"

using namespace std;

int tokenizeString(string &instr, vector < string > &outtokens)
{
    vector < int >splitpos;
    for (int n = instr.find(' '); n != instr.npos; n = instr.find(' ', n)) {
        splitpos.push_back(n);
        n++;
    }

    splitpos.push_back(instr.npos);

    int sz = splitpos.size();
    int nstart = 0;
    int nend = instr.npos;
    for (int i = 0; i < sz; i++) {
        nend = splitpos[i];
        if (nend == -1) {
            outtokens.push_back(instr.substr(nstart));
        } else {
            outtokens.push_back(instr.substr(nstart, nend - nstart));
        }
        nstart = nend + 1;
    }

    return true;
}

int splitString(string &instr, vector < string > &outtokens,
                const char *needle)
{
    vector < int >splitpos;
    for (int n = instr.find(needle); n != instr.npos;
            n = instr.find(needle, n)) {
        instr.erase(n, strlen(needle));
        splitpos.push_back(n);
    }

    //  sort (splitpos.begin(), splitpos.end());
    splitpos.push_back(instr.npos);

    int sz = splitpos.size();
    int nstart = 0;
    int nend = instr.npos;
    int i;

    for (i = 0; i < sz; i++) {
        nend = splitpos[i];
        if (nend == -1) {
            outtokens.push_back(instr.substr(nstart));
        } else {
            outtokens.push_back(instr.substr(nstart, nend - nstart));
        }
        nstart = nend;
    }

    return i;
}

string joinString(vector < string > &tokens)
{
    string str;

    int sz = tokens.size();
    for (int i = 0; i < sz; i++) {
        str += tokens[i];
        if (i + 1 != sz) {
            str += ' ';
        }
    }

    return str;
}


int lowerString(string &str)
{
    char *cmess = strdup(str.c_str());
    lowerString(cmess);
    str = cmess;
    free(cmess);

    return true;
}

int lowerString(char *str)
{
    int sz = strlen(str);
    for (int i = 0; i < sz; i++) {
        str[i] = tolower(str[i]);
    }

    return true;
}


int trimString(string &str)
{
    // TODO: Optimize
    while (1) {
        if (str.empty()) {
            break;
        }
        if (str[0] == ' ') {
            str.erase(0, 1);
        } else if (str[0] == '\r') {
            str.erase(0, 1);
        } else if (str[0] == '\n') {
            str.erase(0, 1);
        } else {
            break;
        }
    }

    while (1) {
        if (str.empty()) {
            break;
        }
        if (str[str.size() - 1] == ' ') {
            str.erase(str.size() - 1);
        } else if (str[str.size() - 1] == '\r') {
            str.erase(str.size() - 1, 1);
        } else if (str[str.size() - 1] == '\n') {
            str.erase(str.size() - 1, 1);
        } else {
            break;
        }
    }
    return true;
}



// Reads string from file until we reach EOF or newline
// ---
int fReadStringLine(FILE *f, string &str)
{
    str.erase(0, str.npos);
    char s[65536];
    s[65535] = 104;

    for (;;) {
        if (fgets((char *) &s, sizeof(s), f) == NULL) {
            return false;
        }
        str += s;

        if (s[65535] != 104) {
            s[65535] = 104;
            continue;
        } else {
            break;
        }
    }
    return true;
}



// Arguments from string
// ---
#define MAX_CMA_TOKENS 80

static int cma_argc = 0;
static char *cma_argv[MAX_CMA_TOKENS];
static const char *cma_nullstring = "";	// Need to return address of this sometimes


// Call with NULL to free any resources used by CMA
void CMA_TokenizeString(const char *string)
{
    // TODO: parse whitespace inside double quotes as one token
    for (int i = 0; i < cma_argc; i++) {
        delete[]cma_argv[i];
    }

    cma_argc = 0;

    if (!string) {
        return;
    }

    const char *text = string;
    const char *text2 = string;

    while (1) {
        // skip whitespace, stop on \n
        while (*text && *text <= ' ' && *text != '\n' && *text > 0) {
            text++;
        }
        if (!*text) {
            return;
        }
        text2 = text;
        while (*text && *text != ' ' && *text != '\n') {
            text++;
        }

        int len = text - text2;
//              printf ("%i\n", len);

        cma_argv[cma_argc] = new char[len + 1];
        char *str = cma_argv[cma_argc];
        for (int i = 0; i < len; i++) {
            *(str + i) = *(text2 + i);
        }
        *(str + len) = 0;

        cma_argc++;
    }

}

int CMA_Argc(void)
{
    return cma_argc;
}

const char *CMA_Argv(unsigned int c)
{
    if (c >= (unsigned int) cma_argc) {
        return cma_nullstring;
    }
    return cma_argv[c];
}
