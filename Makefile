obj-m += chardev.o 

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load: 
	sudo insmod chardev.ko
	sudo dmesg -c

unload:
	sudo rmmod chardev.ko
	sudo dmesg -c
