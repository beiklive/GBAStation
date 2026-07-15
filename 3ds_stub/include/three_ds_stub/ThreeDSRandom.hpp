#pragma once

#include <cstdlib>
#include <string>

#include <cryptopp/cryptlib.h>

namespace CryptoPP {

class AutoSeededRandomPool final : public RandomNumberGenerator {
public:
    explicit AutoSeededRandomPool(bool = false, unsigned int = 32) {}

    std::string AlgorithmName() const override {
        return "Nintendo Switch arc4random";
    }

    void GenerateBlock(byte* output, std::size_t size) override {
        arc4random_buf(output, size);
    }
};

} // namespace CryptoPP
