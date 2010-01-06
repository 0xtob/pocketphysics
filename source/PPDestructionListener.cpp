#include "PPDestructionListener.h"

#include "Pin.h"

PPDestructionListener::~PPDestructionListener()
{
}

void PPDestructionListener::SayGoodbye(b2Joint* joint)
{
	Pin *pin = (Pin*)joint->GetUserData();
	pin->setb2Joint(0);
}

void PPDestructionListener::SayGoodbye(b2Shape* shape)
{
	// Don't care
}
