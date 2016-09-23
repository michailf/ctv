AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
#CFLAGS = -mfloat-abi=hard -fno-short-enums -Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s
CFLAGS := -nostdlib \
-fno-pic -static \
-O2 -Wall -MD -ggdb -Werror -mfloat-abi=hard \
-Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s \
-I${HOME}/w \
-I${HOME}/b/rpi-root2/usr/include \
-I${HOME}/b/rpi-root2/usr/include/arm-linux-gnueabihf

LIBS=-L${HOME}/b/rpi-root2/usr/lib/arm-linux-gnueabihf

all: ctv-rpi libsvc.a

net.o : ~/w/common/net.c

libsvc.a: net.o
	${AR} -vsr $@ $< 

ctv-rpi: api.o crt0.o libsvc.a
	${CC} ${CFLAGS} api.o -L. -lsvc ${LIBS} -ljson-c -lc -lm -lgcc -lstdc++ -o $@

clean:
	rm -f *.o *.d test-rpi ctv-rpi

