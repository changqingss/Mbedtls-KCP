/*
 * ntpclient.c - NTP client
 *
 * Copyright (C) 1997, 1999, 2000, 2003, 2006, 2007, 2010  Larry Doolittle  <larry@doolittle.boa.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License (Version 2,
 *  June 1991) as published by the Free Software Foundation.  At the
 *  time of writing, that license was published by the FSF with the URL
 *  http://www.gnu.org/copyleft/gpl.html, and is incorporated herein by
 *  reference.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Possible future improvements:
 *      - Write more documentation  :-(
 *      - Support leap second processing
 *      - Support IPv6
 *      - Support multiple (interleaved) servers
 *
 *  Compile with -DPRECISION_SIOCGSTAMP if your machine really has it.
 *  There are patches floating around to add this to Linux, but
 *  usually you only get an answer to the nearest jiffy.
 *  Hint for Linux hacker wannabes: look at the usage of get_fast_time()
 *  in net/core/dev.c, and its definition in kernel/time.c .
 *
 *  If the compile gives you any flak, check below in the section
 *  labelled "XXX fixme - non-automatic build configuration".
 */

#define _POSIX_C_SOURCE 199309
#define _BSD_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h> /* gethostbyname */
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#ifdef PRECISION_SIOCGSTAMP
#include <sys/ioctl.h>
#endif
#ifdef USE_OBSOLETE_GETTIMEOFDAY
#include <sys/time.h>
#endif

#include "ntpc.h"

/* Default to the RFC-4330 specified value */
#ifndef MIN_INTERVAL
#define MIN_INTERVAL 15
#endif

#ifdef ENABLE_NTP_DEBUG
#define DEBUG_OPTION "d"
int debug = 0;
#else
#define DEBUG_OPTION
#endif

#ifdef ENABLE_NTP_REPLAY
#define REPLAY_OPTION "r"
#else
#define REPLAY_OPTION
#endif

extern char *optarg; /* according to man 2 getopt */

#include <stdint.h>
typedef uint32_t u32; /* universal for C99 */
/* typedef u_int32_t u32;   older Linux installs? */

/* XXX fixme - non-automatic build configuration */
#ifdef __linux__
#include <netdb.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/utsname.h>
#else
extern struct hostent *gethostbyname(const char *name);
extern int h_errno;
#define herror(hostname) NTP_ERR("Error %d looking up hostname %s", h_errno, hostname)
#endif
/* end configuration for host systems */

#define JAN_1970 0x83aa7e80 /* 2208988800 1970 - 1900 in seconds */
#define NTP_PORT (123)

/* How to multiply by 4294.967296 quickly (and not quite exactly)
 * without using floating point or greater than 32-bit integers.
 * If you want to fix the last 12 microseconds of error, add in
 * (2911*(x))>>28)
 */
#define NTPFRAC(x) (4294 * (x) + ((1981 * (x)) >> 11))

/* The reverse of the above, needed if we want to set our microsecond
 * clock (via clock_settime) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))

/* Converts NTP delay and dispersion, apparently in seconds scaled
 * by 65536, to microseconds.  RFC-1305 states this time is in seconds,
 * doesn't mention the scaling.
 * Should somehow be the same as 1000000 * x / 65536
 */
#define sec2u(x) ((x)*15.2587890625)

struct ntptime {
  unsigned int coarse;
  unsigned int fine;
};

