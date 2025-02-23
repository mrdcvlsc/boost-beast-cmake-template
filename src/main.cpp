#include <iostream>
#include "libcurl.h"

int main() {
    std::cout << "testing start\n";
    perform_curl_request();
    std::cout << "program test success\n";
    return 0;
}