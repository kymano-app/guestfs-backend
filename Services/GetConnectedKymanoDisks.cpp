#include <Services/ExistsInArray.cpp>
#include <Services/GetAlreadyMountedDiskIds.cpp>
#include <Services/Explode.cpp>
#include <Struct/connectedKymanoDisksStruct.cpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace std;
namespace fs = filesystem;

vector<connectedKymanoDisksStruct> GetConnectedKymanoDisks() {
    auto alreadyMounted = GetAlreadyMountedDiskIds();
    vector<connectedKymanoDisksStruct> connectedKymanoDisks;
    for (const auto& entry : fs::directory_iterator("/dev/")) {
        smatch match;
        const string diskpath = entry.path();
        if (!regex_search(diskpath, match, regex("^/dev/(sd[a-z]+)$"))) {
            continue;
        }
        string diskId = match[1];
        string diskPath = "/dev/" + diskId;

        if (ExistsInArray(alreadyMounted, diskPath)) {
            continue;
        }

        string cmd = "smartctl -a " + diskPath + " -d scsi -j";
        string smartctl = ExecAndReturnResult(cmd.c_str());
        json diskInfoJson = json::parse(smartctl);

        if (diskInfoJson["serial_number"] == nlohmann::detail::value_t::null) {
            continue;
        }
        auto splittedSerial = Explode(diskInfoJson["serial_number"], "KY-");
        string driveKymanoHash = splittedSerial[1];

        connectedKymanoDisksStruct connectedKymanoDisks_;
        connectedKymanoDisks_.disk = diskId;
        connectedKymanoDisks_.kymanoHash = driveKymanoHash;
        connectedKymanoDisks.push_back(connectedKymanoDisks_);
    }

    return connectedKymanoDisks;
}