struct ntp_control {
  u32 time_of_send[2]; /**< 发送 NTP 请求的时间戳 [0]=秒部分, [1]=微秒部分 */
  int live;            /**< 持续运行模式标志: 0=单次同步后退出, 非0=持续运行 */
  int set_clock;   /**< 是否调整系统时钟: 0=只计算偏移不修改, 非0=修改系统时钟(需root权限) */
  int probe_count; /**< NTP 请求发送次数: 0=无限循环, >0=发送指定次数后退出 */
  int cycle_time;  /**< 两次 NTP 请求间隔时间(秒)，最小值通常为15秒(遵循RFC-4330) */
  int goodness;    /**< 时间精度阈值(微秒)，当误差小于此值时可以控制程序退出 */
  int cross_check; /**< 是否执行一致性检查: 0=不检查, 非0=按RFC-4330执行严格检查 */
  char serv_addr[4]; /**< NTP 服务器IPv4地址，用于验证响应来源 */
  int sync_status;   /**< 校时成功标志: 0=未成功校时, 1=成功校时 */
};

/* prototypes for some local routines */
static void send_packet(int usd, u32 time_sent[2]);
static int rfc1305print(u32 *data, struct ntptime *arrival, struct ntp_control *ntpc, int *error);
/* static void udp_handle(int usd, char *data, int data_len, struct sockaddr *sa_source, int sa_len); */

static int get_current_freq(void) {
  /* OS dependent routine to get the current value of clock frequency.
   */
#ifdef __linux__
  struct timex txc;
  txc.modes = 0;
  if (adjtimex(&txc) < 0) {
    NTP_ERR("adjtimex: %d, %s", errno, strerror(errno));
    return -1;
  }
  return txc.freq;
#else
  return 0;
#endif
}

static int set_freq(int new_freq) {
  /* OS dependent routine to set a new value of clock frequency.
   */
#ifdef __linux__
  struct timex txc;
  txc.modes = ADJ_FREQUENCY;
  txc.freq = new_freq;
  if (adjtimex(&txc) < 0) {
    NTP_ERR("adjtimex: %d, %s", errno, strerror(errno));
    return -1;
  }
  return txc.freq;
#else
  return 0;
#endif
}

static int set_time(struct ntptime *new) {
#ifndef USE_OBSOLETE_GETTIMEOFDAY
  /* POSIX 1003.1-2001 way to set the system clock
   */
  struct timespec tv_set;
  /* it would be even better to subtract half the slop */
  tv_set.tv_sec = new->coarse - JAN_1970;
  /* divide xmttime.fine by 4294.967296 */
  tv_set.tv_nsec = USEC(new->fine) * 1000;
  if (clock_settime(CLOCK_REALTIME, &tv_set) < 0) {
    NTP_ERR("clock_settime: %d, %s", errno, strerror(errno));
    return -1;
  }
#ifdef ENABLE_NTP_DEBUG
  if (debug) {
    NTP_INFO("set time to %lu.%.9lu", tv_set.tv_sec, tv_set.tv_nsec);
  }
#endif
#else
  /* Traditional Linux way to set the system clock
   */
  struct timeval tv_set;
  /* it would be even better to subtract half the slop */
  tv_set.tv_sec = new->coarse - JAN_1970;
  /* divide xmttime.fine by 4294.967296 */
  tv_set.tv_usec = USEC(new->fine);
  if (settimeofday(&tv_set, NULL) < 0) {
    NTP_ERR("settimeofday: %d, %s", errno, strerror(errno));
    return -1;
  }
#ifdef ENABLE_NTP_DEBUG
  if (debug) {
    NTP_INFO("set time to %lu.%.6lu", tv_set.tv_sec, tv_set.tv_usec);
  }
#endif
#endif
  return 0;
}

static void ntpc_gettime(u32 *time_coarse, u32 *time_fine) {
#ifndef USE_OBSOLETE_GETTIMEOFDAY
  /* POSIX 1003.1-2001 way to get the system time
   */
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  *time_coarse = now.tv_sec + JAN_1970;
  *time_fine = NTPFRAC(now.tv_nsec / 1000);
#else
  /* Traditional Linux way to get the system time
   */
  struct timeval now;
  gettimeofday(&now, NULL);
  *time_coarse = now.tv_sec + JAN_1970;
  *time_fine = NTPFRAC(now.tv_usec);
#endif
}

