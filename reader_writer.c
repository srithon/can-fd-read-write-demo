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

// for `getenv`
#include <stdlib.h>

#if !defined(DEFAULT_CAN_INTERFACE_NAME)
#define DEFAULT_CAN_INTERFACE_NAME "vcan0"
#endif

#if !defined(DEFAULT_CAN_FILTER_ID)
#define DEFAULT_CAN_FILTER_ID 5
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

  int flags;

  // ask for CAN ID
  // 32 bits
  printf("Please enter a CAN ID (number from 0 to 999): ");
  fflush(stdout);
  scanf("%d", &frame.can_id);

  do {
    // 8 bits
    printf("Please enter CAN flags (number from 0-15): ");
    fflush(stdout);
    // MISTAKE: the %d modifier doesn't consume trailing whitespace, so %c was
    // consuming the newline character.
    // https://stackoverflow.com/questions/14484431/scanf-getting-skipped.
    // putting the leading space makes it skip whitespace first.
    scanf(" %d", &flags);
  } while (flags < 0 || flags > 15);

  frame.flags = flags;

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

/**
 * @brief Reads environment variables `DISABLE_CAN_FILTER` and `CAN_FILTER_ID`
 * to determine the CAN filter id and whether to use it at all.
 *
 * @return If the filter should be disabled, -1; otherwise the id to read for.
 */
int get_can_filter_id() {
  char *can_filter_id_env;
  char *disable_can_filter_env;

  int can_filter_id;

  char *strtol_end_ptr;

  disable_can_filter_env = getenv("DISABLE_CAN_FILTER");

  // if it has any value, then disable the filter
  if (disable_can_filter_env) {
    can_filter_id = -1;
  } else {
    can_filter_id_env = getenv("CAN_FILTER_ID");

    // read string `can_filter_id_env`, store pointer to last read character in
    // `strtol_end_ptr` and interpret number as base 10.
    can_filter_id = strtol(can_filter_id_env, &strtol_end_ptr, 10);

    // if end pointer is same as start pointer, nothing was read, meaning the
    // string was not a valid integer
    if (strtol_end_ptr == can_filter_id_env) {
      // sanity check; in this case, the documentation says that the function
      // should return zero.
      assert(can_filter_id == 0);
      // then, default to `DEFAULT_CAN_FILTER_ID`.
      can_filter_id = DEFAULT_CAN_FILTER_ID;
    }
  }

  return can_filter_id;
}

/**
 * @brief Reads environment variable `CAN_INTERFACE_NAME` or defaults to PP
 * definition to yield interface to read.
 *
 * @return Name of CAN interface device.
 */
char *get_can_interface_name() {
  char *env_val = getenv("CAN_INTERFACE_NAME");
  char *interface_name;

  // if env variable isn't defined, default
  if (!(interface_name = env_val)) {
    interface_name = DEFAULT_CAN_INTERFACE_NAME;
  }

  return interface_name;
}

int main() {
  // pre-declaration for C90 compatibility
  int s;
  int canfd_enabled = 1;
  struct sockaddr_can addr;
  // structure for requesting a specific interface
  struct ifreq ifr;

  int can_filter_id;
  char *can_interface_name;

  // open socket communicating using the raw socket protocol
  s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  // https://stackoverflow.com/questions/61368853/socketcan-read-function-never-returns
  assert(setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_enabled,
                    sizeof(canfd_enabled)) >= 0);

  can_filter_id = get_can_filter_id();

  if (can_filter_id != -1) {
    // can set several filters and they will be OR'd by default; there is a
    // sockopt defined alongside CAN_RAW_FILTER that allows you to AND them.
    struct can_filter rfilter[1];
    rfilter[0].can_id = can_filter_id;
    rfilter[0].can_mask = CAN_SFF_MASK;
    // returns -1 when socket doesn't support the operation
    assert(setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter,
                      sizeof(rfilter)) >= 0);
  }

  can_interface_name = get_can_interface_name();

  // we copy the name of our device into the struct.
  // note that `ifr_name` is actually a macro that hides the internal structure
  // of the `ifreq`
  strcpy(ifr.ifr_name, can_interface_name);

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
