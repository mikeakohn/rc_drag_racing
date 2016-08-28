
PROGRAM=track_end

default:
	naken_asm -l -o $(PROGRAM).hex $(PROGRAM).asm
	@echo "Pololu Wixel Application - www.pololu.com" > $(PROGRAM).wxl
	@echo "1.0" >> $(PROGRAM).wxl
	@echo "====== cdb" >> $(PROGRAM).wxl
	@echo "====== hex" >> $(PROGRAM).wxl
	@cat $(PROGRAM).hex >> $(PROGRAM).wxl

clean:
	@rm -f *.wxl *.hex *.lst

