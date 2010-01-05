#include "world.h"

PPBoundaryListener::PPBoundaryListener(World *world_):
	world(world_)
{
}


void PPBoundaryListener::Violation(b2Body *body)
{
	Thing *thing = (Thing*)body->GetUserData();
	world->markForMakingUnphysical(thing);
}
