#include <fstream>
#include <regex>
#include <string>
#include <vector>

using namespace std;

vector<string> GetAlreadyMountedDiskIds() {
    smatch match;
    vector<string> alreadyMounted;
    ifstream file("/etc/mtab");
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            if (regex_search(line, match, regex("^(/dev/sd[a-z]+)"))) {
                string disk = match[1];
                alreadyMounted.push_back(disk);
            }
        }
        file.close();
    }

    return alreadyMounted;
}

