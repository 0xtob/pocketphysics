#include "polygon.h"

Polygon::Polygon(Type _type, CreatedBy _createdby):
	Thing(_type, _createdby, Thing::Polygon),
	n_vertices(0)
{
	
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
			
			b2Mat22 rot = b2body->GetRotationMatrix();
			pos = b2Mul(rot, pos);
			
			b2Vec2 origin = b2body->GetOriginPosition();
			*_x = (int)(pos.x + origin.x*10);
			*_y = (int)(pos.y + origin.y*10);
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
