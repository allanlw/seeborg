#ifndef __SEEUTIL_H__
#define __SEEUTIL_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

inline float randFloat(float min, float max)
{
    float retval = (float) rand() / (float) RAND_MAX;
    retval *= (max - min);
    retval += min;
    return retval;
}

inline int randInt(int min, int max)
{
    return (int) roundf(randFloat(min,max));
}

int tokenizeString(string &str, vector < string > &tokens);
int splitString(string &str, vector < string > &tokens,
                const char *needle);
string joinString(vector < string > &tokens);
int lowerString(string &str);
int lowerString(char *str);
int trimString(string &str);

int fReadStringLine(FILE *f, string &);

// Arguments from string
void CMA_TokenizeString(const char *str);
int CMA_Argc(void);
const char *CMA_Argv(unsigned int c);

#endif
