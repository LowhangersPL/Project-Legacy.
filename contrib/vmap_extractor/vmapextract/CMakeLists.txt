# This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

cmake_minimum_required(VERSION 3.5)

project(MANGOS_IOMAP_EXTRACTOR C CXX)

set(LIBMPQ_LIBRARY
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../dep/libmpq/build/Release/libmpq.lib"
)

add_executable(vmapextractor
    adtfile.cpp
    dbcfile.cpp
    gameobject_extract.cpp
    model.cpp
    mpq_libmpq.cpp
    vmapexport.cpp
    wdtfile.cpp
    wmo.cpp
)

set_target_properties(vmapextractor PROPERTIES
    FOLDER "Extractors"
)

target_include_directories(vmapextractor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../dep/libmpq"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../dep/libmpq/libmpq"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../dep/include/bzip2"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../dep/src/zlib"
)

if(MSVC)
    target_compile_definitions(vmapextractor PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

if(NOT EXISTS "${LIBMPQ_LIBRARY}")
    message(FATAL_ERROR
        "libmpq.lib was not found at: ${LIBMPQ_LIBRARY}"
    )
endif()

target_link_libraries(vmapextractor PRIVATE
    "${LIBMPQ_LIBRARY}"
    bz2
)

install(TARGETS vmapextractor DESTINATION ${BIN_DIR})