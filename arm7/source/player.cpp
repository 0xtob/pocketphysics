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

#include "player.h"

#include "song.h"
extern "C" {
  #include "demokit.h"
}
#include "tools.h"
#include "../../generic/command.h"
#include "xtoa.h"

#include <stdlib.h>

#define MIN(x,y)	((x)<(y)?(x):(y))

/* ===================== PUBLIC ===================== */

Player::Player(void (*_externalTimerHandler)(void))
	:externalTimerHandler(_externalTimerHandler)
{
	initState();
	initEffState();
	
	demoInit();
	
	// Init callbacks
	onRow = 0;
	onPatternChange = 0;
	onSampleFinish = 0;
	
	startPlayTimer();
}

void* Player::operator new (size_t size) {
 
	return malloc(size);
 
} // default ctor implicitly called here
 
void Player::operator delete (void *p) {
 
	if ( NULL != p ) free(p);
 
} // default dtor implicitly called here

void Player::setSong(Song *_song)
{
	song = _song;
	initState();
	
	// Init fading
	my_memset(state.channel_fade_active, 0, sizeof(state.channel_fade_active));
	my_memset(state.channel_fade_ms, 0, sizeof(state.channel_fade_ms));
	my_memset(state.channel_volume, 0, sizeof(state.channel_volume));
	my_memset(state.channel_fade_target_volume, 0, sizeof(state.channel_fade_target_volume));
}

// Set the current pattern to looping
void Player::setPatternLoop(bool loopstate) {
	state.patternloop = loopstate;
}

// Plays the song till the end starting at pattern order table position potpos and row row
void Player::play(u8 potpos, u16 row)
{
	// Mark all channels inactive
	if(state.playing == false) {
		my_memset(state.channel_active, 0, sizeof(state.channel_active));
		my_memset(state.channel_ms_left, 0, sizeof(state.channel_ms_left));
		my_memset(state.channel_loop, 0, sizeof(state.channel_loop));
	}
	
	state.potpos = potpos;
	state.row = row;
	state.pattern = song->pattern_order_table[state.potpos];
	
	// Reset ms and tick counter
	state.tick_ms = 0;
	state.row_ticks = 0;
	
	state.juststarted = true;
	
	lastms = getTicks();

	initEffState();
	
	//startPlayTimer();
	
	state.playing = true;
}

void Player::stop(void)
{
	// If stop was pressed although we were not playing,
	// playing samples shall be stopped. For this, the play
	// timer needs to be turned on, because it periodiacally
	// calls the fade handler
	/*
	if(state.playing == false) {
		startPlayTimer();
	}
	*/
	state.playing = false;
	
	// Stop all playing samples
	u8 end;
	if(song->n_channels < MAX_CHANNELS) {
		end = song->n_channels;
	} else {
		end = MAX_CHANNELS;
	}
	
	for(u8 chn = 0; chn < end; chn++)
	{
		state.channel_fade_active[chn] = 1;
		state.channel_fade_ms[chn] = FADE_OUT_MS;
		state.channel_fade_target_volume[chn] = 0;
	}
}

// Play the note with the given settings. channel == 255 -> search for free channel
void Player::playNote(u8 note, u8 volume, u8 channel, u8 instidx)
{
	if( (state.playing == true) && (song->channelMuted(channel) == true) )
		return;
	
	if(channel == 255) // Find a free channel
	{
		u8 c = 0;
		while( ( state.channel_active[c] == 1) && ( c < MAX_CHANNELS ) )
			++c;
		
		if( state.channel_active[c] == 1 )
			return;
		else
		{
			channel = c;
			state.last_autochannel = c;
		}
	}
	
	// Stop possibly active fades
	state.channel_fade_active[channel] = 0;
	state.channel_fade_ms[channel] = 0;
	state.channel_instrument[channel] = instidx;
	
	Instrument *inst = song->instruments[instidx];
	
	if(volume == NO_VOLUME) {
		state.channel_volume[channel] = MAX_VOLUME * inst->getSampleForNote(note)->getVolume() / 255;
	} else {
		state.channel_volume[channel] = volume * inst->getSampleForNote(note)->getVolume() / 255;
	}
	
	state.channel_fade_vol[channel] = state.channel_volume[channel];
	
	state.channel_note[channel]   = note;
	state.channel_active[channel] = 1;
	
	if(inst != 0)
	{
		//CommandDbgOut("a7: play %d %d %u\n",instidx, note, volume);
		
		inst->play(note, volume, channel);
	}
}

