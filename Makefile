# To run "make", you can either set up environment variables via
#		source /opt/iot-devkit/1.7.2/environment-setup-i586-poky-linux
# or set up the following make variables
#

CC = i586-poky-linux-gcc
CROSS_COMPILE = i586-poky-linux-
KDIR:=/opt/iot-devkit/1.7.2/sysroots/i586-poky-linux/usr/src/kernel
export PATH:=/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin:/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux:$(PATH)

PWD:= $(shell pwd)

ARCH = x86
SROOT=/opt/iot-devkit/1.7.2/sysroots/i586-poky-linux
EXTRA_CFLAGS += -Wall

LDLIBS = -L$(SROOT)/usr/lib
CCFLAGS = -I$(SROOT)/usr/include/libnl3


APP = spi_tester

obj-m:= genl_drv.o

.PHONY:all
all: genl_ex genl_ex.ko

genl_ex.ko:
	make ARCH=x86 CROSS_COMPILE=$(CROSS_COMPILE) -Wall -C $(KDIR) M=$(PWD) modules

genl_ex:
	$(CC) -Wall -o $(APP) genl_ex.c $(CCFLAGS) -lnl-genl-3 -lnl-3 -lpthread

.PHONY:clean
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f *.o $(EXAMPLE) $(APP)

deploy:
	tar czf programs.tar.gz $(APP) $(EXAMPLE) genl_drv.ko
	scp programs.tar.gz root@10.0.1.100:/home/root
	ssh root@10.0.1.100 'tar xzf programs.tar.gz'
