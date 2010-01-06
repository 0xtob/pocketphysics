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
