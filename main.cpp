#include <iostream>
#include <functional>
#include <utility>
#include <memory>
#include <thread>
#include <chrono>
#include "promise.hpp"

using namespace std::chrono_literals;
int main()
{
    Promise<int> p{[](auto resolve) {
        // resolve(42);
    }};

    p.then([](int value) {
         std::cout << value << std::endl;
         return Promise<std::string>{[](auto resolve) {
             std::this_thread::sleep_for(1s);
             resolve("Hello");
         }};
     })
        .then([](const std::string &str) {
            std::cout << str << " Hey" << std::endl;
        })
        .then([](...) {
            std::cout << "The end";
        });

    std::cout << "I am not the last one" << std::endl;
    p.get_state_pointer()->resolve(42);
    return 0;
}
