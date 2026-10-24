obj-m := cipherdev.o
KDIR ?= $(HOME)/kursovaya/linux
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
all:
	$(MAKE) -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(CURDIR) modules
clean:
	$(MAKE) -C $(KDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(CURDIR) clean
