#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <Exceptions/MountUnknownFilesystemException.cpp>
#include <Services/ExecAndReturnResult.cpp>
#include <Services/GetConnectedKymanoDisks.cpp>
#include <Services/GetAlreadyMounted.cpp>
#include <Services/GetDiskIdsAndFs.cpp>
#include <Services/RemoveDirectoryIfUnmounted.cpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <thread>
#include <vector>

namespace fs = filesystem;

using namespace std;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

const string VERSION = "0.0.7";

struct stat info;

int main() {
    try {
        if (stat("/mnt/kymano", &info) != 0) {
            fs::create_directories("/mnt/kymano");
        }

        auto connectedKymanoDisks = GetConnectedKymanoDisks();
        if (connectedKymanoDisks.size() == 0) {
            ordered_json j2 = {{"v", VERSION}, {"result", "nothing"}};
            cout << j2.dump() << endl;
            return 0;
        }
        for (connectedKymanoDisksStruct connectedKymanoDisk :
             connectedKymanoDisks) {
            auto diskIdsAndFs = GetDiskIdsAndFs(connectedKymanoDisk.disk);

            for (int i = 0; i < diskIdsAndFs.size(); i++) {
                diskIdAndFS diskIdAndFS_ = diskIdsAndFs[i];

                string diskAndDiskId =
                    connectedKymanoDisk.disk + diskIdAndFS_.diskId;

                string mountDirName = "/mnt/kymano/" +
                                      connectedKymanoDisk.kymanoHash + "/" +
                                      diskAndDiskId;
                string fsType = "";
                if (diskIdAndFS_.fs != "") {
                    fsType = " -t '" + diskIdAndFS_.fs + "'";
                }

                if (stat(mountDirName.c_str(), &info) != 0) {
                    fs::create_directories(mountDirName);
                }
                string mountCmd = "mount -o ro " + fsType + " /dev/" +
                                  diskAndDiskId + " " + mountDirName;
                string mountCmdSuppressedOutput = mountCmd + " >/dev/null 2>&1";
                int returnCode = system(mountCmdSuppressedOutput.c_str());
                if (returnCode > 0) {
                    string mountCmdWithOutput = mountCmd + " 2>&1";
                    string mountReturn =
                        ExecAndReturnResult(mountCmdWithOutput.c_str());
                    smatch match;
                    if (regex_search(mountReturn, match,
                                     regex("unknown filesystem type '(.*)'"))) {
                        throw MountUnknownFilesystemException(match[1]);
                    }

                    throw runtime_error(mountReturn);
                } else {
                    continue;
                }
                string umount = "umount " + mountDirName;
                ExecAndReturnResult(umount.c_str());
                RemoveDirectoryIfUnmounted(connectedKymanoDisk.kymanoHash);
            }
        }
        ordered_json jn = {{"v", VERSION}, {"result", "mounted"}};
        cout << jn.dump() << endl;
        return 0;

    } catch (const MountUnknownFilesystemException& e) {
        ordered_json jn = {{"v", VERSION},
                           {"error", "MountUnknownFilesystem"},
                           {"message", e.what()}};
        cout << jn.dump() << endl;
    } catch (const std::exception& e) {
        ordered_json jn = {{"v", VERSION},
                           {"error", "UnknownException"},
                           {"message", e.what()}};
        cout << jn.dump() << endl;
    }
    
    return 1;
}