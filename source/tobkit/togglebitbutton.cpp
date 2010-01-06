#include "togglebitbutton.h"

#include <string.h>
#include <stdlib.h>

/* ===================== PUBLIC ===================== */

ToggleBitButton::ToggleBitButton(u8 _x, u8 _y, u8 _width, u8 _height, u16 **_vram,
		const u8 *_bitmap, u8 _bmpwidth, u8 _bmpheight, u8 _bmpx, u8 _bmpy,
		ToggleBitButtonGroup *_tbbgroup, bool _visible)
	:Widget(_x, _y, _width, _height, _vram, _visible),
	penIsDown(false), on(false),
	bitmap(_bitmap), bmpwidth(_bmpwidth), bmpheight(_bmpheight), bmpx(_bmpx), bmpy(_bmpy),
	tbbgroup(_tbbgroup)
{
	onToggle = 0;
	
	_tbbgroup->add(this);
}
	
// Callback registration
void ToggleBitButton::registerToggleCallback(void (*onToggle_)(bool)) {
	onToggle = onToggle_;
}
		
// Drawing request
void ToggleBitButton::pleaseDraw(void) {
	draw();
}
		
// Event calls
void ToggleBitButton::penDown(u8 x, u8 y) {
	penIsDown = true;
	
	if(tbbgroup == 0) // self-managed
	{
		on = !on;
		draw();
		if(onToggle) {
			onToggle(on);
		}
	} else {
		tbbgroup->pushed(this);
	}
}

void ToggleBitButton::penUp(u8 x, u8 y) {
	penIsDown = false;
	draw();
}

void ToggleBitButton::buttonPress(u16 button) {
	if(tbbgroup == 0) // self-managed
	{
		on = !on;
		draw();	
		if(onToggle) {
			onToggle(on);
		}
	} else {
		tbbgroup->pushed(this);
	}
}

void ToggleBitButton::setState(bool _on) {
	if(on != _on) {
		on = _on;
		draw();
	}
}

bool ToggleBitButton::getState(void)
{
	return on;
}

/* ===================== PRIVATE ===================== */

void ToggleBitButton::draw(void)
{
	if(penIsDown) {
		if(on) {
			drawGradient(theme->col_darker_ctrl, theme->col_darkest_ctrl, 0, 0, width, height);
		} else {
			drawGradient(theme->col_light_ctrl, theme->col_dark_ctrl, 0, 0, width, height);
		}
	} else {
		if(on) {
			drawGradient(theme->col_darker_ctrl, theme->col_darkest_ctrl, 0, 0, width, height);
		} else {
			drawGradient(theme->col_light_ctrl, theme->col_dark_ctrl, 0, 0, width, height);
		}
	}
	drawBorder();
	
	drawMonochromeIcon(bmpx, bmpy, bmpwidth, bmpheight, bitmap);
}