static void send_packet(int usd, u32 time_sent[2]) {
  u32 data[12];
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4
#define PREC -6

#ifdef ENABLE_NTP_DEBUG
  if (debug) NTP_ERR("Sending ...");
#endif
  if (sizeof data != 48) {
    NTP_ERR("size error");
    return;
  }
  memset(data, 0, sizeof data);
  data[0] = htonl((LI << 30) | (VN << 27) | (MODE << 24) | (STRATUM << 16) | (POLL << 8) | (PREC & 0xff));
  data[1] = htonl(1 << 16); /* Root Delay (seconds) */
  data[2] = htonl(1 << 16); /* Root Dispersion (seconds) */
  ntpc_gettime(time_sent, time_sent + 1);
  data[10] = htonl(time_sent[0]); /* Transmit Timestamp coarse */
  data[11] = htonl(time_sent[1]); /* Transmit Timestamp fine   */
  send(usd, data, 48, 0);
}

static void get_packet_timestamp(int usd, struct ntptime *udp_arrival_ntp) {
#ifdef PRECISION_SIOCGSTAMP
  /* XXX broken */
  struct timeval udp_arrival;
  if (ioctl(usd, SIOCGSTAMP, &udp_arrival) < 0) {
    NTP_ERR("ioctl-SIOCGSTAMP: %d, %s", errno, strerror(errno));
    gettimeofday(&udp_arrival, NULL);
  }
  udp_arrival_ntp->coarse = udp_arrival.tv_sec + JAN_1970;
  udp_arrival_ntp->fine = NTPFRAC(udp_arrival.tv_usec);
#else
  (void)usd; /* not used */
  ntpc_gettime(&udp_arrival_ntp->coarse, &udp_arrival_ntp->fine);
#endif
}

static int check_source(int data_len, struct sockaddr *sa_source, unsigned int sa_len, struct ntp_control *ntpc) {
  struct sockaddr_in *sa_in = (struct sockaddr_in *)sa_source;
  (void)sa_len; /* not used */
#ifdef ENABLE_NTP_DEBUG
  if (debug) {
    NTP_INFO("packet of length %d received", data_len);
    if (sa_source->sa_family == AF_INET) {
      NTP_INFO("Source: INET Port %d host %s", ntohs(sa_in->sin_port), inet_ntoa(sa_in->sin_addr));
    } else {
      NTP_INFO("Source: Address family %d", sa_source->sa_family);
    }
  }
#endif
  /* we could check that the source is the server we expect, but
   * Denys Vlasenko recommends against it: multihomed hosts get it
   * wrong too often. */
#if 0
	if (memcmp(ntpc->serv_addr, &(sa_in->sin_addr), 4)!=0) {
		return 1;  /* fault */
	}
#else
  (void)ntpc; /* not used */
#endif
  if (NTP_PORT != ntohs(sa_in->sin_port)) {
    return 1; /* fault */
  }
  return 0;
}

static double ntpdiff(struct ntptime *start, struct ntptime *stop) {
  int a;
  unsigned int b;
  a = stop->coarse - start->coarse;
  if (stop->fine >= start->fine) {
    b = stop->fine - start->fine;
  } else {
    b = start->fine - stop->fine;
    b = ~b;
    a -= 1;
  }

  return a * 1.e6 + b * (1.e6 / 4294967296.0);
}

/* Does more than print, so this name is bogus.
 * It also makes time adjustments, both sudden (-s)
 * and phase-locking (-l).
 * sets *error to the number of microseconds uncertainty in answer
 * returns 0 normally, 1 if the message fails sanity checks
 */
