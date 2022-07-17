#include <string>
#include <vector>
#include <algorithm> 

using namespace std;

bool ExistsInArray(vector<string>& array, string search) {
    return find(begin(array), end(array), search) != end(array);
}