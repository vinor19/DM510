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

#define DM510_MR _IOW('a','a',int32_t*)
#define DM510_BS _IOW('a','b',int32_t*)

int main(int argc, char *argv[])
{
    //pid_t pid;
    int fd;
    int32_t mr = 0; 
    printf("hello\n");
    //*mr =(int32_t) 1;
    printf("Hello2\n");
    printf("%d\n", mr);
    fd = open("/dev/dm510-0", O_RDWR);
    ioctl(fd, DM510_MR, &mr);
    return 0;
}
