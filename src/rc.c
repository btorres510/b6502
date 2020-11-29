#include "b6502/rc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct Rc {
  size_t strong_count;
  size_t weak_count;
  Destructor destructor;
} Rc;

static inline Rc* get_rc(void* obj) { return (Rc*)obj - 1; }
static inline void* get_obj(Rc* rc) { return rc + 1; }

void* rc_alloc(size_t size, Destructor destructor) {
  Rc* rc = calloc(1, size + sizeof(*rc));
  rc->strong_count = 1;
  rc->destructor = destructor;
  return get_obj(rc);
}

void* rc_strong_retain(void* obj) {
  Rc* rc = get_rc(obj);
  assert(rc->strong_count != 0);
  rc->strong_count += 1;
  return get_obj(rc);
}

void rc_strong_release(void** obj) {
  Rc* rc = get_rc(*obj);
  assert(rc->strong_count != 0);
  if (--rc->strong_count) {
    *obj = NULL;
    return;
  }

  if (rc->destructor) {
    (rc->destructor)(*obj);
  }

  if (!rc->weak_count) {
    free(rc);
  }

  *obj = NULL;
}

size_t rc_strong_count(void* obj) { return get_rc(obj)->strong_count; }

void* rc_weak_retain(void* obj) {
  Rc* rc = get_rc(obj);
  assert(rc->strong_count != 0);
  rc->weak_count += 1;
  return get_obj(rc);
}

void rc_weak_release(void** obj) {
  Rc* rc = get_rc(*obj);
  assert(rc->weak_count != 0);
  *obj = NULL;
  if (--rc->weak_count) {
    return;
  }

  if (!rc->weak_count && !rc->strong_count) {
    free(rc);
  }
}

size_t rc_weak_count(void* obj) { return get_rc(obj)->weak_count; }

void* rc_weak_check(void** obj) {
  Rc* rc = get_rc(*obj);

  assert(rc->weak_count != 0);

  if (!rc->strong_count) {
    rc_weak_release(obj);
  }

  return *obj;
}

void* rc_upgrade(void** obj) {
  Rc* rc = get_rc(*obj);
  assert(rc->weak_count != 0);

  rc->weak_count -= 1;

  if (!rc->strong_count) {
    *obj = NULL;

    if (!rc->weak_count) {
      free(rc);
    }

    return NULL;
  }

  rc->strong_count += 1;

  return get_obj(rc);
}

void* rc_downcast(void** obj) {
  Rc* rc = get_rc(*obj);
  assert(rc->strong_count != 0);

  rc->strong_count -= 1;
  if (!rc->strong_count) {
    if (rc->destructor) {
      (rc->destructor)(*obj);
    }

    *obj = NULL;

    if (!rc->weak_count) {
      free(rc);
    }

    return NULL;
  }

  rc->weak_count += 1;

  return get_obj(rc);
}
