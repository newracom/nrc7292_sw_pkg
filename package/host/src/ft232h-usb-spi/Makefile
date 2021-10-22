
KDIR ?= /lib/modules/$(shell uname -r)/build
MDIR ?= $(PWD)/drivers/spi

all:
	@$(MAKE) -C $(KDIR) M=$(MDIR) CONFIG_SPI_FT232H=m modules

debug:
	@$(MAKE) -C $(KDIR) M=$(MDIR) CONFIG_SPI_FT232H=m CONFIG_SPI_FT232H_DEBUG=y modules

clean:
	@$(MAKE) -C $(KDIR) M=$(MDIR) clean

