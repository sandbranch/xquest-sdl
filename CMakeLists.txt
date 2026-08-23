cmake_minimum_required(VERSION 3.16)
project(xquest C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

# ── Runtime asset path (baked into the binary) ───────────────────────────────
# For development: points at the adjacent ../xquest/ original distribution.
# For packaging:   set to the installed data dir, e.g. /usr/share/games/xquest.
set(XQUEST_ASSET_DIR "${CMAKE_SOURCE_DIR}/../xquest"
    CACHE PATH "Runtime directory the game searches for data files")

# ── Data-source directory for cmake --install ─────────────────────────────────
# Defaults to XQUEST_ASSET_DIR.  Set to ${CMAKE_SOURCE_DIR}/assets when
# packaging so cmake installs from the pre-populated assets/ tree rather
# than from the adjacent original distribution.
set(XQUEST_DATA_SOURCE "${XQUEST_ASSET_DIR}"
    CACHE PATH "Source directory for 'cmake --install' game data (defaults to XQUEST_ASSET_DIR)")

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
)

target_include_directories(xquest PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(xquest PRIVATE ${SDL2_LIBRARIES} m)
target_compile_definitions(xquest PRIVATE
    ASSET_DIR="${XQUEST_ASSET_DIR}"
)
target_compile_options(xquest PRIVATE -Wall -Wextra -Wpedantic)

# ── Install ────────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

# Binary goes to /usr/games (Debian convention for games)
install(TARGETS xquest RUNTIME DESTINATION games)

# Game data (sprites/sounds/etc.) is Mark Mackey's copyrighted freeware, not
# checked into this repo (see assets/README) and never present on a clean CI
# checkout. Only wire up data conversion/install when a real data source is
# actually present — CI and any engine-only build skip it entirely, and the
# resulting binary picks up data at runtime via XQUEST_DATA_DIR (see main.c).
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

# Icons — hicolor theme sizes
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
