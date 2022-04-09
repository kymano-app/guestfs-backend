#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
namespace fs = filesystem;

int main(int argc, char* argv[]) {
    for (const auto& disk : fs::directory_iterator("/mnt/kymano")) {
        string diskPath = disk.path();
        for (const auto& disk1 : fs::directory_iterator(diskPath.c_str())) {
            string umount = "umount " + string(disk1.path());
            cout << "umount cmd: " << umount << endl;
            int returnCode = system(umount.c_str());
            cout << "returnCode: " << returnCode << endl;
        }
        cout << "rm -rf " << diskPath << endl;
        fs::remove_all(diskPath);
    }
    return 0;
}