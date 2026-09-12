import os
import glob
import re

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

def main():
    asm_files = glob.glob(os.path.join(ROOT_DIR, "data/**/*.s"), recursive=True) + \
                glob.glob(os.path.join(ROOT_DIR, "data/**/*.inc"), recursive=True) + \
                glob.glob(os.path.join(ROOT_DIR, "sound/**/*.s"), recursive=True) + \
                glob.glob(os.path.join(ROOT_DIR, "asm/**/*.inc"), recursive=True)

    print(f"==> Sanitizing {len(asm_files)} assembly files for 64-bit build...")
    for asm_path in asm_files:
        with open(asm_path, 'r', encoding='utf-8', errors='ignore') as f:
            text = f.read()

        # 1. 剔除单行 @ 注释，保留宏内部标号 \@
        text = re.sub(r'(?<!\\)@.*', '', text)

        # 2. 替换 GBA 汇编伪指令为 64 位标准格式
        text = re.sub(r'\.4byte', '.word', text)
        text = re.sub(r'\.2byte', '.short', text)
        text = re.sub(r'\.rodata', '.data', text)
        text = re.sub(r'\.ewram', '.data', text)
        text = re.sub(r'ewram_data', '.data', text)
        text = re.sub(r'common_data', '.data', text)
        text = re.sub(r'gbadata', '.data', text)
        text = re.sub(r'\.sbss', '.bss', text)

        with open(asm_path, 'w', encoding='utf-8') as f:
            f.write(text)

    print("==> Assembly sanitization complete.")

if __name__ == "__main__":
    main()