#ifndef __linux__
#include <pthread.h>
#include <deque>
#include <cerrno>
#include "pseudo_pipe.h"
#include "AFakeNative/AFakeNative_Utils.h"

#define PIPEFD_MARGIN 384
#define PIPEFD_MAX 64

#define MSGPIPE_MEMTYPE_USER_MAIN 0x40
#define MSGPIPE_THREAD_ATTR_PRIO (0x8 | 0x4)

struct PipeBuffer {
    pthread_mutex_t mtx;
    pthread_cond_t read_cv;
    pthread_cond_t write_cv;

    std::deque<uint8_t> buffer;
    size_t capacity;
    bool closed;
};

void pipe_init(PipeBuffer* p, size_t cap = 4 * 4096) {
    pthread_mutex_init(&p->mtx, nullptr);
    pthread_cond_init(&p->read_cv, nullptr);
    pthread_cond_init(&p->write_cv, nullptr);

    p->capacity = cap;
    p->closed = false;
}

ssize_t pipe_write(PipeBuffer* p, const void* buf, size_t count) {
    pthread_mutex_lock(&p->mtx);

    if (p->closed) {
        pthread_mutex_unlock(&p->mtx);
        return -1;
    }

    const uint8_t* data = (const uint8_t*)buf;
    size_t written = 0;

    while (written < count) {
        while (p->buffer.size() >= p->capacity) {
            pthread_cond_wait(&p->write_cv, &p->mtx);
            if (p->closed) {
                pthread_mutex_unlock(&p->mtx);
                return -1;
            }
        }

        p->buffer.push_back(data[written++]);
        pthread_cond_signal(&p->read_cv);
    }

    pthread_mutex_unlock(&p->mtx);
    return written;
}

ssize_t pipe_read(PipeBuffer* p, void* buf, size_t count) {
    pthread_mutex_lock(&p->mtx);

    while (p->buffer.empty() && !p->closed) {
        pthread_cond_wait(&p->read_cv, &p->mtx);
    }

    if (p->buffer.empty() && p->closed) {
        pthread_mutex_unlock(&p->mtx);
        return 0;
    }

    uint8_t* out = (uint8_t*)buf;
    size_t n = 0;

    while (n < count && !p->buffer.empty()) {
        out[n++] = p->buffer.front();
        p->buffer.pop_front();
    }

    pthread_cond_signal(&p->write_cv);
    pthread_mutex_unlock(&p->mtx);

    return n;
}

void pipe_close(PipeBuffer* p) {
    pthread_mutex_lock(&p->mtx);
    p->closed = true;

    pthread_cond_broadcast(&p->read_cv);
    pthread_cond_broadcast(&p->write_cv);

    pthread_mutex_unlock(&p->mtx);
}

typedef struct pipefd_internal {
    int readfd; // >=0 indicates that it's in use
    int writefd;
    PipeBuffer msgpipe;
    bool readable;
    bool writeable;
} pipefd_internal;

static pipefd_internal pipefd_pool[PIPEFD_MAX];
pthread_mutex_t pipefd_pool_mutex;
bool pipefd_pool_mutex_initialized;

int pseudo_pipe(int pipefd[2]) {
#ifdef DEBUG_PIPEFD
    ALOGD("pseudo_pipe: called\n");
#endif


    if (!pipefd_pool_mutex_initialized) {
        pthread_mutex_init(&pipefd_pool_mutex, nullptr);
        pthread_mutex_lock(&pipefd_pool_mutex);
        pipefd_pool_mutex_initialized = true;

        for (int i = 0; i < PIPEFD_MAX; ++i) {
            pipefd_pool[i].readfd = -1;
            pipefd_pool[i].writefd = -1;
            pipefd_pool[i].readable = false;
            pipefd_pool[i].writeable = false;
        }

        #ifdef DEBUG_PIPEFD
            ALOGD("pseudo_pipe: initialized the pool\n");
        #endif
    } else {
        pthread_mutex_lock(&pipefd_pool_mutex);
    }

    pipefd_internal * pipe = nullptr;
    for (int i = 0, u = 0; u < PIPEFD_MAX; i++, u+=2) {
        if (pipefd_pool[i].readfd == -1) {
            pipe = &pipefd_pool[i];

            pipe->readfd = u + PIPEFD_MARGIN;
            pipe->writefd = u + 1 + PIPEFD_MARGIN;
            pipe->readable = false;
            pipe->writeable = true;
            pipe_init(&pipe->msgpipe);

            break;
        }
    }

    if (!pipe) {
        pthread_mutex_unlock(&pipefd_pool_mutex);
        errno = EMFILE;
        return -1;
    }

    pipefd[0] = pipe->readfd;
    pipefd[1] = pipe->writefd;

#ifdef DEBUG_PIPEFD
    ALOGD("pseudo_pipe: pipe<%i, %i> initialized", pipe->readfd, pipe->writefd);
#endif

    pthread_mutex_unlock(&pipefd_pool_mutex);
    return 0;
}

