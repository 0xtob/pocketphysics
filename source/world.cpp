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

#include "world.h"

#include "polygon.h"
#include "Circle.h"

#include "tools.h"
//#include "defines.h"

#include <nds.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include "tinyxml.h"

World::World(int _width, int _height, bool allow_sleep):
	n_things(0), width(_width), height(_height), gravity_x(0), gravity_y(DEFAULT_GRAVITY), mouse_joint(0), make_unphysical_list_length(0)
{
	initPhysics(allow_sleep);
	
	makeJointDummy();
}

World::~World()
{
	
}

bool World::add(Thing *thing)
{
	if(n_things == MAX_THINGS)
		return false;
	
	things[n_things] = thing;
	n_things++;
	
	return true;
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
		printf("pinning %x to bg\n", (uint)body1);
	}
	else
	{
		body2 = thing2->getb2Body();
		printf("pinning %x to %x\n", (uint)body1, (uint)body2);
	}
	
	if(body1 != 0)
	{
		b2RevoluteJointDef *jointDef = new b2RevoluteJointDef();
		//jointDef->body1 = body1;
		//jointDef->body2 = body2;
		jointDef->userData = pin;
		int px, py;
		pin->getPosition(&px, &py);
		b2Vec2 pinpos;
		pinpos.x = float32(px) / PIXELS_PER_UNIT;
		pinpos.y = float32(py) / PIXELS_PER_UNIT;
		
		//jointDef->localAnchor1 = pinpos;
		//jointDef->localAnchor2 = pinpos;
		//jointDef->SetInWorld(pinpos);
		jointDef->Initialize(body1, body2, pinpos);
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

// Attaches a mouse joint to a thing for dragging it around
void World::grab(Thing *thing, int x, int y)
{
	if(thing->getShape() == Thing::Pin)
		return;
	
	b2Body *body = thing->getb2Body();
	
	if(!body)
		return;
	
	b2MouseJointDef md;
	md.body1 = bgbody;
	md.body2 = body;
	b2Vec2 p((float32)x/PIXELS_PER_UNIT, (float32)y/PIXELS_PER_UNIT);
	md.target = p;
	md.maxForce = (float32)1000 * body->m_mass;
	mouse_joint = (b2MouseJoint*)b2world->CreateJoint(&md);
	body->WakeUp();
}

// Drags the grabbed thing around
void World::drag(int x, int y)
{	
	if(mouse_joint)
    {
		b2Vec2 p((float32)x/PIXELS_PER_UNIT, (float32)y/PIXELS_PER_UNIT);
		mouse_joint->SetTarget(p);
    }
}

// Lets go off the grabbed thing
void World::letGo(void)
{
	if(mouse_joint)
    {
		b2world->DestroyJoint(mouse_joint);
		mouse_joint = NULL;
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

int World::getThingsAt(int x, int y, Thing ** res_things, int n, bool include_pins)
{
	b2AABB *touchAABB = new b2AABB();
	touchAABB->lowerBound.Set((float32)(x-2)/PIXELS_PER_UNIT, (float32)(y-2)/PIXELS_PER_UNIT);
	touchAABB->upperBound.Set((float32)(x+2)/PIXELS_PER_UNIT, (float32)(y+2)/PIXELS_PER_UNIT);
	b2Vec2 point = b2Vec2(x/PIXELS_PER_UNIT, y/PIXELS_PER_UNIT);
	
	b2Shape *shape[16];
	b2Body *body;
	
	int returncount = 0;
	
	if(include_pins)
	{
		// Check for pins
		for(int i=0; (i<n_things)&&(returncount<n); ++i)
		{
			Thing *thing = things[i];
			
			if(thing->getShape() == Thing::Pin)
			{
				int px, py;
				thing->getPosition(&px, &py);

				// If the query position is in a 7x7 square around the pin center, the pin is hit
				if((x>=px-3)&&(x<=px+3)&&(y>=py-3)&&(y<=py+3))
				{
					res_things[returncount] = thing;
					returncount++;
				}
			}
		}
	}
	
	// Check for other objects
	int count = b2world->Query(*touchAABB, shape, 16);
	
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
				if(res_things[b]->getb2Body() == body)
					alreadythere = true;
			
			if(!alreadythere)
			{
				res_things[returncount] = (Thing*)body->GetUserData();
				returncount++;
			}
		}
	}
	return returncount;
}

Thing **World::getConnectedThings(Thing *thing, int *n)
{
	Thing **res = (Thing**)malloc(sizeof(Thing*) * MAX_THINGS);
	(*n) = 0;
	
	res[0] = thing;
	(*n)++;
	
	// For each queue item, add everything that's connected to it
	int queue_next = 0;
	while(queue_next < (*n))
	{
		Thing *t = res[queue_next];
		if(t->getShape() == Thing::Pin)
		{
			// If it's a pin, add all connected objects
			Pin *p = (Pin*)t;
			Thing *pinthings[2] = {p->getThing(1), p->getThing(2)};
			
			// Check if they aren't already in the list (which happens in case of a cycle)
			for(int pt=0; pt<2; ++pt)
			{
				bool found = false;
				for(int q=0; q < (*n); ++q)
				{
					if(res[q] == pinthings[pt])
						found = true;
				}
				if((!found)&&(pinthings[pt]!=0))
				{
					res[*n] = pinthings[pt];
					(*n)++;
				}
			}
		}
		else
		{
			// If it's not a pin, add all pins connected to it
			for(int i=0; i<n_things; ++i)
			{
				if(things[i]->getShape() == Thing::Pin)
				{
					Pin *p = (Pin*)things[i];
					if( (p->getThing(1) == t) || (p->getThing(2) == t) ) 
					{
						// Check if the pin is already in the list
						bool found = false;
						for(int q=0; q < (*n); ++q)
						{
							if(res[q] == p)
								found = true;
						}
						if(!found)
						{
							res[*n] = p;
							(*n)++;
						}
					}
				}
			}
		}
		queue_next++;
	}
	
	return res;
}

bool World::makePhysical(Thing *thing)
{
	if(thing->getShape() == Thing::Pin)
	{
		Pin *p = (Pin*)thing;
		if( p->getThing(1) == 0 )
			return false;
		
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
				
				b2BodyDef *bodyDef = new b2BodyDef();
				bodyDef->userData = thing; // So you can always get the thing pointer from a b2body
				
				int posx, posy;
				polygon->getPosition(&posx, &posy);
				bodyDef->position.Set(float32(float32(posx)/PIXELS_PER_UNIT), float32(float32(posy)/PIXELS_PER_UNIT));
				bodyDef->angle = polygon->getRotation();
				
				b2Body* body = 0;
				if(thing->getType() == Thing::Dynamic)
					body = b2world->CreateDynamicBody(bodyDef);
				else
					body = b2world->CreateStaticBody(bodyDef);
				
				// Closed polygon: Convert to a set of convex polygons and add them to the body
				if(polygon->getClosed() == true)
				{
					printf("making closed poly\n");
					
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
						points_x[i] = float32(vx)/PIXELS_PER_UNIT;
						points_y[i] = float32(vy)/PIXELS_PER_UNIT;
					}
					
					b2Polygon *pgon = new b2Polygon(points_x, points_y, n_points);
					
					//b2Polygon *tracedPgon = TraceEdge(pgon);
					b2PolygonDef* deleteMe = DecomposeConvexAndAddTo(b2world, pgon, body, polyDef);
					//b2PolygonDef* deleteMe = DecomposeConvexAndAddTo(b2world, tracedPgon, bodyDef, polyDef);
					//delete tracedPgon;
					
					if(deleteMe)
					{
						polygon->setb2Body(body); // So you can always get the b2body pointer from a thing
						delete[] deleteMe;
					}
					
					delete polyDef;
					  
					delete points_x;
					delete points_y;
					
					delete pgon;
					
					if(!deleteMe)
					{
						printf("Convex decomposition failed!\n");
						b2world->DestroyBody(body);
						return false;
					}
					
					if(!body)
					{
						printf("Making the body failed!\n");
						return false;
					}
				}
				else
				// Open Polygon: Convert to a set of lines (i.e. thin boxes) and add them to the body
				//
				// Procedure:
				// Get a polygon segment, calculate the normal, multiply it by 4, add that to the vertices,
				// add the resulting vertices to the polygon.
				{
					printf("making open poly\n");
					
					for(int i=0; i<polygon->getNVertices()-1; ++i)
					{
						b2PolygonDef *polyDef = new b2PolygonDef();
						
						if( (polygon->getType() == Thing::Solid) || (polygon->getType() == Thing::NonSolid) )
							polyDef->density = 0.0f;
						else
							polyDef->density = DEFAULT_DENSITY;
						
						polyDef->friction = DEFAULT_FRICTION;
						polyDef->restitution = DEFAULT_RESTITUTION;
						
						int x1, y1, x2, y2;
						
						polygon->getVertex(i, &x1, &y1, true);
						polygon->getVertex(i+1, &x2, &y2, true);
						
						int odx = y1 - y2;
						int ody = x2 - x1;
						//int len = mysqrt(odx*odx + ody*ody);
						int len = sqrt64(odx*odx + ody*ody);
						//printf("%d ", len);
						odx = (6 * (odx<<8) / len)>>8; // using 24.8 fixed point for normal calculation
						ody = (6 * (ody<<8) / len)>>8;
						printf("%d: %d - %d %d (%d %d, %d %d)\n", i, len, odx, ody, x1, y1, x2, y2);
						
						polyDef->vertexCount = 4;
						polyDef->vertices[0].Set(float32(x1)/PIXELS_PER_UNIT, float32(y1)/PIXELS_PER_UNIT);
						polyDef->vertices[1].Set(float32(x2)/PIXELS_PER_UNIT, float32(y2)/PIXELS_PER_UNIT);
						polyDef->vertices[2].Set(float32(x2+odx)/PIXELS_PER_UNIT, float32(y2+ody)/PIXELS_PER_UNIT);
						polyDef->vertices[3].Set(float32(x1+odx)/PIXELS_PER_UNIT, float32(y1+ody)/PIXELS_PER_UNIT);
						
						// Print vertices
						//for(int v=0; v<4; ++v)
						//	printf("%f,%f\n", (float)polyDef->vertices[v].x, (float)polyDef->vertices[v].y);
						//printf("\n");
						
						body->CreateShape(polyDef);
						body->SetMassFromShapes();
						//bodyDef->AddShape(b2world->Create(polyDef));
						
						delete polyDef;
					}
					
					polygon->setb2Body(body); // So you can always get the b2body pointer from a thing
				}
				
				delete bodyDef;
			}
			break;
			
			case Thing::Circle:
			{
				Circle *circle = (Circle*)thing;
				
				b2CircleDef *circledef = new b2CircleDef();
				circledef->radius = float32(circle->getRadius()) / PIXELS_PER_UNIT;
				
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
				bodydef->position.Set(float32((float)posx/PIXELS_PER_UNIT), float32((float)posy/PIXELS_PER_UNIT));
				bodydef->angle = circle->getRotation();
				
				b2Body* body = 0;
				
				if(thing->getType() == Thing::Dynamic)
					body = b2world->CreateDynamicBody(bodydef);
				else
					body = b2world->CreateStaticBody(bodydef);
				
				body->CreateShape(circledef);
				body->SetMassFromShapes();
				
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
		
		// Store the address, set the joint pointer in the pin to 0, then destroy the joint.
		// This way, it's thread-safe (i.e. we won't get difficulties when the drawing function
		// calls getPostition of the point.
		b2Joint* j = p->getb2Joint();
		p->setb2Joint(0);
		b2world->DestroyJoint(j);
	}
	else
	{
		if(thing->getb2Body() == 0)
			return;
		
		b2world->DestroyBody(thing->getb2Body());
		thing->setb2Body(0);
	}
	
	// Commented this out because I have no idea why I did this.
	//b2world->Step(float32(0), 0);
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
	
	makeMarkedThingsUnphysical();
	
	// wrapping test code
	/*
	for(int i=0;i<n_things;++i)
	{
		int px, py;
		bool changed = false;
		things[i]->getPosition(&px, &py);
		if(py > height)
		{
			py -= height;
			changed = true;
		}
		if(px > width)
		{
			px -= width;
			changed = true;
		}
		if(changed)
			things[i]->setPosition(px, py);
	}
	*/
	//
}

