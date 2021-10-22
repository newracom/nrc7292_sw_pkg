# SPDX-License-Identifier: GPL-2.0
KDIR ?= /lib/modules/$(shell uname -r)/build
CHECKPATCH ?= $(KDIR)/scripts/checkpatch.pl

include $(KDIR)/.config

M_SRC = nrc

obj-m += $(M_SRC).o
$(M_SRC)-objs = \
		nrc-mac80211.o \
		nrc-trx.o \
		nrc-init.o \
		nrc-debug.o \
		hif.o \
		wim.o \
		nrc-hif-debug.o \
		nrc-hif-uart.o \
		nrc-hif-ssp.o \
		nrc-fw.o \
		nrc-netlink.o \
		nrc-ssp.o \
		nrc-hif-cspi.o \
		mac80211-ext.o \
		nrc-stats.o \
		nrc-pm.o \
		nrc-dump.o \
		nrc-hif-sdio.o \
		nrc-bd.o \
		nrc-recovery.o

ccflags-y := \
	-DDEBUG \
	-DBUILD_UMAC_DRIVER \
	-DCONFIG_NRC_HIF_CSPI \
	-DENABLE_DYNAMIC_PS
	#-DENABLE_HW_RESET
	#-DCONFIG_NRC_HIF_SDIO
	#-DCONFIG_NRC_HIF_DEBUG \
	#-DCONFIG_NRC_HIF_UART \
	#-DCONFIG_NRC_HIF_AH_UART \
	#-DCONFIG_NRC_HIF_SSP

ldflags-y += -T$(M)/nrc.lds


ifeq ($(TESTMODE), y)
ccflags-y += \
		-DTEST_SELF_GENERATION
endif

#ccflags-y += -Wall -Werror -g
ccflags-y += -Wall -Werror

all: modules

cleanall: clean all

modules:
	@echo $(NRCLINUXHOSTDIR)
	@$(MAKE) -C $(KDIR) M=$$PWD modules

modules_install:
	@$(MAKE) -C $(KDIR) M=$$PWD modules_install

install: modules
	@sudo insmod $(M_SRC).ko

test: modules
	@sudo insmod $(M_SRC).ko
	@sudo rmmod -f $(M_SRC)

doxyfile.inc:
	@echo INPUT         =  $(NRCLINUXHOSTDIR) > doxyfile.inc
	@echo EXTRACT_ALL   = yes >> doxyfile.inc
	@echo EXTRACT_STATIC = yes >> doxyfile.inc
	@echo INLINE_SOURCES = yes >> doxyfile.inc
	@echo HAVE_DOT = yes = yes >> doxyfile.inc

doxy: doxyfile.inc
	@doxygen doxyfile.inc

clean:
	@rm -f *.o
	@rm -f *.ko
	@rm -f .*.cmd
	@rm -f *.mod *.mod.c
	@rm -f modules.order
	@rm -f Module.symvers
	@rm -rf .tmp_versions
	@rm -rf html
	@rm -rf latex
	@rm -f doxyfile.inc

check: checkstyle

checkstyle:
	@$(CHECKPATCH) --root=$(KDIR) -file -terse --ignore=LINUX_VERSION_CODE -no-tree *.c *.h

checkstyle2:
	@if [ $$($(CHECKPATCH) --root=$(KDIR) -file --quiet -no-tree *.c *.h | awk '{if($$1 == "ERROR:") print $$1;}' | wc -l) -ne 0 ]; \
	then \
		echo >&2 "checkstyle2 error"; \
		false; \
	fi
