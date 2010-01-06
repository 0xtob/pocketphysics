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

#ifndef _POLYGON_H_
#define _POLYGON_H_

#include "thing.h"

#define MAX_VERTICES 64

class Polygon: public Thing
{
	public:
		Polygon(Type _type, CreatedBy _createdby);
		Polygon(TiXmlElement *thingelement);
		void addVertex(int x, int y);
		void removeVertex(int index);
		int getNVertices(void);
		
		// This returns the vtx coordinate in screen-coordinates, NOT in
		// local coordinates of the polygon! (unless relative==true)
		void getVertex(int index, int *_x, int *_y, bool relative=false);
		
		void setVertex(int index, int _x, int _y);
		
		void setClosed(bool closed_);
		bool getClosed(void);
		
		TiXmlElement *toXML(void);
		
	private:
		int vertices_x[MAX_VERTICES];
		int vertices_y[MAX_VERTICES];
		int n_vertices;
		bool closed;
};

#endif
