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

#include "Pin.h"

Pin::Pin(int _x, int _y):
	Thing(Thing::Solid, Thing::User, Thing::Pin),
	thing1(0), thing2(0), b2joint(0)
{
	x=_x;
	y=_y;
}

Pin::Pin(TiXmlElement *thingelement):
	Thing(thingelement), thing1(0), thing2(0), b2joint(0)
{
	shape = Thing::Pin;
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
		*_x = (int)(pos.x * PIXELS_PER_UNIT);
		*_y = (int)(pos.y * PIXELS_PER_UNIT);
	}
}

TiXmlElement *Pin::toXML(void)
{
	TiXmlElement *element = new TiXmlElement("pin");

	element->SetAttribute("x", x);
	element->SetAttribute("y", y);
	
	return element;
}
