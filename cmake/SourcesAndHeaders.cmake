# cmake-format: off
set(sources
    src/base.c
    src/bus.c
    src/memory.c
    src/mos6502.c
    src/rc.c
    src/reset_manager.c
    src/nes/ppu.c
)

set(headers
    include/b6502/base.h
    include/b6502/bus.h
    include/b6502/component.h
    include/b6502/memory.h
    include/b6502/mos6502.h
    include/b6502/rc.h
    include/b6502/reset_manager.h
    include/b6502/nes/ppu.h
)

set(standalone 
    src/main.c
)

set(test_sources
    src/main.c
    src/test_mos6502.c
    src/test_rc.c
) 
# cmake-format: on