// Play the given sample (and send a notification when done)
void Player::playSample(Sample *sample, u8 note, u8 volume, u8 channel)
{
	// Stop playing sample if necessary
	if(state.playing_single_sample == true) {
		state.playing_single_sample = false;
		state.single_sample_ms_remaining = 0;
		CommandSampleFinish();
	}
	
	// Calculate length
	u32 length = sample->calcPlayLength(note);
	
	// Set sample playing state
	state.playing_single_sample = true;
	state.single_sample_ms_remaining = length;
	state.single_sample_channel = 0;
	state.single_sample_loop = (sample->getLoop() != 0);
	state.channel_active[channel] = 1;
	state.channel_ms_left[channel] = length;
	state.channel_volume[channel] = volume;
	state.channel_fade_vol[channel] = 127;
	state.channel_env_vol[channel] = 63;
	state.channel_loop[channel] = state.single_sample_loop;
	
	// Play
	sample->play(note, volume, channel);
}

// Stop playback on a channel
void Player::stopChannel(u8 channel)
{
	if(channel == 255) // Autochannel
		channel = state.last_autochannel;
	
	state.channel_fade_active[channel]        = 1;
	state.channel_fade_ms[channel]            = FADE_OUT_MS;
	state.channel_fade_target_volume[channel] = 0;
	//state.channel_note[channel]               = EMPTY_NOTE;
	//state.channel_instrument[channel]         = NO_INSTRUMENT;
	
	// Stop single sample if it's played on this channel
	if((state.playing_single_sample == true) && (state.single_sample_channel == channel))
	{
		state.playing_single_sample = 0;
		state.single_sample_ms_remaining = 0;
		CommandSampleFinish();
	}
}

void Player::registerRowCallback(void (*onRow_)(u16)) {
	onRow = onRow_;
}

void Player::registerPatternChangeCallback(void (*onPatternChange_)(u8)) {
	onPatternChange = onPatternChange_;
}

void Player::registerSampleFinishCallback(void (*onSampleFinish_)()) {
	onSampleFinish = onSampleFinish_;
}

