cmake_minimum_required(VERSION 3.24)
project(LedTimerSchedulerPlayground)

set(CMAKE_CXX_STANDARD 20)

file(TO_CMAKE_PATH $ENV{GITHUB_DIR} GITHUB_DIR)

add_executable(LedTimerSchedulerPlayground
        main.cpp
        ${GITHUB_DIR}/googletest/googletest/src/gtest-all.cc
        ${GITHUB_DIR}/fmt/src/format.cc
        ${GITHUB_DIR}/fmt/src/os.cc
)

target_include_directories(LedTimerSchedulerPlayground
    PRIVATE
        ${GITHUB_DIR}/googletest/googletest/include/
        ${GITHUB_DIR}/googletest/googletest/
        ${GITHUB_DIR}/fmt/include
)