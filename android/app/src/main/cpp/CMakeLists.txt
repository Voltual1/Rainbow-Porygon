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
        COMMAND /bin/sh -c "cd '${ROOT}' && '${CMAKE_C_COMPILER}' --target=${CMAKE_C_COMPILER_TARGET} --sysroot='${CMAKE_SYSROOT}' -E -std=gnu99 -iquote'${ROOT}/include' -iquote'${ROOT}/gflib' -iquote'${ROOT}/src/platform' -I'${ROOT}/android/SDL2/include' -DNONMATCHING -DPORTABLE -DPLATFORM_SDL2 -DRENDERER_EASY_DRAW -DMODERN=1 -DUBFIX -DTESTING=0 -DEMERALD=1 '${SOURCE}' -o '${OUTPUT}.i' && '${ROOT}/tools/preproc/preproc' -g '${ASSETS_DIR}' '${OUTPUT}.i' '${ROOT}/charmap.txt' > '${OUTPUT}'"
        DEPENDS "${SOURCE}" "${ROOT}/tools/preproc/preproc" "${ROOT}/charmap.txt"
        VERBATIM)
    list(APPEND GENERATED_C "${OUTPUT}")
endforeach()

file(GLOB DATA_ASM CONFIGURE_DEPENDS "${ROOT}/data/*.s" "${ROOT}/sound/*.s")
file(GLOB SONG_ASM CONFIGURE_DEPENDS "${ROOT}/sound/songs/*.s" "${ROOT}/sound/songs/midi/*.s")
set(GENERATED_ASM)
set(SPECIAL_IDS "${CMAKE_BINARY_DIR}/generated/special_ids.inc")
set(ENUM_DEFINES "${CMAKE_BINARY_DIR}/generated/enum_defines.h")

# 1. 自动生成 special_ids.inc
file(WRITE "${CMAKE_BINARY_DIR}/gen_specials.py" [[
import sys, re
with open(sys.argv[2], 'w') as out:
    idx = 0
    with open(sys.argv[1], 'r') as f:
        for line in f:
            m = re.match(r'^\s*def_special\s+([A-Za-z0-9_]+)', line)
            if m:
                out.write('.set SPECIAL_{}, {}\n'.format(m.group(1), idx))
                idx += 1
]])

add_custom_command(
    OUTPUT "${SPECIAL_IDS}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/generated"
    COMMAND python3 "${CMAKE_BINARY_DIR}/gen_specials.py" "${ROOT}/data/specials.inc" "${SPECIAL_IDS}"
    DEPENDS "${ROOT}/data/specials.inc"
    VERBATIM)

# 2. 自动生成 enum_defines.h
file(WRITE "${CMAKE_BINARY_DIR}/gen_enum_defines.py" [[
import re, os, sys
def parse_enum_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception: return []
    content = re.sub(r'//.*', '', content)
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    enum_blocks = re.findall(r'\benum\b\s*(?:[a-zA-Z_][a-zA-Z0-9_]*)?\s*\{(.*?)\}', content, re.DOTALL)
    defines = []
    for block in enum_blocks:
        entries = block.split(',')
        current_val, base_expr, offset = 0, None, 0
        for entry in entries:
            entry = entry.strip()
            if not entry: continue
            if '=' in entry:
                parts = entry.split('=')
                name = parts[0].strip()
                val_str = parts[1].strip()
                if re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', name):
                    base_expr, offset = val_str, 0
                    defines.append((name, f"({base_expr})"))
            else:
                name = re.sub(r'\s+.*', '', entry.strip())
                if re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', name):
                    if base_expr is not None:
                        offset += 1
                        defines.append((name, f"({base_expr}) + {offset}"))
                    else:
                        defines.append((name, f"{current_val}"))
                        current_val += 1
    return defines

include_dir, output_file = sys.argv[1], sys.argv[2]
all_defines, seen = [], set()
for root, _, files in os.walk(include_dir):
    for file in files:
        if file.endswith('.h'):
            all_defines.extend(parse_enum_file(os.path.join(root, file)))
with open(output_file, 'w', encoding='utf-8') as f:
    f.write("#ifndef GENERATED_ENUM_DEFINES_H\n#define GENERATED_ENUM_DEFINES_H\n\n")
    for name, val in all_defines:
        if name not in seen:
            f.write(f"#define {name} {val}\n")
            seen.add(name)
    f.write("\n#endif\n")
]])

add_custom_command(
    OUTPUT "${ENUM_DEFINES}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/generated"
    COMMAND python3 "${CMAKE_BINARY_DIR}/gen_enum_defines.py" "${ROOT}/include" "${ENUM_DEFINES}"
    DEPENDS "${CMAKE_BINARY_DIR}/gen_enum_defines.py"
    VERBATIM)

# 3. 编译汇编文件 (引入 -P 和 -D__ASSEMBLER__ 以及 -D__GBA__)
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
            # 升级版 cpp 调用：
            # -P: 禁止行标记，防止干扰 preproc
            # -D__ASSEMBLER__: 让头文件屏蔽 C 专用语法
            # -include ${ENUM_DEFINES}: 强制展开无法被 guard 屏蔽的 enum 符号
            COMMAND /bin/sh -c "cd '${ROOT}' && { printf \".include \\\"${SPECIAL_IDS}\\\"\\n\"; PORTABLE=1 '${ROOT}/tools/preproc/preproc' -s '${SOURCE}' '${ROOT}/charmap.txt' | /usr/bin/cpp -P -I'${ROOT}' -I'${ROOT}/include' -I'${ROOT}/include/constants' -I'${ROOT}/sound' -I'${ROOT}/src/platform' -DMODERN=1 -DPORTABLE=1 -DUBFIX -DTESTING=0 -DEMERALD=1 -D__ASSEMBLER__ -D__GBA__ -include '${ENUM_DEFINES}' | PORTABLE=1 '${ROOT}/tools/preproc/preproc' -ie -g '${ASSETS_DIR}' '${SOURCE}' '${ROOT}/charmap.txt'; } > '${OUTPUT}.s' && '${ARM_NONE_EABI_AS}' -march=armv7-a -I'${ROOT}' -I'${ROOT}/sound' -o '${OUTPUT}' '${OUTPUT}.s'"
            DEPENDS "${SOURCE}" "${ROOT}/tools/preproc/preproc" "${ROOT}/charmap.txt" "${SPECIAL_IDS}" "${ENUM_DEFINES}"
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
    NONMATCHING PORTABLE PLATFORM_SDL2 RENDERER_EASY_DRAW MODERN=1 UBFIX TESTING=0 EMERALD=1)

target_compile_options(main PRIVATE
    -O3 -fno-builtin -Wno-implicit-function-declaration -Wno-trigraphs -Wno-gnu-alignof-expression -Wno-null-dereference
    "SHELL:-iquote '${ROOT}/include'"
    "SHELL:-iquote '${ROOT}/src/platform'"
    "SHELL:-iquote '${ROOT}/gflib'")

target_link_libraries(main PRIVATE SDL2::SDL2 m)
