# pack_repo.py
import os

# Configuration
OUTPUT_FILE = "entire_codebase.txt"
ALLOWED_EXTENSIONS = {'.hpp', '.cpp', '.h', '.c', '.txt'}
EXCLUDE_DIRS = {'build', '.git', 'bin', 'obj', 'assets', 'libs', '.vs', 'resources'}

def pack_codebase():
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as out:
        out.write("# EXTRACTED ENGINE CODEBASE\n\n")
        
        # 1. Generate Directory Tree Structure
        out.write("## DIRECTORY TREE\n```text\n")
        for root, dirs, files in os.walk('.'):
            dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
            level = root.replace('.', '').count(os.sep)
            indent = ' ' * 4 * level
            out.write(f"{indent}{os.path.basename(root)}/\n")
            sub_indent = ' ' * 4 * (level + 1)
            for f in files:
                if os.path.splitext(f)[1] in ALLOWED_EXTENSIONS:
                    out.write(f"{sub_indent}{f}\n")
        out.write("```\n\n")
        
        # 2. Append File Contents
        out.write("## FILE CONTENTS\n\n")
        for root, dirs, files in os.walk('.'):
            dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
            for file in files:
                if os.path.splitext(file)[1] in ALLOWED_EXTENSIONS:
                    relative_path = os.path.relpath(os.path.join(root, file), '.')
                    out.write(f"// ==========================================\n")
                    out.write(f"// FILE: {relative_path}\n")
                    out.write(f"// ==========================================\n")
                    out.write(f"```cpp\n")
                    try:
                        with open(os.path.join(root, file), 'r', encoding='utf-8') as f:
                            out.write(f.read())
                    except Exception as e:
                        out.write(f"// ERROR READING FILE: {e}\n")
                    out.write(f"\n```\n\n")

    print(f"Successfully packed codebase into {OUTPUT_FILE}")

if __name__ == "__main__":
    pack_codebase()