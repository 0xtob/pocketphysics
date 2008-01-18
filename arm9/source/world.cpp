#include "world.h"

#include "polygon.h"
#include "Circle.h"

#include <stdlib.h>

World::World(int _width, int _height):
	n_things(0), width(_width), height(_height), gravity_x(0), gravity_y(DEFAULT_GRAVITY)
{
	initPhysics();
	
	makeJointDummy();
}

World::~World()
{
	
}

void World::add(Thing *thing)
{
	if(n_things == MAX_THINGS)
		return;
	
	things[n_things] = thing;
	n_things++;
}

void World::pin(Pin *pin, Thing *thing1, Thing *thing2)
{
	pin->setThing(1, thing1);
	pin->setThing(2, thing2);
	
	b2Body *body1 = thing1->getb2Body();
	b2Body *body2;
	if(thing2 == 0)
	{
		body2 = bgbody;
		printf("pinning %x to bg\n", body1);
	}
	else
	{
		body2 = thing2->getb2Body();
		printf("pinning %x to %x\n", body1, body2);
	}
	
	if(body1 != 0)
	{
		b2RevoluteJointDef *jointDef = new b2RevoluteJointDef();
		jointDef->body1 = body1;
		jointDef->body2 = body2;
		jointDef->userData = pin;
		int px, py;
		pin->getPosition(&px, &py);
		b2Vec2 pinpos;
		pinpos.x = float32(px) / float32(10);
		pinpos.y = float32(py) / float32(10);
		
		//jointDef->localAnchor1 = pinpos;
		//jointDef->localAnchor2 = pinpos;
		jointDef->SetInWorld(pinpos);
		
		b2Joint* joint = b2world->Create(jointDef);
		pin->setb2Joint(joint);
		
		//b2Vec2 a1 = pin->getb2Joint()->GetAnchor1();
		//b2Vec2 a2 = pin->getb2Joint()->GetAnchor2();
		
		//printf("anchor1: %f %f\n", (float)a1.x, (float)a1.y);
		//printf("anchor2: %f %f\n", (float)a2.x, (float)a2.y);
		
		delete jointDef;
	}
	else
	{
		printf("Pin not attached to body!\n"); //<- warum kann das passieren?
	}
}

void World::remove(Thing *thing)
{
	makeUnphysical(thing);
	
	bool found=false;
	for(int i=0;i<n_things;++i)
	{
		if(things[i] == thing)
			found=true;
		if(found==true)
			things[i] = things[i+1];
	}
	if(found==true)
	{
		n_things--;
		
		// Remove all attached pins
		int i=0;
		while(i<n_things)
		{
			Thing *t = things[i];
			if(t->getShape() == Thing::Pin)
			{
				Pin *p = (Pin*)t;
				if( (p->getThing(1) == thing) || (p->getThing(2) == thing) )
					remove(p);
				else
					i++;
			} else
				i++;
		}
	}
}

Thing *World::removeAt(int x, int y)
{
	Thing *thing;
	int count = getThingsAt(x, y, &thing, 1);
	if(count == 0)
		return 0;
	
	remove(thing);
	return thing;
}

int World::getThingsAt(int x, int y, Thing ** things, int n)
{
	b2AABB *touchAABB = new b2AABB();
	touchAABB->minVertex.Set((float)(x-2)/10.0f, (float)(y-2)/10.0f);
	touchAABB->maxVertex.Set((float)(x+2)/10.0f, (float)(y+2)/10.0f);
	b2Vec2 point = b2Vec2(x/10.0f, y/10.0f);
	
	b2Shape *shape[16];
	b2Body *body;
	
	int count = b2world->Query(*touchAABB, shape, 16);
	int returncount = 0;
	for(int i=0;(i<count)&&(returncount<n);++i)
	{
		body = shape[i]->GetBody();
		
		// We know (x,y) lies in the bounding box of the body.
		// Now check if it lies in the actual body
		if( shape[i]->TestPoint(body->GetXForm(), point) )
		{
			// Do we already have this body? (Happens for multi-shape bodies)
			bool alreadythere = false;
			for(int b=0;b<returncount;++b)
				if(things[b]->getb2Body() == body)
					alreadythere = true;
			
			if(!alreadythere)
			{
				things[returncount] = (Thing*)body->GetUserData();
				returncount++;
			}
		}
	}
	return returncount;
}

