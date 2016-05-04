CC = arm-none-eabi-gcc
#CFLAGS = -mfloat-abi=hard -fno-short-enums -Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s
CFLAGS := -nostdlib \
-fno-pic -static  \
-O2 -Wall -MD -ggdb -Werror -mfloat-abi=hard \
-Wa,-march=armv6 -Wa,-mcpu=arm1176jzf-s \
-I${HOME}/w \
-I${HOME}/b/rpi-root/usr/include \
-I${HOME}/b/rpi-root/usr/include/arm-linux-gnueabihf \
-I${HOME}/b/rpi-root/usr/src/linux-headers-3.16.0-4-common/include/uapi \
-I${HOME}/b/rpi-root/linux-4.1.6/include

LIBS=-L${HOME}/b/rpi-root/usr/lib/arm-linux-gnueabihf/ -L${HOME}/b/rpi-root/usr/lib/gcc-cross/arm-linux-gnueabihf/5

all: test-rpi ctv-rpi

test-rpi: test.o crt0.o
	${CC} ${CFLAGS} test.o -o $@ -L.

ctv-rpi: api.o crt0.o
	${CC} ${CFLAGS} api.o ${LIBS} -ljson-c -lc -lm -lgcc -lstdc++ -o $@

clean:
	rm *.o

