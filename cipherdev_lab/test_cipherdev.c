#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include "cipherdev_ioctl.h"
int main(void)
{
    const char *msg = "Hello ARM64";
    unsigned char out[128];
    struct cipherdev_stats stats;
    uint32_t key = 7, mode = 2, checksum = 0;
    int fd = open("/dev/cipherdev", O_RDWR);
    ssize_t n;
    size_t i;
    if (fd < 0) { perror("open"); return 1; }
    if (ioctl(fd, CIPHERDEV_SET_KEY, &key) ||
        ioctl(fd, CIPHERDEV_SET_MODE, &mode)) { perror("set ioctl"); goto fail; }
    if (write(fd, msg, strlen(msg)) != (ssize_t)strlen(msg)) { perror("write"); goto fail; }
    n = read(fd, out, sizeof(out));
    if (n < 0) { perror("read"); goto fail; }
    printf("Result (%zd bytes):", n);
    for (i = 0; i < (size_t)n; ++i) printf(" %02x", out[i]);
    puts("");
    if (ioctl(fd, CIPHERDEV_GET_CHECKSUM, &checksum) ||
        ioctl(fd, CIPHERDEV_GET_STATS, &stats)) { perror("get ioctl"); goto fail; }
    printf("Checksum=%08x writes=%llu reads=%llu in=%llu out=%llu\n", checksum,
        (unsigned long long)stats.writes, (unsigned long long)stats.reads,
        (unsigned long long)stats.bytes_in, (unsigned long long)stats.bytes_out);
    close(fd);
    return 0;
fail:
    close(fd);
    return 1;
}
