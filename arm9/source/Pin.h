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
