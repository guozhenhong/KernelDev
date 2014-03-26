ifneq ($(KERNELRELEASE),)
	obj-m := hello.o
else
	KERNELDIR := /lib/modules/3.13.3/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules


clean:
	rm -rf  *.o *.ko *.mod.c *.markers *.order *.symvers

endif