static int rfc1305print(u32 *data, struct ntptime *arrival, struct ntp_control *ntpc, int *error) {
  /* straight out of RFC-1305 Appendix A */
  int li, vn, mode, stratum, poll, prec;
  int delay, disp, refid;
  struct ntptime reftime, orgtime, rectime, xmttime;
  double el_time, st_time, skew1, skew2;
  int freq;
#ifdef ENABLE_NTP_DEBUG
  const char *drop_reason = NULL;
#endif

#define Data(i) ntohl(((u32 *)data)[i])
  li = Data(0) >> 30 & 0x03;
  vn = Data(0) >> 27 & 0x07;
  mode = Data(0) >> 24 & 0x07;
  stratum = Data(0) >> 16 & 0xff;
  poll = Data(0) >> 8 & 0xff;
  prec = Data(0) & 0xff;
  if (prec & 0x80) prec |= 0xffffff00;
  delay = Data(1);
  disp = Data(2);
  refid = Data(3);
  reftime.coarse = Data(4);
  reftime.fine = Data(5);
  orgtime.coarse = Data(6);
  orgtime.fine = Data(7);
  rectime.coarse = Data(8);
  rectime.fine = Data(9);
  xmttime.coarse = Data(10);
  xmttime.fine = Data(11);
#undef Data

#ifdef ENABLE_NTP_DEBUG
  if (debug) {
    NTP_INFO("LI=%d  VN=%d  Mode=%d  Stratum=%d  Poll=%d  Precision=%d", li, vn, mode, stratum, poll, prec);
    NTP_INFO("Delay=%.1f  Dispersion=%.1f  Refid=%u.%u.%u.%u", sec2u(delay), sec2u(disp), refid >> 24 & 0xff,
             refid >> 16 & 0xff, refid >> 8 & 0xff, refid & 0xff);
    NTP_INFO("Reference %u.%.6u", reftime.coarse, USEC(reftime.fine));
    NTP_INFO("(sent)    %u.%.6u", ntpc->time_of_send[0], USEC(ntpc->time_of_send[1]));
    NTP_INFO("Originate %u.%.6u", orgtime.coarse, USEC(orgtime.fine));
    NTP_INFO("Receive   %u.%.6u", rectime.coarse, USEC(rectime.fine));
    NTP_INFO("Transmit  %u.%.6u", xmttime.coarse, USEC(xmttime.fine));
    NTP_INFO("Our recv  %u.%.6u", arrival->coarse, USEC(arrival->fine));
  }
#endif
  el_time = ntpdiff(&orgtime, arrival);  /* elapsed */
  st_time = ntpdiff(&rectime, &xmttime); /* stall */
  skew1 = ntpdiff(&orgtime, &rectime);
  skew2 = ntpdiff(&xmttime, arrival);
  freq = get_current_freq();
#ifdef ENABLE_NTP_DEBUG
  if (debug) {
    NTP_INFO(
        "Total elapsed: %9.2f\n"
        "Server stall:  %9.2f\n"
        "Slop:          %9.2f",
        el_time, st_time, el_time - st_time);
    NTP_INFO(
        "Skew:          %9.2f\n"
        "Frequency:     %9d\n"
        " day   second     elapsed    stall     skew  dispersion  freq",
        (skew1 - skew2) / 2, freq);
  }
#endif

  /* error checking, see RFC-4330 section 5 */
#ifdef ENABLE_NTP_DEBUG
#define FAIL(x)        \
  do {                 \
    drop_reason = (x); \
    goto fail;         \
  } while (0)
#else
#define FAIL(x) goto fail;
#endif
  if (ntpc->cross_check) {
    if (li == 3) FAIL("LI==3"); /* unsynchronized */
    if (vn < 3) FAIL("VN<3");   /* RFC-4330 documents SNTP v4, but we interoperate with NTP v3 */
    if (mode != 4) FAIL("MODE!=3");
    if (orgtime.coarse != ntpc->time_of_send[0] || orgtime.fine != ntpc->time_of_send[1]) FAIL("ORG!=sent");
    if (xmttime.coarse == 0 && xmttime.fine == 0) FAIL("XMT==0");
    if (delay > 65536 || delay < -65536) FAIL("abs(DELAY)>65536");
    if (disp > 65536 || disp < -65536) FAIL("abs(DISP)>65536");
    if (stratum == 0) FAIL("STRATUM==0"); /* kiss o' death */
#undef FAIL
  }

  /* XXX should I do this if debug flag is set? */
  if (ntpc->set_clock) { /* you'd better be root, or ntpclient will exit here! */
    if (set_time(&xmttime)) {
      NTP_ERR("set_time failed");
    } else {
      ntpc->sync_status = 1;
    }
  }

  /* Not the ideal order for printing, but we want to be sure
   * to do all the time-sensitive thinking (and time setting)
   * before we start the output, especially fflush() (which
   * could be slow).  Of course, if debug is turned on, speed
   * has gone down the drain anyway. */
  if (ntpc->live) {
    int new_freq;
    new_freq = contemplate_data(arrival->coarse, (skew1 - skew2) / 2, el_time + sec2u(disp), freq);
    if (!debug && new_freq != freq) set_freq(new_freq);
  }
  NTP_INFO("%d %.5d.%.3d  %8.1f %8.1f  %8.1f %8.1f %9d", arrival->coarse / 86400, arrival->coarse % 86400,
           arrival->fine / 4294967, el_time, st_time, (skew1 - skew2) / 2, sec2u(disp), freq);
  fflush(stdout);
  *error = el_time - st_time;

  return 0;
fail:
#ifdef ENABLE_NTP_DEBUG
  NTP_INFO("%d %.5d.%.3d  rejected packet: %s", arrival->coarse / 86400, arrival->coarse % 86400,
           arrival->fine / 4294967, drop_reason);
#else
  NTP_INFO("%d %.5d.%.3d  rejected packet", arrival->coarse / 86400, arrival->coarse % 86400, arrival->fine / 4294967);
#endif
  return 1;
}

