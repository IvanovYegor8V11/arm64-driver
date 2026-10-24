obj-m += mychar.o

KDIR := $(HOME)/kursovaya/linux
PWD := $(shell pwd)

ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

all:
	$(MAKE) -C $(KDIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) clean