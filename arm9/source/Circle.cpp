#include "Circle.h"

Circle::Circle(Type _type, CreatedBy _createdby):
	Thing(_type, _createdby, Thing::Circle),
	radius(0)
{
}

Circle::Circle(TiXmlElement *thingelement):
	Thing(thingelement), radius(0)
{
	shape = Thing::Circle;
	thingelement->QueryIntAttribute("radius", &radius);
}

void Circle::setRadius(int _radius)
{
	radius = _radius;
}

int Circle::getRadius(void)
{
	return radius;
}

TiXmlElement *Circle::toXML(void)
{
	TiXmlElement *element = new TiXmlElement("circle");
	addGenericXMLAttributes(element);
	element->SetAttribute("radius", radius);
	return element;
}
