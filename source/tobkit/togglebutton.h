#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#include "widget.h"

class ToggleButton: public Widget {
	public:
		ToggleButton(u8 _x, u8 _y, u8 _width, u8 _height, u16 **_vram, bool _visible=false);
	
		// Callback registration
		void registerToggleCallback(void (*onToggle_)(bool));
		
		// Drawing request
		void pleaseDraw(void);
		
		// Event calls
		void penDown(u8 x, u8 y);
		void penUp(u8 x, u8 y);
		void buttonPress(u16 button);
		
		void setCaption(const char *_caption);
		
		void setState(bool _on);
	
	private:
		void (*onToggle)(bool);
		
		bool penIsDown;
	
		bool on;	
	
		void draw(void);
		
		char *caption;
};

#endif
