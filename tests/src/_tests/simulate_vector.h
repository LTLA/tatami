#ifndef SIMULATE_VECTOR_H
#define SIMULATE_VECTOR_H

#include <random>
#include <vector>
#include <memory>

#include "tatami/base/Matrix.hpp"
#include "tatami/base/DenseMatrix.hpp"

template<typename T>
std::vector<T> simulate_dense_vector(size_t length, double lower = 0, double upper = 100, size_t seed = 1234567890) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<> unif(lower, upper);
    std::vector<T> values(length);
    for (auto& v : values) {
        v = unif(rng);
    }
    return values;
}

template<typename T>
std::vector<T> simulate_sparse_vector(size_t length, double density, double lower = -10, double upper = 10, size_t seed = 1234567890) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<> nonzero(0.0, 1.0);
    std::uniform_real_distribution<> unif(lower, upper);
    std::vector<T> values(length);
    for (auto& v : values) {
        if (nonzero(rng) < density) {
            v = unif(rng);
        }
    }
    return values;
}

template<typename T>
struct SparseDetails {
    std::vector<T> value;
    std::vector<int> index;
    std::vector<size_t> ptr;
};

template<typename T>
SparseDetails<T> simulate_sparse_triplets(size_t primary, size_t secondary, double density, double lower = -10, double upper = 10, size_t seed = 1234567890) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<> nonzero(0.0, 1.0);
    std::uniform_real_distribution<> unif(lower, upper);

    SparseDetails<T> output;
    output.ptr.resize(primary + 1);
    for (size_t p = 0; p < primary; ++p) {
        auto& idx = output.ptr[p + 1];
        idx = output.ptr[p];

        for (size_t s = 0; s < secondary; ++s) {
            if (nonzero(rng) < density) {
                output.value.push_back(unif(rng));
                output.index.push_back(s);
                ++idx;
            }
        }
    }

    return output;
}

#endif
