
#pragma once

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>

inline std::mutex printf_lock;

inline void log(std::string fmt, ...) {
    std::lock_guard<std::mutex> lock(printf_lock);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt.c_str(), args);
    va_end(args);
    fflush(stdout);
}

inline std::string last_file(std::string path) {
    size_t p = path.find_last_of("\\/") + 1;
    return path.substr(p, path.size() - p);
}

/// Log informational message
#define info(fmt, ...)                                                                             \
    (void)(log("%s:%u [info] " fmt "\n", last_file(__FILE__).c_str(), __LINE__, ##__VA_ARGS__))

/// Log warning (red)
#define warn(fmt, ...)                                                                             \
    (void)(log("\033[0;31m%s:%u [warn] " fmt "\033[0m\n", last_file(__FILE__).c_str(), __LINE__,   \
               ##__VA_ARGS__))

/// Log fatal error and exit program
#define die(fmt, ...)                                                                              \
    (void)(log("\033[0;31m%s:%u [fatal] " fmt "\033[0m\n", last_file(__FILE__).c_str(), __LINE__,  \
               ##__VA_ARGS__),                                                                     \
           std::exit(__LINE__));

#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#elif defined(__GNUC__)
#define DEBUG_BREAK __builtin_trap()
#elif defined(__clang__)
#define DEBUG_BREAK __builtin_debugtrap()
#else
#error Unsupported compiler.
#endif

#define fail_assert(msg, file, line)                                                               \
    (void)(log("\033[1;31m%s:%u [ASSERT] " msg "\033[0m\n", file, line), DEBUG_BREAK,              \
           std::exit(__LINE__), 0)

#undef assert
#define assert(expr)                                                                               \
    (void)((!!(expr)) || (fail_assert(#expr, last_file(__FILE__).c_str(), __LINE__), 0))
