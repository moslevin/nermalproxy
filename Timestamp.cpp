#include "Timestamp.hpp"

#include <time.h>

std::uint64_t Timestamp()
{
    auto ts = timespec{};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    auto rc = std::uint64_t{};
    rc = (ts.tv_sec * 1000) + (ts.tv_nsec / 10000000);
    return rc;
}
