#---------------------------------------------------------------------------------
# Soundbar Mode — Tesla / Ultrahand Overlay (.ovl) Makefile
#
# Требования: devkitPro (devkitA64), libnx ≥ 4.12, libtesla (header-only).
#
# Совместим с devkitA64 r30+: использует switch_rules (не switch.mk).
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=/opt/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# Метаданные
#---------------------------------------------------------------------------------
APP_TITLE    := Soundbar Mode
APP_AUTHOR   := soundbar-nx
APP_VERSION  := 1.0.0

#---------------------------------------------------------------------------------
# Пути
#---------------------------------------------------------------------------------
TARGET       := soundbar-mode
OUTDIR       := out
BUILD        := build
SOURCES      := source
INCLUDES     := include

# libtesla (header-only): submodule в lib/libtesla ИЛИ portlibs
LIBTESLA_DIR := $(CURDIR)/lib/libtesla
ifneq ($(wildcard $(LIBTESLA_DIR)/include/tesla.hpp),)
    TESLA_INCLUDE := $(LIBTESLA_DIR)/include
else
    TESLA_INCLUDE := $(PORTLIBS)/include
endif

#---------------------------------------------------------------------------------
# Архитектура
#---------------------------------------------------------------------------------
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

#---------------------------------------------------------------------------------
# Флаги компиляции
#---------------------------------------------------------------------------------
CFLAGS   := -g -Wall -Wextra -Wno-unused-parameter \
            -O2 -ffunction-sections -fdata-sections \
            $(ARCH) $(DEFINES)
CFLAGS   += $(INCLUDE) -D__SWITCH__

CXXFLAGS := $(CFLAGS) -std=c++20 -fno-exceptions

ASFLAGS  := -g $(ARCH)

LDFLAGS  := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) \
            -Wl,-Map,$(notdir $*.map) \
            -Wl,--gc-sections

#---------------------------------------------------------------------------------
# Библиотеки (libtesla — header-only, линкуем только libnx)
#---------------------------------------------------------------------------------
LIBS     := -lnx

LIBDIRS  := $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# Сборка
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES     := $(OFILES_SRC)

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD) \
                  -I$(TESLA_INCLUDE)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export OUTPUT := $(CURDIR)/$(OUTDIR)/$(TARGET)

.PHONY: all clean

all: $(BUILD)
	@mkdir -p $(OUTDIR)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD) $(OUTDIR)

#---------------------------------------------------------------------------------
else  # внутри BUILD-директории
#---------------------------------------------------------------------------------

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).ovl: $(OUTPUT).elf $(OUTPUT).nacp
	@echo "  OVL   $(notdir $@)"
	@elf2nro $< $@ --nacp=$(OUTPUT).nacp

$(OUTPUT).elf: $(OFILES)
	@echo "  LINK  $(notdir $@)"
	@$(CXX) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

$(OUTPUT).nacp:
	@echo "  NACP  $(notdir $@)"
	@nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@

%.o: %.cpp
	@echo "  CXX   $(notdir $<)"
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.c
	@echo "  CC    $(notdir $<)"
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.s
	@echo "  AS    $(notdir $<)"
	@$(AS) $(ASFLAGS) -c $< -o $@

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