static int stuff_net_addr(struct in_addr *p, const char *hostname) {
  struct hostent *ntpserver;
  ntpserver = gethostbyname(hostname);
  if (ntpserver == NULL) {
    herror(hostname);
    return -1;
  }
  if (ntpserver->h_length != 4) {
    /* IPv4 only, until I get a chance to test IPv6 */
    NTP_INFO("oops %d", ntpserver->h_length);
    return -1;
  }
  memcpy(&(p->s_addr), ntpserver->h_addr_list[0], 4);
  return 0;
}

static int setup_receive(int usd, unsigned int interface, short port) {
  struct sockaddr_in sa_rcvr;
  memset(&sa_rcvr, 0, sizeof sa_rcvr);
  sa_rcvr.sin_family = AF_INET;
  sa_rcvr.sin_addr.s_addr = htonl(interface);
  sa_rcvr.sin_port = htons(port);
  if (bind(usd, (struct sockaddr *)&sa_rcvr, sizeof sa_rcvr) == -1) {
    NTP_ERR("could not bind to udp port %d: %d, %s", port, errno, strerror(errno));
    return -1;
  }
  return 0;
  /* listen(usd,3); this isn't TCP; thanks Alexander! */
}

static int setup_transmit(int usd, const char *host, short port, struct ntp_control *ntpc) {
  struct sockaddr_in sa_dest;
  memset(&sa_dest, 0, sizeof sa_dest);
  sa_dest.sin_family = AF_INET;
  if (stuff_net_addr(&(sa_dest.sin_addr), host)) {
    NTP_ERR("stuff_net_addr failed for %s", host);
    return -1;
  }
  memcpy(ntpc->serv_addr, &(sa_dest.sin_addr), 4); /* XXX asumes IPv4 */
  sa_dest.sin_port = htons(port);
  if (connect(usd, (struct sockaddr *)&sa_dest, sizeof sa_dest) == -1) {
    NTP_ERR("could not connect: %d, %s", errno, strerror(errno));
    return -1;
  }
  return 0;
}

