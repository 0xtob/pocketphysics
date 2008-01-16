// libNTXM - XM Player Library for the Nintendo DS
// Copyright (C) 2005-2007 Tobias Weyand (0xtob)
//                         me@nitrotracker.tobw.net
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


/*
  Structures and functions to allow the ARM9 to send commands to the
  ARM7. Based on code from the MOD player example posted to the GBADEV
  forums.
  Chris Double (chris.double@double.co.nz)
  http://www.double.co.nz/nintendo_ds
*/
#include <nds.h>
#include <string.h>
#include <stdio.h>

#include "../../generic/command.h"
#include "state.h"

extern void handlePotPosChangeFromSong(u16 newpotpos);
extern void handleRowChanged(void);
extern void handlePreviewSampleFinished(void);

void CommandInit() {
	memset(commandControl, 0, sizeof(CommandControl));
}

void CommandPlaySample(Sample *sample, u8 note, u8 volume, u8 channel)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	PlaySampleCommand* ps = &command->playSample;
	
	command->commandType = PLAY_SAMPLE;
	
	ps->sample = sample;
	ps->note = note;
	ps->volume = volume;
	ps->channel = channel;
	
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandStopSample(int channel)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	StopSampleSoundCommand *ss = &command->stopSample;

	command->commandType = STOP_SAMPLE; 
	ss->channel = channel;

	commandControl->currentCommand++;
	//commandControl->currentCommand &= MAX_COMMANDS-1;  // I don't know why dekutree did it like this. Didn't work too well.
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandStartRecording(u16* buffer, int length)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	StartRecordingCommand* sr = &command->startRecording;

	command->commandType = START_RECORDING; 
	sr->buffer = buffer;
	sr->length = length;

	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

int CommandStopRecording(void)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	command->commandType = STOP_RECORDING;
	commandControl->return_data = -1;
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
	while(commandControl->return_data == -1)
		swiDelay(1);
	return commandControl->return_data;
}

void CommandProcessCommands(void)
{
	static int currentCommand = 0;
	while(currentCommand != commandControl->currentCommand) {
		Command* command = &commandControl->command[currentCommand];
		
		if(command->destination == DST_ARM9) {
		
			switch(command->commandType)
			{
				case DBG_OUT:
					iprintf("%s", command->dbgOut.msg);
					break;
			
				default:
					break;
			}
		
		}
			
		currentCommand++;
		currentCommand %= MAX_COMMANDS;
	}
}

void CommandSetSong(void *song)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	SetSongCommand* c = &command->setSong;

	command->commandType = SET_SONG; 
	c->ptr = song;

	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandStartPlay(u8 potpos, u16 row)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	StartPlayCommand* c = &command->startPlay;

	command->commandType = START_PLAY; 
	c->potpos = potpos;
	c->row = row;

	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandStopPlay(void) {
	
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	command->commandType = STOP_PLAY; 

	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;	
}

void CommandPlayInst(u8 inst, u8 note, u8 volume, u8 channel)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	command->commandType = PLAY_INST; 
	
	PlayInstCommand* c = &command->playInst;

	c->inst    = inst;
	c->note    = note;
	c->volume  = volume;
	c->channel = channel;

	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;	
}

void CommandStopInst(u8 channel)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	command->commandType = STOP_INST; 
	
	StopInstCommand* c = &command->stopInst;
	
	c->channel = channel;
	
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;	
}

void CommandMicOn(void)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	command->commandType = MIC_ON; 

	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;	
}

void CommandMicOff(void)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	command->commandType = MIC_OFF;

	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;	
}

void CommandSetPatternLoop(bool state)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM7;
	command->commandType = PATTERN_LOOP;
	
	PatternLoopCommand* c = &command->ptnLoop;
	c->state = state;
	
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}
