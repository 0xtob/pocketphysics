#include <nds.h>
#include <stdlib.h>

#include <nds/bios.h>
#include <nds/arm7/touch.h>
#include <nds/arm7/clock.h>
#include <dswifi7.h>

#include "../../generic/command.h"

#include "player.h"

extern "C" {
  #include "linear_freq_table.h"
  #include "tobmic.h"
}

//#define WIFI

#define LID_BIT BIT(7)

#define abs(x)	((x)>=0?(x):-(x))

Player *player;
bool recording = false;

// Thanks to LiraNuna for this cool function
void PM_SetRegister(int reg, int control)
{
	SerialWaitBusy();
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER |SPI_BAUD_1MHz | SPI_CONTINUOUS;
	REG_SPIDATA = reg;
	SerialWaitBusy();
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER |SPI_BAUD_1MHz;
	REG_SPIDATA = control;
}

/*
//---------------------------------------------------------------------------------
void startSound(int sampleRate, const void* data, u32 bytes, u8 channel, u8 vol,  u8 pan, u8 format) {
//---------------------------------------------------------------------------------
	SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2 ;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_8BIT:SOUND_16BIT);
}


//---------------------------------------------------------------------------------
s32 getFreeSoundChannel() {
//---------------------------------------------------------------------------------
	int i;
	for (i=0; i<16; i++) {
		if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
	}
	return -1;
}
*/

int vcount;
touchPosition first,tempPos;

//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	if(recording == true)
		return;
	
	static int lastbut = -1;

	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0;

	but = REG_KEYXY;

	if (!( (but ^ lastbut) & (1<<6))) {

		tempPos = touchReadXY();

		if ( tempPos.x == 0 || tempPos.y == 0 ) {
			but |= (1 <<6);
			lastbut = but;
		} else {
			x = tempPos.x;
			y = tempPos.y;
			xpx = tempPos.px;
			ypx = tempPos.py;
			z1 = tempPos.z1;
			z2 = tempPos.z2;
		}

	} else {
		lastbut = but;
		but |= (1 <<6);
	}

	if ( vcount == 80 ) {
		first = tempPos;
	} else {
		if (	abs( xpx - first.px) > 10 || abs( ypx - first.py) > 10 ||
				(but & ( 1<<6)) ) {

			but |= (1 <<6);
			lastbut = but;

		} else {
			IPC->mailBusy = 1;
			IPC->touchX			= x;
			IPC->touchY			= y;
			IPC->touchXpx		= xpx;
			IPC->touchYpx		= ypx;
			IPC->touchZ1		= z1;
			IPC->touchZ2		= z2;
			IPC->mailBusy = 0;
		}
	}
	IPC->buttons		= but;
	vcount ^= (80 ^ 130);
	SetYtrigger(vcount);

	// Check if the lid has been closed.
	if(but & BIT(7)) {
	   // Save the current interrupt sate.
	   u32 ie_save = REG_IE;
	   // Turn the speaker down.
	   swiChangeSoundBias(0,0x400);
	   // Save current power state.
	   int power = readPowerManagement(PM_CONTROL_REG);
	   // Set sleep LED.
	   writePowerManagement(PM_CONTROL_REG, PM_LED_CONTROL(1));
	   // Register for the lid interrupt.
	   REG_IE = IRQ_LID;
	   
	   // Power down till we get our interrupt.
	   swiSleep(); //waits for PM (lid open) interrupt
	   
	   REG_IF = ~0;
	   // Restore the interrupt state.
	   REG_IE = ie_save;
	   // Restore power state.
	   writePowerManagement(PM_CONTROL_REG, power);
	   
	   // Turn the speaker up.
	   swiChangeSoundBias(1,0x400);
	} 
}

//---------------------------------------------------------------------------------
void VblankHandler(void) {
//---------------------------------------------------------------------------------
/*
	u32 i;


	//sound code  :)
	TransferSound *snd = IPC->soundData;
	IPC->soundData = 0;

	if (0 != snd) {

		for (i=0; i<snd->count; i++) {
			s32 chan = getFreeSoundChannel();

			if (chan >= 0) {
				startSound(snd->data[i].rate, snd->data[i].data, snd->data[i].len, chan, snd->data[i].vol, snd->data[i].pan, snd->data[i].format);
			}
		}
	}
	*/
	
	CommandProcessCommands();
}

// This is the callback for the timer
void playTimerHandler(void) {
	player->playTimerHandler();
}

void onTick(u16 row) {
	CommandUpdateRow(row);
}

void onPatternChange(u8 potpos) {
	CommandUpdatePotPos(potpos);
}

void onSampleFinish(void) {
	CommandSampleFinish();
}
#ifdef WIFI
// callback to allow wifi library to notify arm9
void arm7_synctoarm9() { // send fifo message
	REG_IPC_FIFO_TX = 0x87654321;
}
// interrupt handler to allow incoming notifications from arm9
void arm7_fifo() { // check incoming fifo messages
	u32 msg = REG_IPC_FIFO_RX;
	if(msg==0x87654321) Wifi_Sync();
}
#endif

//---------------------------------------------------------------------------------
int main(int argc, char ** argv) {
//---------------------------------------------------------------------------------

    irqInit();

	// Reset the clock if needed
	rtcReset();

	irqInit();
	irqSet(IRQ_VBLANK, VblankHandler);
	SetYtrigger(80);
	vcount = 80;
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqEnable(IRQ_VBLANK);
	irqEnable(IRQ_VCOUNT);

	
	irqSet(IRQ_TIMER1, tob_ProcessMicrophoneTimerIRQ);
	irqEnable(IRQ_TIMER1);
	
	irqSet(IRQ_TIMER0, playTimerHandler);

	player = new Player(playTimerHandler);
	player->registerRowCallback(onTick);
	player->registerPatternChangeCallback(onPatternChange);
	player->registerSampleFinishCallback(onSampleFinish);

	irqEnable(IRQ_TIMER0);
	
	//enable sound
	powerON(POWER_SOUND);
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);
	IPC->soundData = 0;

	// Turn upper screen off
	//PM_SetRegister(0, 0x05);
	
	// Keep the ARM7 idle
	while (1) swiWaitForVBlank();
}


