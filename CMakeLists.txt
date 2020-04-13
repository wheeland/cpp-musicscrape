cmake_minimum_required(VERSION 2.8)
project(musicscrape)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ".")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(MUSICSCRAPE_GUMBO_SRC "3rdparty/gumbo-parser/src/" CACHE STRING "Path to gumbo sources")
set(MUSICSCRAPE_JSON_INCLUDE "3rdparty/rapidjson/include/" CACHE STRING "Path to rapidjson headers")

option(MUSICSCRAPE_BUILD_QMUSICSCRAPE "Build QMusicScrape" OFF)
option(MUSICSCRAPE_BUILD_TESTS "Build Tests using Qt" OFF)

include_directories(.)
include_directories(${MUSICSCRAPE_GUMBO_SRC})
include_directories(${MUSICSCRAPE_JSON_INCLUDE})

set(MUSICSCRAPE_SRC 
    "musicscrape/musicscrape.cpp"
    "${MUSICSCRAPE_GUMBO_SRC}/attribute.c"
    "${MUSICSCRAPE_GUMBO_SRC}/char_ref.c"
    "${MUSICSCRAPE_GUMBO_SRC}/error.c"
    "${MUSICSCRAPE_GUMBO_SRC}/parser.c"
    "${MUSICSCRAPE_GUMBO_SRC}/string_buffer.c"
    "${MUSICSCRAPE_GUMBO_SRC}/string_piece.c"
    "${MUSICSCRAPE_GUMBO_SRC}/tag.c"
    "${MUSICSCRAPE_GUMBO_SRC}/tokenizer.c"
    "${MUSICSCRAPE_GUMBO_SRC}/utf8.c"
    "${MUSICSCRAPE_GUMBO_SRC}/util.c"
    "${MUSICSCRAPE_GUMBO_SRC}/vector.c"
)

if(MUSICSCRAPE_BUILD_QMUSICSCRAPE)
    find_package(Qt5 COMPONENTS Core Network)
    set(CMAKE_AUTOMOC ON)
    set(MUSICSCRAPE_SRC ${MUSICSCRAPE_SRC} "musicscrape/qmusicscrape.cpp")
endif()

add_library(musicscrape STATIC ${MUSICSCRAPE_SRC})

if(MUSICSCRAPE_BUILD_QMUSICSCRAPE)
    qt5_use_modules(musicscrape Core Network)
endif()

if(MUSICSCRAPE_BUILD_TESTS)
    find_package(Qt5 COMPONENTS Core Network)
    set(CMAKE_AUTOMOC ON)
    include_directories("musicscrape")

    add_executable(test_plain "test/test_plain.cpp")
    qt5_use_modules(test_plain Core Network)
    target_link_libraries(test_plain musicscrape)

    if(MUSICSCRAPE_BUILD_QMUSICSCRAPE)
        add_executable(test_qmusicscrape "test/test_qmusicscrape.cpp")
        qt5_use_modules(test_qmusicscrape Core Network)
        target_link_libraries(test_qmusicscrape musicscrape)
    endif()
endif()
