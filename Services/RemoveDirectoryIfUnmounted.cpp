#include <string>
#include <vector>

using namespace std;

void RemoveDirectoryIfUnmounted(string driveKymanoHash) {
    auto alreadyMounted = getAlreadyMounted();
    if (!ExistsInArray(alreadyMounted, driveKymanoHash)) {
        fs::remove_all("/mnt/kymano/" + driveKymanoHash);
    }
}