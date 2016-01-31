#*********************************************************************************************************
# bsploongson1b Makefile
# target -> bsploongson1b.elf
#           bsploongson1b.bin
#*********************************************************************************************************

#*********************************************************************************************************
# include config.mk
#*********************************************************************************************************
CONFIG_MK_EXIST = $(shell if [ -f ../config.mk ]; then echo exist; else echo notexist; fi;)
ifeq ($(CONFIG_MK_EXIST), exist)
include ../config.mk
else
CONFIG_MK_EXIST = $(shell if [ -f config.mk ]; then echo exist; else echo notexist; fi;)
ifeq ($(CONFIG_MK_EXIST), exist)
include config.mk
else
CONFIG_MK_EXIST =
endif
endif

#*********************************************************************************************************
# check configure
#*********************************************************************************************************
check_defined = \
    $(foreach 1,$1,$(__check_defined))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $(value 2), ($(strip $2)))))

$(call check_defined, CONFIG_MK_EXIST, Please configure this project in RealEvo-IDE or \
create a config.mk file!)
$(call check_defined, SYLIXOS_BASE_PATH, SylixOS base project path)
$(call check_defined, TOOLCHAIN_PREFIX, the prefix name of toolchain)
$(call check_defined, DEBUG_LEVEL, debug level(debug or release))

#*********************************************************************************************************
# toolchain select
#*********************************************************************************************************
CC  = $(TOOLCHAIN_PREFIX)gcc
CXX = $(TOOLCHAIN_PREFIX)g++
AS  = $(TOOLCHAIN_PREFIX)gcc
AR  = $(TOOLCHAIN_PREFIX)ar
LD  = $(TOOLCHAIN_PREFIX)g++
OC  = $(TOOLCHAIN_PREFIX)objcopy
SZ  = $(TOOLCHAIN_PREFIX)size

#*********************************************************************************************************
# symbol.c symbol.h path
#*********************************************************************************************************
SYMBOL_PATH = SylixOS/bsp

#*********************************************************************************************************
# do not change the following code
# buildin internal application source
#*********************************************************************************************************

#*********************************************************************************************************
# symbol.c
#*********************************************************************************************************
SYM_SRCS = $(SYMBOL_PATH)/symbol.c

#*********************************************************************************************************
# bsp src(s) file
#*********************************************************************************************************
BSP_SRCS = \
SylixOS/bsp/startup.S \
SylixOS/bsp/bspInit.c \
SylixOS/bsp/bspLib.c

#*********************************************************************************************************
# drivers src(s) file
#*********************************************************************************************************
DRV_SRCS = \
SylixOS/driver/clock/ls1x_clock.c \
SylixOS/driver/int/ls1b_int.c \
SylixOS/driver/8250/8250_uart.c \
SylixOS/driver/16c550/16c550_sio.c \
SylixOS/driver/16c550/ls1b_16c550_sio.c \
SylixOS/driver/mtd/ls1x_nand.c \
SylixOS/driver/mtd/k9f1g08.c \
SylixOS/driver/designware_eth/designware_eth.c \
SylixOS/driver/rtc/ls1x_rtc.c \
SylixOS/driver/gpio/ls1x_gpio.c \
SylixOS/driver/lcd/ls1x_lcd.c \
SylixOS/driver/watchdog/ls1x_wdt.c \
SylixOS/driver/i2c/ls1x_i2c.c \
SylixOS/driver/spi/ls1x_spi.c \
SylixOS/driver/touch/ls1x_touch.c \
SylixOS/driver/led/ls1x_led.c \
SylixOS/driver/sdi/spi_sdi.c \
SylixOS/driver/key/ls1x_key.c

#*********************************************************************************************************
# user src(s) file
#*********************************************************************************************************
USR_SRCS = \
SylixOS/user/main.c

#*********************************************************************************************************
# all bsploongson1b source
#*********************************************************************************************************
SRCS  = $(BSP_SRCS)
SRCS += $(DRV_SRCS)
SRCS += $(USR_SRCS)
SRCS += $(SYM_SRCS)

#*********************************************************************************************************
# build path
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
OUTDIR = Debug
else
OUTDIR = Release
endif

OUTPATH = ./$(OUTDIR)
OBJPATH = $(OUTPATH)/obj
DEPPATH = $(OUTPATH)/dep

#*********************************************************************************************************
# target
#*********************************************************************************************************
O_IMG = $(OUTPATH)/bsploongson1b.elf
O_BIN = $(OUTPATH)/bsploongson1b.bin
O_SIZ = $(OUTPATH)/bsploongson1b.siz

#*********************************************************************************************************
# bsploongson1b objects
#*********************************************************************************************************
OBJS = $(addprefix $(OBJPATH)/, $(addsuffix .o, $(basename $(SRCS))))
DEPS = $(addprefix $(DEPPATH)/, $(addsuffix .d, $(basename $(SRCS))))

#*********************************************************************************************************
# include path
#*********************************************************************************************************
INCDIR  = -I"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS"
INCDIR += -I"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS/include"
INCDIR += -I"$(SYLIXOS_BASE_PATH)/libsylixos/SylixOS/include/inet"

INCDIR += -I"./SylixOS"
INCDIR += -I"./SylixOS/bsp"

#*********************************************************************************************************
# compiler preprocess
#*********************************************************************************************************
DSYMBOL  = -DSYLIXOS
DSYMBOL += -D__BOOT_INRAM=1

