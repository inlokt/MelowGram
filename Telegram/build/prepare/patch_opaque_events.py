import os
import glob

def patch_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
        
    if 'setAttribute(Qt::WA_OpaquePaintEvent)' in content:
        patched = content.replace('setAttribute(Qt::WA_OpaquePaintEvent)', '// setAttribute(Qt::WA_OpaquePaintEvent)')
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(patched)
        print(f"Patched: {filepath}")

def main():
    base_dir = r"c:\Users\1337\Pictures\melowgram\Telegram\SourceFiles"
    for root, _, files in os.walk(base_dir):
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                patch_file(os.path.join(root, file))

if __name__ == '__main__':
    main()