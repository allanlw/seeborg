#include <cctype>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <sstream>

#include "seeutil.h"

using namespace std;

void tokenizeString(const string &instr, vector<string> &outtokens)
{
    istringstream iss(instr);
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter<vector<string> >(outtokens));
}

int splitString(string &instr, vector<string> &outtokens,
                const char *needle)
{
    vector<string::size_type> splitpos;
    for (string::size_type n = instr.find(needle); n != instr.npos;
            n = instr.find(needle, n)) {
        instr.erase(n, strlen(needle));
        splitpos.push_back(n);
    }

    //  sort (splitpos.begin(), splitpos.end());
    splitpos.push_back(instr.npos);

    vector<string::size_type>::size_type sz = splitpos.size(), i;
    string::size_type nstart = 0, nend = instr.npos;

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

string joinString(vector<string> &tokens)
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

void lowerString(string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), ptr_fun<int, int>(tolower));
}

void lowerString(char *str)
{
    for (char *p = str; *p; p++) {
      *p = tolower(*p);
    }
}

void trimString(string &str)
{
    // creds to http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
    // do left trim then right trim
    str.erase(str.begin(), find_if(str.begin(), str.end(), not1(ptr_fun<int, int>(isspace))));
    str.erase(find_if(str.rbegin(), str.rend(), not1(ptr_fun<int, int>(isspace))).base(), str.end());
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
