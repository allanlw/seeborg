#ifndef __SEEUTIL_H__
#define __SEEUTIL_H__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

inline int randInt(int min, int max) {
  return (int)(min+((max-min)*(float)rand()/((float)RAND_MAX)));
}

inline float randFloat(float min, float max) {
#if 0
  float retval = (float)rand()/(float)RAND_MAX;
  retval *= (max - min);
  retval += min;
  return retval;
#else
  return (min+((max-min)*(float)rand()/((float)RAND_MAX)));
#endif
}


int tokenizeString(string &str, vector<string> &tokens);
int splitString(string &str, vector<string> &tokens, const char* needle);
string joinString(vector<string> &tokens);
int lowerString(string &str);
int lowerString(char* str);
int trimString(string &str);

int fReadStringLine(FILE* f, string &);

// Arguments from string
void CMA_TokenizeString (const char* str);
int CMA_Argc(void);
const char* CMA_Argv(unsigned int c);

#endif
