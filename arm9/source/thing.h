#ifndef _THING_H_
#define _THING_H_

#include <Box2D.h>

class Thing
{
	public:
		enum Type { Dynamic, Solid, NonSolid };
		enum CreatedBy { User, Simulation };
		enum Shape { Circle, Polygon, Pin };
		
		Thing(Type _type, CreatedBy _createdby, Shape _shape);
		virtual ~Thing() {};
		Shape getShape(void);
		Type getType(void);
		void setPosition(int _x, int _y);
		virtual void getPosition(int *_x, int*_y);
		void setb2Body(b2Body *_b2body);
		b2Body* getb2Body(void);
		void setRotation(float _rotation);
		float getRotation(void);
		
		// Reset to original position/rotation
		void reset(void);
		
	protected:
		Type type;
		CreatedBy createdby;
		Shape shape;
		int x, y;
		float rotation; // Radians
		b2Body* b2body;
};

#endif
