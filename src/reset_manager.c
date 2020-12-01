#include "b6502/reset_manager.h"

#include <assert.h>

#include "b6502/rc.h"

static void deinit(void* obj) {
  ResetManager* rm = obj;
  for (size_t i = 0; i < rm->num_devices; i++) {
    if (LIKELY(rm->devices[i].obj)) {
      rc_weak_release((void*)&rm->devices[i].obj);
    }
  }
}

ResetManager* reset_manager_create(void) {
  ResetManager* rm = rc_alloc(sizeof(*rm), deinit);
  return rm;
}

void add_rm_device(ResetManager* rm, void* obj, reset_handler reset) {
  assert(rm->num_devices < MAX_DEVICES);
  rm->devices[rm->num_devices].obj = rc_weak_retain(obj);
  rm->devices[rm->num_devices++].reset = reset;
}

void reset_devices(ResetManager* rm) {
  for (size_t i = 0; i < rm->num_devices; i++) {
    if (LIKELY(rm->devices[i].obj)) {
      void* obj = rm->devices[rm->num_devices].obj;
      reset_handler reset = rm->devices[rm->num_devices].reset;
      (reset)(obj);
    }
  }
}
