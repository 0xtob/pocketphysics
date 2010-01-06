#include "togglebutton.h"

#include <string.h>
#include <stdlib.h>

/* ===================== PUBLIC ===================== */

ToggleButton::ToggleButton(u8 _x, u8 _y, u8 _width, u8 _height, u16 **_vram, bool _visible)
	:Widget(_x, _y, _width, _height, _vram, _visible),
	penIsDown(false), on(false)
{
	onToggle = 0;
}
	
// Callback registration
void ToggleButton::registerToggleCallback(void (*onToggle_)(bool)) {
	onToggle = onToggle_;
}
		
// Drawing request
void ToggleButton::pleaseDraw(void) {
	draw();
}
		
// Event calls
void ToggleButton::penDown(u8 x, u8 y) {
	penIsDown = true;
	on = !on;
	draw();
	if(onToggle) {
		onToggle(on);
	}
}

void ToggleButton::penUp(u8 x, u8 y) {
	penIsDown = false;
	draw();
}

void ToggleButton::buttonPress(u16 button) {
	on = !on;
	draw();	
	if(onToggle) {
		onToggle(on);
	}
}
		
void ToggleButton::setCaption(const char *_caption) {
	caption = (char*)malloc(strlen(_caption)+1);
	strcpy(caption, _caption);
}

void ToggleButton::setState(bool _on) {
	if(on != _on) {
		on = _on;
		draw();
	}
}

/* ===================== PRIVATE ===================== */

void ToggleButton::draw(void)
{
	if(penIsDown) {
		if(on) {
			drawGradient(theme->col_dark_ctrl, theme->col_dark_ctrl, 0, 0, width, height);
		} else {
			drawGradient(theme->col_light_ctrl, theme->col_light_ctrl, 0, 0, width, height);
		}
	} else {
		if(on) {
			drawGradient(theme->col_dark_ctrl, theme->col_dark_ctrl, 0, 0, width, height);
		} else {
			drawGradient(theme->col_light_ctrl, theme->col_light_ctrl, 0, 0, width, height);
		}
	}
	drawBorder();
	
	drawString(caption, (width-getStringWidth(caption))/2, height/2-5);
}
