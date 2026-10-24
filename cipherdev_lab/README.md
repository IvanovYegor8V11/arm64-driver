# ARM64 cipherdev coursework lab

## Build module (host Ubuntu with ARM64 cross toolchain)

```sh
make KDIR=$HOME/kursovaya/linux
file cipherdev.ko
```

The kernel tree must be fully built (including modules / Module.symvers) using the same configuration as the QEMU kernel. Do not run `make modules_prepare` as a substitute for the full build when CONFIG_MODVERSIONS is enabled.

## Build test executable for ARM64

```sh
aarch64-linux-gnu-gcc -static -O2 -Wall -Wextra -o test_cipherdev test_cipherdev.c
file test_cipherdev
```

## Place into existing initramfs

```sh
mkdir -p ~/kursovaya/initramfs/lib/modules ~/kursovaya/initramfs/usr/bin
cp cipherdev.ko ~/kursovaya/initramfs/lib/modules/
cp test_cipherdev ~/kursovaya/initramfs/usr/bin/
(cd ~/kursovaya/initramfs && find . -print | cpio -o -H newc | gzip -9 > ../initramfs.cpio.gz)
```

## QEMU shell

```sh
insmod /lib/modules/cipherdev.ko
ls -l /dev/cipherdev
# If devtmpfs isn't mounted, mount it: mount -t devtmpfs devtmpfs /dev
# If necessary, create the misc device using the minor printed in /proc/misc:
# mknod /dev/cipherdev c 10 <minor_from_proc_misc>
/usr/bin/test_cipherdev
dmesg | grep cipherdev
rmmod cipherdev
```

The driver uses `misc_register()` and a dynamically allocated misc minor, not `register_chrdev()`. Each open has its own session buffer/key/mode/stats. `write` processes up to 4096 bytes and rewinds the current file offset; `read` uses the offset and returns EOF after the buffer. Mode 0 XORs with the low byte key; mode 1 rotates each byte left by key modulo 8; mode 2 accumulates previous output plus input plus key, modulo 256. The checksum is FNV-1a 32-bit over the processed bytes. RESET clears data, counters and restores default key 0x5a and mode 0. Stats count successful positive reads/writes and actual bytes. ioctls take pointers to `uint32_t` or `struct cipherdev_stats`.

## Student challenge (suggested)

Give students the stripped `.ko`, an ARM64 kernel image and matching build tree, plus a behavior specification and tests (do not give them the driver source). Require reconstructing the ioctl ABI, algorithm modes, checksum, offset behavior, bounds/error cases, and concurrent independent opens. Score functional equivalence using tests, not binary identity.

Strip only debug information on a COPY:

```sh
cp cipherdev.ko cipherdev_student.ko
aarch64-linux-gnu-strip --strip-debug cipherdev_student.ko
```

A `.ko` may retain symbol names and strings; `--strip-all` is unsafe for kernel modules. Validate the stripped module under QEMU before distribution.