void Player::playTimerHandler(void)
{
	u32 passed_time = getTicks() - lastms;
	lastms = getTicks();
	
	// Fading stuff
	handleFade(passed_time);
	
	// Are we playing a single sample (a sample not from the song)?
	// (Built in for games etc)
	
	if( (state.playing_single_sample) && (!state.single_sample_loop) )
	{
		// Count down, and send signal when done
		if(state.single_sample_ms_remaining < passed_time)
		{
			state.playing_single_sample = false;
			state.single_sample_ms_remaining = 0;
			state.single_sample_channel = 0;
			
			if(onSampleFinish != 0) {
				onSampleFinish();
			}
		}
		else
		{
			state.single_sample_ms_remaining -= passed_time;
		}
	}
	
	/*
	if(state.playing==false)
	{
		// If the song was stopped, just wait for the fades to finish
		
		bool fades_finished = true;
		for(u8 channel=0; channel<MAX_CHANNELS; ++channel) {
			if(state.channel_fade_active[channel] != 0) {
				fades_finished = false;
			}
		}
		
		return; // Don't process the stuff after this
	}
	*/
	
	if(state.playing)
	{
		// Update tick ms
		state.tick_ms += passed_time;
		
		// Check if we are shortly before the next tick. (As long as a fade would take)
		if(state.tick_ms >= song->getMsPerTick() - FADE_OUT_MS)
		{
			// Is there a request to set the volume?
			for(u8 channel=0; channel<song->n_channels && channel<MAX_CHANNELS; ++channel)
			{
				if(effstate.channel_setvol_requested[channel])
				{
					state.channel_fade_active[channel] = 1;
					state.channel_fade_ms[channel] = FADE_OUT_MS;
					effstate.channel_setvol_requested[channel] = false;
				}
			}
			
			// Is this the last tick before the next row?
			if(state.row_ticks >= song->getTempo()-1)
			{
				// If so, check if for any of the active channels a new note starts in the next row.
				u8 nextNote;
				for(u8 channel=0; channel<song->n_channels && channel<MAX_CHANNELS; ++channel)
				{
					if(state.channel_active[channel] == 1)
					{
						u16 nextrow;
						u8 nextpattern, nextpotpos;
						
						calcNextPos(&nextrow, &nextpotpos);
						nextpattern = song->pattern_order_table[nextpotpos];
						
						nextNote = song->patterns[nextpattern][channel][nextrow].note;
						if((nextNote!=EMPTY_NOTE) && (state.channel_fade_active[channel] == 0))
						{
							// If so, fade out to avoid a click.
							state.channel_fade_active[channel] = 1;
							state.channel_fade_ms[channel] = FADE_OUT_MS;
							state.channel_fade_target_volume[channel] = 0;
						}
					}
				}
			}
		}
	}
	
	// Update active channels
	for(u8 channel=0; channel<song->n_channels && channel<MAX_CHANNELS; ++channel)
	{
		if(state.channel_ms_left[channel] > 0)
		{
			if(state.channel_ms_left[channel] > passed_time) {
				state.channel_ms_left[channel] -= passed_time;
			} else {
				state.channel_ms_left[channel] = 0;
			}
			//state.channel_ms_left[channel]--; // WTF?
			if((state.channel_ms_left[channel]==0)&&(state.channel_loop[channel]==false)) {
				state.channel_active[channel] = 0;
			}
		}
	}
	
	// Update envelopes
	for(u8 channel=0; channel<song->n_channels && channel<MAX_CHANNELS; ++channel)
	{
		if((state.channel_active[channel])&&(!(state.playing_single_sample && (state.single_sample_channel==channel) )))
		{
			Instrument *inst = song->getInstrument(state.channel_instrument[channel]);
			inst->updateEnvelopePos(song->getBPM(), passed_time, channel);
			state.channel_env_vol[channel] = inst->getEnvelopeAmp(channel);
		}
	}
	
	// Update channel volumes
	for(u8 channel=0; channel<song->n_channels && channel<MAX_CHANNELS; ++channel)
	{
		if(state.channel_active[channel])
		{
			// The master formula!
			// FIXME: Use inst volume too!
			u8 chnvol = state.channel_env_vol[channel] * state.channel_fade_vol[channel] / 64;
			
			//CommandDbgOut("vol %d\n", chnvol);
			
			SCHANNEL_VOL(channel) = SOUND_VOL(chnvol);
			
			if(state.channel_active[channel] == CHANNEL_TO_BE_DISABLED)
				state.channel_active[channel] = 0;
		}
	}
	
	if(state.playing == false)
		return;
		
	if( state.juststarted == true ) // Play current row
	{
		state.juststarted = false;
		
		playRow();
		
		handleEffects();
		
		handleTickEffects();
		
		if(onRow != NULL) {
			onRow(state.row);
		}
	}
	
	// if the number of ms per tick is reached, go to the next tick
	if(state.tick_ms >= song->getMsPerTick())
	{
		// Go to the next tick
		state.row_ticks++;
		
		if(state.row_ticks >= song->getTempo())
		{
			state.row_ticks = 0;
			
			calcNextPos(&state.row, &state.potpos);
			state.pattern = song->pattern_order_table[state.potpos];
			/*
			state.row++;
			
			// Go to the next pattern if the end of current pattern is reached
			if(state.row >= song->patternlengths[state.pattern])
			{
				if(state.patternloop == false) // Don't jump when looping is enabled
				{
					if(state.potpos < song->getPotLength()-1) {
						state.potpos++;
						state.pattern = song->pattern_order_table[state.potpos];
					} else {
						state.potpos = song->getRestartPosition();
						state.pattern = song->pattern_order_table[state.potpos];
					}
				}
				state.row = 0;
			}
			*/
			if(state.waitrow == true) {
				stop();
				CommandNotifyStop();
				state.waitrow = false;
				return;
			}
			
			if(effstate.pattern_break_requested == true)
				onPatternChange(state.potpos);
			
			finishEffects();
			
			playRow();
			
			handleEffects();
			
			if(onRow != NULL) {
				onRow(state.row);
			}
			
			if( (state.row == 0) && (onPatternChange != 0) ) {
				onPatternChange(state.potpos);
			}
		}
		
		handleTickEffects();
		
		state.tick_ms -= song->getMsPerTick();
	}
}

