#include <ctime>
#include <clocale>
#include <iostream>

#include "seeborg.h"
#include "seeutil.h"

using namespace std;

static void PrintReply(const string& text)
{
    cout << "<Seeborg> " << text  << endl;
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    SeeBorg lSeeBorg;

    cout << "SeeBorg v" SEEBORGVERSIONSTRING
        <<", copyright (C) 2003 Eugene Bujak." << endl << endl;

    string body;
    srand(time(NULL));

    cout << "Loading dictionary..." << endl;
    lSeeBorg.LoadSettings();

    cout << endl << "SeeBorg offline chat, learning disabled."
         << " Press CTRL-C to quit." << endl << endl;

    while (1) {
        cout << "> ";
        getline(cin, body);
        if (cin.bad() || cin.eof()) {
          break;
        } else if (body[0] == '!') {
          PrintReply(lSeeBorg.ParseCommands(body));
        } else {
          PrintReply(lSeeBorg.Reply(body));
        }
    }

    lSeeBorg.SaveSettings();
    return 0;
}
