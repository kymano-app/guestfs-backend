#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#define DEV_PATH "/dev/virtio-ports/guestexec"

using namespace std;
using namespace chrono;
using namespace this_thread;

void th(int dev_fd, string cmd) {
    cout << "cmd: " << cmd << endl;

    array<char, 128> cmdResultBuffer;
    string result;

    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        perror("popen() failed!");
        exit(1);
    }
    while (fgets(cmdResultBuffer.data(), cmdResultBuffer.size(), pipe.get()) !=
           nullptr) {
        write(dev_fd, cmdResultBuffer.data(), strlen(cmdResultBuffer.data()));
    }
    cout << "end" << endl;

    write(dev_fd, "end\n", strlen("end\n"));
}
int main(int argc, char* argv[]) {
    int dev_fd;
    if ((dev_fd = open(DEV_PATH, O_RDWR)) == -1) {
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
