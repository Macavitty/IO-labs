obj-m :=lab2.o
all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
do:
	sudo insmod lab2.ko
rm:
	sudo rmmod lab2.ko
