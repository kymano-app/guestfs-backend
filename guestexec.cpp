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

const int bufSize = 256;

void send0Byte(int sockfd) {
    uint8_t zero_byte(0);
    write(sockfd, &zero_byte, 1);
}

void th(int dev_fd, string cmd) {
    cout << "cmd: " << cmd << endl;
    smatch match;
    regex_search(cmd, match, regex("^(.*?)#kymano#(.*)"));
    string cmdId = match[1];
    string endWithCmdId = "end" + cmdId + "\0";
    string cmd_ = match[2];
    cout << "endWithCmdId: " << endWithCmdId << endl;
    cout << "cmd_: " << cmd_ << endl;

    array<char, bufSize> cmdResultBuffer;
    string result;

    FILE* pipe = NULL;
    pipe = popen(cmd_.c_str(), "r");
    // FILE* pipe = popen(cmd_.c_str(), "r");
    // unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd_.c_str(), "r"),
    // pclose);
    // auto pipe = shared_ptr<FILE>( popen(cmd_.c_str(), "r"), &pclose );
    //  if (!pipe) {
    //      perror("popen() failed!");
    //      exit(1);
    //  }

    // int nulls = 0;
    string szResult;
    // while (nulls < 10) {
    // cout << "nulls: " << nulls << endl;
    cmdResultBuffer = {0};
    // while (fgets(cmdResultBuffer.data(), cmdResultBuffer.size(),
    //              pipe) != NULL) {
    while (!feof(pipe)) {
        // try to read 255 bytes from the stream, this operation is BLOCKING
        int nRead = fread(cmdResultBuffer.data(), 1, bufSize - 1, pipe);
        // cout << std::string(&cmdResultBuffer[0], 256) << endl;

        // there are something or nothing to read because the stream is
        // closed or the program catch an error signal
        cout << "nRead: " << nRead << endl;
        if (nRead > 0) {
            // cmdResultBuffer[nRead] = '\0';
            // szResult += cmdResultBuffer.data();
        }
        // cout << szResult << endl;
        // cout << "fgets" << endl;
        // nulls = 0;
        //  cmdResultBuffer[strlen(cmdResultBuffer.data()) - 1] = '\n';
        //  string data = string(cmdResultBuffer.data());
        //  int dataSize = data.size();
        //  m.lock();
        //  if (dataSize < bufSize - 1) {
        //      cout << "dataSize: " << dataSize << endl;
        //      dataSize = dataSize + 1;
        //  }
        write(dev_fd, cmdResultBuffer.data(), nRead);
        // m.unlock();
    }
    // nulls++;
    //}
    // m.lock();
    send0Byte(dev_fd);
    write(dev_fd, endWithCmdId.c_str(), endWithCmdId.size());
    send0Byte(dev_fd);
    // m.unlock();
    cout << "unlocked: " << cmd_ << endl;
}

int main(int argc, char* argv[]) {
    int dev_fd;
    string devPath = string(DEV_PATH) + string(argv[1]);
    if ((dev_fd = open(devPath.c_str(), O_RDWR)) == -1) {
        perror("open");
        exit(1);
    }

    while (true) {
        char cmd[512] = {0};
        memset(cmd, 0, sizeof cmd);
        int valread = read(dev_fd, cmd, 512);
        if (valread == 0) {
            sleep_for(milliseconds(100));
            continue;
        }
        std::thread t(th, dev_fd, string(cmd));
        t.detach();
    }
    return 0;
}