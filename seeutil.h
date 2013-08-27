#ifndef __SEEUTIL_H__
#define __SEEUTIL_H__

#include <string>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <cstring>

inline float randFloat(float min, float max) {
    float retval = (float) std::rand() / (float) RAND_MAX;
    return retval * (max - min) + min;
}

inline int randInt(int min, int max) {
    return std::rand() % (max - min) + min;
}

void tokenizeString(const std::string &str, std::vector<std::string> &tokens);
int splitString(std::string &str, std::vector<std::string> &tokens,
                const char *needle);

template <class T>
std::string joinString(T &tokens) {
    std::string str;

    int sz = tokens.size();
    for (int i = 0; i < sz-1; i++) {
        str += tokens[i];
        if (i + 1 != sz) {
            str += ' ';
        }
    }

    return str;
}

void lowerString(std::string &str);
void lowerString(char *str);
void trimString(std::string &str);

inline bool equalIString(const std::string &a, const std::string &b) {
    return !strcasecmp(a.c_str(), b.c_str());
}

// Arguments from string
std::vector<std::string> CMA_TokenizeString(const std::string& str);

template <class Z>
Z& getRandom(std::vector<Z>& container) {
    return container[rand() % container.size()];
}

#endif
