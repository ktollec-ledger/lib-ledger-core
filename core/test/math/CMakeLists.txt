cmake_minimum_required(VERSION 3.0)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include/ledger/core)

add_executable(ledger-core-math-tests
        main.cpp
        bigint_tests.cpp
        bigint_api_tests.cpp
        base58_test.cpp
        fibonacci_test.cpp
        base_converter_tests.cpp)

target_link_libraries(ledger-core-math-tests gtest gtest_main)
target_link_libraries(ledger-core-math-tests ledger-core-static)

include(CopyAndInstallImportedTargets)
if (SYS_OPENSSL)
    find_package(OpenSSL)
    if(WIN32)
        target_link_libraries( ledger-core-math-tests crypt32 ws2_32)
    endif()
    copy_install_imported_targets(ledger-core-math-tests OpenSSL::Crypto)
else()
    copy_install_imported_targets(ledger-core-math-tests crypto)
endif()

include(GoogleTest)
gtest_discover_tests(ledger-core-math-tests PROPERTIES "LABELS;unit")