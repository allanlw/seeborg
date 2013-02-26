#ifndef __SEEUTIL_H__
#define __SEEUTIL_H__

#include <string>
#include <cstdlib>
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

void tokenizeString(const std::string &str, std::vector<std::string> &tokens);
int splitString(std::string &str, std::vector < std::string > &tokens,
                const char *needle);
std::string joinString(std::vector < std::string > &tokens);
void lowerString(std::string &str);
void lowerString(char *str);
void trimString(std::string &str);

// Arguments from string
void CMA_TokenizeString(const char *str);
int CMA_Argc(void);
const char *CMA_Argv(unsigned int c);

#endif
