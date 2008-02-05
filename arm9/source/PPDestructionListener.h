#ifndef PPDESTRUCTIONLISTENER_H_
#define PPDESTRUCTIONLISTENER_H_

#include <Box2D.h>

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
