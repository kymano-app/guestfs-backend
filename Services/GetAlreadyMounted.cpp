#include <fstream>
#include <regex>
#include <string>
#include <vector>

using namespace std;

vector<string> getAlreadyMounted() {
    smatch match;
    vector<string> alreadyMounted;
    ifstream file("/etc/mtab");
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            if (regex_search(line, match, regex("^.*/mnt/kymano/(\\w+).*"))) {
                alreadyMounted.push_back(match[1]);
            }
        }
        file.close();
    }

    return alreadyMounted;
}