#*********************************************************************************************************
# load script
#*********************************************************************************************************
LD_SCRIPT = SylixOSBSP.ld

#*********************************************************************************************************
# depend dynamic library
#*********************************************************************************************************
DEPEND_DLL = -lsylixos

#*********************************************************************************************************
# depend dynamic library search path
#*********************************************************************************************************
DEPEND_DLL_PATH = -L"$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)"

#*********************************************************************************************************
# compiler optimize
#*********************************************************************************************************
ifeq ($(DEBUG_LEVEL), debug)
OPTIMIZE = -O0 -g3 -gdwarf-2
else
OPTIMIZE = -O2 -g1 -gdwarf-2											# Do NOT use -O3 and -Os
endif										    						# -Os is not align for function
																		# loop and jump.
#*********************************************************************************************************
# depends and compiler parameter (cplusplus in kernel MUST NOT use exceptions and rtti)
#*********************************************************************************************************
DEPENDFLAG  = -MM
CXX_EXCEPT  = -fno-exceptions -fno-rtti
COMMONFLAGS = $(CPUFLAGS) $(OPTIMIZE) -Wall -fmessage-length=0 -fsigned-char -fno-short-enums
ASFLAGS     = -x assembler-with-cpp $(DSYMBOL) $(INCDIR) $(COMMONFLAGS) -c
CFLAGS      = $(DSYMBOL) $(INCDIR) $(COMMONFLAGS) -c
CXXFLAGS    = $(DSYMBOL) $(INCDIR) $(CXX_EXCEPT) $(COMMONFLAGS) -c
ARFLAGS     = -r

#*********************************************************************************************************
# define some useful variable
#*********************************************************************************************************
DEPEND          = $(CC)  $(DEPENDFLAG) $(CFLAGS)
DEPEND.d        = $(subst -g ,,$(DEPEND))
COMPILE.S       = $(AS)  $(ASFLAGS)
COMPILE_VFP.S   = $(AS)  $(ASFLAGS) $(FPUFLAGS)
COMPILE.c       = $(CC)  $(CFLAGS)
COMPILE.cxx     = $(CXX) $(CXXFLAGS)

#*********************************************************************************************************
# target
#*********************************************************************************************************
all: $(O_IMG)
		@echo create "$(O_IMG) $(O_BIN)" success.

#*********************************************************************************************************
# include depends
#*********************************************************************************************************
ifneq ($(MAKECMDGOALS), clean)
ifneq ($(MAKECMDGOALS), clean_project)
sinclude $(DEPS)
endif
endif

#*********************************************************************************************************
# auto copy symbol.c symbol.h
#*********************************************************************************************************
EMPTY=
SPACE= $(EMPTY) $(EMPTY)
$(SYMBOL_PATH)/symbol.c:$(subst $(SPACE),\ ,$(SYLIXOS_BASE_PATH))/libsylixos/$(OUTDIR)/symbol.c
		cp "$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.c" $(SYMBOL_PATH)/symbol.c
		cp "$(SYLIXOS_BASE_PATH)/libsylixos/$(OUTDIR)/symbol.h" $(SYMBOL_PATH)/symbol.h

#*********************************************************************************************************
# create depends files
#*********************************************************************************************************
$(DEPPATH)/%.d: %.c
		@echo creating $@
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		@rm -f $@; \
		echo -n '$@ $(addprefix $(OBJPATH)/, $(dir $<))' > $@; \
		$(DEPEND.d) $< >> $@ || rm -f $@; exit;

$(DEPPATH)/%.d: %.cpp
		@echo creating $@
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		@rm -f $@; \
		echo -n '$@ $(addprefix $(OBJPATH)/, $(dir $<))' > $@; \
		$(DEPEND.d) $< >> $@ || rm -f $@; exit;

#*********************************************************************************************************
# compile source files
#*********************************************************************************************************
$(OBJPATH)/%.o: %.S
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		$(COMPILE.S) $< -o $@

$(OBJPATH)/%.o: %.c
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		$(COMPILE.c) $< -o $@

$(OBJPATH)/%.o: %.cpp
		@if [ ! -d "$(dir $@)" ]; then mkdir -p "$(dir $@)"; fi
		$(COMPILE.cxx) $< -o $@

#*********************************************************************************************************
# link bsploongson1b.elf object files
#*********************************************************************************************************
$(O_IMG): $(OBJS) $(LD_SCRIPT)
		$(LD) $(CPUFLAGS) -nostdlib $(addprefix -T,$(LD_SCRIPT)) -o $(O_IMG) $(OBJS) \
		$(DEPEND_DLL_PATH) $(DEPEND_DLL) -lm -lgcc
		$(OC) -O binary $(O_IMG) $(O_BIN)
		$(SZ) --format=berkeley $(O_IMG) > $(O_SIZ)

#*********************************************************************************************************
# clean
#*********************************************************************************************************
.PHONY: clean
.PHONY: clean_project

#*********************************************************************************************************
# clean objects
#*********************************************************************************************************
clean:
		-rm -rf $(O_SIZ)
		-rm -rf $(O_IMG)
		-rm -rf $(O_BIN)
		-rm -rf $(SYMBOL_PATH)/symbol.c $(SYMBOL_PATH)/symbol.h
		-rm -rf $(OBJPATH)
		-rm -rf $(DEPPATH)

#*********************************************************************************************************
# clean project
#*********************************************************************************************************
clean_project:
		-rm -rf $(OUTPATH)

#*********************************************************************************************************
# END
#*********************************************************************************************************