/* ===================== PRIVATE ===================== */

void Player::startPlayTimer(void)
{
	TIMER0_DATA = TIMER_FREQ_64(1000); // Call handler every millisecond
	TIMER0_CR = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_64;
	//irqSet(IRQ_TIMER0, externalTimerHandler);
	//irqEnable(IRQ_TIMER0);
}

void Player::playRow(void)
{
	// Play all notes in this row
	for(u8 channel=0; channel < song->n_channels && channel<MAX_CHANNELS; ++channel)
	{
		u8 note   = song->patterns[state.pattern][channel][state.row].note;
		u8 volume = song->patterns[state.pattern][channel][state.row].volume;
		u8 inst   = song->patterns[state.pattern][channel][state.row].instrument;
	
		if((note!=EMPTY_NOTE)&&(note!=STOP_NOTE)&&(song->instruments[inst]!=0))
		{
			playNote(note, volume, channel, inst);
			
			state.channel_active[channel] = 1;
			if(song->instruments[inst]->getSampleForNote(note)->getLoop() != 0) {
				state.channel_loop[channel] = true;
				state.channel_ms_left[channel] = 0;
			} else {
				state.channel_loop[channel] = false;
				state.channel_ms_left[channel] = song->instruments[inst]->calcPlayLength(note);
			}
		}
	}
}

void Player::handleEffects(void)
{
	effstate.pattern_loop_jump_now = false;
	effstate.pattern_break_requested = false;
	
	for(u8 channel=0; channel < song->n_channels && channel<MAX_CHANNELS; ++channel)
	{
		u8 effect = song->patterns[state.pattern][channel][state.row].effect;
		u8 param  = song->patterns[state.pattern][channel][state.row].effect_param;
		
		if(effect != NO_EFFECT)
		{
			switch(effect)
			{
				case(EFFECT_E): // If the effect is E, the effect type is specified in the 1st param nibble
				{
					u8 e_effect_type  = (param >> 4);
					u8 e_effect_param = (param & 0x0F);
					
					switch(e_effect_type)
					{
						case(EFFECT_E_SET_LOOP):
						{
							// If param is 0, the loop start is set at the current row.
							// If param is >0, the loop end is set at the current row and
							// the effect param is the loop count
							if(e_effect_param == 0)
							{
								effstate.pattern_loop_begin = state.row;
							}
							else
							{
								if(effstate.pattern_loop_count > 0) // If we are already looping
								{
									effstate.pattern_loop_count--;
									if(effstate.pattern_loop_count == 0) {
										effstate.pattern_loop_begin = 0;
									}
								} else {
									effstate.pattern_loop_count = e_effect_param;
								}
								
								if(effstate.pattern_loop_count > 0)
								{
									effstate.pattern_loop_jump_now = true;
								}
							}
							break;
						}
					}
					
					break;
				}
				
				case EFFECT_SET_VOLUME:
				{
					u8 target_volume = MIN(MAX_VOLUME, param * 2);
					
					// Request volume chnge and set target volume
					effstate.channel_setvol_requested[channel] = true;
					state.channel_fade_target_volume[channel] = target_volume;
					
					// TODO: ARGH! Fade out dammit!
					//SCHANNEL_VOL(channel) = SOUND_VOL(target_volume);
					//state.channel_volume[channel] = target_volume;
					
					break;
				}
				
				case EFFECT_PATTERN_BREAK:
				{
					// The row at which the next pattern is continued
					// is calculated strangely:
					u8 b1, b2, newrow;
					b1 = param >> 4;
					b2 = param & 0x0F;
					
					newrow = b1 * 10 + b2;
					
					effstate.pattern_break_requested = true;
					effstate.pattern_break_row = newrow;
					
					
					break;
				}
				
			}
		}
	}
}

