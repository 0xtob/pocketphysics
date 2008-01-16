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
		pinpos.x = Fixed(px) / Fixed(10);
		pinpos.y = Fixed(py) / Fixed(10);
		//jointDef->anchorPoint = pinpos;
		jointDef->localAnchor1 = pinpos;
		
		b2Joint* joint = b2world->CreateJoint(jointDef);
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
	  
	b2Shape *shape[16];
	b2Body *body;
	
	int count = b2world->Query(*touchAABB, shape, 16);
	int returncount = 0;
	for(int i=0;(i<count)&&(returncount<n);++i)
	{
		body = shape[i]->GetBody();
		
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
	return returncount;
}

void World::makePhysical(Thing *thing)
{
	if(thing->getShape() == Thing::Pin)
	{
		Pin *p = (Pin*)thing;
		pin(p, p->getThing(1), p->getThing(2));
	}
	else
	{
		if(thing->getb2Body() != 0)
			return;
		
		if(thing->getType() == Thing::NonSolid)
			return;
		
		switch(thing->getShape())
		{
			case(Thing::Polygon):
			{
				Polygon *polygon = (Polygon*)thing;
				
				//b2PolyDef *polyDef = new b2PolyDef();
				b2PolygonDef *polyDef = new b2PolygonDef();
				
				int n_points = polygon->getNVertices();
				polyDef->vertexCount = n_points;
				  
				if( (polygon->getType() == Thing::Solid) || (polygon->getType() == Thing::NonSolid) )
					polyDef->density = 0.0f;
				else
					polyDef->density = DEFAULT_DENSITY;
				  
				polyDef->friction = DEFAULT_FRICTION;
				polyDef->restitution = DEFAULT_RESTITUTION;
				
				//polyDef->categoryBits = 0x0002;
				//polyDef->maskBits = 0x0002;
				
				Fixed *points_x = new Fixed[n_points];
				Fixed *points_y = new Fixed[n_points];
				
				int vx, vy;
				for(int i=0;i<n_points;++i)
				{
					polygon->getVertex(i, &vx, &vy, true);
					points_x[i] = Fixed(vx)/Fixed(10);
					points_y[i] = Fixed(vy)/Fixed(10);
				}
				  
				b2Polygon *pgon = new b2Polygon(points_x, points_y, n_points);
					
				b2BodyDef *bodyDef = new b2BodyDef();
				bodyDef->userData = thing; // So you can always get the thing pointer from a b2body
				
				int posx, posy;
				polygon->getPosition(&posx, &posy);
				bodyDef->position.Set(Fixed((float)posx/10.0f), Fixed((float)posy/10.0f));
				bodyDef->rotation = polygon->getRotation();
				
				b2PolyDef* deleteMe = DecomposeConvexAndAddTo(pgon, bodyDef, polyDef);
				
				b2Body* body = b2world->CreateBody(bodyDef);
				
				polygon->setb2Body(body); // So you can always get the b2body pointer from a thing
				
				delete[] deleteMe;
				delete polyDef;
				delete bodyDef;
				  
				delete points_x;
				delete points_y;
					
				delete pgon;
				
				if(!body)
					printf("Making the body failed!\n");
			}
			break;
			
			case Thing::Circle:
			{
				Circle *circle = (Circle*)thing;
				
				b2CircleDef *circledef = new b2CircleDef();
				circledef->radius = Fixed(circle->getRadius()) / Fixed(10);
				
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
				bodydef->position.Set(Fixed((float)posx/10.0f), Fixed((float)posy/10.0f));
				bodydef->rotation = circle->getRotation();
				
				bodydef->AddShape(circledef);
				
				b2Body* body = b2world->CreateBody(bodydef);
				
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
		
		b2world->DestroyBody(thing->getb2Body());
		thing->setb2Body(0);
	}
	
	b2world->Step(Fixed(0), 0);
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

void World::getGravity(Fixed *grav_x, Fixed *grav_y)
{
	*grav_x = gravity_x;
	*grav_y = gravity_y;
}

void World::setGravity(Fixed grav_x, Fixed grav_y)
{
	gravity_x = grav_x;
	gravity_y = grav_y;
	
	b2Vec2 grav(gravity_x, gravity_y);
	b2world->setGravity(grav);
}

void World::step(Fixed timestep)
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
	b2BoxDef *gd = new b2BoxDef();
    gd->type = e_boxShape;
    gd->extents.Set(0.1f, 0.1f);
    gd->density = 0;
    //gd->categoryBits = 0x0004;
    //gd->maskBits = 0x0004;
    
    b2BodyDef *bd = new b2BodyDef();
    bd->position.Set(-10.0f, -10.0f);
    //bd->position.Set(30.0f, 30.0f);
    bd->AddShape(gd);
    bgbody = b2world->CreateBody(bd);
    
    delete gd;
    delete bd;
}

void World::destroyJointDummy(void)
{
	b2world->DestroyBody(bgbody);
	bgbody = 0;
}
