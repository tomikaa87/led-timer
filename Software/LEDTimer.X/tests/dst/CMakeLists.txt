add_executable(tests-dst
    main.cpp
    ../../Utils.c
    ../../Utils.h
)

setup_common_test_params(tests-dst)

target_include_directories(tests-dst
    PRIVATE
        ../../
)

add_test(
    NAME DST
    COMMAND $<TARGET_FILE:tests-dst>
)
