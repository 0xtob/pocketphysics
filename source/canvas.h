#ifndef _CANVAS_H_
#define _CANVAS_H_


#include "world.h"
#include <ulib/ulib.h>

class Canvas
{
	public:
		enum PenMode {pmNormal, pmBox, pmPolygon, pmCircle, pmPin, pmMove, pmDelete};
		enum ObjectMode {omDynamic, omSolid, omNonSolid};
		
		Canvas(World *_world);
		
		void draw(void);
		void setPenMode(PenMode _pen_mode);
		void setObjectMode(ObjectMode _object_mode);
		void penDown(int x, int y);
		void penMove(int x, int y);
		void penUp(int x, int y);
		
		void drawScreenRect(int sx, int sy);
		
		void startSimulationMode(void);
		void stopSimulationMode(void);
		
	private:
		void drawLine(u16 col, int x1, int y1, int x2, int y2);
		
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
		
		UL_IMAGE *crayon;
};

#endif
