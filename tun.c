#include <fcntl.h> 
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

int tun_open(char* devname) {
  struct ifreq ifr;
  int fd, err;

  if ((fd = open("/dev/net/tun", O_RDWR)) == -1 ) {
       perror("open /dev/net/tun");
       exit(1);
  }
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN;
  strncpy(ifr.ifr_name, devname, IFNAMSIZ);

  if ( (err = ioctl(fd, TUNSETIFF, (void*) &ifr)) == -1 ) {
    perror("ioctl TUNSETIFF");
    close(fd);
    exit(1);
  }

  return fd;
}


int main(int argc, char** argv) {
  int fd, nbytes;
  char buf[1600];

  fd = tun_open("tun0");
  printf("Device tun0 opened\n");
  while(1) {
    nbytes = read(fd, buf, sizeof(buf));
    printf("Read %d bytes from tun0\n", nbytes);
  }
  return 0;
}
