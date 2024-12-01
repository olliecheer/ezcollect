#include "nl_task_stats.h"

NetlinkTaskstats::~NetlinkTaskstats() { close(fd_); }

int NetlinkTaskstats::create_nl_socket() {
  int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
  if (fd < 0)
    throw std::system_error(errno, std::generic_category());

  struct sockaddr_nl addr {
    .nl_family = AF_NETLINK,
  };
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    throw std::system_error(errno, std::generic_category());
  }

  return fd;
}

int NetlinkTaskstats::get_family_id() {
  static thread_local struct MsgStruct buf;

  if (!send_cmd(GENL_ID_CTRL, gettid(), CTRL_CMD_GETFAMILY,
                CTRL_ATTR_FAMILY_NAME, (void *)TASKSTATS_GENL_NAME,
                sizeof(TASKSTATS_GENL_NAME)))
    throw std::runtime_error("failed to send cmd");

  if (!recv_msg(buf))
    throw std::runtime_error("failed to receive msg");

  if (buf.nl_msg_header.nlmsg_type == NLMSG_ERROR)
    throw std::runtime_error("received error message");

  struct nlattr *attr = (struct nlattr *)GENLMSG_DATA(&buf);
  attr = (struct nlattr *)((char *)attr + NLA_ALIGN(attr->nla_len));

  int id = *(uint16_t *)NLA_DATA(attr);

  return id;
}

bool NetlinkTaskstats::query(pid_t tid, struct taskstats &out) {
  static thread_local NetlinkTaskstats nl_taskstats;
  static thread_local MsgStruct msg;

  if (!nl_taskstats.send_cmd(nl_taskstats.family_id_, tid, TASKSTATS_CMD_GET,
                             TASKSTATS_CMD_ATTR_PID, &tid, sizeof(tid))) {
    return false;
  }

  if (!nl_taskstats.recv_msg(msg)) {
    return false;
  }

  if (msg.nl_msg_header.nlmsg_type == NLMSG_ERROR) {
    return false;
  }

  struct nlattr *attr = (struct nlattr *)GENLMSG_DATA(&msg);
  attr = (struct nlattr *)NLA_DATA(attr);
  attr = (struct nlattr *)((char *)attr + NLA_ALIGN(attr->nla_len));
  struct taskstats *stats = (struct taskstats *)NLA_DATA(attr);
  memcpy(&out, stats, sizeof(out));

  return true;
}

bool NetlinkTaskstats::query_tgid(pid_t tid, struct taskstats &out) {
  static thread_local NetlinkTaskstats nl_taskstats;
  static thread_local MsgStruct msg;

  if (!nl_taskstats.send_cmd(nl_taskstats.family_id_, tid, TASKSTATS_CMD_GET,
                             TASKSTATS_CMD_ATTR_TGID, &tid, sizeof(tid))) {
    return false;
  }

  if (!nl_taskstats.recv_msg(msg)) {
    return false;
  }

  if (msg.nl_msg_header.nlmsg_type == NLMSG_ERROR) {
    return false;
  }

  struct nlattr *attr = (struct nlattr *)GENLMSG_DATA(&msg);
  attr = (struct nlattr *)NLA_DATA(attr);
  attr = (struct nlattr *)((char *)attr + NLA_ALIGN(attr->nla_len));
  struct taskstats *stats = (struct taskstats *)NLA_DATA(attr);
  memcpy(&out, stats, sizeof(out));

  return true;
}

NetlinkTaskstats::NetlinkTaskstats() {
  fd_ = create_nl_socket();
  set_socket_flags(O_NONBLOCK);
  family_id_ = get_family_id();
}

bool NetlinkTaskstats::send_cmd(uint16_t msg_type, uint32_t msg_pid,
                                uint8_t genl_cmd, uint16_t attr_type,
                                void *attr_data, uint16_t attr_data_len) {
  struct MsgStruct msg {
    .nl_msg_header =
        {
            .nlmsg_len = (uint32_t)NLMSG_LENGTH(GENL_HDRLEN) +
                         NLMSG_ALIGN(NLA_HDRLEN + attr_data_len),
            .nlmsg_type = msg_type,
            .nlmsg_flags = NLM_F_REQUEST,
            .nlmsg_seq = 0,
            .nlmsg_pid = msg_pid,
        },
    .genl_msg_header = {
        .cmd = genl_cmd,
        .version = 0x1,
    },
  };

  struct nlattr *attr = (struct nlattr *)GENLMSG_DATA(&msg);
  attr->nla_type = attr_type;
  attr->nla_len = NLA_HDRLEN + attr_data_len;
  memcpy(NLA_DATA(attr), attr_data, attr_data_len);

  struct sockaddr_nl nladdr {
    .nl_family = AF_NETLINK,
  };

  ssize_t sz = sendto(fd_, (char *)&msg, msg.nl_msg_header.nlmsg_len, 0,
                      (struct sockaddr *)&nladdr, sizeof(nladdr));

  if (sz == -1) {
    throw std::system_error(errno, std::generic_category());
  } else if (sz != msg.nl_msg_header.nlmsg_len) {
    throw std::runtime_error(
        "failed to send cmd sz != msg.nl_msg_header.nlmsg_len");
  }

  return true;
}

void NetlinkTaskstats::set_socket_flags(int flags_to_set) {
  int flags = 0;
  flags = fcntl(fd_, F_GETFL, 0);
  if (flags == -1) {
    close(fd_);
    throw std::system_error(errno, std::generic_category());
  }

  if (fcntl(fd_, F_SETFL, flags | flags_to_set) == -1) {
    close(fd_);
    throw std::system_error(errno, std::generic_category());
  }
}

bool NetlinkTaskstats::recv_msg(struct MsgStruct &msg) {
  ssize_t sz = recv(fd_, &msg, sizeof(msg), 0);
  if (sz < 0)
    throw std::system_error(errno, std::generic_category());

  return true;
}
