#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <mutex>
#include <regex>
#include <thread>

#define DEV_PATH "/dev/virtio-ports/"

using namespace std;
using namespace chrono;
using namespace this_thread;
std::mutex m;

void th(int dev_fd, string cmd) {
    cout << "cmd: " << cmd << endl;
    smatch match;
    regex_search(cmd, match, regex("^(.*?)#kymano#(.*)"));
    string cmdId = match[1];
    string endWithCmdId = "end" + cmdId + '\n';
    string cmd_ = match[2];
    cout << "endWithCmdId: " << endWithCmdId << endl;
    cout << "cmd_: " << cmd_ << endl;

    array<char, 128> cmdResultBuffer;
    string result;

    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd_.c_str(), "r"), pclose);
    if (!pipe) {
        perror("popen() failed!");
        exit(1);
    }

    while (fgets(cmdResultBuffer.data(), cmdResultBuffer.size(), pipe.get()) !=
           nullptr) {
        m.lock();
        write(dev_fd, cmdResultBuffer.data(), strlen(cmdResultBuffer.data()));
        m.unlock();
    }
    m.lock();
    write(dev_fd, endWithCmdId.c_str(), endWithCmdId.size());
    m.unlock();
    cout << "unlocked: " << cmd_ << endl;
}

int main(int argc, char* argv[]) {
    int dev_fd;
    if ((dev_fd = open(DEV_PATH + argv[1], O_RDWR)) == -1) {
        perror("open");
        exit(1);
    }

    while (true) {
        char cmd[512] = {0};
        memset(cmd, 0, sizeof cmd);
        int valread = read(dev_fd, cmd, 512);
        if (valread == 0) {
            sleep_for(milliseconds(50));
            continue;
        }
        std::thread t(th, dev_fd, string(cmd));
        t.detach();
    }
    return 0;
}