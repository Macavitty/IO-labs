#!/usr/bin/env python3
import subprocess
import sys
import os

file_size = sys.argv[1]

os.system(f"truncate -s {file_size}M file.txt")
print(f"[+] Created file of {file_size}M")

os.system("dd if=file.txt of=/mnt/blocklab1/file.txt")

