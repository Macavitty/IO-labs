#!/usr/bin/env python3
import os

partitions = ["blocklab1", "blocklab5", "blocklab6"]

for p in partitions:
    os.system(f"mkfs.vfat /dev/{p}")
    os.system(f"mkdir /mnt/{p}")
    os.system(f"mount /dev/{p} /mnt/{p}")
    
    print(f"[+] Partition {p} mounted")

#subprocess.run(["mkfs.vfat", f"/dev/{p}"])
#subprocess.run(["mkdir", f"/mnt/{p}"])
#subprocess.run(["mount", f"/dev/{p} /mnt/{p}"])
