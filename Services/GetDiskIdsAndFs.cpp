#include <Struct/diskIdAndFS.cpp>
#include <string>
#include <vector>

using namespace std;

vector<diskIdAndFS> GetDiskIdsAndFs(string diskName) {
    vector<string> excludeFlags = {"esp", "boot", "msftres", "swap"};
    auto parted =
        Explode(ExecAndReturnResult("parted -lm 2>/dev/null"), "BYT;");
    vector<diskIdAndFS> diskIds;
    for (int i0 = 1; i0 < parted.size(); i0++) {
        auto oneDiskArray = Explode(parted[i0], "\n");
        smatch match;
        if (!regex_search(oneDiskArray[1], match,
                          regex("(/dev/" + diskName + ":)"))) {
            continue;
        }
        for (int i = 2; i < oneDiskArray.size(); i++) {
            auto lineArray = Explode(oneDiskArray[i], ":");
            if (lineArray.size() == 1) {
                continue;
            }
            string diskId = lineArray[0];
            string flagsStr = lineArray[6];
            flagsStr = flagsStr.substr(0, flagsStr.find(";"));
            auto flags = Explode(flagsStr, ", ");
            bool skip = false;
            for (string flag : flags) {
                if (ExistsInArray(excludeFlags, flag)) {
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
