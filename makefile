FILENAME = ServoBot
LIBEMB	= 1

${FILENAME}.elf: ${FILENAME}.c

ifdef LIBEMB
	msp430-gcc -mmcu=msp430g2553 -o ${FILENAME}.elf ${FILENAME}.c ./libconio.a ./libshell.a ./libserial.a
else
	msp430-gcc -mmcu=msp430g2553 -o ${FILENAME}.elf ${FILENAME}.c
endif

flash: ${FILENAME}.elf
	mspdebug rf2500 'prog ${FILENAME}.elf'

clean:
	rm -rf *.elf

erase:
	mspdebug rf2500 erase
