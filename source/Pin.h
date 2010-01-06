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

#ifndef PIN_H_
#define PIN_H_

#include "thing.h"

class Pin : public Thing
{
	public:
		Pin(int _x, int _y);
		Pin(TiXmlElement *thingelement);
		
		// Pins can have 2 things, so nr must be 1 or 2
		void setThing(int nr, Thing *_thing);
		Thing *getThing(int nr);
		void setb2Joint(b2Joint *_joint);
		b2Joint *getb2Joint(void);
		void getPosition(int *_x, int*_y);
		
		TiXmlElement *toXML(void);
		
	private:
		Thing *thing1, *thing2;
		b2Joint *b2joint;
};

#endif /*PIN_H_*/
