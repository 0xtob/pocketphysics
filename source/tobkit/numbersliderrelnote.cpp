#include "numbersliderrelnote.h"

#include "numberbox.h"
#include <stdlib.h>
#include <stdio.h>

/* ===================== PUBLIC ===================== */

NumberSliderRelNote::NumberSliderRelNote(u8 _x, u8 _y, u8 _width, u8 _height, uint16 **_vram, s32 _value)
	:Widget(_x, _y, _width, _height, _vram),
	value(_value), btnstate(0), min(-48), max(71)
{
	onChange = 0;
}

// Drawing request
void NumberSliderRelNote::pleaseDraw(void) {
	draw();
}

// Event calls
void NumberSliderRelNote::penDown(u8 px, u8 py)
{
	if((px>x)&&(px<x+32)&&(py>y)&&(py<y+17)) {
		btnstate = true;
		lasty = py;
	}
	
	draw();
}

void NumberSliderRelNote::penUp(u8 px, u8 py)
{
	btnstate = false;
	draw();
}

void NumberSliderRelNote::penMove(u8 px, u8 py)
{
	s16 dy = lasty-py;
	if(abs(dy)>1) {
		s16 newval = value+dy/2;
		if(newval > max) {
			value=max;
		} else if(newval<min) {
			value=min;
		} else {
			value=newval;
		}
		draw();
		
		if(onChange!=0) {
			onChange(value);
		}
		
		lasty = py;
	}
}

void NumberSliderRelNote::setValue(s32 val)
{
	if((val<=max)&&(val>=min)) {
		value = val;
		if(onChange!=0) {
			onChange(value);
		}
		draw();
	}
}

s32 NumberSliderRelNote::getValue(void) {
	return value;
}

// Callback registration
void NumberSliderRelNote::registerChangeCallback(void (*onChange_)(s32)) {
	onChange = onChange_;
}

/* ===================== PRIVATE ===================== */

void NumberSliderRelNote::draw(void)
{
	if(!visible) return;
	
	// Slider thingy
	if(btnstate==true) {
		drawGradient(theme->col_dark_ctrl, theme->col_light_ctrl, 1, 1, 8, 15);
	} else {
		drawGradient(theme->col_light_ctrl, theme->col_dark_ctrl, 1, 1, 8, 15);
	}
	
	// This draws the up-arrow
	s8 i,j;
	for(j=0;j<3;j++) {
		for(i=-j;i<=j;++i) {
			*(*vram+SCREEN_WIDTH*(y+j+3)+x+4+i) = theme->col_icon;
		}
	}
	
	// This draws the connection
	drawLine(4, 6, 5, 0, theme->col_outline);
	
	// This draws the down-arrow
	for(j=2;j>=0;j--) {
		for(i=-j;i<=j;++i) {
			*(*vram+SCREEN_WIDTH*(y-j+13)+x+4+i) = theme->col_icon;
		}
	}
	
	drawBox(0, 0, 9, 17);
	
	// Number display
	drawFullBox(9,1,width-10,height-2,theme->col_lighter_bg);
	
	u8 octave = (value+4*12) / 12;
	u8 note = (value+4*12)%12;
	
	char *nstr = "";
	
	switch(note) {
		case(0): nstr = "c-"; break;
		case(1): nstr = "c#"; break;
		case(2): nstr = "d-"; break;
		case(3): nstr = "d#"; break;
		case(4): nstr = "e-"; break;
		case(5): nstr = "f-"; break;
		case(6): nstr = "f#"; break;
		case(7): nstr = "g-"; break;
		case(8): nstr = "g#"; break;
		case(9): nstr = "a-"; break;
		case(10): nstr = "a#"; break;
		case(11): nstr = "h-"; break;
	}
	
	char *notestr = (char*)malloc(4);
	sprintf(notestr, "%s%u", nstr, octave);
	drawString(notestr, 10, 5);
	free(notestr);
	
	// Border
	drawBorder();
}
