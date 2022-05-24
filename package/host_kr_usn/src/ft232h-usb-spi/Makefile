
KDIR ?= /lib/modules/$(shell uname -r)/build
MDIR ?= $(PWD)/drivers/spi

include $(KDIR)/.config

all: modules

modules:
	@$(MAKE) -C $(KDIR) M=$(MDIR) CONFIG_SPI_FT232H=m modules

modules_install:
	@$(MAKE) -C $(KDIR) M=$(MDIR) modules_install

debug:
	@$(MAKE) -C $(KDIR) M=$(MDIR) CONFIG_SPI_FT232H=m CONFIG_SPI_FT232H_DEBUG=y modules

clean:
	@$(MAKE) -C $(KDIR) M=$(MDIR) clean

