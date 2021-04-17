/* Test program for 3rd mandatory assignment.
 *
 * A process writes ITS integers to /dev/dm510-0 while
 * another process read ITS integers from /dev/dm510-1.
 * A checksum of the written data is compared with a
 * checksum of the read data.
 *
 * This is done in both directions.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#define DM510_MR _IOW('a','a',int*)
#define DM510_BS _IOW('a','b',int*)

int main(int argc, char *argv[])
{
    //pid_t pid;
    int fd, fd1, mr = 10, count = 42, ret;
    char *buf = "Hello this message is over 10 characters\n", *buf1 = malloc(count), *buf2 = malloc(count); 
   
    fd = open("/dev/dm510-0", O_RDWR);
    ret = write(fd, buf, count);
    if (ret == -1){
        perror("write");
	exit(1);
    }
    fd1 = open("/dev/dm510-1", O_RDWR);
    ret = read(fd1, buf1, count);
    if (ret == -1) {
        perror("read");
        exit(1);
    }
    printf("%s\n", buf1);
    ioctl(fd1, DM510_BS, &mr);

    ret = write(fd, buf, count);
    if (ret == -1){
        perror("write");
        exit(1);
    }

    ret = read(fd1, buf2, count);
    if (ret == -1) {
        perror("read");
        exit(1);
    }
    printf("%s\n", buf2);
    free(buf1);
    free(buf2);
    return 0;
}
