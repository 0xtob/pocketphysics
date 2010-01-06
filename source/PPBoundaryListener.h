#ifndef PPBOUNDARYLISTENER_H_
#define PPBOUNDARYLISTENER_H_

#include "Box2D.h"
#include "world.h"

class World;

class PPBoundaryListener: public b2BoundaryListener
{
public:
	PPBoundaryListener(World *world_);
	
	void Violation(b2Body *body);
	
private:
	World *world;
};

#endif /*PPBOUNDARYLISTENER_H_*/