ssize_t pseudo_pipe_read(int fd, void *buf, size_t count) {
    if (!pipefd_pool_mutex_initialized) {
        return -1;
    }
    pthread_mutex_lock(&pipefd_pool_mutex);

    pipefd_internal * pipe = nullptr;
    for (int i = 0; i < PIPEFD_MAX; i++) {
        if (pipefd_pool[i].readfd == fd) {
#ifdef DEBUG_PIPEFD
            ALOGD("pseudo_pipe_read: found pipe<%i, %i> for reading", pipefd_pool[i].readfd, pipefd_pool[i].writefd);
#endif
            pipe = &pipefd_pool[i];
            break;
        }
    }

    if (!pipe) {
        pthread_mutex_unlock(&pipefd_pool_mutex);
        errno = EINVAL;
        return -1;
    }
    ssize_t rlen = count;
    if (rlen > 4 * 4096) rlen = 4 * 4096;
    ssize_t ret = pipe_read(pipe->msgpipe, buf, rlen);
    if (ret == 0 && pipe->msgpipe.buffer.empty()) {
#ifdef DEBUG_PIPEFD
        ALOGD("pseudo_pipe_read: pipe<%i, %i> set as NOT readable", pipe->readfd, pipe->writefd);
#endif
        pipe->readable = false;
    }

#ifdef DEBUG_PIPEFD
    ALOGD("pseudo_pipe_read: pipe<%i, %i>, count %i, ret %i", pipe->readfd, pipe->writefd, count, ret);
#endif
    pthread_mutex_unlock(&pipefd_pool_mutex);
    return ret;
}

#define SCE_KERNEL_MSG_PIPE_MODE_FULL 0x00000001U

ssize_t pseudo_pipe_write(int fd, const void *buf, size_t count) {
    if (!pipefd_pool_mutex_initialized) {
        return -1;
    }
    pthread_mutex_lock(&pipefd_pool_mutex);

    pipefd_internal * pipe = nullptr;
    for (int i = 0; i < PIPEFD_MAX; ++i) {
        if (pipefd_pool[i].writefd == fd) {
#ifdef DEBUG_PIPEFD
            ALOGD("pseudo_pipe_write: found pipe<%i, %i> for writing", pipefd_pool[i].readfd, pipefd_pool[i].writefd);
#endif
            pipe = &pipefd_pool[i];
            break;
        }
    }

    if (!pipe) {
        pthread_mutex_unlock(&pipefd_pool_mutex);
        errno = EINVAL;
        return -1;
    }

    size_t len = count;
    if (len > 4 * 4096) len = 4 * 4096;

    ssize_t ret = pipe_write(pipe->msgpipe, (void *)buf, len);
    if (ret == 0) {
        ret = len;

#ifdef DEBUG_PIPEFD
        ALOGD("pseudo_pipe_write: pipe<%i, %i> set as readable", pipe->readfd, pipe->writefd);
#endif

        pipe->readable = true;
    }

    pthread_mutex_unlock(&pipefd_pool_mutex);
#ifdef DEBUG_PIPEFD
    ALOGD("pseudo_pipe_write: pipe<%i, %i>, count %i, ret %i", pipe->readfd, pipe->writefd, count, ret);
#endif
    return ret;
}

void pseudo_pipe_status(int fd, bool * is_readable, bool * is_writeable) {
    if (!pipefd_pool_mutex_initialized) {
        return;
    }
    pthread_mutex_lock(&pipefd_pool_mutex);

    for (int u = 0; u < PIPEFD_MAX; u++) {
        if (pipefd_pool[u].writefd == fd || pipefd_pool[u].readfd == fd) {
            *is_readable = pipefd_pool[u].readable;
            *is_writeable = pipefd_pool[u].writeable;
            pipefd_pool[u].readable = false;
            pipefd_pool[u].writeable = false;
            break;
        }
    }

    pthread_mutex_unlock(&pipefd_pool_mutex);
}

bool is_pipe(int fd) {
    pipefd_internal * p = nullptr;

    pthread_mutex_lock(&pipefd_pool_mutex);

    for (int i = 0; i < PIPEFD_MAX; ++i) {
        if (pipefd_pool[i].readfd == fd || pipefd_pool[i].writefd == fd) {
            p = &pipefd_pool[i];
            break;
        }
    }

    pthread_mutex_unlock(&pipefd_pool_mutex);

    return p != nullptr;
}
#endif