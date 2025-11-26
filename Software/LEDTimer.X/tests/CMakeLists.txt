cmake_minimum_required(VERSION 3.30)

project(Tests)

enable_testing()

##
# Catch2 must be built
# 1. cmake -B build -DCMAKE_INSTALL_PREFIX=install
# 2. cmake --build build --parallel
# 3. cmake --install build
##
#list(APPEND CMAKE_PREFIX_PATH "../../../3rdparty/catch2/install")
#find_package(Catch2 3 REQUIRED)

##
# Catch2
##
Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.11.0
)

FetchContent_MakeAvailable(Catch2)
##

function(setup_common_test_params target)
    target_link_libraries(${target}
        PRIVATE
            Catch2::Catch2WithMain
    )

    target_include_directories(${target}
        PRIVATE
            ../src
    )
endfunction()

add_subdirectory(dst)
