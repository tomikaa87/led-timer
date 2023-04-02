cmake_minimum_required(VERSION 3.24)
project(LedTimerSchedulerPlayground)

set(CMAKE_CXX_STANDARD 17)

add_executable(LedTimerSchedulerPlayground
        main.cpp
        /Users/tkarpati/github/googletest/googletest/src/gtest-all.cc
        /Users/tkarpati/github/fmt/src/format.cc
        /Users/tkarpati/github/fmt/src/os.cc
)

target_include_directories(LedTimerSchedulerPlayground
    PRIVATE
        /Users/tkarpati/github/googletest/googletest/include/
        /Users/tkarpati/github/googletest/googletest/
        /Users/tkarpati/github/fmt/include
)