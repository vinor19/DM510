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

#define ITS 10000

#define DM510_MR _IOW('a','a',int32_t*)
#define DM510_BS _IOW('a','b',int32_t*)


void read_all(int fd, void *buf, int count) {
    while (count > 0) {
        int ret;
        ret = read(fd, buf, count);
        if (ret == -1) {
            perror("read");
            exit(1);
        }

        count -= ret;
        buf += ret;
    }
}

void write_all(int fd, void *buf, int count) {
    while (count > 0) {
        int ret;
        ret = write(fd, buf, count);
        if (ret == -1) {
            perror("write");
            exit(1);
        }

        count -= ret;
        buf += ret;
    }
}

int main(int argc, char *argv[])
{
    pid_t pid;
    int fd, fd1;
    int sum = 0, i;
    int val;
    int cnt;
    int32_t *mr;
    *mr =(int32_t) 1;
    printf("%d\n", *mr);

    pid = fork();

    if (pid == 0) {
        fd = open("/dev/dm510-0", O_RDWR);
	
	perror("w open");	
        for (i=0; i<ITS; i++) {
            val++;
            sum += val;
            cnt = 4;
            write_all(fd, &val, 4);
        }
        printf("1. expected result: %d\n", sum);
	
        sum = 0;

        for (i=0; i<ITS; i++) {
            read_all(fd, &val, 4);
            sum += val;
        }
        printf("2. result: %d\n", sum);
    } else {
        fd = open("/dev/dm510-1", O_RDWR);
        perror("r open");

        for (i=0; i<ITS; i++) {
            read_all(fd, &val, 4);
	    sum+=val;
        }
        
       	printf("1. result: %d\n", sum);
	
        sum = 0;
	
        for (i=0; i<ITS; i++) {
            val++;
	    sum += val;
	    write_all(fd, &val, 4);
        }
      	printf("2. expected result: %d\n", sum);
   	ioctl(fd,DM510_MR,mr);
	*mr = 10;
	ioctl(fd,DM510_BS,mr);
	wait(NULL);
   }
    return 0;
}