static int primary_loop(int usd, struct ntp_control *ntpc) {
  fd_set fds;
  struct sockaddr sa_xmit;
  int i, pack_len, probes_sent, error;
  socklen_t sa_xmit_len;
  struct timeval to;
  struct ntptime udp_arrival_ntp;
  static u32 incoming_word[325];
  int current_timeout; /* 当前超时时间 */
#define incoming ((char *)incoming_word)
#define sizeof_incoming (sizeof incoming_word)

#ifdef ENABLE_NTP_DEBUG
  if (debug) NTP_INFO("Listening...");
#endif

  probes_sent = 0;
  sa_xmit_len = sizeof sa_xmit;
  current_timeout = ntpc->cycle_time; /* 初始超时时间 */

  /* 发送第一个数据包 */
  send_packet(usd, ntpc->time_of_send);
  ++probes_sent;

  while (probes_sent <= ntpc->probe_count || ntpc->probe_count == 0) {
    FD_ZERO(&fds);
    FD_SET(usd, &fds);

    /* 设置超时时间 */
    to.tv_sec = current_timeout;
    to.tv_usec = 0;

    i = select(usd + 1, &fds, NULL, NULL, &to);
    NTP_INFO("select returned:%d probe_count:%d probes_sent:%d current_timeout:%d", i, ntpc->probe_count, probes_sent,
             current_timeout);

    if (i < 0) {
      /* select 错误处理 */
      if (errno == EINTR || errno == EAGAIN) {
        /* 被信号中断，继续循环 */
        continue;
      }
      NTP_ERR("select error: %d, %s", errno, strerror(errno));
      return -2;

    } else if (i == 0) {
      /* 超时，发送数据包并加倍超时时间 */
      if (probes_sent >= ntpc->probe_count && ntpc->probe_count != 0) {
        return -2; /* 达到最大探测次数 */
      }

      if (ntpc->probe_count == 0) {
        return -2;
      }

      send_packet(usd, ntpc->time_of_send);
      ++probes_sent;

      /* 超时时间加倍，但不超过最大值 */
      current_timeout = ntpc->cycle_time * probes_sent;
      if (current_timeout > 60) { /* 最大超时时间限制为1分钟 */
        current_timeout = 60;
      }

      NTP_INFO("Timeout, sent packet %d, next timeout: %d seconds", probes_sent, current_timeout);
      continue;
    }

    /* 收到数据，重置超时时间为初始值 */
    current_timeout = ntpc->cycle_time;

    if (!FD_ISSET(usd, &fds)) {
      /* 文件描述符未就绪，这种情况不应该发生 */
      NTP_ERR("Unexpected: socket not ready after select returned > 0");
      continue;
    }

    pack_len = recvfrom(usd, incoming, sizeof_incoming, 0, &sa_xmit, &sa_xmit_len);
    error = ntpc->goodness;

    if (pack_len < 0) {
      NTP_ERR("recvfrom error: %d, %s", errno, strerror(errno));
      continue;
    } else if (pack_len > 0 && (unsigned)pack_len < sizeof_incoming) {
      NTP_INFO("Received packet of length %d", pack_len);
      get_packet_timestamp(usd, &udp_arrival_ntp);

      if (check_source(pack_len, &sa_xmit, sa_xmit_len, ntpc) != 0) {
        NTP_INFO("Source check failed, continuing...");
        continue;
      }

      if (rfc1305print(incoming_word, &udp_arrival_ntp, ntpc, &error) != 0) {
        NTP_INFO("rfc1305print failed");
        continue;
      } else {
        NTP_INFO("rfc1305print succeeded");
        break;
      }
    } else {
      NTP_INFO("Invalid packet length: %d", pack_len);
      fflush(stdout);
    }

    /* 检查是否满足退出条件 */
    if ((error < ntpc->goodness && ntpc->goodness != 0) ||
        (probes_sent >= ntpc->probe_count && ntpc->probe_count != 0)) {
      ntpc->set_clock = 0;
      if (!ntpc->live) break;
    }
  }
#undef incoming
#undef sizeof_incoming

  if (ntpc->sync_status) {
    return 1;
  } else {
    return 0;
  }
}

