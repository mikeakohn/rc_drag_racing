
TRACK_START=track_start
TRACK_END=track_end
INCLUDES=../../naken_asm/include/8051

default:
	naken_asm -l -I $(INCLUDES) -o $(TRACK_START).hex $(TRACK_START).asm
	@echo "Pololu Wixel Application - www.pololu.com" > $(TRACK_START).wxl
	@echo "1.0" >> $(TRACK_START).wxl
	@echo "====== cdb" >> $(TRACK_START).wxl
	@echo "====== hex" >> $(TRACK_START).wxl
	@cat $(TRACK_START).hex >> $(TRACK_START).wxl
	naken_asm -l -I $(INCLUDES) -o $(TRACK_END).hex $(TRACK_END).asm
	@echo "Pololu Wixel Application - www.pololu.com" > $(TRACK_END).wxl
	@echo "1.0" >> $(TRACK_END).wxl
	@echo "====== cdb" >> $(TRACK_END).wxl
	@echo "====== hex" >> $(TRACK_END).wxl
	@cat $(TRACK_END).hex >> $(TRACK_END).wxl

clean:
	@rm -f *.wxl *.hex *.lst

