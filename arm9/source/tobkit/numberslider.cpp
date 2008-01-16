#include "numberslider.h"

#include "numberbox.h"
#include <stdlib.h>
#include <stdio.h>

/* ===================== PUBLIC ===================== */

NumberSlider::NumberSlider(u8 _x, u8 _y, u8 _width, u8 _height, uint16 **_vram, s32 _value, s32 _min, s32 _max, bool _hex)
	:Widget(_x, _y, _width, _height, _vram),
	value(_value), btnstate(0), min(_min), max(_max), hex(_hex)
{
	onChange = 0;
}

// Drawing request
void NumberSlider::pleaseDraw(void) {
	draw();
}

// Event calls
void NumberSlider::penDown(u8 px, u8 py)
{
	if((px>x)&&(px<x+32)&&(py>y)&&(py<y+17)) {
		btnstate = true;
		lasty = py;
	}
	
	draw();
}

void NumberSlider::penUp(u8 px, u8 py)
{
	btnstate = false;
	draw();
}

void NumberSlider::penMove(u8 px, u8 py)
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

void NumberSlider::setValue(s32 val)
{
	s32 oldval = value;
	
	if(val > max)
		value = max;
	else if (val < min)
		value = min;
	else
		value = val;

	if(oldval != value)
	{
		if(onChange!=0)
			onChange(val);
		if(visible==true)
			draw();
	}
}

s32 NumberSlider::getValue(void) {
	return value;
}

// Callback registration
void NumberSlider::registerChangeCallback(void (*onChange_)(s32)) {
	onChange = onChange_;
}

/* ===================== PRIVATE ===================== */

void NumberSlider::draw(void)
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
	char *numberstr = (char*)malloc(4);
	if(hex==true) {
		sprintf(numberstr, "%3x", (u16)value);
	} else {
		sprintf(numberstr, "%3d", (u16)value);
	}
	drawString(numberstr, 10, 5);
	free(numberstr);
	
	// Border
	drawBorder();
}
