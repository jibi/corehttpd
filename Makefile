obj-m += corehttpd.o
corehttpd-objs := server.o parser.o

KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	@rm *mod*; rm *o; rm Module.symvers; rm .c*; rm -r .tmp_versions
