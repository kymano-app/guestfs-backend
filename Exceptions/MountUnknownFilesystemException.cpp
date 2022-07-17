#include <iostream>

using namespace std;

class MountUnknownFilesystemException : virtual public runtime_error {
   public:
    explicit MountUnknownFilesystemException(const string& msg)
        : runtime_error(msg) {}
};
