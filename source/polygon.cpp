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

#include "polygon.h"

Polygon::Polygon(Type _type, CreatedBy _createdby):
	Thing(_type, _createdby, Thing::Polygon),
	n_vertices(0), closed(false)
{
	
}

Polygon::Polygon(TiXmlElement *thingelement):
	Thing(thingelement), n_vertices(0), closed(false)
{
	shape = Thing::Polygon;
	
	int closed_;
	thingelement->QueryIntAttribute("closed", &closed_);
	if(closed_ == 0)
		closed = false;
	else
		closed = true;
	
	for(TiXmlElement *vtxelement = thingelement->FirstChildElement(); vtxelement; vtxelement = vtxelement->NextSiblingElement())
	{
		int vx, vy;
		vtxelement->QueryIntAttribute("x", &vx);
		vtxelement->QueryIntAttribute("y", &vy);
		addVertex(vx, vy);
		//printf("  vtx: %d %d\n", vx, vy);
	}
}

void Polygon::addVertex(int x, int y)
{
	if(n_vertices == MAX_VERTICES)
		return;
	
	vertices_x[n_vertices] = x;
	vertices_y[n_vertices] = y;
	
	n_vertices++;
}

void Polygon::removeVertex(int index)
{
	if(index >= n_vertices)
		return;
	
	for(int i=index; i<n_vertices-1; ++i)
	{
		vertices_x[i] = vertices_x[i+1];
		vertices_y[i] = vertices_y[i+1];
	}
	
	n_vertices--;
}

int Polygon::getNVertices(void)
{
	return n_vertices;
}

void Polygon::getVertex(int index, int *_x, int *_y, bool relative)
{
	if(index >= n_vertices)
	{
		x=0;
		y=0;
		return;
	}
	
	if(relative)
	{
		*_x = vertices_x[index];
		*_y = vertices_y[index];
	}
	else
	{
		if(!b2body)
		{
			b2Vec2 pos;
			pos.x = float32(vertices_x[index]);
			pos.y = float32(vertices_y[index]);
			
			b2Mat22 rot;
			rot.Set(float32(rotation));
			pos = b2Mul(rot, pos);
			
			*_x = (int)pos.x + x;
			*_y = (int)pos.y + y;
		}
		else
		{
			// TODO: Cache vertex positions for fixed shapes!
			b2Vec2 pos;
			pos.x = float32(vertices_x[index]);
			pos.y = float32(vertices_y[index]);
			
			//b2Mat22 rot = b2body->GetRotationMatrix();
			float32 angle = b2body->GetAngle();
			b2Mat22 rot;
			rot.Set(float32(angle));
			pos = b2Mul(rot, pos);
			
			b2Vec2 origin = b2body->GetPosition();
			*_x = (int)(pos.x + origin.x*PIXELS_PER_UNIT);
			*_y = (int)(pos.y + origin.y*PIXELS_PER_UNIT);
		}
	}
}

void Polygon::setVertex(int index, int _x, int _y)
{
	if(index >= n_vertices)
		return;
	
	vertices_x[index] = _x;
	vertices_y[index] = _y;
	
	if(b2body)
	{
		printf("TODO: change vtx in b2body\n");
	}
}

void Polygon::setClosed(bool closed_)
{
	closed = closed_;
}

bool Polygon::getClosed(void)
{
	return closed;
}

TiXmlElement *Polygon::toXML(void)
{
	TiXmlElement *element = new TiXmlElement("polygon");
	addGenericXMLAttributes(element);
	element->SetAttribute("closed", closed?1:0);
	for(int i=0; i<n_vertices; ++i)
	{
		TiXmlElement *vtxelement = new TiXmlElement("vertex");
		vtxelement->SetAttribute("x", vertices_x[i]);
		vtxelement->SetAttribute("y", vertices_y[i]);
		element->LinkEndChild(vtxelement);
	}
	return element;
}