void Player::handleTickEffects(void)
{
	for(u8 channel=0; channel < song->n_channels && channel<MAX_CHANNELS; ++channel)
	{
		u8 effect  = song->patterns[state.pattern][channel][state.row].effect;
		u8 param   = song->patterns[state.pattern][channel][state.row].effect_param;
		u8 instidx = state.channel_instrument[channel];
		Instrument *inst = song->instruments[instidx];
		
		if(effect != NO_EFFECT)
		{
			state.channel_effect[channel] = effect;
			state.channel_effect_param[channel] = param;
			
			switch(effect)
			{
				case(EFFECT_ARPEGGIO):
				{
					u8 halftone1, halftone2;
					halftone2 = (param & 0xF0) >> 4;
					halftone1 = param & 0x0F;
					
					if (inst == 0)
						continue;
					
					switch(state.row_ticks % 3)
					{
						case(0):
							inst->bendNote(state.channel_note[channel] + 0,
									state.channel_note[channel], 0, channel);
							break;
						case(1):
							inst->bendNote(state.channel_note[channel] + halftone1,
									state.channel_note[channel], 0, channel);
							break;
						case(2):
							inst->bendNote(state.channel_note[channel] + halftone2,
									state.channel_note[channel], 0, channel);
							break;
					}
					
					break;
				}
				
				case(EFFECT_E): // If the effect is E, the effect type is specified in the 1st param nibble
				{
					u8 e_effect_type  = (param >> 4);
					u8 e_effect_param = (param & 0x0F);
					
					switch(e_effect_type)
					{
						case(EFFECT_E_NOTE_CUT):
						{
							if(e_effect_param == state.row_ticks)
							{
								effstate.channel_setvol_requested[channel] = true;
								state.channel_fade_target_volume[channel] = 0;
								//SCHANNEL_VOL(channel) = SOUND_VOL(0);
								//state.channel_volume[channel] = 0;
							}
							break;
						}
					}
					break;
				}
				
				case(EFFECT_VOLUME_SLIDE):
				{
					if(state.row_ticks == 0) //
						break;
					
					s8 slidespeed;
					
					if(param == 0)
						slidespeed = effstate.channel_last_slidespeed[channel];
					else if( (param & 0x0F) == 0 )
						slidespeed = (param >> 4) * 2;
					else
						slidespeed = -(param & 0x0F) * 2;
					
					effstate.channel_last_slidespeed[channel] = slidespeed;
					
					s16 targetvolume = state.channel_volume[channel] + slidespeed;
					if(targetvolume > MAX_VOLUME)
						targetvolume = MAX_VOLUME;
					else if(targetvolume < 0)
						targetvolume = 0;
					
					effstate.channel_setvol_requested[channel] = true;
					state.channel_fade_target_volume[channel] = targetvolume;
					
					break;
				}
				
			}
		}
	}
}

void Player::finishEffects(void)
{
	for(u8 channel = 0; channel < song->n_channels && channel<MAX_CHANNELS; ++channel)
	{
		u8 effect = state.channel_effect[channel];
		u8 new_effect = song->patterns[state.pattern][channel][state.row].effect;
		//u8 param = state.channel_effect_param[channel];
		u8 instidx = state.channel_instrument[channel];
		Instrument *inst = song->instruments[instidx];
		
		if( (effect != NO_EFFECT) && (new_effect != effect) )
		{
			switch(effect)
			{
				case(EFFECT_ARPEGGIO):
				{
					if (inst == 0)
						continue;
					
					// Reset note
					inst->bendNote(state.channel_note[channel] + 0,
									state.channel_note[channel], 0, channel);
					
					break;
				}
			}
		}
	}
}

