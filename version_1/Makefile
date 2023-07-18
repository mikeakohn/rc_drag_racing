PROGRAM1=drag_race
PROGRAM2=drag_race_end

default: $(PROGRAM1).hex $(PROGRAM2).hex

$(PROGRAM1).hex: $(PROGRAM1).asm
	naken_asm -I/usbdisk/devkits/atmel -o $(PROGRAM1).hex $(PROGRAM1).asm

$(PROGRAM2).hex: $(PROGRAM2).asm
	naken_asm -I/usbdisk/devkits/atmel -o $(PROGRAM2).hex $(PROGRAM2).asm

install: $(PROGRAM1).hex $(PROGRAM2).hex
	cp $(PROGRAM1).hex /storage/samba/hexfiles/
	cp $(PROGRAM2).hex /storage/samba/hexfiles/

clean:
	@rm -f *.hex
	@rm -f *.lst
	@echo "Clean!"


