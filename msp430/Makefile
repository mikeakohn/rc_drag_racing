
PROGRAM=track_end
INCLUDE=/storage/git/naken_asm/include

default: track_start.hex track_end.hex

%.hex: %.asm
	naken_asm -l -o $*.hex -I$(INCLUDE) $*.asm

clean:
	@rm -f *.hex *.lst
	@echo "Clean!"

