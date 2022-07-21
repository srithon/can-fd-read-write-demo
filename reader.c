#include <linux/can.h>
// for access to CAN FD
#include <linux/can/raw.h>

// contains ifreq
// MISTAKE: included linux/if.h rather than net/if.h
#include <net/if.h>
// for socket syscall
#include <sys/socket.h>

// strcpy
#include <string.h>

// contains SIOCGIFINDEX, ioctl
#include <sys/ioctl.h>

// for `read`
#include <unistd.h>

// for general user interaction
#include <stdio.h>

#define CAN_INTERFACE_NAME "vcan0"

void read_from_socket(int s) {
  struct canfd_frame frame;
  int nbytes = read(s, &frame, sizeof(frame));

  if (nbytes < 0) {
    perror("CAN FD Raw socket read");
  }

  if (nbytes < sizeof(frame)) {
    perror("read: Incomplete CAN FD frame");
  }

  printf("CAN ID: %d\n", frame.can_id);
  printf("data length: %d\n", frame.len);
  printf("Received frame data: %s\n", frame.data);
}

int main() {
  // pre-declaration for C90 compatibility
  int s;
  int canfd_enabled = 1;
  struct sockaddr_can addr;
  // structure for requesting a specific interface
  struct ifreq ifr;

  // open socket communicating using the raw socket protocol
  s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  // https://stackoverflow.com/questions/61368853/socketcan-read-function-never-returns
  setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_enabled,
             sizeof(canfd_enabled));

  // we copy the name of our device into the struct.
  // note that `ifr_name` is actually a macro that hides the internal structure
  // of the `ifreq`
  strcpy(ifr.ifr_name, CAN_INTERFACE_NAME);

  // SIOCGIFINDEX; from docs: "name -> if_index mapping"
  ioctl(s, SIOCGIFINDEX, &ifr);

  // stands for AddressFamily_CAN
  addr.can_family = AF_CAN;
  // `ifr_ifindex` is also a macro that hides the internal structure of the
  // `ifreq`
  addr.can_ifindex = ifr.ifr_ifindex;

  // TODO: figure out this line
  bind(s, (struct socketaddr *)&addr, sizeof(addr));

  while (1) {
    read_from_socket(s);
  }

  return 0;
}
