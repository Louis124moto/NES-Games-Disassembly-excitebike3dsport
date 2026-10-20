# Native Nintendo 3DS homebrew build using devkitARM and libctru.
ifeq ($(strip $(DEVKITPRO)),)
$(error "DEVKITPRO is not set. Install/configure devkitPro first.")
endif

include $(DEVKITPRO)/libctru/base_rules

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

# Include the devkitARM rules that produce .elf and .3dsx homebrew files.
include $(DEVKITPRO)/devkitARM/3ds_rules
