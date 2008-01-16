#include "Circle.h"

Circle::Circle(Type _type, CreatedBy _createdby):
	Thing(_type, _createdby, Thing::Circle),
	radius(0)
{
}

void Circle::setRadius(int _radius)
{
	radius = _radius;
}

int Circle::getRadius(void)
{
	return radius;
}
