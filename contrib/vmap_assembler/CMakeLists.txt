# This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.

cmake_minimum_required(VERSION 3.5)

project(MANGOS_VMAP_ASSEMB_IO C CXX)

if(NOT MSVC)
    add_definitions("-Wall")
    add_definitions("-O3")
endif()

set(G3DLITE_LIBRARY
    "C:/Users/maidt/Desktop/MaNGOS/source/Repack 16 Source/build/dep/src/g3dlite/Release/g3dlite.lib"
)

set(ZLIB_LIBRARY
    "C:/Users/maidt/Desktop/MaNGOS/source/Repack 16 Source/build/dep/src/zlib/Release/zlib.lib"
)

set(SHARED_LIBRARY
    "C:/Users/maidt/Desktop/MaNGOS/source/Repack 16 Source/build/src/shared/Release/shared.lib"
)

set(FRAMEWORK_LIBRARY
    "C:/Users/maidt/Desktop/MaNGOS/source/Repack 16 Source/build/src/framework/Release/framework.lib"
)

include_directories(
    ../../src/shared
    ../../src/game
    ../../src/game/vmap
    ../../src/game/Commands
    ../../dep/include/g3dlite
    ../../dep/include/zlib
    ../../src/framework
    ${ACE_INCLUDE_DIR}
)

add_library(vmap STATIC
    ../../src/game/vmap/BIH.cpp
    ../../src/game/vmap/VMapManager2.cpp
    ../../src/game/vmap/MapTree.cpp
    ../../src/game/vmap/TileAssembler.cpp
    ../../src/game/vmap/WorldModel.cpp
    ../../src/game/vmap/ModelInstance.cpp
)

set_target_properties(vmap PROPERTIES
    FOLDER "Game Libs"
)

target_link_libraries(vmap PRIVATE
    "${G3DLITE_LIBRARY}"
    "${ZLIB_LIBRARY}"
)

add_executable(vmap_assembler
    vmap_assembler.cpp
)

set_target_properties(vmap_assembler PROPERTIES
    FOLDER "Extractors"
)

target_link_libraries(vmap_assembler PRIVATE
    vmap
    "${G3DLITE_LIBRARY}"
    "${ZLIB_LIBRARY}"
    "${ACE_LIBRARIES}"
    "${SHARED_LIBRARY}"
    "${FRAMEWORK_LIBRARY}"
)

install(
    TARGETS vmap_assembler
    DESTINATION ${BIN_DIR}
)