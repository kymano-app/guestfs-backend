#include <Services/ExecAndReturnResult.cpp>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace std;
namespace fs = filesystem;
using ordered_json = nlohmann::ordered_json;

const string VERSION = "0.0.1";

int main(int argc, char* argv[]) {
    try {
        for (const auto& disk : fs::directory_iterator("/mnt/kymano")) {
            string diskPath = disk.path();
            for (const auto& disk1 : fs::directory_iterator(diskPath.c_str())) {
                string cmd = "umount " + string(disk1.path());
                string cmdSuppressedOutput = cmd + " >/dev/null 2>&1";
                system(cmdSuppressedOutput.c_str());
            }
            fs::remove_all(diskPath);
        }

        return 0;
    } catch (const exception& e) {
        ordered_json jn = {{"v", VERSION},
                           {"error", "UnknownException"},
                           {"message", e.what()}};
        cout << jn.dump() << endl;
    }

    return 1;
}