include(FetchContent)

set(DOCTEST_WITH_TESTS OFF)
set(DOCTEST_NO_INSTALL ON)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/onqtam/doctest.git
    GIT_TAG v2.4.12
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(doctest)

list(APPEND CMAKE_MODULE_PATH ${doctest_SOURCE_DIR}/scripts/cmake)
include(doctest)
 
add_executable(coop_test test.cpp)
target_link_libraries(
    coop_test
    PUBLIC
    coop
    doctest::doctest
)