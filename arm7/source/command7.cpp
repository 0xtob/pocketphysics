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
  Functions for the ARM7 to process the commands from the ARM9.Based
  on code from the MOD player example posted to the GBADEV forums.
  Chris Double (chris.double@double.co.nz)
  http://www.double.co.nz/nintendo_ds
*/
#include <nds.h>
#include <stdarg.h>

#include "../../generic/command.h"
#include "player.h"
#include "linear_freq_table.h"
extern "C" {
  #include "xtoa.h"
  #include "tobmic.h"
}

extern Player *player;
extern bool recording;

static void RecvCommandPlaySample(PlaySampleCommand *ps)
{
	player->playSample(ps->sample, ps->note, ps->volume, ps->channel);
}

static void RecvCommandStopSample(StopSampleSoundCommand* ss) {
	player->stopChannel(ss->channel);
}

static void RecvCommandMicOn(void)
{
	tob_MIC_On();
}

static void RecvCommandMicOff(void)
{
	tob_MIC_Off();
}

static void RecvCommandStartRecording(StartRecordingCommand* sr)
{
	recording = true;
	tob_StartRecording(sr->buffer, sr->length);
	commandControl->return_data = 0;
}

static void RecvCommandStopRecording()
{
	commandControl->return_data = tob_StopRecording();
	recording = false;
}

static void RecvCommandSetSong(SetSongCommand *c) {
	player->setSong((Song*)c->ptr);
}

static void RecvCommandStartPlay(StartPlayCommand *c) {
	player->play(c->potpos, c->row);
}

static void RecvCommandStopPlay(StopPlayCommand *c) {
	player->stop();
}

static void RecvCommandPlayInst(PlayInstCommand *c) {
	player->playNote(c->note, c->volume, c->channel, c->inst);
}

static void RecvCommandStopInst(StopInstCommand *c) {
	player->stopChannel(c->channel);
}

static void RecvCommandPatternLoop(PatternLoopCommand *c) {
	player->setPatternLoop(c->state);
}

void CommandDbgOut(const char *formatstr, ...)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM9;
	command->commandType = DBG_OUT;
	
	DbgOutCommand *cmd = &command->dbgOut;
	
	va_list marker;
	va_start(marker, formatstr);
	
	char *debugstr = cmd->msg;
	
	for(u16 i=0;i<DEBUGSTRSIZE; ++i)
		debugstr[i] = 0;
	
	u16 strpos = 0;
	char *outptr = debugstr;
	char c;
	while((strpos < DEBUGSTRSIZE-1)&&(formatstr[strpos]!=0))
	{
		c=formatstr[strpos];
		
		if(c!='%') {
			*outptr = c;
			outptr++;
		} else {
			strpos++;
			c=formatstr[strpos];
			if(c=='d') {
				long l = va_arg(marker, long);
				outptr = ltoa(l, outptr, 10);
			} else if(c=='u'){
				unsigned long ul = va_arg(marker, unsigned long);
				outptr = ultoa(ul, outptr, 10);
			}
		}
		
		strpos++;
	}
	
	va_end(marker);
	
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandUpdateRow(u16 row)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM9;
	command->commandType = UPDATE_ROW;
	
	UpdateRowCommand *c = &command->updateRow;
	c->row = row;
	
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandUpdatePotPos(u16 potpos)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM9;
	command->commandType = UPDATE_POTPOS;
	
	UpdatePotPosCommand *c = &command->updatePotPos;
	c->potpos = potpos;
	
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandNotifyStop(void)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM9;
	command->commandType = NOTIFY_STOP;
	
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandSampleFinish(void)
{
	Command* command = &commandControl->command[commandControl->currentCommand];
	command->destination = DST_ARM9;
	command->commandType = SAMPLE_FINISH;
	
	commandControl->currentCommand++;
	commandControl->currentCommand %= MAX_COMMANDS;
}

void CommandProcessCommands(void)
{
	static int currentCommand = 0;
	while(currentCommand != commandControl->currentCommand) {
		Command* command = &commandControl->command[currentCommand];
		
		if(command->destination == DST_ARM7) {
		
			switch(command->commandType) {
				case PLAY_SAMPLE:
					RecvCommandPlaySample(&command->playSample);
					break;
				case STOP_SAMPLE:
					RecvCommandStopSample(&command->stopSample);
					break;
				case START_RECORDING:
					RecvCommandStartRecording(&command->startRecording);
					break;
				case STOP_RECORDING:
					RecvCommandStopRecording();
					break;
				case SET_SONG:
					RecvCommandSetSong(&command->setSong);
					break;
				case START_PLAY:
					RecvCommandStartPlay(&command->startPlay);
					break;
				case STOP_PLAY:
					RecvCommandStopPlay(&command->stopPlay);
					break;
				case PLAY_INST:
					RecvCommandPlayInst(&command->playInst);
					break;
				case STOP_INST:
					RecvCommandStopInst(&command->stopInst);
					break;
				case MIC_ON:
					RecvCommandMicOn();
					break;
				case MIC_OFF:
					RecvCommandMicOff();
					break;
				case PATTERN_LOOP:
					RecvCommandPatternLoop(&command->ptnLoop);
					break;
				default:
					break;
			}
		
		}
		currentCommand++;
		currentCommand %= MAX_COMMANDS;
	}
}
