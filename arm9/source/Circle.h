#ifndef CIRCLE_H_
#define CIRCLE_H_

#include "thing.h"

class Circle : public Thing
{
	public:
		Circle(Type _type, CreatedBy _createdby);
		Circle(TiXmlElement *thingelement);
		
		void setRadius(int radius);
		int getRadius(void);
		
		TiXmlElement *toXML(void);
		
	private:
		int radius;
};

#endif /*CIRCLE_H_*/