#ifdef ENABLE_NTP_REPLAY
static void do_replay(void) {
  char line[100];
  int n, day, freq, absolute;
  float sec, el_time, st_time, disp;
  double skew, errorbar;
  int simulated_freq = 0;
  unsigned int last_fake_time = 0;
  double fake_delta_time = 0.0;

  while (fgets(line, sizeof line, stdin)) {
    n = sscanf(line, "%d %f %f %f %lf %f %d", &day, &sec, &el_time, &st_time, &skew, &disp, &freq);
    if (n == 7) {
      fputs(line, stdout);
      absolute = day * 86400 + (int)sec;
      errorbar = el_time + disp;
#ifdef ENABLE_NTP_DEBUG
      if (debug) NTP_INFO("contemplate %u %.1f %.1f %d", absolute, skew, errorbar, freq);
#endif
      if (last_fake_time == 0) simulated_freq = freq;
      fake_delta_time += (absolute - last_fake_time) * ((double)(freq - simulated_freq)) / 65536;
#ifdef ENABLE_NTP_DEBUG
      if (debug) NTP_INFO("fake %f %d ", fake_delta_time, simulated_freq);
#endif
      skew += fake_delta_time;
      freq = simulated_freq;
      last_fake_time = absolute;
      simulated_freq = contemplate_data(absolute, skew, errorbar, freq);
    } else {
      NTP_ERR("Replay input error");
      exit(2);
    }
  }
}
#endif

static void usage(char *argv0) {
  NTP_ERR(
      "Usage: %s [-c count]"
#ifdef ENABLE_NTP_DEBUG
      " [-d]"
#endif
      " [-f frequency] [-g goodness] -h hostname\n"
      "\t[-i interval] [-l] [-p port] [-q ntp_min_delay]"
#ifdef ENABLE_NTP_REPLAY
      " [-r]"
#endif
      " [-s] [-t]\n",
      argv0);
}

