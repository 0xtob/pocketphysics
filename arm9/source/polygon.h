#ifndef _POLYGON_H_
#define _POLYGON_H_

#include "thing.h"

#define MAX_VERTICES 64

class Polygon: public Thing
{
	public:
		Polygon(Type _type, CreatedBy _createdby);
		void addVertex(int x, int y);
		void removeVertex(int index);
		int getNVertices(void);
		
		// This returns the vtx coordinate in screen-coordinates, NOT in
		// local coordinates of the polygon! (unless relative==true)
		void getVertex(int index, int *_x, int *_y, bool relative=false);
		
		void setVertex(int index, int _x, int _y);
		
		void setClosed(bool closed_);
		bool getClosed(void);
		
	private:
		int vertices_x[MAX_VERTICES];
		int vertices_y[MAX_VERTICES];
		int n_vertices;
		bool closed;
};

#endif
