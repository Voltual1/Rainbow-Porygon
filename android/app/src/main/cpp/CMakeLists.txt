cmake_minimum_required(VERSION 3.22)
project(pokeemerald_android C ASM)

set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

# 自动寻找交叉编译工具
find_program(ARM_NONE_EABI_AS arm-none-eabi-as REQUIRED)
find_program(ARM_NONE_EABI_LD arm-none-eabi-ld REQUIRED)
find_program(ARM_NONE_EABI_OBJCOPY arm-none-eabi-objcopy REQUIRED)

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
        # 修正1：C 文件预处理，不再使用 -i，仅使用 -g 定位资产
        COMMAND /bin/sh -c "cd '${ROOT}' && '${CMAKE_C_COMPILER}' --target=${CMAKE_C_COMPILER_TARGET} --sysroot='${CMAKE_SYSROOT}' -E -std=gnu99 -iquote'${ROOT}/include' -iquote'${ROOT}/gflib' -iquote'${ROOT}/src/platform' -I'${ROOT}/android/SDL2/include' -DNONMATCHING -DPORTABLE -DPLATFORM_SDL2 -DRENDERER_EASY_DRAW -DMODERN=1 -DUBFIX '${SOURCE}' -o '${OUTPUT}.i' && '${ROOT}/tools/preproc/preproc' -g '${ASSETS_DIR}' '${OUTPUT}.i' '${ROOT}/charmap.txt' > '${OUTPUT}'"
        DEPENDS "${SOURCE}" "${ROOT}/tools/preproc/preproc" "${ROOT}/charmap.txt"
        VERBATIM)
    list(APPEND GENERATED_C "${OUTPUT}")
endforeach()

# 包含 data 和 sound 的汇编文件
file(GLOB DATA_ASM CONFIGURE_DEPENDS "${ROOT}/data/*.s" "${ROOT}/sound/*.s")
file(GLOB SONG_ASM CONFIGURE_DEPENDS "${ROOT}/sound/songs/*.s" "${ROOT}/sound/songs/midi/*.s")
set(GENERATED_ASM)
set(SPECIAL_IDS "${CMAKE_BINARY_DIR}/generated/special_ids.inc")

# 修正2：彻底修复 awk，使用 $2 恢复正常语义，并消除 Expansion 中可能存在的逗号
add_custom_command(
    OUTPUT "${SPECIAL_IDS}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/generated"
    COMMAND /bin/sh -c "awk 'BEGIN { id=0 } /^[[:space:]]*def_special[[:space:]]+/ { gsub(/,/, \"\", $2); printf \".set SPECIAL_%s, %d\\n\", $2, id++ }' '${ROOT}/data/specials.inc' > '${SPECIAL_IDS}'"
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
            COMMAND /bin/sh -c "sed -e 's/\\.4byte/\\.word/g;s/\\.2byte/\\.short/g' '${SOURCE}' > '${OUTPUT}.s' && '${ARM_NONE_EABI_AS}' -march=armv7-a -I'${ROOT}/sound' -o '${OUTPUT}' '${OUTPUT}.s'"
            DEPENDS "${SOURCE}"
            VERBATIM)
    else()
        add_custom_command(
            OUTPUT "${OUTPUT}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
            # 修正3：绝地修复！第二层 preproc 虽然有 -i (包含在 -ie 中) 表示读取 stdin，但必须传入 '${SOURCE}' 作为文件名给它提取 .s 后缀，绝不能用 '-'！
            COMMAND /bin/sh -c "cd '${ROOT}' && { printf \".include \\\"${SPECIAL_IDS}\\\"\\n\"; '${ROOT}/tools/preproc/preproc' -s '${SOURCE}' '${ROOT}/charmap.txt' | /usr/bin/cpp -I'${ROOT}/include' -I'${ROOT}/sound' -I'${ROOT}/src/platform' -DMODERN=1 -DPORTABLE -DUBFIX | '${ROOT}/tools/preproc/preproc' -ie -g '${ASSETS_DIR}' '${SOURCE}' '${ROOT}/charmap.txt'; } > '${OUTPUT}.s' && '${ARM_NONE_EABI_AS}' -march=armv7-a -I'${ROOT}' -I'${ROOT}/sound' -o '${OUTPUT}' '${OUTPUT}.s'"
            DEPENDS "${SOURCE}" "${ROOT}/tools/preproc/preproc" "${ROOT}/charmap.txt" "${SPECIAL_IDS}"
            VERBATIM)
    endif()
    list(APPEND GENERATED_ASM "${OUTPUT}")
endforeach()

set(GAME_DATA_OBJECT "${CMAKE_BINARY_DIR}/generated/game_data.o")
add_custom_command(
    OUTPUT "${GAME_DATA_OBJECT}"
    COMMAND "${ARM_NONE_EABI_LD}" -r -o "${GAME_DATA_OBJECT}.tmp" ${GENERATED_ASM}
    COMMAND "${ARM_NONE_EABI_OBJCOPY}" --rename-section .rodata=.data,alloc,load,data,contents "${GAME_DATA_OBJECT}.tmp" "${GAME_DATA_OBJECT}"
    DEPENDS ${GENERATED_ASM}
    VERBATIM)
set_source_files_properties("${GAME_DATA_OBJECT}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)

add_library(main SHARED ${GENERATED_C} "${GAME_DATA_OBJECT}")
target_include_directories(main PRIVATE
    "${ROOT}"
    "${ROOT}/include"
    "${ROOT}/src/platform"
    "${ROOT}/sound"
    "${ROOT}/android/SDL2/include")
target_compile_definitions(main PRIVATE
    NONMATCHING PORTABLE PLATFORM_SDL2 RENDERER_EASY_DRAW MODERN=1 UBFIX)
target_compile_options(main PRIVATE
    -O3
    -fno-builtin
    -Wno-implicit-function-declaration
    -Wno-trigraphs
    "SHELL:-iquote '${ROOT}/include'"
    "SHELL:-iquote '${ROOT}/src/platform'"
    "SHELL:-iquote '${ROOT}/gflib'")
target_link_libraries(main PRIVATE SDL2::SDL2 m)
