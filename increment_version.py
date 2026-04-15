import os

version_file = "version.txt"
header_file = "src/Version.h"

major, minor, patch = 1, 1, 0

if os.path.exists(version_file):
    with open(version_file, "r") as f:
        content = f.read().strip()
        if content:
            parts = content.split('.')
            if len(parts) == 3:
                major, minor, patch = int(parts[0]), int(parts[1]), int(parts[2])

patch += 1
version_str = f"{major}.{minor}.{patch}"

with open(version_file, "w") as f:
    f.write(version_str)

with open(header_file, "w") as f:
    f.write("#pragma once\n")
    f.write(f'#define APP_VERSION "{version_str}"\n')

print(f"==== Incremented build version to {version_str} ====")