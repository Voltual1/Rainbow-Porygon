cmake_minimum_required(VERSION 3.22)
project(pokeemerald_android C ASM)

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory("${ROOT}/android/SDL2" "${CMAKE_BINARY_DIR}/SDL2")

# 设置预编译资产目录
set(ASSETS_DIR "${ROOT}/build/assets")

file(GLOB_RECURSE GAME_C CONFIGURE_DEPENDS
    "${ROOT}/src/*.c"
    "${ROOT}/gflib/*.c")
list(FILTER GAME_C EXCLUDE REGEX "\\.inc\\.c$")

set(GENERATED_C)
foreach(SOURCE IN LISTS GAME_C)
    file(RELATIVE_PATH RELATIVE_SOURCE "${ROOT}" "${SOURCE}")
    set(OUTPUT "${CMAKE_BINARY_DIR}/generated/${RELATIVE_SOURCE}")
    get_filename_component(OUTPUT_DIR "${OUTPUT}" DIRECTORY)
    add_custom_command(
        OUTPUT "${OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
        COMMAND /bin/sh -c "cd '${ROOT}' && '${CMAKE_C_COMPILER}' --target=${CMAKE_C_COMPILER_TARGET} --sysroot='${CMAKE_SYSROOT}' -E -std=gnu99 -iquote'${ROOT}/include' -iquote'${ROOT}/gflib' -iquote'${ROOT}/src/platform' -I'${ROOT}/android/SDL2/include' -DNONMATCHING -DPORTABLE -DPLATFORM_SDL2 -DRENDERER_EASY_DRAW -DMODERN=1 -DUBFIX -DTESTING=0 -DEMERALD=1 '${SOURCE}' -o '${OUTPUT}.i' && '${ROOT}/tools/preproc/preproc' -g '${ASSETS_DIR}' '${OUTPUT}.i' '${ROOT}/charmap.txt' > '${OUTPUT}'"
        DEPENDS "${SOURCE}" "${ROOT}/tools/preproc/preproc" "${ROOT}/charmap.txt"
        VERBATIM)
    list(APPEND GENERATED_C "${OUTPUT}")
endforeach()

file(GLOB DATA_ASM CONFIGURE_DEPENDS "${ROOT}/data/*.s" "${ROOT}/sound/*.s")
file(GLOB SONG_ASM CONFIGURE_DEPENDS "${ROOT}/sound/songs/*.s" "${ROOT}/sound/songs/midi/*.s")
set(GENERATED_ASM)
set(SPECIAL_IDS "${CMAKE_BINARY_DIR}/generated/special_ids.inc")

# Python 脚本生成 special_ids.inc
file(WRITE "${CMAKE_BINARY_DIR}/gen_specials.py" "
import sys, re
with open(sys.argv[2], 'w') as out:
    idx = 0
    with open(sys.argv[1], 'r') as f:
        for line in f:
            m = re.match(r'^\\s*def_special\\s+([A-Za-z0-9_]+)', line)
            if m:
                out.write('.set SPECIAL_{}, {}\\n'.format(m.group(1), idx))
                idx += 1
")

add_custom_command(
    OUTPUT "${SPECIAL_IDS}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/generated"
    COMMAND python3 "${CMAKE_BINARY_DIR}/gen_specials.py" "${ROOT}/data/specials.inc" "${SPECIAL_IDS}"
    DEPENDS "${ROOT}/data/specials.inc"
    VERBATIM)

foreach(SOURCE IN LISTS DATA_ASM SONG_ASM)
    file(RELATIVE_PATH RELATIVE_SOURCE "${ROOT}" "${SOURCE}")
    set(OUTPUT "${CMAKE_BINARY_DIR}/generated/${RELATIVE_SOURCE}.o")
    get_filename_component(OUTPUT_DIR "${OUTPUT}" DIRECTORY)
    if(SOURCE MATCHES "/sound/songs/")
        add_custom_command(
            OUTPUT "${OUTPUT}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
            COMMAND /bin/sh -c "sed -e 's/\\.4byte/\\.word/g;s/\\.2byte/\\.short/g' '${SOURCE}' > '${OUTPUT}.s' && '${CMAKE_C_COMPILER}' --target=${CMAKE_C_COMPILER_TARGET} --sysroot='${CMAKE_SYSROOT}' -fPIC -march=armv7-a -x assembler -c -I'${ROOT}/sound' -o '${OUTPUT}' '${OUTPUT}.s'"
            DEPENDS "${SOURCE}"
            VERBATIM)
    else()
        add_custom_command(
            OUTPUT "${OUTPUT}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
            COMMAND /bin/sh -c "cd '${ROOT}' && { printf \".include \\\"${SPECIAL_IDS}\\\"\\n\"; '${ROOT}/tools/preproc/preproc' -s '${SOURCE}' '${ROOT}/charmap.txt' | /usr/bin/cpp -I'${ROOT}' -I'${ROOT}/include' -I'${ROOT}/include/constants' -I'${ROOT}/sound' -I'${ROOT}/src/platform' -include '${ROOT}/include/constants/battle_anim.h' -DMODERN=1 -DPORTABLE -DUBFIX -DTESTING=0 -DEMERALD=1 | '${ROOT}/tools/preproc/preproc' -ie -g '${ASSETS_DIR}' '${SOURCE}' '${ROOT}/charmap.txt'; } > '${OUTPUT}.s' && '${CMAKE_C_COMPILER}' --target=${CMAKE_C_COMPILER_TARGET} --sysroot='${CMAKE_SYSROOT}' -fPIC -march=armv7-a -x assembler -c -I'${ROOT}' -I'${ROOT}/sound' -o '${OUTPUT}' '${OUTPUT}.s'"
            DEPENDS "${SOURCE}" "${ROOT}/tools/preproc/preproc" "${ROOT}/charmap.txt" "${SPECIAL_IDS}"
            VERBATIM)
    endif()
    list(APPEND GENERATED_ASM "${OUTPUT}")
endforeach()

add_library(main SHARED ${GENERATED_C} ${GENERATED_ASM})
target_include_directories(main PRIVATE
    "${ROOT}"
    "${ROOT}/include"
    "${ROOT}/src/platform"
    "${ROOT}/sound"
    "${ROOT}/android/SDL2/include")

target_compile_definitions(main PRIVATE
    NONMATCHING
    PORTABLE
    PLATFORM_SDL2
    RENDERER_EASY_DRAW
    MODERN=1
    UBFIX
    TESTING=0
    EMERALD=1)

target_compile_options(main PRIVATE
    -O3
    -fPIC
    -fno-builtin
    -Wno-implicit-function-declaration
    -Wno-trigraphs
    -Wno-gnu-alignof-expression
    -Wno-null-dereference
    "SHELL:-iquote '${ROOT}/include'"
    "SHELL:-iquote '${ROOT}/src/platform'"
    "SHELL:-iquote '${ROOT}/gflib'")

target_link_libraries(main PRIVATE SDL2::SDL2 m)
