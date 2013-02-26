#ifndef __SEEBORG_H__
#define __SEEBORG_H__

#include <map>
#include <vector>
#include <string>
#include <set>

#define SEEBORGVERSIONMINOR 0
#define SEEBORGVERSIONMAJOR 51
#define SEEBORGVERSIONSTRING "0.51 beta"

typedef struct botcommand_s {
    const char *command;
    const char *description;
    std::string(*func) (class SeeBorg *self, const std::string cmd);
} botcommand_t;

typedef std::pair < std::set < std::string >::iterator, int >context_t;
typedef std::vector < context_t > word_t;

class SeeBorg
{
public:
    SeeBorg() {
        num_contexts = 0;
        min_context_depth = 1;
        max_context_depth = 4;
    } int Learn(std::string &body);
    std::string Reply(std::string message);

    int LoadSettings(void);
    int SaveSettings(void);

    std::string ParseCommands(const std::string command);

/* private: */
    int LearnLine(std::string &line);
    int FilterMessage(std::string &message);

    int num_contexts;

    int min_context_depth;
    int max_context_depth;

    std::set < std::string > lines;
    std::map < std::string, word_t > words;

};

typedef class SeeBorg seeborg_t;

extern seeborg_t gSeeBorg;

// Bot commands
std::string CMD_Help_f(class SeeBorg *self, const std::string command);
std::string CMD_Version_f(class SeeBorg *self, const std::string command);
std::string CMD_Words_f(class SeeBorg *self, const std::string command);
std::string CMD_Known_f(class SeeBorg *self, const std::string command);
std::string CMD_Contexts_f(class SeeBorg *self, const std::string command);
std::string CMD_Unlearn_f(class SeeBorg *self, const std::string command);
std::string CMD_Replace_f(class SeeBorg *self, const std::string command);
std::string CMD_Quit_f(class SeeBorg *self, const std::string command);

extern const botcommand_t botcmds[];
extern const int numbotcmds;

// ---------------------------------------------------------------------------

#endif
