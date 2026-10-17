cmake_minimum_required(VERSION 3.5)

project(libmpq C)

# Paths relative to dep/libmpq
set(ZLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/zlib")
set(BZIP2_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include/bzip2")

set(libMPQ_SRCS
    libmpq/common.c
    libmpq/common.h
    libmpq/crypt_buf.h
    libmpq/explode.c
    libmpq/explode.h
    libmpq/extract.c
    libmpq/extract.h
    libmpq/huffman.c
    libmpq/huffman.h
    libmpq/mpq.c
    libmpq/mpq.h
    libmpq/mpq-internal.h
    libmpq/pack_begin.h
    libmpq/pack_end.h
    libmpq/platform.h
    libmpq/wave.c
    libmpq/wave.h
)

# Build zlib directly into libmpq so the resulting libmpq.lib
# does not depend on a separate zlib.lib file.
set(ZLIB_SRCS
    "${ZLIB_DIR}/adler32.c"
    "${ZLIB_DIR}/compress.c"
    "${ZLIB_DIR}/crc32.c"
    "${ZLIB_DIR}/deflate.c"
    "${ZLIB_DIR}/infback.c"
    "${ZLIB_DIR}/inffast.c"
    "${ZLIB_DIR}/inflate.c"
    "${ZLIB_DIR}/inftrees.c"
    "${ZLIB_DIR}/trees.c"
    "${ZLIB_DIR}/uncompr.c"
    "${ZLIB_DIR}/zutil.c"
)

add_library(libmpq STATIC
    ${libMPQ_SRCS}
    ${ZLIB_SRCS}
)

target_include_directories(libmpq PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/libmpq"
    "${ZLIB_DIR}"
    "${BZIP2_INCLUDE_DIR}"
)

if(MSVC)
    target_compile_definitions(libmpq PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

set_target_properties(libmpq PROPERTIES
    FOLDER "3rd Party"
    OUTPUT_NAME "libmpq"
)