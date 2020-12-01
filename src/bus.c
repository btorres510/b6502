#include "b6502/bus.h"

#include "b6502/component.h"
#include "b6502/rc.h"

void map_handler(Bus *bus, void *obj, uint16_t start, uint16_t end) {
  size_t page_start = start / NUMBER_OF_PAGES;
  size_t page_end = end / NUMBER_OF_PAGES;
  for (size_t i = page_start; i <= page_end; i++) {
    if (bus->handlers[i]) {
      rc_weak_release((void *)&bus->handlers[i]);
    }

    bus->handlers[i] = rc_weak_retain(obj);
  }
}

uint8_t read(Bus *bus, uint16_t addr) {
  size_t page = addr / NUMBER_OF_PAGES;
  Component *c = bus->handlers[page];
  if (LIKELY(c)) {
    return c->read(bus->handlers[page], addr);
  } else {
    return 0;
  }
}

void write(Bus *bus, uint16_t addr, uint8_t val) {
  size_t page = addr / NUMBER_OF_PAGES;
  Component *c = bus->handlers[page];
  if (c->write) {
    c->write(bus->handlers[page], addr, val);
  }
}
