#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
export DEVKITARM=/home/tob/coding/dsdev/devkitpro/devkitARM
export DEVKITPRO=/home/tob/coding/dsdev/devkitpro

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	$(shell basename $(CURDIR))
export TOPDIR		:=	$(CURDIR)


#---------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#---------------------------------------------------------------------------------
export PATH		:=	$(DEVKITARM)/bin:$(PATH)

.PHONY: $(TARGET).arm7 $(TARGET).arm9

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(TARGET).ds.gba

$(TARGET).ds.gba	: $(TARGET).nds

$(TARGET).gba.nds: $(TARGET).nds
	cat ndsloader.bin $(TARGET).nds > $(TARGET).gba.nds

cp: all
	dlditool ~/coding/dsdev/tools/dldi/mpcf.dldi $(TARGET).nds
	cp $(TARGET).nds /media/GBAMP
	pumount /media/GBAMP

r4: all
	dlditool ~/coding/dsdev/tools/dldi/r4tf.dldi $(TARGET).nds
	cp $(TARGET).nds /media/R4
	pumount /media/R4

wmb: all
	sudo $(WMB) $(TARGET).nds $(WLANIF)

wmb2: all
	$(WMB2) $(TARGET).nds

dsftp: all
	echo  'put -f $(TARGET).nds\nquote boot $(TARGET).nds' | ncftp  -u tob -p tob 192.168.1.88

#---------------------------------------------------------------------------------
$(TARGET).nds	:	$(TARGET).arm7 $(TARGET).arm9
	ndstool -c $(TARGET).nds -7 $(TARGET).arm7 -9 $(TARGET).arm9 -b ppicon.bmp "Pocket Physics"

#---------------------------------------------------------------------------------
$(TARGET).arm7	: arm7/$(TARGET).elf
$(TARGET).arm9	: arm9/$(TARGET).elf

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7

#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	rm -f $(TARGET).ds.gba $(TARGET).nds $(TARGET).arm7 $(TARGET).arm9

emu: all
	~/bin/nocash.sh $(TARGET).nds

fcsr: all
	~/coding/dsdev/tools/fcsr/build.sh pp.img worlds
	padbin 512 $(TARGET).ds.gba
	cat $(TARGET).ds.gba pp.img > $(TARGET)_fcsr.ds.gba
	dlditool ~/coding/dsdev/tools/dldi/fcsr.dldi $(TARGET)_fcsr.ds.gba