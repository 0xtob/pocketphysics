#ifndef _CANVAS_H_
#define _CANVAS_H_

#include <ulib/ulib.h>

#include "world.h"

class Canvas
{
	public:
		enum PenMode {pmNormal, pmBox, pmPolygon, pmCircle, pmPin, pmDelete};
		enum ObjectMode {omDynamic, omSolid, omNonSolid};
		
		Canvas(World *_world);
		
		void draw(void);
		void setPenMode(PenMode _pen_mode);
		void setObjectMode(ObjectMode _object_mode);
		void penDown(int x, int y);
		void penMove(int x, int y);
		void penUp(int x, int y);
		
		void drawScreenRect(int sx, int sy);
		
	private:
		void drawLine(u16 col, int x1, int y1, int x2, int y2);
		
		PenMode pen_mode;
		ObjectMode object_mode;
		World *world;
		Thing *currentthing;
		bool drawing;
		
		UL_IMAGE *crayon;
};

#endif