void World::reset(bool allow_sleep)
{
	// TODO: Delete dynamically created things
	
	destroyJointDummy();
	
	// Reset all things to their original position / rotation
	for(int i=0;i<n_things;++i)
		makeUnphysical(things[i]);
	
	delete b2world;
	initPhysics(allow_sleep);
	makeJointDummy();
	
	for(int i=0;i<n_things;++i)
		things[i]->reset();
	
	for(int i=0;i<n_things;++i)
		makePhysical(things[i]);
}

void World::save(const char *filename, char *thumbnail)
{
	// Since 0.5, data is stored in data/pocketphysics/sketches instead of pocketphysics/sketches
	// backwards compatibility for the old path is still provided
	bool use_data_dir = true;
	if(opendir("pocketphysics/sketches"))
		use_data_dir = false;
	else
	{
		if(!opendir("data"))
		{
			mkdir("data", 777);
		}
		if(!opendir("data/pocketphysics"))
		{
			mkdir("data/pocketphysics", 777);
		}
		if(!opendir("data/pocketphysics/sketches"))
		{
			mkdir("data/pocketphysics/sketches", 777);
		}
		if(!opendir("data/pocketphysics/sketches"))
		{
			printf("diropen failed!\n");
			return;
		}
	}
	
	// Header
	
	TiXmlDocument doc;
	TiXmlDeclaration *decl = new TiXmlDeclaration( "1.0", "", "" );
	doc.LinkEndChild( decl );
	
	// Root node
	TiXmlElement *ppsketchelement = new TiXmlElement( "ppsketch" );
	doc.LinkEndChild(ppsketchelement);
	
	
	Thing **idtable = (Thing**)malloc(sizeof(Thing*) * (n_things+1));
	
	// Version
	TiXmlElement *versionelement = new TiXmlElement( "creator" );
	TiXmlText *versiontext = new TiXmlText( "Pocket Physics 0.6" );
	versionelement->LinkEndChild(versiontext);	
	ppsketchelement->LinkEndChild(versionelement);
	
	// Author
	char authorname[10] = {0};
	for(int i=0; i<10; ++i) {
		authorname[i] = PersonalData->name[i] & 0xFF;
	}
	
	TiXmlElement *authorelement = new TiXmlElement( "author" );
	TiXmlText *authortext = new TiXmlText( authorname );
	authorelement->LinkEndChild(authortext);	
	ppsketchelement->LinkEndChild(authorelement);

	// World
	TiXmlElement *worldelement = new TiXmlElement( "world" );
	//TODO: Swap x and y axis!
	worldelement->SetDoubleAttribute("gravity_x", DEFAULT_GRAVITY);
	worldelement->SetDoubleAttribute("gravity_y", 0.0);
	
	for(int i=0; i<n_things; ++i)
	{
		int id = i+1;
		Thing *thing = things[i];
		idtable[id] = thing;
		
		TiXmlElement *thingelement = thing->toXML();
		
		if(thing->getShape() == Thing::Pin)
		{
			Pin *pin = (Pin*)thing;
			
			for(int j=0; j<2; ++j)
			{
				int pinned_id = 0;
				
				Thing *pinned_thing = pin->getThing(j+1);
				if(pinned_thing)
				{
					while( (idtable[pinned_id] != pinned_thing) && (pinned_id < n_things+1) )
						++pinned_id;
					
					if(pinned_id == n_things+1)
						printf("Weird: Pinned thing not found!\n");
				}
				
				TiXmlElement *pinnedthingelement = new TiXmlElement("pinned_thing");
				pinnedthingelement->SetAttribute("id", pinned_id);
				thingelement->LinkEndChild(pinnedthingelement);
			}
		}
		else
		{
			thingelement->SetAttribute("id", id);
		}
		worldelement->LinkEndChild(thingelement);
	}
	
	ppsketchelement->LinkEndChild(worldelement);
	
	free(idtable);
	
	// Thumbnail
#ifndef SCLITE
	TiXmlElement *imageelement = new TiXmlElement( "image" );
	TiXmlText *imagetext = new TiXmlText( thumbnail );
	imageelement->LinkEndChild(imagetext);
	
	ppsketchelement->LinkEndChild(imageelement);
#endif
	// Save
	
	char *f = (char*)calloc(1, 255);
	if(use_data_dir)
		sprintf(f, "%s/%s", "data/pocketphysics/sketches", filename);
	else
		sprintf(f, "%s/%s", "pocketphysics/sketches", filename);
	doc.SaveFile(f);
	free(f);
}

