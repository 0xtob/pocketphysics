/**********************
DS Tracker Widget Class
Has rectangular area
**********************/

#ifndef WIDGET_H
#define WIDGET_H

#include <nds.h>
#include <limits.h>

#include "Theme.h"

class Widget {
	public:
		// Constructor sets base variables
		Widget(u8 _x, u8 _y, u8 _width, u8 _height, u16 **_vram, bool _visible=false);
		virtual ~Widget(void) {}
		
		// Callback registration
		// ... is done in the class
		
		// Drawing request
		virtual void pleaseDraw(void) {};
		
		// Get position
		void getPos(u8 *_x, u8 *_y, u8 *_width, u8 *_height);
		
		// Toggle visibility
		virtual void show(void);
		virtual void hide(void);
		bool is_visible(void) { return visible; }
		
		// Toggle enabled/disabled
		virtual void enable(void);
		virtual void disable(void);
		bool is_enabled(void) { return enabled; }
		
		// Event calls
		virtual void penDown(u8 px, u8 py) {};
		virtual void penUp(u8 px, u8 py) {};
		virtual void penMove(u8 px, u8 py) {};
		virtual void buttonPress(u16 button) {};
		virtual void buttonRelease(u16 button) {};
		
		virtual void setTheme(Theme *theme_) { theme = theme_; }
		
	protected:
		u16 x, y, width, height;
		bool visible, enabled;
		u16 **vram;
		Theme *theme;
		
		// Draw utility functions
		void drawString(const char* str, u8 tx, u8 ty, u8 maxwidth=255, u16 color=RGB15(0,0,0)|BIT(15));
		void drawBox(u8 tx, u8 ty, u8 tw, u8 th, u16 col=RGB15(0,0,0)|BIT(15));
		void drawFullBox(u8 tx, u8 ty, u8 tw, u8 th, u16 col);
		void drawBorder(u16 col = RGB15(0,0,0)|BIT(15));
		void drawLine(u8 tx, u8 ty, u8 length, u8 horizontal, u16 col);
		void drawBresLine(u8 tx1, u8 ty1, u8 tx2, u8 ty2, u16 col);
		void drawPixel(u8 tx, u8 ty, u16 col);
		void drawGradient(u16 col1, u16 col2, u8 tx, u8 ty, u8 tw, u8 th);
		inline const u16 interpolateColor(u16 col1, u16 col2, u8 alpha);
		void drawMonochromeIcon(u8 tx, u8 ty, u8 tw, u8 th, const u8 *icon, u16 col=(RGB15(0,0,0)|BIT(15)));
		void drawImage(u8 tx, u8 ty, u8 tw, u8 th, const u16 *image);
		
		// Stylus utility functions
		bool isInRect(u8 x, u8 y, u8 x1, u8 y1, u8 x2, u8 y2);
		
		// How wide is the string when rendered?
		u8 getStringWidth(const char *str, u16 limit=USHRT_MAX);
};

#endif
