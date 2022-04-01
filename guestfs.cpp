#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <thread>
#include <vector>

using namespace std;
struct stat info;
namespace fs = filesystem;
using json = nlohmann::json;

const string VERSION = "0.0.4";

bool existsInArray(vector<string>& array, string search) {
    return find(begin(array), end(array), search) != end(array);
}

vector<string> explode(string s, string const& delimiter) {
    vector<string> result;
    size_t pos = 0;
    string token;
    while ((pos = s.find(delimiter)) != string::npos) {
        token = s.substr(0, pos);
        result.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    result.push_back(s);

    return result;
}

string execAndReturnResult(const char* cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

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

vector<string> getAlreadyMountedDiskIds() {
    smatch match;
    vector<string> alreadyMounted;
    ifstream file("/etc/mtab");
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            if (regex_search(line, match, regex("^(/dev/sd[a-z]+)"))) {
                alreadyMounted.push_back(match[1]);
                cout << "alreadyMounted: " << match[1] << '\n';
            }
        }
        file.close();
    }

    return alreadyMounted;
}

struct diskIdAndFS {
    string diskId;
    string fs;
};

vector<diskIdAndFS> getDiskIdsAndFs(string diskName) {
    cout << "getDiskIdsAndFs: " << diskName << endl;
    vector<string> excludeFlags = {"esp", "boot", "msftres", "swap"};
    auto parted =
        explode(execAndReturnResult("parted -lm 2>/dev/null"), "BYT;");
    vector<diskIdAndFS> diskIds;
    for (int i0 = 1; i0 < parted.size(); i0++) {
        auto oneDiskArray = explode(parted[i0], "\n");
        smatch match;
        if (!regex_search(oneDiskArray[1], match,
                          regex("(/dev/" + diskName + ":)"))) {
            continue;
        }
        cout << "oneDiskArray[1]" << oneDiskArray[1] << endl;
        for (int i = 2; i < oneDiskArray.size(); i++) {
            auto lineArray = explode(oneDiskArray[i], ":");
            if (lineArray.size() == 1) {
                continue;
            }
            string diskId = lineArray[0];
            string flagsStr = lineArray[6];
            flagsStr = flagsStr.substr(0, flagsStr.find(";"));
            auto flags = explode(flagsStr, ", ");
            bool skip = false;
            for (string flag : flags) {
                if (existsInArray(excludeFlags, flag)) {
                    skip = true;
                    continue;
                }
            }
            if (skip) {
                continue;
            }
            diskIdAndFS diskIdAndFS_;
            diskIdAndFS_.diskId = diskId;
            diskIdAndFS_.fs = lineArray[4];
            diskIds.push_back(diskIdAndFS_);
        }
    }

    return diskIds;
}

struct connectedKymanoDisksStruct {
    string disk;
    string kymanoHash;
};

vector<connectedKymanoDisksStruct> getConnectedKymanoDisks() {
    auto alreadyMounted = getAlreadyMountedDiskIds();
    vector<connectedKymanoDisksStruct> connectedKymanoDisks;
    for (const auto& entry : fs::directory_iterator("/dev/")) {
        smatch match;
        const string diskpath = entry.path();
        if (!regex_search(diskpath, match, regex("^/dev/(sd[a-z]+)$"))) {
            continue;
        }
        string diskId = match[1];
        string diskPath = "/dev/" + diskId;
        cout << diskPath << endl;

        if (existsInArray(alreadyMounted, diskPath)) {
            cout << diskPath << ": "
                 << "continue" << endl;
            continue;
        }
        cout << "new:" << diskPath << endl;

        string cmd = "smartctl -a " + diskPath + " -d scsi -j";
        string smartctl = execAndReturnResult(cmd.c_str());
        json diskInfoJson = json::parse(smartctl);

        if (diskInfoJson["serial_number"] == nlohmann::detail::value_t::null) {
            continue;
        }
        auto splittedSerial = explode(diskInfoJson["serial_number"], "KY-");
        string driveKymanoHash = splittedSerial[1];

        connectedKymanoDisksStruct connectedKymanoDisks_;
        connectedKymanoDisks_.disk = diskId;
        connectedKymanoDisks_.kymanoHash = driveKymanoHash;
        connectedKymanoDisks.push_back(connectedKymanoDisks_);

        cout << diskId << " : " << driveKymanoHash << endl;
    }

    return connectedKymanoDisks;
}

void removeDirectoryIfUnmounted(string driveKymanoHash) {
    auto alreadyMounted = getAlreadyMounted();
    if (!existsInArray(alreadyMounted, driveKymanoHash)) {
        fs::remove_all("/mnt/kymano/" + driveKymanoHash);
    }
}

int main() {
    cout << VERSION << '\n';
    if (stat("/mnt/kymano", &info) != 0) {
        fs::create_directories("/mnt/kymano");
    }

    auto connectedKymanoDisks = getConnectedKymanoDisks();
    cout << "connectedKymanoDisks: " << connectedKymanoDisks.size() << endl;
    for (connectedKymanoDisksStruct connectedKymanoDisk :
         connectedKymanoDisks) {
        auto diskIdsAndFs = getDiskIdsAndFs(connectedKymanoDisk.disk);

        for (int i = 0; i < diskIdsAndFs.size(); i++) {
            cout << "diskIdsAndFs[i]: " << diskIdsAndFs[i].diskId << endl;
        }
        for (int i = 0; i < diskIdsAndFs.size(); i++) {
            diskIdAndFS diskIdAndFS_ = diskIdsAndFs[i];

            string diskAndDiskId =
                connectedKymanoDisk.disk + diskIdAndFS_.diskId;
            cout << "diskAndDiskId: " << diskAndDiskId << endl;

            string mountDirName = "/mnt/kymano/" +
                                  connectedKymanoDisk.kymanoHash + "/" +
                                  diskAndDiskId;
            string mountCmd = "mount -o ro -t '" + diskIdAndFS_.fs + "' /dev/" +
                              diskAndDiskId + " " + mountDirName;
            if (stat(mountDirName.c_str(), &info) != 0) {
                fs::create_directories(mountDirName);
            }
            cout << "mountCmd: " << mountCmd << endl;
            int returnCode = system(mountCmd.c_str());
            cout << "returnCode: " << returnCode << endl;
            if (returnCode == 0) {
                continue;
            }
            string umount = "umount " + mountDirName;
            cout << "umount: " << umount << endl;
            execAndReturnResult(umount.c_str());
            removeDirectoryIfUnmounted(connectedKymanoDisk.kymanoHash);
        }
    }
    this_thread::sleep_for(chrono::milliseconds(2000));
}