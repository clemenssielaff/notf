#include <iostream>
#include <limits>
using namespace std;

//int main() {
int notmain() {
    cout << "unsigned short:\t" << std::numeric_limits<unsigned short>::max() << endl;
    cout << "unsigned int:  \t" << std::numeric_limits<unsigned int>::max() << endl;
    cout << "int:           \t" << std::numeric_limits<int>::max() << endl;
    cout << "size_t:        \t" << std::numeric_limits<size_t>::max() << endl;
    cout << "long:          \t" << std::numeric_limits<long>::max() << endl;
    cout << "long long:     \t" << std::numeric_limits<long long>::max() << endl;
    cout << "int64_t:       \t" << std::numeric_limits<int64_t>::max() << endl;
    return 0;
}
