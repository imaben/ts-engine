#pragma once

#include <pthread.h>

struct _ts_lock {
    pthread_mutex_t mutex;
    int (*lock)(struct _ts_lock *lock);
    int (*unlock)(struct _ts_lock *lock);
};

struct _ts_rwlock {
    pthread_rwlock_t mutex;
    int (*rdlock)(struct _ts_rwlock *lock);
    int (*wrlock)(struct _ts_rwlock *lock);
    int (*unlock)(struct _ts_rwlock *lock);
};

struct _ts_spinlock {
    volatile unsigned int mutex;
    int (*lock)(struct _ts_spinlock *lock);
    int (*unlock)(struct _ts_spinlock *lock);
};

typedef struct _ts_lock ts_lock_t;
typedef struct _ts_rwlock ts_rwlock_t;
typedef struct _ts_spinlock ts_spinlock_t;

void ts_lock_init(ts_lock_t *lock);
void ts_lock_destroy(ts_lock_t *lock);

void ts_rwlock_init(ts_rwlock_t *lock);
void ts_rwlock_destroy(ts_rwlock_t *lock);

void ts_spinlock_init(ts_spinlock_t *lock);
void ts_spinlock_destroy(ts_spinlock_t *lock);
