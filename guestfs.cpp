#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

using namespace std;
struct stat info;
namespace fs = filesystem;

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

struct diskIdAndFS {
    string diskId;
    string fs;
};

vector<diskIdAndFS> getDiskIdsAndFs(string diskName) {
    vector<string> excludeFlags = {"esp", "boot", "msftres"};
    auto parted =
        explode(execAndReturnResult("parted -lm 2>/dev/null"), "BYT;");
    vector<diskIdAndFS> diskIds;
    for (int i0 = 1; i0 < parted.size(); i0++) {
        auto oneDiskArray = explode(parted[i0], "\n");
        if ((oneDiskArray[1].find(diskName) != string::npos)) {
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
    }

    return diskIds;
}

struct connectedKymanoDisksStruct {
    string disk;
    string kymanoHash;
};

vector<connectedKymanoDisksStruct> getConnectedKymanoDisks() {
    auto alreadyMounted = getAlreadyMounted();
    vector<connectedKymanoDisksStruct> connectedKymanoDisks;
    for (const auto& entry : fs::directory_iterator("/dev/disk/by-id")) {
        string line = entry.path();
        smatch match;
        if (regex_search(line, match, regex("^.*HARDDISK_KY-(\\w+)-0:0$"))) {
            string driveKymanoHash = match[1];
            if (!existsInArray(alreadyMounted, driveKymanoHash)) {
                string disk = fs::path(fs::read_symlink(line)).filename();
                connectedKymanoDisksStruct connectedKymanoDisks_;
                connectedKymanoDisks_.disk = disk;
                connectedKymanoDisks_.kymanoHash = driveKymanoHash;
                connectedKymanoDisks.push_back(connectedKymanoDisks_);
            }
        }
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
    if (stat("/mnt/kymano", &info) != 0) {
        fs::create_directories("/mnt/kymano");
    }

    auto connectedKymanoDisks = getConnectedKymanoDisks();
    for (connectedKymanoDisksStruct connectedKymanoDisk :
         connectedKymanoDisks) {
        auto diskIdsAndFs = getDiskIdsAndFs(connectedKymanoDisk.disk);
        for (diskIdAndFS diskIdAndFS_ : diskIdsAndFs) {
            string diskAndDiskId =
                connectedKymanoDisk.disk + diskIdAndFS_.diskId;
            string mountDirName = "/mnt/kymano/" +
                                  connectedKymanoDisk.kymanoHash + "/" +
                                  diskAndDiskId;
            string mountCmd = "mount -t " + diskIdAndFS_.fs + " /dev/" +
                              diskAndDiskId + " " + mountDirName;
            if (stat(mountDirName.c_str(), &info) != 0) {
                fs::create_directories(mountDirName);
            }
            int returnCode = system(mountCmd.c_str());
            if (returnCode != 0) {
                string umount = "umount " + mountDirName;
                execAndReturnResult(umount.c_str());
                removeDirectoryIfUnmounted(connectedKymanoDisk.kymanoHash);
            }
        }
    }
    
    cout << "OK";
}