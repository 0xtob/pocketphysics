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

#ifndef _WORLD_H_
#define _WORLD_H_

#include "Box2D.h"

#include "thing.h"
#include "Pin.h"
#include "PPDestructionListener.h"
#include "PPBoundaryListener.h"

#define MAX_THINGS			50
#define DEFAULT_GRAVITY		2.0f//8.0f //60.0f

#define TIMESTEP			(float32(1.0f / 20.0f))
#define ITERATIONS			3 //6

class PPBoundaryListener;

class World
{
	public:
		World(int _width, int _height, bool allow_sleep=true);
		~World();
		
		// Adds a thing , but doesn't make it physical yet
		bool add(Thing *thing);
		
		// Fixes a thing with a pin (a revolute joint)
		void pin(Pin *pin, Thing *thing1, Thing *thing2=0);
		
		// Attaches a mouse joint to a thing for dragging it around
		void grab(Thing *thing, int x, int y);
		
		// Drags the grabbed thing around
		void drag(int x, int y);
		
		// Lets go off the grabbed thing
		void letGo(void);
		
		// Removes the object from the world, but you have to delete it yourself
		void remove(Thing *thing);
		
		// Gets the things at a certain position. things is a pointer to a buffer,
		// is is the maimum nr of things to return. Returns the number of things
		// found.
		int getThingsAt(int x, int y, Thing **res_things, int n, bool include_pins);
		
		// Gets all things (including pins) that are connected to a thing by being
		// directly or indirectly pinned to it
		// Returns an array that the user has to delete
		Thing **getConnectedThings(Thing *thing, int *n);
		
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
		
		// Reset the world (allow_sleep says if objects may fall asleep)
		void reset(bool allow_sleep);
		
		void save(const char *filename, char *thumbnail);
		bool load(const char *filename);
		
		// Marks a thing for being made unphysical after the current physics iteration
		void markForMakingUnphysical(Thing *thing);
		
		int getWidth() { return width; }
		int getHeight() { return height; }

	private:
		
		Thing* things[MAX_THINGS];
		int n_things;
		int width, height;
		float32 gravity_x, gravity_y;
		
		b2World *b2world;
		
		b2Body *bgbody; // Dummy body for attaching joints
		
		PPDestructionListener *destruction_listener;
		PPBoundaryListener *boundary_listener;
		b2MouseJoint *mouse_joint;
		
		Thing *make_unphysical_list[MAX_THINGS];
		int make_unphysical_list_length;

		// Makes things unphysical that were marked
		void makeMarkedThingsUnphysical(void);
		
		void initPhysics(bool allow_sleep);
		void makeJointDummy(void);
		void destroyJointDummy(void);
};

#endif
