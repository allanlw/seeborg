#ifndef __SEEBORG_H__
#define __SEEBORG_H__

#include <map>
#include <vector>
#include <string>
#include <set>

#define SEEBORGVERSIONMINOR 0
#define SEEBORGVERSIONMAJOR 51
#define SEEBORGVERSIONSTRING "0.51 beta"

class SeeBorg;

struct BotCommand {
    const char *command;
    const char *description;
    std::string(*func) (SeeBorg *self,
          const std::vector<std::string>& toks);
};

typedef std::pair<std::set<std::string>::iterator, int> context_t;
typedef std::vector<context_t> word_t;

class SeeBorg
{
public:
    SeeBorg();

    int Learn(std::string &body);
    std::string Reply(std::string message);

    int LoadSettings(void);
    int SaveSettings(void);

    std::string ParseCommands(const std::string& command);

    void AddCommands(const BotCommand *);

/* private: */
    int LearnLine(std::string &line);
    int FilterMessage(std::string &message);

    int num_contexts;

    int min_context_depth;
    int max_context_depth;

    std::set<std::string> lines;
    std::map<std::string, word_t> words;

    std::vector<BotCommand> cmds;
};

extern SeeBorg gSeeBorg;

// ---------------------------------------------------------------------------

#endif
