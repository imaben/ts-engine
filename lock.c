#include "lock.h"
#include "memory.h"
#include <assert.h>
#include <strings.h>

static int ts_lock_lock(ts_lock_t *lock);
static int ts_lock_unlock(ts_lock_t *lock);

static int ts_rwlock_rdlock(ts_rwlock_t *lock);
static int ts_rwlock_wrlock(ts_rwlock_t *lock);
static int ts_rwlock_unlock(ts_rwlock_t *lock);

static int ts_spinlock_lock(ts_spinlock_t *lock);
static int ts_spinlock_unlock(ts_spinlock_t *lock);

void ts_lock_init(ts_lock_t *lock) {
    pthread_mutex_init(&lock->mutex, NULL);
    lock->lock = ts_lock_lock;
    lock->unlock = ts_lock_unlock;
}

static int ts_lock_lock(ts_lock_t *lock) {
    return pthread_mutex_lock(&lock->mutex);
}

static int ts_lock_unlock(ts_lock_t *lock) {
    return pthread_mutex_unlock(&lock->mutex);
}

void ts_lock_destroy(ts_lock_t *lock) {
    pthread_mutex_destroy(&lock->mutex);
}

void ts_rwlock_init(ts_rwlock_t *lock) {
    pthread_rwlock_init(&lock->mutex, NULL);
    lock->rdlock = ts_rwlock_rdlock;
    lock->wrlock = ts_rwlock_wrlock;
    lock->unlock = ts_rwlock_unlock;
}

static int ts_rwlock_rdlock(ts_rwlock_t *lock) {
    return pthread_rwlock_rdlock(&lock->mutex);
}

static int ts_rwlock_wrlock(ts_rwlock_t *lock) {
    return pthread_rwlock_wrlock(&lock->mutex);
}

static int ts_rwlock_unlock(ts_rwlock_t *lock) {
    return pthread_rwlock_unlock(&lock->mutex);
}

void ts_rwlock_destroy(ts_rwlock_t *lock) {
    pthread_rwlock_destroy(&lock->mutex);
}

void ts_spinlock_init(ts_spinlock_t *lock) {
    lock->mutex = 0;
    lock->lock = ts_spinlock_lock;
    lock->unlock = ts_spinlock_unlock;
}

static int ts_spinlock_lock(ts_spinlock_t *lock) {
    int i, n;
    for (;;) {
        if (lock->mutex == 0 && 
                __sync_bool_compare_and_swap(&lock->mutex, 0, 1)) {
            return 0;
        }

        for (n = 1; n < 129; n <<= 1) {
            for (i = 0; i < n; i++) {
                __asm__("pause");
            }
            if (lock->mutex == 0 && 
                    __sync_bool_compare_and_swap(&lock->mutex, 0, 1)) {
                return 0;
            }
        }

        sched_yield();
    }
    return 0;
}

static int ts_spinlock_unlock(ts_spinlock_t *lock) {
    __sync_bool_compare_and_swap(&lock->mutex, 1, 0);
    return 0;
}

void ts_spinlock_destroy(ts_spinlock_t *lock) {
}

