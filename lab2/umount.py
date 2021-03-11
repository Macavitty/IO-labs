#!/usr/bin/env python3
import subprocess

partitions = ["blocklab1", "blocklab2", "blocklab3"]

for p in partitions:
    subprocess.run(["umount", f"/mnt/{p}"])
    subprocess.run(["rmdir", f"/mnt/{p}"])
    print(f"[-] Partition {p} unmounted")
