cmake_minimum_required(VERSION 3.16)

project(DistributedCacheEngine
    VERSION 1.0.0
    DESCRIPTION "Production-grade modular distributed cache engine skeleton"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DCE_BUILD_TESTS "Build DistributedCacheEngine tests" ON)

set(DCE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB_RECURSE DCE_LIBRARY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/storage/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/network/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/persistence/*.cpp"
)

add_library(distributed_cache_engine STATIC ${DCE_LIBRARY_SOURCES})

target_include_directories(distributed_cache_engine
    PUBLIC
        "${DCE_INCLUDE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_features(distributed_cache_engine PUBLIC cxx_std_17)

target_compile_options(distributed_cache_engine
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_executable(cache_server main.cpp)
target_link_libraries(cache_server PRIVATE distributed_cache_engine)

find_path(READLINE_INCLUDE_DIR readline/readline.h)
find_library(READLINE_LIBRARY NAMES readline)
find_library(READLINE_TERMCAP_LIBRARY NAMES ncurses curses termcap)

if(READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
    target_compile_definitions(cache_server PRIVATE USE_READLINE=1)
    target_include_directories(cache_server PRIVATE "${READLINE_INCLUDE_DIR}")
    target_link_libraries(cache_server PRIVATE "${READLINE_LIBRARY}")

    if(READLINE_TERMCAP_LIBRARY)
        target_link_libraries(cache_server PRIVATE "${READLINE_TERMCAP_LIBRARY}")
    endif()
endif()

if(DCE_BUILD_TESTS)
    enable_testing()

    add_executable(cache_engine_tests
        tests/test_cache_engine.cpp
    )

    target_link_libraries(cache_engine_tests PRIVATE distributed_cache_engine)
    add_test(NAME cache_engine_tests COMMAND cache_engine_tests)
endif()
