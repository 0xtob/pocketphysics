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

#ifndef PPDESTRUCTIONLISTENER_H_
#define PPDESTRUCTIONLISTENER_H_

#include "Box2D.h"

class PPDestructionListener: public b2DestructionListener
{
public:
	~PPDestructionListener();

	/// Called when any joint is about to be destroyed due
	/// to the destruction of one of its attached bodies.
	void SayGoodbye(b2Joint* joint);

	/// Called when any shape is about to be destroyed due
	/// to the destruction of its parent body.
	void SayGoodbye(b2Shape* shape);
};

#endif /*PPDESTRUCTIONLISTENER_H_*/
