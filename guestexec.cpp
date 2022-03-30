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

    // int server_fd, new_socket, valread;
    // struct sockaddr_in address;
    // int opt = 1;
    // int addrlen = sizeof(address);
    // char buffer[1024] = {0};

    // // Creating socket file descriptor
    // if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    //     perror("socket failed");
    //     exit(EXIT_FAILURE);
    // }

    // // Forcefully attaching socket to the port 8080
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
    //                sizeof(opt))) {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }
    // address.sin_family = AF_INET;
    // address.sin_addr.s_addr = INADDR_ANY;
    // address.sin_port = htons(PORT);

    // // Forcefully attaching socket to the port 8080
    // if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    //     perror("bind failed");
    //     exit(EXIT_FAILURE);
    // }
    // if (listen(server_fd, 3) < 0) {
    //     perror("listen");
    //     exit(EXIT_FAILURE);
    // }
    // if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
    //                          (socklen_t *)&addrlen)) < 0) {
    //     perror("accept");
    //     exit(EXIT_FAILURE);
    // }

    // for (int i = 0; i < 10; i++) {
    //     cout << i << endl;

    //     valread = read(new_socket, buffer, 1024);
    //     if (valread == 0) {
    //         cout << "break" << endl;
    //         break;
    //     }
    //     cout << "valread:" << valread;
    //     cout << "buffer:" << buffer;

    //     array<char, 128> buffer2;
    //     string result;
    //     string cmd = "find ./ -name '" + string(buffer) + "'";
    //     cout << cmd << endl;
    //     unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
    //                                              pclose);
    //     if (!pipe) {
    //         throw runtime_error("popen() failed!");
    //     }
    //     while (fgets(buffer2.data(), buffer2.size(), pipe.get()) != nullptr)
    //     {
    //         cout << "line: " << buffer2.data();
    //         try {
    //             int ret =
    //                 send(new_socket, buffer2.data(), strlen(buffer2.data()),
    //                 0);
    //             cout << "ret:" << ret << endl;
    //         } catch (const std::exception &e) {
    //             cout << e.what();
    //         }
    //         cout << "sent" << endl;
    //     }
    //     send(new_socket, "end\n", strlen("end\n"), 0);
    //     printf("Hello message sent\n");
    // }

    printf("return 0\n");

    return 0;
}
