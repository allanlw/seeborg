#include <cctype>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <sstream>
#include <iostream>

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

vector<string> CMA_TokenizeString(const string& str)
{
    vector<string> res;

    size_t i = 0, j = 0;

    while (1) {
        // skip whitespace, stop on \n
        while (i < str.size() && str[i] <= ' ' && str[i] != '\n' && str[i] > 0) {
            i++;
        }
        if (i == str.size()) {
            return res;
        }

        j = i;
        while (i < str.size() && str[i] != ' ' && str[i] != '\n') {
            i++;
        }

        int len = i - j;
        res.push_back(str.substr(j, len));
    }
}
