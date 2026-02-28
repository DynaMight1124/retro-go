ifdef CROSS_COMPILE
 ifndef KERNEL_DIR
 $(error specify KERNEL_DIR)
 endif
else
 $(error specify CROSS_COMPILE)
 RELEASE = $(shell uname -r)
 KERNEL_DIR = /lib/modules/$(RELEASE)/build
endif

obj-m += warm.o
warm-objs += warm_main.o warm_ops.o


all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
	$(RM) modules.order Module.markers

