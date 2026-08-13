#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

namespace cardmesh::testing {

inline int& failureCount() {
    static int count = 0;
    return count;
}

inline void check(bool condition, const std::string& description, const char* file, int line) {
    if (!condition) {
        std::cerr << "FAIL " << file << ":" << line << " - " << description << "\n";
        ++failureCount();
    }
}

}  // namespace cardmesh::testing

#define CARDMESH_CHECK(condition) \
    cardmesh::testing::check((condition), #condition, __FILE__, __LINE__)

#define CARDMESH_TEST_MAIN_BEGIN() int main() {
#define CARDMESH_TEST_MAIN_END()                                    \
    return cardmesh::testing::failureCount() == 0 ? EXIT_SUCCESS : EXIT_FAILURE; \
    }
