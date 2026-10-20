# Native Nintendo 3DS homebrew build using devkitARM and libctru.
ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM is not set. Install/configure devkitPro first.")
endif

TARGET      := excitebike3ds
BUILD       := build
SOURCES     := source
DATA        := assets
INCLUDES    := include

ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS      := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections $(ARCH)
CFLAGS      += $(INCLUDE) -D__3DS__
CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS        := -lctru -lm

# libctru's 3DS rules build the .elf and package it as a .3dsx.
include $(DEVKITARM)/3ds_rules
