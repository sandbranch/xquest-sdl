cmake_minimum_required(VERSION 3.16)
project(xquest C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

# ── Runtime asset path (baked into the binary) ───────────────────────────────
# Defaults to the game data bundled in this repo's assets/ dir. Override with
# -DXQUEST_ASSET_DIR=... to point at an installed data dir instead, e.g.
# /usr/share/games/xquest for packaging.
set(XQUEST_ASSET_DIR "${CMAKE_SOURCE_DIR}/assets"
    CACHE PATH "Runtime directory the game searches for data files")

# ── Data-source directory for cmake --install ─────────────────────────────────
# Defaults to the bundled assets/. Packaging scripts that bake a different
# runtime XQUEST_ASSET_DIR (e.g. /usr/share/games/xquest) still set this to
# the actual on-disk source to install from.
set(XQUEST_DATA_SOURCE "${CMAKE_SOURCE_DIR}/assets"
    CACHE PATH "Source directory for 'cmake --install' game data (defaults to the bundled assets/)")

# ── Build ──────────────────────────────────────────────────────────────────────
add_executable(xquest
    src/main.c
    src/assets.c
    src/render.c
    src/input.c
    src/game.c
    src/entities.c
    src/collision.c
    src/starfield.c
    src/menu.c
    src/hiscore.c
    src/audio.c
    src/config.c
    src/demo.c
)

target_include_directories(xquest PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(xquest PRIVATE ${SDL2_LIBRARIES})
if(NOT MSVC)
    target_link_libraries(xquest PRIVATE m)
endif()
target_compile_definitions(xquest PRIVATE
    ASSET_DIR="${XQUEST_ASSET_DIR}"
)
if(MSVC)
    target_compile_options(xquest PRIVATE /W4)
else()
    target_compile_options(xquest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Install ────────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

# Binary goes to /usr/games (Debian convention for games)
install(TARGETS xquest RUNTIME DESTINATION games)

# Game data (sprites/sounds/etc.) ships in this repo's assets/. Only guard
# on it actually being present so someone who deliberately strips assets/
# still gets an engine-only build instead of a hard failure; the resulting
# binary picks up data at runtime via XQUEST_DATA_DIR (see main.c).
find_program(PYTHON3_EXE python3)
if(PYTHON3_EXE AND EXISTS "${XQUEST_DATA_SOURCE}/startpic.pbm")
    # Generates <XQUEST_DATA_SOURCE>/startpic.raw alongside the source .pbm.
    set(STARTPIC_RAW "${XQUEST_DATA_SOURCE}/startpic.raw")
    add_custom_command(
        OUTPUT  "${STARTPIC_RAW}"
        COMMAND "${PYTHON3_EXE}"
                "${CMAKE_SOURCE_DIR}/tools/convert_pbm.py"
                "${XQUEST_DATA_SOURCE}/startpic.pbm"
        DEPENDS "${XQUEST_DATA_SOURCE}/startpic.pbm"
                "${CMAKE_SOURCE_DIR}/tools/convert_pbm.py"
        COMMENT "Converting startpic.pbm -> startpic.raw"
    )
    add_custom_target(startpic_raw ALL DEPENDS "${STARTPIC_RAW}")
    add_dependencies(xquest startpic_raw)

    # Read-only game data  → share/games/xquest
    foreach(f xquest.gfx xquest.enm xquest.fnt xquest2.fnt xquest.snd)
        install(FILES "${XQUEST_DATA_SOURCE}/${f}"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/games/xquest"
        )
    endforeach()
    install(FILES "${STARTPIC_RAW}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/games/xquest"
    )
endif()

# Icons - hicolor theme sizes
foreach(sz 16 32 48 64 128 256)
    install(FILES "${CMAKE_SOURCE_DIR}/assets/icons/xquest_${sz}.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${sz}x${sz}/apps"
        RENAME xquest.png
        OPTIONAL
    )
endforeach()

# Desktop entry and man page
install(FILES "${CMAKE_SOURCE_DIR}/debian/xquest.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    OPTIONAL
)
install(FILES "${CMAKE_SOURCE_DIR}/debian/xquest.6"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man6"
    OPTIONAL
)

# ── Tests ─────────────────────────────────────────────────────────────────────
# ctest --test-dir build   (or: cmake --build build && ctest --test-dir build)
# Disable with -DXQUEST_BUILD_TESTS=OFF.
option(XQUEST_BUILD_TESTS "Build the test suite" ON)

if(XQUEST_BUILD_TESTS)
    enable_testing()

    # Engine sources the tests link against: everything except main.c, which
    # owns the SDL window and the game loop.
    set(XQUEST_TEST_SRC
        src/assets.c src/game.c src/entities.c src/collision.c
        src/config.c src/demo.c src/audio.c src/render.c
        src/starfield.c src/input.c src/menu.c src/hiscore.c
    )

    foreach(t test_config test_demo test_replay)
        add_executable(${t} tests/${t}.c ${XQUEST_TEST_SRC})
        target_include_directories(${t} PRIVATE src ${SDL2_INCLUDE_DIRS})
        target_link_libraries(${t} PRIVATE ${SDL2_LIBRARIES})
        if(NOT MSVC)
            target_link_libraries(${t} PRIVATE m)
        endif()
        target_compile_definitions(${t} PRIVATE ASSET_DIR="${XQUEST_ASSET_DIR}")
    endforeach()

    add_test(NAME config  COMMAND test_config)
    add_test(NAME demo    COMMAND test_demo  "${CMAKE_SOURCE_DIR}/tests/data/reference.dmo")
    # Replay needs the game data and the reference recording.
    add_test(NAME replay  COMMAND test_replay
             "${XQUEST_DATA_SOURCE}" "${CMAKE_SOURCE_DIR}/tests/data/reference.dmo")
endif()
