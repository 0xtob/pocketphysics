/*
 *    Pocket Physics - A mechanical construction kit for Nintendo DS
 *                   Copyright 2005-2010 Tobias Weyand (me@tobw.net)
 *                            http://code.google.com/p/pocketphysics
 *
 * TobKit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * TobKit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with Pocket Physics. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _CANVAS_H_
#define _CANVAS_H_

#include "world.h"
#include "tobkit/tobkit.h"
#include <ulib/ulib.h>

namespace TobKit
{

class Canvas: public Widget
{
	public:
		enum PenMode {pmNormal, pmBox, pmPolygon, pmCircle, pmPin, pmMove, pmDelete};
		enum ObjectMode {omDynamic, omSolid, omNonSolid};
		
		Canvas(WidgetManager *owner, World *_world, int x, int y, int width, int height, u16 listening_buttons,
				bool dual_screen=true);
		
		void draw(void);
		void setPenMode(PenMode _pen_mode);
		void setObjectMode(ObjectMode _object_mode);
		void penDown(int x, int y);
		void penMove(int x, int y);
		void penUp();
		
        void buttonPress(u16 button);
        void buttonRelease(u16 button);

		void drawScreenRect(int sx, int sy);
		
		void startSimulationMode(void);
		void stopSimulationMode(void);
		
		// Disables drawing objects.
		void disableDrawing() {dont_draw_things = true; };

		// Enables drawing objects.
		void enableDrawing() {dont_draw_things = false; };

		// Set/get whether we draw the lower screen window on the upper screen.
		void setDrawWindow(bool v) { draw_window = v; };
		bool getDrawWindow() { return draw_window; }

	private:
		void drawThings(void);
		void drawImageQuad(UL_IMAGE *img, s16 x1, s16 y1, s16 x2, s16 y2, s16 x3, s16 y3, s16 x4, s16 y4);
		void drawLine(u16 col, int x1, int y1, int x2, int y2);
		void handleScrolling();
		
		PenMode pen_mode;
		ObjectMode object_mode;
		World *world;
		Thing *currentthing;
		bool drawing;
		Thing *pinthing1;
		Thing *pinthing2;
		Thing *highlightthing;
		int move_startx, move_starty;
		int move_things_x[MAX_THINGS], move_things_y[MAX_THINGS];
		Thing **move_connected_things;
		int move_n_connected_things;
		bool simulation_mode;
		int lastx, lasty;
		bool dual_screen;
		bool dont_draw_things;
		bool draw_window;
		int scroll_x, scroll_y, scroll_vx, scroll_vy;
		int keysheld;
		
		UL_IMAGE *crayon;
		UL_IMAGE *imgbg;
};

};

#endif
