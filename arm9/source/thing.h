#ifndef _THING_H_
#define _THING_H_

#include <Box2D.h>
#include <tinyxml.h>

#define PIXELS_PER_UNIT	(float32(50))
#define UNITS_PER_PIXEL	(float32(1)/PIXELS_PER_UNIT)

#define DEFAULT_DENSITY		1.0f//5.0f
#define DEFAULT_FRICTION	0.3f
#define DEFAULT_RESTITUTION	0.2f

class Thing
{
	public:
		enum Type { Dynamic, Solid, NonSolid };
		enum CreatedBy { User, Simulation };
		enum Shape { Circle, Polygon, Pin };
		
		Thing(Type _type, CreatedBy _createdby, Shape _shape);
		Thing(TiXmlElement *thingelement);
		virtual ~Thing() {};
		Shape getShape(void);
		Type getType(void);
		void setPosition(int _x, int _y);
		virtual void getPosition(int *_x, int*_y);
		void setb2Body(b2Body *_b2body);
		b2Body* getb2Body(void);
		void setRotation(float32 _rotation);
		float32 getRotation(void);
		
		// Reset to original position/rotation
		void reset(void);
		
		virtual TiXmlElement *toXML(void) = 0;
		
	protected:
		void addGenericXMLAttributes(TiXmlElement *thingelement);
		
		Type type;
		CreatedBy createdby;
		Shape shape;
		int x, y;
		float32 rotation; // Radians
		b2Body* b2body;
};

#endif
