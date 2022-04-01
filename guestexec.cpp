#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#define DEV_PATH "/dev/virtio-ports/guestexec"

using namespace std;
using namespace chrono;
using namespace this_thread;

int main(int argc, char* argv[]) {
    int server_fd, new_socket, valread, valwrite;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char cmd[512] = {0};
    int dev_fd;

    if ((dev_fd = open(DEV_PATH, O_RDWR)) == -1) {
        perror("open");
        exit(1);
    }

    while (true) {
        memset(cmd, 0, sizeof cmd);
        valread = read(dev_fd, cmd, 512);
        if (valread == 0) {
            sleep_for(milliseconds(50));
            continue;
        }
        cout << "command: " << cmd << endl;

        array<char, 128> cmdResultBuffer;
        string result;

        unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            perror("popen() failed!");
            exit(1);
        }
        while (fgets(cmdResultBuffer.data(), cmdResultBuffer.size(),
                     pipe.get()) != nullptr) {
            int ret = write(dev_fd, cmdResultBuffer.data(),
                            strlen(cmdResultBuffer.data()));
        }
        int ret = write(dev_fd, "end\n", strlen("end\n"));
        if (ret == -1) {
            break;
        }
    }
    return 0;
}
