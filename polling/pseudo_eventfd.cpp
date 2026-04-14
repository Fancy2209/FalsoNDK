#ifndef __linux__
#include <cstdint>
#include <psp2/kernel/threadmgr.h>
#include <malloc.h>
#include <cerrno>
#include <cstdio>
#include <sys/unistd.h>
#include "AFakeNative/AFakeNative_Utils.h"
#include "AFakeNative/PseudoEpoll.h"

#include "pseudo_eventfd.h"

#define EVENTFD_MARGIN 256
#define EVENTFD_MAX 64

typedef struct eventfd_internal {
    int fd = -1; // >=0 indicates that it's in use
    uint64_t value{};
    int flags{};
    pthread_mutex_t * mutex{};
} eventfd_internal;

static eventfd_internal eventfd_pool[EVENTFD_MAX];
pthread_mutex_t eventfd_pool_mutex;
bool eventfd_pool_mutex_initialized = false;

int pseudo_eventfd(unsigned int initval, int flags) {
    if (!eventfd_pool_mutex_initialized) {
        pthread_mutex_init(&eventfd_pool_mutex, NULL);
        pthread_mutex_lock(&eventfd_pool_mutex);

        for (int i = 0; i < EVENTFD_MAX; ++i) {
            eventfd_pool[i].fd = -1;
            eventfd_pool[i].value = 0;
            eventfd_pool[i].flags = 0;
            eventfd_pool[i].mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
        }
    } else {
        pthread_mutex_lock(&eventfd_pool_mutex);
    }

    eventfd_internal * fd = nullptr;
    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == -1) {
            eventfd_pool[i].fd = i + EVENTFD_MARGIN;
            fd = &eventfd_pool[i];
            pthread_mutex_init(eventfd_pool[i].mutex, NULL);
            break;
        }
    }

    if (!fd) {
        pthread_mutex_unlock(&eventfd_pool_mutex);
        errno = EMFILE;
        return -1;
    }

    fd->value = initval;
    fd->flags = flags;

    pthread_mutex_unlock(&eventfd_pool_mutex);
#ifdef DEBUG_POLL_AND_WAKE
    ALOGD("Created eventfd #%i from addr %p", fd->fd, __builtin_return_address(0));
#endif
    return fd->fd;
}

bool is_eventfd(int fd) {
    eventfd_internal * p = nullptr;

    pthread_mutex_lock(&eventfd_pool_mutex);

    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == fd) {
            p = &eventfd_pool[i];
            break;
        }
    }

    pthread_mutex_unlock(&eventfd_pool_mutex);

    return p != nullptr;
}

ssize_t pseudo_eventfd_read(int fd, void *buf, size_t count) {
    if (!eventfd_pool_mutex_initialized) {
        return -1;
    }
    pthread_mutex_lock(&eventfd_pool_mutex);

    eventfd_internal * efd = nullptr;

    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == fd) {
            efd = &eventfd_pool[i];
            break;
        }
    }

    if (!efd) {
        pthread_mutex_unlock(&eventfd_pool_mutex);
        errno = EINVAL;
        return -1;
    }

    if (count < 8 || !buf) {
        pthread_mutex_unlock(&eventfd_pool_mutex);
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(efd->mutex);

    if (efd->value == 0) {
        if (efd->flags & PSEUDO_EFD_NONBLOCK) {
            pthread_mutex_unlock(efd->mutex);
            pthread_mutex_unlock(&eventfd_pool_mutex);
            errno = EAGAIN;
            return -1;
        } else {
            for (;;) {
                pthread_mutex_unlock(efd->mutex);
                pthread_mutex_unlock(&eventfd_pool_mutex);
                usleep(10000);
                pthread_mutex_lock(&eventfd_pool_mutex);
                pthread_mutex_lock(efd->mutex);

                if (efd->value != 0) {
                    break;
                }
            }
        }
    }

    if (efd->flags & PSEUDO_EFD_SEMAPHORE && efd->value != 0) {
        *(uint64_t *)buf = (uint64_t) 1;
        efd->value--;
        pthread_mutex_unlock(efd->mutex);
        pthread_mutex_unlock(&eventfd_pool_mutex);
        return 8;
    }

    // Non-semaphore, non-zero value
    *(uint64_t *)buf = efd->value;
    efd->value = 0;
    pthread_mutex_unlock(efd->mutex);
    pthread_mutex_unlock(&eventfd_pool_mutex);
    return 8;
}

ssize_t pseudo_eventfd_write(int fd, const void *buf, size_t count) {
    if (!eventfd_pool_mutex_initialized) {
        return -1;
    }
    pthread_mutex_lock(&eventfd_pool_mutex);

    uint64_t val;
    eventfd_internal * efd = nullptr;

    for (int i = 0; i < EVENTFD_MAX; ++i) {
        if (eventfd_pool[i].fd == fd) {
            efd = &eventfd_pool[i];
            break;
        }
    }

    if (!efd) {
        pthread_mutex_unlock(&eventfd_pool_mutex);
        errno = EINVAL;
        return -1;
    }

    if (count < 8 || !buf) {
        pthread_mutex_unlock(&eventfd_pool_mutex);
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(efd->mutex);

    val = *(uint64_t *) buf;
    if (0xfffffffffffffffe - efd->value < val) {
        if (efd->flags & PSEUDO_EFD_NONBLOCK) {
            pthread_mutex_unlock(efd->mutex);
            pthread_mutex_unlock(&eventfd_pool_mutex);
            errno = EAGAIN;
            return -1;
        } else {
            for (;;) {
                pthread_mutex_unlock(efd->mutex);
                pthread_mutex_unlock(&eventfd_pool_mutex);
                usleep(10000);
                pthread_mutex_lock(&eventfd_pool_mutex);
                pthread_mutex_lock(efd->mutex);

                if (0xfffffffffffffffe - efd->value >= val) {
                    break;
                }
            }
        }
    }

    efd->value += val;
    pthread_mutex_unlock(efd->mutex);
    pthread_mutex_unlock(&eventfd_pool_mutex);
    return 8;
}

void pseudo_eventfd_status(int fd, bool * is_readable, bool * is_writeable) {
    if (!eventfd_pool_mutex_initialized) {
        return;
    }

    pthread_mutex_lock(&eventfd_pool_mutex);

    for (int u = 0; u < EVENTFD_MAX; ++u) {
        if (eventfd_pool[u].fd == fd) {
            pthread_mutex_lock(eventfd_pool[u].mutex);
            *is_readable = eventfd_pool[u].value > 0;
            *is_writeable = eventfd_pool[u].value < 0xfffffffffffffffe;
            pthread_mutex_unlock(eventfd_pool[u].mutex);
            break;
        }
    }

    pthread_mutex_unlock(&eventfd_pool_mutex);
}
#endif
