# Native Nintendo 3DS homebrew build using devkitPro's supported rules.
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. Install devkitPro first.")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET   := excitebike3ds
BUILD    := build
SOURCES  := source/app source/frontend source/engine source/graphics source/audio source/assets source/platform
DATA     :=
INCLUDES := include source source/app source/frontend source/engine source/graphics source/audio source/assets source/platform

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -Wextra -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH)
CFLAGS += $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(OUTPUT).map
LIBS := -lcitro3d -lctru -lm
LIBDIRS := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(BUILD)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

# Build only real module files. source/*.c contains compatibility symlinks
# for host tests and is deliberately excluded to avoid duplicate objects.
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
PICAFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES_BIN := $(addsuffix .o,$(BINFILES)) $(PICAFILES:.v.pica=.shbin.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(PICAFILES:.v.pica=_shbin.h)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean
all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh

else
OUTPUT := $(TOPDIR)/$(BUILD)/$(TARGET)
.PHONY: all
all: $(OUTPUT).elf $(OUTPUT).3dsx

$(OFILES_SOURCES): $(HFILES)
$(OUTPUT).elf: $(OFILES)

# PICA200 shader is compiled from the graphics module.
.PRECIOUS: %.shbin
%.shbin: %.v.pica
	@echo $(notdir $<)
	@picasso -o $@ $<

%.shbin.o %_shbin.h: %.shbin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPSDIR)/*.d
endif
