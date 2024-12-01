#include <fcntl.h>
#include <linux/genetlink.h>
#include <linux/taskstats.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <system_error>

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

#define gettid() ((pid_t)syscall(SYS_gettid))

/*
 * Generic macros for dealing with netlink sockets. Might be duplicated
 * elsewhere. It is recommended that commercial grade applications use
 * libnl or libnetlink and use the interfaces provided by the library
 */
#define NLMSG_DATA_CHAR(nlh)                                                   \
  ((char *)(((char *)nlh) + NLMSG_LENGTH(0))) // NOLINT

#define GENLMSG_DATA(glh)                                                      \
  ((char *)(NLMSG_DATA_CHAR(glh) + GENL_HDRLEN)) // NOLINT
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((char *)((char *)(na) + NLA_HDRLEN)) // NOLINT
#define NLA_PAYLOAD(len) (len - NLA_HDRLEN)

class NetlinkTaskstats {
  static int create_nl_socket();

  int get_family_id();

  static const int MAX_MSG_SIZE = 512;

  struct MsgStruct {
    struct nlmsghdr nl_msg_header;
    struct genlmsghdr genl_msg_header;
    char buf[MAX_MSG_SIZE];
  };

  bool send_cmd(uint16_t msg_type, uint32_t msg_pid, uint8_t genl_cmd,
                uint16_t attr_type, void *attr_data, uint16_t attr_data_len);
  bool recv_msg(struct MsgStruct &msg);

  void set_socket_flags(int flags_to_set);

  int fd_;
  int family_id_;

private:
  NetlinkTaskstats();

public:
  ~NetlinkTaskstats();

  static bool query(pid_t tid, struct taskstats &out);
  static bool query_tgid(pid_t pid, struct taskstats &out);
};
