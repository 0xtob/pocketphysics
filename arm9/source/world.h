#ifndef _WORLD_H_
#define _WORLD_H_

#include <Box2D.h>

#include "thing.h"
#include "Pin.h"

#define MAX_THINGS			64
#define DEFAULT_GRAVITY		8.0f //60.0f
#define DEFAULT_DENSITY		5.0f
#define DEFAULT_FRICTION	0.3f
#define DEFAULT_RESTITUTION	0.2f

#define TIMESTEP			(float32(1.0f / 30.0f))
#define ITERATIONS			3//6

class World
{
	public:
		World(int _width, int _height);
		~World();
		
		// Adds a thing , but doesn't make it physical yet
		void add(Thing *thing);
		
		// Fixes a thing with a pin (a revolute joint)
		void pin(Pin *pin, Thing *thing1, Thing *thing2=0);
		
		// Removes the object from the world, but you have to delete it yourself
		void remove(Thing *thing);
		
		// Removes the object at (x,y) from the world, but you have to delete it yourself
		// If no object is found, 0 is returned.
		Thing *removeAt(int x, int y);
		
		// Gets the things at a certain position. things is a pointer to a buffer,
		// is is the maimum nr of things to return. Returns the number of things
		// found.
		int getThingsAt(int x, int y, Thing **things, int n);
		
		// Makes a thing physical, i.e. adds it to the simulation
		bool makePhysical(Thing *thing);
		
		// Makes a thing unphysical, i.e. removes it from the simulation
		void makeUnphysical(Thing *thing);
		
		int getNThings(void);
		Thing *getThing(int index);
		void getDimensions(int *_width, int *_height);
		void getGravity(float32 *grav_x, float32 *grav_y);
		void setGravity(float32 grav_x, float32 grav_y);
		
		// do a simulation step
		void step(float32 timestep = TIMESTEP);
		
		void reset(void);
		
	private:
		Thing* things[MAX_THINGS];
		int n_things;
		int width, height;
		float32 gravity_x, gravity_y;
		
		b2World *b2world;
		
		b2Body *bgbody; // Dummy body for attaching joints
		
		void initPhysics(void);
		void makeJointDummy(void);
		void destroyJointDummy(void);
};

#endif
