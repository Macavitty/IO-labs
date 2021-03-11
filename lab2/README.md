# Лабораторная работа 2

**Название:** "Разработка драйверов блочных устройств"

**Цель работы:** получить знания и навыки разработки драйверов блочных устройств для операционной системы Linux.

## Описание функциональности драйвера

При загрузке драйвер создает виртуальный жесткий диск _blocklab2_ размером 50 МБ в оперативной  памяти. Созданный диск разбит на первичный раздел (30 МБ) и  расширенный раздел, содержащий два логических раздела по 10 МБ каждый.

## Инструкция по сборке

$ cd IO-labs/lab2 \
$ make

## Инструкция пользователя

1. Собрать проект
2. Загрузить модуль: $ make do
3. Посмотреть системные логи: $ dmesg
4. Посмотреть информацию о созданных разделах: $ sudo fdisk -l /dev/blocklab
1. Смонтировать разделы в /tmp: $ sudo python3 mount.py
1. Посмотреть скорость копирования между виртуальным разделом и реальным жестким диском: $ sudo python3 test.py _file_size_in_MB_
1. Отмонтировать разделы: $ sudo python3 umount.py
1. Выгрузить модуль ядра: $ make rm

## Примеры использования
Собираем проект: \
$ cd IO-labs/lab1 \
$ make
\
\
Загружаем модуль: \
$ make do \
sudo insmod lab2.ko \
\
\
Созданы следующие разделы: \
$ sudo fdisk -l /dev/blocklab \
Disk /dev/blocklab: 50 MiB, 52428800 bytes, 102400 sectors \
Units: sectors of 1 * 512 = 512 bytes \
Sector size (logical/physical): 512 bytes / 512 bytes \
I/O size (minimum/optimal): 512 bytes / 512 bytes \
Disklabel type: dos \
Disk identifier: 0x36e5756d
\
Device         Boot Start    End Sectors Size Id Type \
/dev/blocklab1          1  61439   61439  30M 83 Linux \
/dev/blocklab2      61440 102399   40960  20M  5 Extended \
/dev/blocklab5      61441  81919   20479  10M 83 Linux \
/dev/blocklab6      81921 102399   20479  10M 83 Linux \
\
\
Монтируем разделы: \
$ sudo python3 mount.py \ 
mkfs.fat 4.1 (2017-01-24) \
[+] Partition blocklab1 mounted \
mkfs.fat 4.1 (2017-01-24) \
[+] Partition blocklab5 mounted \
mkfs.fat 4.1 (2017-01-24) \
[+] Partition blocklab6 mounted 
\
\
Тестируем скорость копирования: \
$ sudo python3 test.py 4 \
[+] Created file of 4M \
8192+0 records in \
8192+0 records out \
4194304 bytes (4,2 MB, 4,0 MiB) copied, 0,0208284 s, 201 MB/s
\
$ sudo python3 test.py 10 \
[+] Created file of 10M \
20480+0 records in \
20480+0 records out \
10485760 bytes (10 MB, 10 MiB) copied, 0,0573781 s, 183 MB/s
\
\
Отмонтируем разделы: \
$ sudo python3 umount.py \ 
[+] Partition blocklab1 unmounted \
[+] Partition blocklab5 unmounted \
[+] Partition blocklab6 unmounted
\
\
Выгружаем модуль: \
$ make rm \
sudo rmmod lab2.ko


