

//
// This is a copy from

//   trunk/src/common/system/concurrency/mutex.hpp
//
#ifndef SYSTEM_MUTEX_H_
#define SYSTEM_MUTEX_H_

#if defined _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0501  // windows xp
# endif
# define NOMINMAX 1
# include <windows.h>
#elif defined __unix__
# include <pthread.h>
#else
# error Unknown platform
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <stdexcept>

#include "system/scoped_locker.h"

class ConditionVariable;

#if defined _WIN32

// if _WIN32_WINNT not defined, TryEnterCriticalSection will not be declared
// in windows.h
extern "C" WINBASEAPI
BOOL WINAPI TryEnterCriticalSection(
    __inout LPCRITICAL_SECTION lpCriticalSection);

class Mutex {
 public:
  typedef ScopedLocker<Mutex> Locker;
 public:
  explicit Mutex(bool recursive = true) {
    ::InitializeCriticalSection(&m_Mutex);
  }

  ~Mutex() {
    ::DeleteCriticalSection(&m_Mutex);
  }

  void Lock() {
    ::EnterCriticalSection(&m_Mutex);
    assert(IsLocked());
  }

  bool TryLock() {
    return ::TryEnterCriticalSection(&m_Mutex) != FALSE;
  }

  void Unlock() {
    assert(IsLocked());
    ::LeaveCriticalSection(&m_Mutex);
  }

  bool IsLocked() const {
    if (IsNewBehavior())  // after win2k3 sp1
      return (m_Mutex.LockCount & 1) == 0;
    else
      return m_Mutex.LockCount >= 0;
  }

 private:
  // In Microsoft Windows Server 2003 Service Pack 1 and later versions of
  // Windows, the LockCount field is parsed as follows:
  // * The lowest bit shows the lock status. If this bit is 0, the critical
  //   section is locked; if it is 1, the critical section is not locked.
  // * The next bit shows whether a thread has been woken for this lock.
  //   If this bit is 0, then a thread has been woken for this lock; if it
  //   is 1, no thread has been woken.
  // * The remaining bits are the ones-complement of the number of threads
  //   waiting for the lock.
  static bool IsNewBehavior() {
    static bool result = DoIsNewBehavior();
    return result;
  }
  static bool DoIsNewBehavior() {
    Mutex mutex;
    int old_lock_count = mutex.m_Mutex.LockCount;
    mutex.Lock();
    int new_lock_count = mutex.m_Mutex.LockCount;
    return new_lock_count < old_lock_count;
  }
 private:
  Mutex(const Mutex& right);
  Mutex& operator = (const Mutex& right);
  CRITICAL_SECTION m_Mutex;
  friend class Cond;
};

#elif defined __unix__

class Mutex {
 public:
  typedef ScopedLocker<Mutex> Locker;
 public:
  explicit Mutex(bool recursive = true) {
    int n;
    if (recursive) {
      pthread_mutexattr_t attr;
      n = pthread_mutexattr_init(&attr);
      if (n == 0) {
        n = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if (n == 0)
          n = pthread_mutex_init(&m_Mutex, &attr);
        n = pthread_mutexattr_destroy(&attr);
      }
    } else {
      n = pthread_mutex_init(&m_Mutex, NULL);
    }
    CheckError("Mutex::Mutex", n);
  }
  ~Mutex() {
    pthread_mutex_destroy(&m_Mutex);
  }

  void Lock() {
    CheckError("Mutex::Lock", pthread_mutex_lock(&m_Mutex));
    assert(IsLocked());
  }

  bool TryLock() {
    int n = pthread_mutex_trylock(&m_Mutex);
    if (n == EBUSY) {
      return false;
    } else {
      CheckError("Mutex::Lock", n);
      return true;
    }
  }

  // by inspect internal data
  bool IsLocked() const {
    return m_Mutex.__data.__lock > 0;
  }

  void Unlock() {
    assert(IsLocked());
    CheckError("Mutex::Unlock", pthread_mutex_unlock(&m_Mutex));
    // NOTE: can't check unlocked here, maybe already locked by
    // other thread
  }

 private:
  static void CheckError(const char* context, int error) {
    if (error != 0) {
      std::string msg = context;
      msg += " error: ";
      msg += strerror(error);
      throw std::runtime_error(msg);
    }
  }
 private:
  Mutex(const Mutex& right);
  Mutex& operator = (const Mutex& right);
 private:
  pthread_mutex_t m_Mutex;
  friend class ConditionVariable;
};

#endif

typedef Mutex::Locker MutexLocker;

/// null mutex for template mutex param placeholder
/// NOTE: don't make this class uncopyable
class NullMutex {
 public:
  typedef ScopedLocker<NullMutex> Locker;
 public:
  NullMutex() : m_locked(false) {
  }

  void Lock() {
    m_locked = true;
  }

  bool TryLock() {
    m_locked = true;
    return true;
  }

  // by inspect internal data
  bool IsLocked() const {
    return m_locked;
  }

  void Unlock() {
    m_locked = false;
  }
 private:
  bool m_locked;
};

#endif  // SYSTEM_MUTEX_H_

