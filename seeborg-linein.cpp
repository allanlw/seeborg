#include <ctime>
#include <clocale>
#include <iostream>

#include "seeborg.h"
#include "seeutil.h"

using namespace std;

seeborg_t gSeeBorg;

void PrintReply(string text)
{
    cout << "<Seeborg> " << text  << endl;
}


int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    cout << "SeeBorg v" SEEBORGVERSIONSTRING
        <<", copyright (C) 2003 Eugene Bujak." << endl << endl;

    string body;
    srand(time(NULL));

    cout << "Loading dictionary..." << endl;
    gSeeBorg.LoadSettings();

#ifndef PROFILE
    cout << endl << "SeeBorg offline chat, learning disabled."
         << " Press CTRL-C to quit." << endl << endl;

    while (1) {
        cout << "> ";
        getline(cin, body);
        if (cin.bad() || cin.eof()) {
          break;
        } else if (body[0] == '!') {
          PrintReply(gSeeBorg.ParseCommands(body));
        } else {
          PrintReply(gSeeBorg.Reply(body));
        }
    }
#endif
    gSeeBorg.SaveSettings();
    return 0;
}
