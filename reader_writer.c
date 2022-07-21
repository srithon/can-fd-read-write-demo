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

#include <assert.h>

#if !defined(CAN_INTERFACE_NAME)
#define CAN_INTERFACE_NAME "vcan0"
#endif

#if !defined(CAN_FILTER_ID)
#define CAN_FILTER_ID 5
#endif

// if neither are specified, then default to `RUN_READER`
#if !defined(RUN_READER) && !defined(RUN_WRITER)
#define RUN_READER
#endif

/**
 * @brief Reads a CANFD frame from a socket and prints out its information on
 * STDOUT. The payload is interpreted as an ASCII string.
 *
 * @param s The id returned by the `socket` syscall corresponding to the socket
 * to read from.
 */
void read_from_socket(int s);
/**
 * @brief Writes a CAN-FD frame to a given socket.
 *
 * @param s The id returned by the `socket` syscall corresponding to the socket
 * to read from.
 * @param frame The frame to write to the socket.
 */
void write_to_socket(int s, struct canfd_frame frame);
/**
 * @brief Uses STDIN to interactively prompt the user to create a CAN-FD frame,
 * and then sends the frame to the given socket. The payload of the frame will
 * be a user-provided ASCII string.
 *
 * @param s The id returned by the `socket` syscall corresponding to the socket
 * to write to.
 */
void write_to_socket_interactive(int s);

// a SocketFunc is a function that takes in an integer (designating the socket
// )and returns void.
typedef void (*SocketFunc)(int);

// for some reason didn't work when I moved the declaration outside
#if defined(RUN_READER)
SocketFunc active_func = read_from_socket;
#else
SocketFunc active_func = write_to_socket_interactive;
#endif

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

void write_to_socket(int s, struct canfd_frame frame) {
  int nbytes = write(s, &frame, sizeof(frame));

  if (nbytes < 0) {
    perror("CAN FD Raw socket write");
  }

  if (nbytes < sizeof(frame)) {
    perror("write: Incomplete CAN FD frame");
  }
}

void write_to_socket_interactive(int s) {
  struct canfd_frame frame;

  int data_length;

  // ask for CAN ID
  // 32 bits
  printf("Please enter a CAN ID (number from 0 to 999): ");
  fflush(stdout);
  scanf("%d", &frame.can_id);

  // 8 bits
  printf("Please enter CAN flags (number from 0-15): ");
  fflush(stdout);
  // MISTAKE: the %d modifier doesn't consume trailing whitespace, so %c was
  // consuming the newline character.
  // https://stackoverflow.com/questions/14484431/scanf-getting-skipped.
  // putting the leading space makes it skip whitespace first.
  scanf(" %c", &frame.flags);

  // get rid of the newline thats still in the buffer after the previous scanf.
  // https://stackoverflow.com/questions/5240789/scanf-leaves-the-newline-character-in-the-buffer
  getchar();

  // 64 is max length for can fd data
  printf("Please enter CAN data: ");
  fflush(stdout);
  fgets(frame.data, sizeof(frame.data), stdin);

  // NOTE: this will not work if char's on the architecture are more than 8
  // bits.
  data_length = strlen(frame.data);
  frame.len = data_length;

  write_to_socket(s, frame);
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
  assert(setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_enabled,
                    sizeof(canfd_enabled)) >= 0);

#if !defined(DISABLE_CAN_FILTER)
  // can set several filters and they will be OR'd by default; there is a
  // sockopt defined alongside CAN_RAW_FILTER that allows you to AND them.
  struct can_filter rfilter[1];
  rfilter[0].can_id = CAN_FILTER_ID;
  rfilter[0].can_mask = CAN_SFF_MASK;
  // returns -1 when socket doesn't support the operation
  assert(setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter,
                    sizeof(rfilter)) >= 0);
#endif

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
    active_func(s);
  }

  return 0;
}
