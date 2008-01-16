#ifndef LISTBOX_H
#define LISTBOX_H

#include <nds.h>
#include <vector>
#include <string>
#include "widget.h"

#define ROW_HEIGHT			11
#define COUNTER_WIDTH			17
#define SCROLLBAR_WIDTH			9
#define SCROLLBUTTON_HEIGHT		9

#define SCROLLUP			1
#define SCROLLDOWN			2
#define SCROLLTHINGY			3

#define MIN_SCROLLTHINGYHEIGHT		5

class ListBox: public Widget {
	public:
		ListBox(u8 _x, u8 _y, u8 _width, u8 _height, uint16 **_vram, u8 n_items,
			bool _show_numbers, bool _visible=false, bool _zero_offset=true);
		
		// Drawing request
		void pleaseDraw(void);
		
		// Event calls
		void penDown(u8 px, u8 py);
		void penUp(u8 px, u8 py);
		void penMove(u8 px, u8 py);
		
		// Callback registration
		void registerChangeCallback(void (*onChange_)(u8));

		// Add / delete /set /get elements
		void add(const char *name);
		void del(void); // Always deletes selected item
		void ins(u8 idx, const char *name);
		void set(u8 idx, const char *name); // Inserts an element at position idx
		const char *get(u8 idx);
		u8 getidx(void); // get index of selected element
		void clear(void);
		void select(u8 idx); // set selected element
		
	protected:
		void draw(void);
		// Calculate height and position of the scroll thingy
		void calcScrollThingy(void);
		
		void (*onChange)(u8);
		
		u8 buttonstate;
		u8 activeelement;
		u8 scrollpos, oldscrollpos;
		u8 scrollthingypos, scrollthingyheight, pen_y_on_scrollthingy;
		bool show_numbers, zero_offset; // zero_offset: If false, offset=1
		std::vector<std::string> elements;
};

#endif