void Player::initState(void)
{
	state.row = 0;
	state.pattern = 0;
	state.potpos = 0;
	state.playing = false;
	state.waitrow = false;
	state.patternloop = false;
	my_memset(state.channel_active, 0, sizeof(state.channel_active));
	my_memset(state.channel_ms_left, 0, sizeof(state.channel_ms_left));
	my_memset(state.channel_loop, 0, sizeof(state.channel_loop));
	my_memset(state.channel_note, EMPTY_NOTE, sizeof(state.channel_note));
	my_memset(state.channel_instrument, NO_INSTRUMENT, sizeof(state.channel_instrument));
	my_memset(state.channel_effect, NO_EFFECT, sizeof(state.channel_effect));
	my_memset(state.channel_effect_param, NO_EFFECT_PARAM, sizeof(state.channel_effect_param));
	my_memset(state.channel_fade_active, 0, sizeof(state.channel_fade_active));
	my_memset(state.channel_fade_ms, 0, sizeof(state.channel_fade_ms));
	my_memset(state.channel_fade_target_volume, 0, sizeof(state.channel_fade_target_volume));
	my_memset(state.channel_volume, 0, sizeof(state.channel_volume));
	my_memset(state.channel_env_vol, 63, sizeof(state.channel_env_vol));
	my_memset(state.channel_fade_vol, 127, sizeof(state.channel_fade_vol));
	state.playing_single_sample = false;
	state.single_sample_ms_remaining = 0;
	state.single_sample_channel = 0;
	state.single_sample_loop = false;
}

void Player::initEffState(void)
{
	effstate.pattern_loop_begin = 0;
	effstate.pattern_loop_count = 0;
	effstate.pattern_loop_jump_now = false;
	my_memset(effstate.channel_setvol_requested, false, sizeof(effstate.channel_setvol_requested));
	my_memset(effstate.channel_last_slidespeed, 0, sizeof(effstate.channel_last_slidespeed));
	effstate.pattern_break_requested = false;
	effstate.pattern_break_row = 0;
}

void Player::handleFade(u32 passed_time)
{
	// Find channels that need to be faded
	for(u8 channel=0; channel<MAX_CHANNELS; ++channel)
	{
		if(state.channel_fade_active[channel] == 1)
		{
			// Decrement ms until fade is complete
			if(state.channel_fade_ms[channel] > passed_time)
				state.channel_fade_ms[channel] -= passed_time;
			else
				state.channel_fade_ms[channel] = 0;
			
			// Calculate volume from initial volume, target volume and remaining fade time
			// Can be done way quicker using bresenham
			float fslope = (float)(state.channel_volume[channel] - state.channel_fade_target_volume[channel])
					/ (float)FADE_OUT_MS;
			
			float fvolume = (float)state.channel_fade_target_volume[channel]
					+ fslope * (float)(state.channel_fade_ms[channel]);
			
			u8 volume = (u8)fvolume;
			
			//u8 volume = state.channel_volume[channel] * 
			//	    state.channel_fade_ms[channel] / FADE_OUT_MS ;
			
			state.channel_fade_vol[channel] = volume;
			
			//CommandDbgOut("fade: %u\n", volume);
			
			// If we reached 0 ms, disable the fader (and the channel)
			if(state.channel_fade_ms[channel] == 0)
			{
				state.channel_fade_active[channel] = 0;
				//SCHANNEL_CR(channel) = 0;
				
				state.channel_volume[channel] = state.channel_fade_target_volume[channel];
				
				// Set channel volume to target volume just to be sure
				//SCHANNEL_VOL(channel) = state.channel_fade_target_volume[channel];
				state.channel_fade_vol[channel] = state.channel_fade_target_volume[channel];
								
				if(state.channel_volume[channel] == 0)
					state.channel_active[channel] = CHANNEL_TO_BE_DISABLED;
			}
		}
	}
}

void Player::calcNextPos(u16 *nextrow, u8 *nextpotpos) // Calculate next row and pot position
{
	if(effstate.pattern_loop_jump_now == true)
	{
		*nextrow = effstate.pattern_loop_begin;
		*nextpotpos = state.potpos;
		
		return;
	}
	
	if(effstate.pattern_break_requested == true)
	{
		*nextrow = effstate.pattern_break_row;
		
		if(state.potpos < song->getPotLength() - 1)
				*nextpotpos = state.potpos + 1;
			else
				*nextpotpos = song->getRestartPosition();
		
		return;
	}
	
	if(state.row + 1 >= song->patternlengths[state.pattern])
	{
		if(state.patternloop == false) // Don't jump when looping is enabled
		{
			if(state.potpos < song->getPotLength() - 1)
				*nextpotpos = state.potpos + 1;
			else
				*nextpotpos = song->getRestartPosition();
		} else {
			*nextpotpos = state.potpos;
		}
		*nextrow = 0;
	}
	else
	{
		*nextrow = state.row + 1;
		*nextpotpos = state.potpos;
	}
}