bool World::load(const char *filename)
{
	const char *datadir = "";
	if(opendir("data/pocketphysics/sketches")) {
		datadir = "data/pocketphysics/sketches";
	} else if(opendir("pocketphysics/sketches")) {
		datadir = "pocketphysics/sketches";
	}
	
	// Clear
	while(n_things > 0)
	{
		Thing *t = getThing(0);
		remove(t);
		delete t;
	}
	
	// Load
	char *f = (char*)calloc(1, 255);
	sprintf(f, "%s/%s", datadir, filename);
	TiXmlDocument doc(f);
	free(f);
	
	if( !doc.LoadFile() )
		return false;
	
	TiXmlNode *root;
	if( doc.FirstChildElement("ppsketch") != 0 )
		root = doc.FirstChildElement("ppsketch");
	else // No root element => old version
		root = &doc;
		
	// World
	TiXmlElement *worldelement = root->FirstChildElement("world");
	float gx=DEFAULT_GRAVITY, gy=0.0;
	worldelement->QueryFloatAttribute("gravity_x", &gx);
	worldelement->QueryFloatAttribute("gravity_y", &gy);
	
	// Due to a stupid mistake of mine, the y and x components of gravity are swapped
	// in the .pp file format.
	gravity_x = gy;
	gravity_y = gx;
	setGravity(gravity_x, gravity_y);
	
	Thing **id_table = (Thing**)calloc(1, sizeof(Thing*)*MAX_THINGS);
	
	for(TiXmlElement *thingelement = worldelement->FirstChildElement(); thingelement;
		thingelement=thingelement->NextSiblingElement())
	{
		int id = 0;
		thingelement->QueryIntAttribute("id", &id);
		
		Thing *thing = 0;
		if(strcmp("circle", thingelement->Value()) == 0)
		{
			printf("%d: circle\n", id);
			thing = new Circle(thingelement);
		}
		else if(strcmp("polygon", thingelement->Value()) == 0)
		{
			thing = new Polygon(thingelement);
		}
		else if(strcmp("pin", thingelement->Value()) == 0)
		{
			thing = new Pin(thingelement);
			Pin *pin_ = (Pin*)thing;
			
			int pinned_ids[2] = {0, 0};
			
			TiXmlElement *pinned_element = thingelement->FirstChildElement();
			pinned_element->QueryIntAttribute("id", &pinned_ids[0]);
			pinned_element = pinned_element->NextSiblingElement();
			pinned_element->QueryIntAttribute("id", &pinned_ids[1]);
			
			for(int j=0; j<2; ++j)
			{
				if(pinned_ids[j] != 0)
					pin_->setThing(j+1, id_table[pinned_ids[j]]);
			}
		}
		else
		{
			printf("Invalid object type!\n");
			delete id_table;
			return false;
		}
		id_table[id] = thing;
		
		add(thing);
		makePhysical(thing);
	}
	printf("Loading finished\n");
	
	delete id_table;
	
	reset(b2world->m_allowSleep);
	
	return true;
}

