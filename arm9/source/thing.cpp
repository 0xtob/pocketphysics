#include "thing.h"

Thing::Thing(Type _type, CreatedBy _createdby, Shape _shape):
	type(_type), createdby(_createdby), shape(_shape), x(0), y(0), rotation(0.0f), b2body(0)
{
	
}

Thing::Thing(TiXmlElement *thingelement):
	x(0), y(0), rotation(0.0f), b2body(0)
{
	if( strcmp(thingelement->Attribute("type"), "solid") == 0 )
		type = Solid;
	else if( strcmp(thingelement->Attribute("type"), "dynamic") == 0 )
		type = Dynamic;
	
	createdby = User;
	
	thingelement->QueryIntAttribute("x", &x);
	thingelement->QueryIntAttribute("y", &y);
	double rot;
	thingelement->QueryDoubleAttribute("rotation", &rot);
	rotation = rot;
	// TODO: density, friction, restitution
}

Thing::Shape Thing::getShape(void)
{
	return shape;
}

Thing::Type Thing::getType(void)
{
	return type;
}

void Thing::setPosition(int _x, int _y)
{
	x = _x;
	y = _y;
	
	if(b2body)
	{
		b2Vec2 origin;
		origin.x = float32(_x)/PIXELS_PER_UNIT;
		origin.y = float32(_y)/PIXELS_PER_UNIT;
		
		if(b2body->IsFrozen())
			printf("cannot set position: body is frozen\n");
		
		b2body->SetXForm(origin, getRotation());
	}
}

void Thing::getPosition(int *_x, int*_y)
{
	if(!b2body) // Not physical: Get default position
	{
		*_x=x;
		*_y=y;
	}
	else // physical: Get b2Body position
	{
		b2Vec2 pos = b2body->GetPosition();
		*_x=pos.x*PIXELS_PER_UNIT;
		*_y=pos.y*PIXELS_PER_UNIT;
	}
}

void Thing::setb2Body(b2Body *_b2body)
{
	b2body = _b2body;
}

b2Body* Thing::getb2Body(void)
{
	return b2body;
}

void Thing::setRotation(float32 _rotation)
{
	// TODO: Physical
	rotation = _rotation;
	
	if(b2body)
	{
		b2Vec2 origin;
		origin.x = float32(x)/PIXELS_PER_UNIT;
		origin.y = float32(y)/PIXELS_PER_UNIT;
		
		if(b2body->IsFrozen())
			printf("cannot set position: body is frozen\n");
		
		b2body->SetXForm(origin, float32(rotation));
	}
}

float32 Thing::getRotation(void)
{
	if(b2body)
	{
		//return (float)b2body->GetRotation();
		return (float)b2body->GetAngle();
	}
	else
		return rotation;
}

void Thing::reset(void)
{
	if(b2body)
	{
		printf("you shall not reset physical bodies!\n");
		return;
	}
	
	setPosition(x, y);
	setRotation(rotation);
}

// ===================================== PROTECTED ===================================== //

void Thing::addGenericXMLAttributes(TiXmlElement *thingelement)
{
	if(type==Solid)
		thingelement->SetAttribute("type", "solid");
	else if(type==Dynamic)
		thingelement->SetAttribute("type", "dynamic");
	thingelement->SetDoubleAttribute("density", DEFAULT_DENSITY);
	thingelement->SetDoubleAttribute("friction", DEFAULT_FRICTION);
	thingelement->SetDoubleAttribute("restitution", DEFAULT_RESTITUTION);
	thingelement->SetAttribute("x", x);
	thingelement->SetAttribute("y", y);
	thingelement->SetDoubleAttribute("rotation", rotation);
}
