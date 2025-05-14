
PROGRAM=racing_tree

default: $(PROGRAM).hex

$(PROGRAM).hex: $(PROGRAM).asm
	naken_asm -l -I/storage/devkits/atmel -o $(PROGRAM).hex $(PROGRAM).asm

program:
	#avrdude -c stk500v2 -p t85 -P /dev/ttyUSB0 -U flash:w:$(PROGRAM).hex
	avrdude -c usbtiny -p t85 -b19200 -v -U flash:w:$(PROGRAM).hex

setfuse:
	avrdude -c usbtiny -p t85 -b19200 -v -U lfuse:w:0x61:m

install: $(PROGRAM).hex
	cp $(PROGRAM).hex /storage/samba/hexfiles/

clean:
	@rm -f *.hex
	@rm -f *.lst
	@echo "Clean!"


