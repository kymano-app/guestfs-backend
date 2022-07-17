#include <string>
#include <vector>

using namespace std;

vector<string> Explode(string s, string const& delimiter) {
    vector<string> result;
    size_t pos = 0;
    string token;
    while ((pos = s.find(delimiter)) != string::npos) {
        token = s.substr(0, pos);
        result.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    result.push_back(s);

    return result;
}
