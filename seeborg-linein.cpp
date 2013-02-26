#include <time.h>
#include <locale.h>

#include "seeborg.h"
#include "seeutil.h"

seeborg_t gSeeBorg;

void PrintReply(string text)
{
    printf("<Seeborg> %s\n", text.c_str());
}


int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    printf("SeeBorg v" SEEBORGVERSIONSTRING
           ", copyright (C) 2003 Eugene Bujak.\n\n");
    string body;
    srand(time(NULL));

    printf("Loading dictionary...\n");
    gSeeBorg.LoadSettings();

#ifndef PROFILE
    printf
    ("\nSeeBorg offline chat, learning disabled. Press CTRL-C to quit.\n\n");

    while (1) {
        printf("> ");
        fReadStringLine(stdin, body);
        if (body == "!quit") {
            break;
            //string trigreply = gSeeBorg.ParseCommands(body);
            //if (trigreply != "") PrintReply (trigreply);
            //continue;
        }

        string seeout = gSeeBorg.Reply(body);
        PrintReply(seeout);
    }
#endif

    gSeeBorg.SaveSettings();
    return 0;
}
