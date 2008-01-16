#include "Pin.h"

Pin::Pin(int _x, int _y):
	Thing(Thing::Solid, Thing::User, Thing::Pin),
	thing1(0), thing2(0), b2joint(0)
{
	x=_x;
	y=_y;
}

void Pin::setThing(int nr, Thing *_thing)
{
	if(nr == 1)
		thing1 = _thing;
	if(nr == 2)
		thing2 = _thing;
}

Thing *Pin::getThing(int nr)
{
	if(nr == 1)
		return thing1;
	if(nr == 2)
		return thing2;
	return 0;
}

void Pin::setb2Joint(b2Joint *_joint)
{
	b2joint = _joint;
}

b2Joint *Pin::getb2Joint(void)
{
	return b2joint;
}

void Pin::getPosition(int *_x, int*_y)
{
	if(b2joint == 0)
	{
		*_x=x;
		*_y=y;
	}
	else
	{
		b2Vec2 pos = b2joint->GetAnchor1();
		*_x = (int)(pos.x * float32(10));
		*_y = (int)(pos.y * float32(10));
	}
}
