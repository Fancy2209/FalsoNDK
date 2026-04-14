#pragma once

#include <stdint.h>
#include <sys/fcntl.h>

#ifndef __linux__
#define PSEUDO_EPOLL_CLOEXEC O_CLOEXEC
#define PSEUDO_EPOLL_CLOEXEC O_CLOEXEC
#define PSEUDO_EPOLL_CTL_ADD 1
#define PSEUDO_EPOLL_CTL_DEL 2
#define PSEUDO_EPOLL_CTL_MOD 3
#define PSEUDO_EPOLLIN 0x00000001
#define PSEUDO_EPOLLPRI 0x00000002
#define PSEUDO_EPOLLOUT 0x00000004
#define PSEUDO_EPOLLERR 0x00000008
#define PSEUDO_EPOLLHUP 0x00000010
#define PSEUDO_EPOLLNVAL 0x00000020
#define PSEUDO_EPOLLRDNORM 0x00000040
#define PSEUDO_EPOLLRDBAND 0x00000080
#define PSEUDO_EPOLLWRNORM 0x00000100
#define PSEUDO_EPOLLWRBAND 0x00000200
#define PSEUDO_EPOLLMSG 0x00000400
#define PSEUDO_EPOLLRDHUP 0x00002000
#define PSEUDO_EPOLLEXCLUSIVE (1U << 28)
#define PSEUDO_EPOLLWAKEUP (1U << 29)
#define PSEUDO_EPOLLONESHOT (1U << 30)
#define PSEUDO_EPOLLET (1U << 31)

/** The eventfd() flag to provide semaphore-like semantics for reads. */
#define PSEUDO_EFD_SEMAPHORE (1 << 0)
/** The eventfd() flag for a close-on-exec file descriptor. */
#define PSEUDO_EFD_CLOEXEC O_CLOEXEC
/** The eventfd() flag for a non-blocking file descriptor. */
#define PSEUDO_EFD_NONBLOCK O_NONBLOCK
#else
#include <unistd.h>
#define PSEUDO_EPOLL_CLOEXEC EPOLL_CLOEXEC
#define PSEUDO_EPOLL_CLOEXEC EPOLL_CLOEXEC
#define PSEUDO_EPOLL_CTL_ADD EPOLL_CTL_ADD
#define PSEUDO_EPOLL_CTL_DEL EPOLL_CTL_DEL
#define PSEUDO_EPOLL_CTL_MOD EPOLL_CTL_MOD
#define PSEUDO_EPOLLIN EPOLLIN
#define PSEUDO_EPOLLPRI EPOLLPRI
#define PSEUDO_EPOLLOUT EPOLLOUT
#define PSEUDO_EPOLLERR EPOLLERR
#define PSEUDO_EPOLLHUP EPOLLHUP
#define PSEUDO_EPOLLNVAL EPOLLNVAL
#define PSEUDO_EPOLLRDNORM EPOLLRDNORM
#define PSEUDO_EPOLLRDBAND EPOLLRDBAND
#define PSEUDO_EPOLLWRNORM EPOLLWRNORM
#define PSEUDO_EPOLLWRBAND EPOLLWRBAND
#define PSEUDO_EPOLLMSG EPOLLMSG
#define PSEUDO_EPOLLRDHUP EPOLLRDHUP
#define PSEUDO_EPOLLEXCLUSIVE EPOLLEXCLUSIVE
#define PSEUDO_EPOLLWAKEUP EPOLLWAKEUP
#define PSEUDO_EPOLLONESHOT EPOLLONESHOT
#define PSEUDO_EPOLLET EPOLLET

/** The eventfd() flag to provide semaphore-like semantics for reads. */
#define PSEUDO_EFD_SEMAPHORE EFD_SEMAPHORE
/** The eventfd() flag for a close-on-exec file descriptor. */
#define PSEUDO_EFD_CLOEXEC EFD_CLOEXEC
/** The eventfd() flag for a non-blocking file descriptor. */
#define PSEUDO_EFD_NONBLOCK EFD_NONBLOCK
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __linux__
typedef union pseudo_epoll_data {
    void    *ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
} pseudo_epoll_data_t;

struct pseudo_epoll_event {
    uint32_t     events;    /* Epoll events */
    pseudo_epoll_data_t data;      /* User data variable */
};

int pseudo_epoll_wait(int epfd, struct pseudo_epoll_event *events, int maxevents, int timeout);

int pseudo_epoll_create(int size);

int pseudo_epoll_create1(int flags);

int pseudo_epoll_ctl(int epfd, int op, int fd, struct pseudo_epoll_event *event);

ssize_t pseudo_read(int fd, void *buf, size_t count);
ssize_t pseudo_write(int fd, const void *buf, size_t count);
#else
#define pseudo_epoll_event epoll_event
#define pseudo_epoll_wait epoll_wait
#define pseudo_epoll_create epoll_create
#define pseudo_epoll_create1 epoll_create1
#define pseudo_epoll_ctl epoll_ctl
#endif

#ifdef __cplusplus
};
#endif
