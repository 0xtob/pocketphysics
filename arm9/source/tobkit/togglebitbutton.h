#ifndef TOGGLEBITBUTTON_H
#define TOGGLEBITBUTTON_H

#include "widget.h"

#include <vector>

class ToggleBitButton: public Widget {
	public:
		class ToggleBitButtonGroup {
			public:
				ToggleBitButtonGroup(bool _forceSelection=true);
				
				void add(ToggleBitButton *rb);
				void pushed(ToggleBitButton *rb);
				void setActive(s8 idx);
				void registerChangeCallback(void (*onChange_)(s8));
	
			private:
				std::vector<ToggleBitButton*> rbvec;
				void (*onChange)(s8);
				bool forceSelection; // If false, al buttons may be up
		};
		
		ToggleBitButton(u8 _x, u8 _y, u8 _width, u8 _height, u16 **_vram, 
				const u8 *_bitmap, u8 _bmpwidth=13, u8 _bmpheight=13, u8 _bmpx=2, u8 _bmpy=2,
				ToggleBitButtonGroup *_tbbgroup = 0, bool _visible=false);
	
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
		bool getState(void);
	
	private:
		void (*onToggle)(bool);
		
		bool penIsDown;
		bool on;	
		const u8 *bitmap;
		u8 bmpwidth, bmpheight, bmpx, bmpy;
		ToggleBitButtonGroup *tbbgroup;
		
		void draw(void);
};

#endif
