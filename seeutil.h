#ifndef __SEEUTIL_H__
#define __SEEUTIL_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>

inline float randFloat(float min, float max)
{
    float retval = (float) std::rand() / (float) RAND_MAX;
    retval *= (max - min);
    retval += min;
    return retval;
}

inline int randInt(int min, int max)
{
    return (int) roundf(randFloat(min,max));
}

int tokenizeString(std::string &str, std::vector < std::string > &tokens);
int splitString(std::string &str, std::vector < std::string > &tokens,
                const char *needle);
std::string joinString(std::vector < std::string > &tokens);
int lowerString(std::string &str);
int lowerString(char *str);
int trimString(std::string &str);

int fReadStringLine(std::FILE *f, std::string &);

// Arguments from string
void CMA_TokenizeString(const char *str);
int CMA_Argc(void);
const char *CMA_Argv(unsigned int c);

#endif
