obj-m += corehttpd.o

KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	# make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean seems not working
	@rm *mod*; rm *o; rm Module.symvers