void World::markForMakingUnphysical(Thing *thing)
{
	make_unphysical_list[make_unphysical_list_length] = thing;
	make_unphysical_list_length++;
}

// ===================================== PRIVATE ===================================== //

void World::makeMarkedThingsUnphysical(void)
{
	for(int i=0; i<make_unphysical_list_length; ++i)
	{
		makeUnphysical(make_unphysical_list[i]);
		make_unphysical_list[i]->hide(); // Also, hide it
	}
	make_unphysical_list_length = 0;
}

void World::initPhysics(bool allow_sleep)
{
	// Define the size of the world. Simulation will still work
	// if bodies reach the end of the world, but it will be slower.
	b2AABB *worldAABB = new b2AABB();
	float32 w = (float32)width/PIXELS_PER_UNIT;
	float32 h = (float32)height/PIXELS_PER_UNIT;
	worldAABB->lowerBound.Set(-256/PIXELS_PER_UNIT, -192/PIXELS_PER_UNIT);
	worldAABB->upperBound.Set(w+256/PIXELS_PER_UNIT, h+192/PIXELS_PER_UNIT);
	
	// Define the gravity vector.
	b2Vec2 grav(gravity_x, gravity_y);

	// Construct a world object, which will hold and simulate the rigid bodies.
	
	b2world = new b2World(*worldAABB, grav, allow_sleep);

	destruction_listener = new PPDestructionListener();
	b2world->SetListener(destruction_listener);
	boundary_listener = new PPBoundaryListener(this);
	b2world->SetListener(boundary_listener);
	
	delete worldAABB;
}

void World::makeJointDummy(void)
{
	b2PolygonDef *gd = new b2PolygonDef();
    gd->SetAsBox(10*UNITS_PER_PIXEL, 10*UNITS_PER_PIXEL);
    gd->density = 0;
    
    b2BodyDef *bd = new b2BodyDef();
    bd->position.Set(-100/PIXELS_PER_UNIT, -100/PIXELS_PER_UNIT);
    
    bgbody = b2world->CreateStaticBody(bd);
    
    bgbody->CreateShape(gd);
    
    delete gd;
    delete bd;
}

void World::destroyJointDummy(void)
{
	b2world->DestroyBody(bgbody);
	bgbody = 0;
}