#if 0
int main(int argc, char *argv[]) {
	int usd;  /* socket */
	int c;
	/* These parameters are settable from the command line
	   the initializations here provide default behavior */
	short int udp_local_port=0;   /* default of 0 means kernel chooses */
	char *hostname=NULL;          /* must be set */
	int initial_freq;             /* initial freq value to use */
	struct ntp_control ntpc;
	ntpc.live=0;
	ntpc.set_clock=0;
	ntpc.probe_count=0;           /* default of 0 means loop forever */
	ntpc.cycle_time=15;          /* seconds */
	ntpc.goodness=0;
	ntpc.cross_check=1;

	for (;;) {
		c = getopt( argc, argv, "c:" DEBUG_OPTION "f:g:h:i:lp:q:" REPLAY_OPTION "st");
		if (c == EOF) break;
		switch (c) {
			case 'c':
				ntpc.probe_count = atoi(optarg);
				break;
#ifdef ENABLE_NTP_DEBUG
			case 'd':
				++debug;
				break;
#endif
			case 'f':
				initial_freq = atoi(optarg);
				if (debug) NTP_INFO("initial frequency %d",
						initial_freq);
				set_freq(initial_freq);
				break;
			case 'g':
				ntpc.goodness = atoi(optarg);
				break;
			case 'h':
				hostname = optarg;
				break;
			case 'i':
				ntpc.cycle_time = atoi(optarg);
				break;
			case 'l':
				(ntpc.live)++;
				break;
			case 'p':
				udp_local_port = atoi(optarg);
				break;
			case 'q':
				ntp_min_delay = atof(optarg);
				break;
#ifdef ENABLE_NTP_REPLAY
			case 'r':
				do_replay();
				exit(0);
				break;
#endif
			case 's':
				(ntpc.set_clock)++;
				break;

			case 't':
				(ntpc.cross_check)=0;
				break;

			default:
				usage(argv[0]);
				exit(1);
		}
	}
	if (hostname == NULL) {
		usage(argv[0]);
		exit(1);
	}

	if (ntpc.set_clock && !ntpc.live && !ntpc.goodness && !ntpc.probe_count) {
		ntpc.probe_count = 1;
	}

	/* respect only applicable MUST of RFC-4330 */
	if (ntpc.probe_count != 1 && ntpc.cycle_time < MIN_INTERVAL) {
		ntpc.cycle_time = MIN_INTERVAL;
	}

	if (debug) {
		NTP_INFO("Configuration:\n"
		"  -c probe_count %d\n"
		"  -d (debug)     %d\n"
		"  -g goodness    %d\n"
		"  -h hostname    %s\n"
		"  -i interval    %d\n"
		"  -l live        %d\n"
		"  -p local_port  %d\n"
		"  -q ntp_min_delay   %f\n"
		"  -s set_clock   %d\n"
		"  -x cross_check %d\n",
		ntpc.probe_count, debug, ntpc.goodness,
		hostname, ntpc.cycle_time, ntpc.live, udp_local_port, ntp_min_delay,
		ntpc.set_clock, ntpc.cross_check );
	}

	/* Startup sequence */
	if ((usd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1)
		{perror ("socket");exit(1);}

	setup_receive(usd, INADDR_ANY, udp_local_port);

	setup_transmit(usd, hostname, NTP_PORT, &ntpc);

	ntpc.sync_status = 0;

	int sync_result = primary_loop(usd, &ntpc);
	close(usd);
	
	if (sync_result == 1) {
        NTP_INFO("NTP time synchronization successful");
        return 0; 
    } else if (sync_result == -2) {
        NTP_INFO("NTP time synchronization timed out");
        return 1;
    } else {
        NTP_INFO("NTP time synchronization failed");
        return 2;
    }
}
#endif

int ntpc(const char *hostname) {
  int usd; /* socket */
  int c;
  /* These parameters are settable from the command line
     the initializations here provide default behavior */
  short int udp_local_port = 0; /* default of 0 means kernel chooses */
  int initial_freq;             /* initial freq value to use */
  struct ntp_control ntpc;
  ntpc.live = 0;
  ntpc.set_clock = 1;
  ntpc.probe_count = 3; /* default of 0 means loop forever */
  ntpc.cycle_time = 15; /* seconds */
  ntpc.goodness = 0;
  ntpc.cross_check = 1;

  NTP_INFO("Starting ntpc with hostname: %s", hostname);

  if (ntpc.set_clock && !ntpc.live && !ntpc.goodness && !ntpc.probe_count) {
    ntpc.probe_count = 1;
  }

  /* respect only applicable MUST of RFC-4330 */
  if (ntpc.probe_count != 1 && ntpc.cycle_time < MIN_INTERVAL) {
    ntpc.cycle_time = MIN_INTERVAL;
  }

  /* Startup sequence */
  if ((usd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    NTP_ERR("Failed to create UDP socket %d: %s", errno, strerror(errno));
    return -2;
  }

  if (setup_receive(usd, INADDR_ANY, udp_local_port)) {
    NTP_ERR("Failed to set up receive socket");
    close(usd);
    return -2;
  }

  if (setup_transmit(usd, hostname, NTP_PORT, &ntpc)) {
    NTP_ERR("Failed to set up transmit socket");
    close(usd);
    return -2;
  }

  ntpc.sync_status = 0;

  int sync_result = primary_loop(usd, &ntpc);
  close(usd);

  if (sync_result == 1) {
    NTP_INFO("NTP time synchronization successful");
    return 0;
  } else if (sync_result == -2) {
    NTP_INFO("NTP time synchronization timed out");
    return -1;
  } else {
    NTP_INFO("NTP time synchronization failed");
    return -2;
  }
}