bool World::makePhysical(Thing *thing)
{
	if(thing->getShape() == Thing::Pin)
	{
		Pin *p = (Pin*)thing;
		pin(p, p->getThing(1), p->getThing(2));
	}
	else
	{
		if(thing->getb2Body() != 0)
			return false;
		
		if(thing->getType() == Thing::NonSolid)
			return false;
		
		switch(thing->getShape())
		{
			case(Thing::Polygon):
			{
				Polygon *polygon = (Polygon*)thing;
				
				b2PolygonDef *polyDef = new b2PolygonDef();
				
				int n_points = polygon->getNVertices();
				polyDef->vertexCount = n_points;
				  
				if( (polygon->getType() == Thing::Solid) || (polygon->getType() == Thing::NonSolid) )
					polyDef->density = 0.0f;
				else
					polyDef->density = DEFAULT_DENSITY;
				  
				polyDef->friction = DEFAULT_FRICTION;
				polyDef->restitution = DEFAULT_RESTITUTION;
				
				float32 *points_x = new float32[n_points];
				float32 *points_y = new float32[n_points];
				
				int vx, vy;
				for(int i=0;i<n_points;++i)
				{
					polygon->getVertex(i, &vx, &vy, true);
					points_x[i] = float32(vx)/float32(10);
					points_y[i] = float32(vy)/float32(10);
				}
				  
				b2Polygon *pgon = new b2Polygon(points_x, points_y, n_points);
					
				b2BodyDef *bodyDef = new b2BodyDef();
				bodyDef->userData = thing; // So you can always get the thing pointer from a b2body
				
				int posx, posy;
				polygon->getPosition(&posx, &posy);
				bodyDef->position.Set(float32((float)posx/10.0f), float32((float)posy/10.0f));
				bodyDef->angle = polygon->getRotation();
				
				b2PolygonDef* deleteMe = DecomposeConvexAndAddTo(b2world, pgon, bodyDef, polyDef);
				
				b2Body* body = 0;
				if(deleteMe)
				{
					body = b2world->Create(bodyDef);
					polygon->setb2Body(body); // So you can always get the b2body pointer from a thing
					delete[] deleteMe;
				}
				
				delete polyDef;
				delete bodyDef;
				  
				delete points_x;
				delete points_y;
				
				delete pgon;
				
				if(!deleteMe)
				{
					printf("Convex decomposition failed!\n");
					return false;
				}
				
				if(!body)
				{
					printf("Making the body failed!\n");
					return false;
				}
			}
			break;
			
			case Thing::Circle:
			{
				Circle *circle = (Circle*)thing;
				
				b2CircleDef *circledef = new b2CircleDef();
				circledef->radius = float32(circle->getRadius()) / float32(10);
				
				if( (circle->getType() == Thing::Solid) || (circle->getType() == Thing::NonSolid) )
					circledef->density = 0.0f;
				else
					circledef->density = DEFAULT_DENSITY;
				
				circledef->friction = DEFAULT_FRICTION;
				circledef->restitution = DEFAULT_RESTITUTION;
				
				b2BodyDef *bodydef = new b2BodyDef();
				bodydef->userData = thing;
				
				int posx, posy;
				circle->getPosition(&posx, &posy);
				bodydef->position.Set(float32((float)posx/10.0f), float32((float)posy/10.0f));
				bodydef->angle = circle->getRotation();
				
				bodydef->AddShape(b2world->Create(circledef));
				
				b2Body* body = b2world->Create(bodydef);
				
				circle->setb2Body(body);
				
				delete bodydef;
				delete circledef;
				
				if(!body)
					printf("Making the body failed!\n");
			}
			break;
			
			default:
				printf("Cannot make unknown shape type!\n");
				break;
		}
	}
	
	return true;
}

// Makes a thing unphysical, i.e. removes it from the simulation
void World::makeUnphysical(Thing *thing)
{
	if(thing->getShape() == Thing::Pin)
	{
		Pin *p = (Pin*)thing;
		if(p->getb2Joint() == 0)
			return;
		
		//b2world->DestroyJoint(p->getb2Joint()); // Is removed automatically
		p->setb2Joint(0);
	}
	else
	{
		if(thing->getb2Body() == 0)
			return;
		
		b2world->Destroy(thing->getb2Body());
		thing->setb2Body(0);
	}
	
	b2world->Step(float32(0), 0);
}

int World::getNThings(void)
{
	return n_things;
}

Thing *World::getThing(int index)
{
	return things[index];
}


void World::getDimensions(int *_width, int *_height)
{
	*_width = width;
	*_height = height;
}

void World::getGravity(float32 *grav_x, float32 *grav_y)
{
	*grav_x = gravity_x;
	*grav_y = gravity_y;
}

void World::setGravity(float32 grav_x, float32 grav_y)
{
	gravity_x = grav_x;
	gravity_y = grav_y;
	
	b2Vec2 grav(gravity_x, gravity_y);
	b2world->SetGravity(grav);
}

void World::step(float32 timestep)
{
	// TODO: Delete objects that move out of the screen
	b2world->Step(timestep, ITERATIONS);
}

void World::reset(void)
{
	// TODO: Delete dynamically created things
	
	destroyJointDummy();
	makeJointDummy();
	
	// Reset all things to their original position / rotation
	for(int i=0;i<n_things;++i)
	{
		makeUnphysical(things[i]);
		things[i]->reset();
		makePhysical(things[i]);
	}
}

// ===================================== PRIVATE ===================================== //

void World::initPhysics(void)
{
	// Define the size of the world. Simulation will still work
	// if bodies reach the end of the world, but it will be slower.
	b2AABB *worldAABB = new b2AABB();
	float w = (float)width/10.0f;
	float h = (float)height/10.0f;
	worldAABB->minVertex.Set(-w, -h);
	worldAABB->maxVertex.Set(2*w, 2*h);
	
	// Define the gravity vector.
	b2Vec2 grav(gravity_x, gravity_y);

	// Do we want to let bodies sleep?
	bool doSleep = false;

	// Construct a world object, which will hold and simulate the rigid bodies.
	
	b2world = new b2World(*worldAABB, grav, doSleep);

	delete worldAABB;
}

void World::makeJointDummy(void)
{
	b2PolygonDef *gd = new b2PolygonDef();
    gd->SetAsBox(0.1f, 0.1f);
    gd->density = 0;
    
    b2BodyDef *bd = new b2BodyDef();
    bd->position.Set(-10.0f, -10.0f);
    bd->AddShape(b2world->Create(gd));
    bgbody = b2world->Create(bd);
    
    delete gd;
    delete bd;
}

void World::destroyJointDummy(void)
{
	b2world->Destroy(bgbody);
	bgbody = 0;
}
