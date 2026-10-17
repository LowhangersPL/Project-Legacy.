# This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

cmake_minimum_required(VERSION 3.5)

project(MANGOS_MAP_EXTRACTOR C CXX)

set(LIBMPQ_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../dep/libmpq"
)

set(LIBMPQ_LIBRARY
    "${LIBMPQ_ROOT}/build/Release/libmpq.lib"
)

add_executable(mapextractor
    dbcfile.cpp
    mpq_libmpq.cpp
    System.cpp
    loadlib/loadlib.cpp
    loadlib/adt.cpp
    loadlib/wdt.cpp
)

set_target_properties(mapextractor PROPERTIES
    FOLDER "Extractors"
)

target_include_directories(mapextractor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/loadlib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/game/Maps"

    # Needed because the source includes <libmpq/mpq.h>
    "${LIBMPQ_ROOT}"

    # Extra dependency headers
    "${CMAKE_CURRENT_SOURCE_DIR}/../../dep/include/bzip2"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../dep/src/zlib"
)

if(MSVC)
    target_compile_definitions(mapextractor PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

if(NOT EXISTS "${LIBMPQ_LIBRARY}")
    message(FATAL_ERROR
        "libmpq.lib was not found at: ${LIBMPQ_LIBRARY}"
    )
endif()

set(BZ2_LIBRARY
    "${CMAKE_CURRENT_SOURCE_DIR}/../vmap_extractor/build/Release/bz2.lib"
)

if(NOT EXISTS "${BZ2_LIBRARY}")
    message(FATAL_ERROR
        "bz2.lib was not found at: ${BZ2_LIBRARY}"
    )
endif()

target_link_libraries(mapextractor PRIVATE
    "${LIBMPQ_LIBRARY}"
    "${BZ2_LIBRARY}"
)

install(TARGETS mapextractor DESTINATION ${BIN_DIR})