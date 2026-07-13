module;

#include <iostream>
#include <random>
#include <string>

export module Engine.Basics.Random;

export namespace Engine::Basics::Random {
auto rand_str(int length) -> std::string
{
    const std::string chars = "abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "0123456789";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 62);

    std::string result;
    result.reserve(length);

    for (int i = 0; i < length; ++i) {
        result.push_back(chars[dist(gen)]);
    }
    return result;
}
}
