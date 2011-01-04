#include "canvas.h"

#include "sample.h"
#include "../../generic/command.h"
#include "tools.h"

#include "thing.h"
#include "polygon.h"
#include "Circle.h"
#include "Pin.h"

#include "crayon_png.h"

#define MAX_POINTS              64
#define DRAW_MIN_POINT_DIST     7 //15
#define DRAW_POLYGON_CLOSE_DIST	10 //15
#define DRAW_NEW_POINT_ANGLE    20

#define COL_SOLID	RGB15(31,0,0)
#define COL_DYNAMIC	RGB15(0,0,31)
#define COL_PIN		RGB15(0,31,0)
#define SHAPE_ALPHA	20
#define COL_SOLID_HL    RGB15(31,15,15)
#define COL_DYNAMIC_HL  RGB15(15,15,31)
#define COL_PIN_HL      RGB15(15,31,15)


Canvas::Canvas(World *_world):
	world(_world), drawing(false), pinthing1(0), pinthing2(0), highlightthing(0), simulation_mode(false)
{
	crayon = ulLoadImageFilePNG((const char*)crayon_png, (int)crayon_png_size, UL_IN_VRAM, UL_PF_PAL3_A5);
}

void Canvas::draw(void)
{
	int n_things = world->getNThings();
	for(int i=0;i<n_things;++i)
	{
		Thing *thing = world->getThing(i);
		
		if(thing->isInvisible()) // Don't draw invisible things
			continue;
			
		u16 col=0;
		if( (thing->getShape() == Thing::Polygon) || (thing->getShape() == Thing::Circle) )
		{
			if(thing->getType() == Thing::Dynamic)
			{
				if( (thing == pinthing1) || (thing == pinthing2) || (thing == highlightthing) )
					col = COL_DYNAMIC_HL;
				else
					col = COL_DYNAMIC;
			}
			else if(thing->getType() == Thing::Solid)
				if(thing == highlightthing)
					col = COL_SOLID_HL;
				else
					col = COL_SOLID;
			else if(thing->getType() == Thing::NonSolid)
				if(thing == highlightthing)
					col = COL_PIN_HL;
				else
					col = COL_PIN;
		}
		
		switch (thing->getShape())
		{
			case Thing::Polygon:
			{
				Polygon *polygon = (Polygon*)thing;
				
				int n_vertices = polygon->getNVertices();
				int lastx=0, lasty=0;
				int x,y;
				
				for(int i=0;i<n_vertices;++i)
				{
					polygon->getVertex(i, &x, &y);
					
					if(i>0)
						drawLine(col, lastx, lasty, x, y);
					
					lastx=x;
					lasty=y;
				}
				
				// Closing line if
				// - user isn't just drawing
				// and
				// - it's a closed polygon
				if( polygon->getClosed() && (! (drawing && (pen_mode == pmPolygon) && (polygon == currentthing) ) ) )
				{
					polygon->getVertex(0, &x, &y);
					drawLine(col, lastx, lasty, x, y);
				}
			}
			break;
				
			case Thing::Circle:
			{
				Circle *circle = (Circle*)thing;
				
				int px, py, radius;
				circle->getPosition(&px, &py);
				radius = circle->getRadius();
				
				int angle = (int)(circle->getRotation() * 5120 / (float32)63) % 512;
				if(angle < 0)
					angle = 512 + angle;
				
				//         (  radius<<12  * cos(initial rotation)<<12 ) >> 12
				int vecx = ( (radius<<6) * (int)COS_bin[(angle * 512 / 360) % 512] ) >> 6;
				int vecy = ( (radius<<6) * (int)SIN_bin[(angle * 512 / 360) % 512] ) >> 6;
				
				int lastx=0, lasty=0;
				int n_segments = max(5, min(16, radius*radius/20));
				for(int i=0;i<=n_segments;++i)
				{
					vecx = ( (radius<<6) * (int)COS_bin[(angle + 512 * i / n_segments) % 512]) >> 6;
					vecy = ( (radius<<6) * (int)SIN_bin[(angle + 512 * i / n_segments) % 512]) >> 6;
					
					int vx = px + (vecx>>12);
					int vy = py + (vecy>>12);
					
					if(i>0)
						drawLine(col, lastx, lasty, vx, vy);
					
					lastx = vx;
					lasty = vy;
				}
			}
			break;
			
			case Thing::Pin:
			{
				if(simulation_mode)
					break;
				int px, py;
				Pin *pin = (Pin*)thing;
				pin->getPosition(&px, &py);
				u16 pincol;
				if(pin == highlightthing)
					pincol = COL_PIN_HL;
				else
					pincol = COL_PIN;
				drawLine(pincol, px-4, py-4, px+4, py+4);
				drawLine(pincol, px-4, py+4, px+4, py-4);
			}
			break;
			
			default:
				printf("Unknown shape!\n");
				break;
		}
	}
}

void Canvas::setPenMode(PenMode _pen_mode)
{
	pen_mode = _pen_mode;
}

void Canvas::setObjectMode(ObjectMode _object_mode)
{
	object_mode = _object_mode;
}

void Canvas::penDown(int x, int y)
{
	Thing::Type type = Thing::Dynamic;
	if( (pen_mode == pmPolygon) || (pen_mode == pmBox) || (pen_mode == pmCircle) )
	{
		type = Thing::Dynamic;
		if(object_mode == omDynamic)
			type = Thing::Dynamic;
		else if(object_mode == omSolid)
			type = Thing::Solid;
		else if(object_mode == omNonSolid)
			type = Thing::NonSolid;
	}
	
	switch(pen_mode)
	{
		case pmNormal:
		{
			
		}
		break;
	
		case pmPolygon:
		{
			drawing = true;
			
			Polygon *poly = new Polygon(type, Thing::User);
			poly->addVertex(x, y);
			
			if(world->add(poly))
				currentthing = poly;
			else
				drawing = false;
		}
		break;
		
		case pmBox:
		{
			drawing = true;
			
			Polygon *poly = new Polygon(type, Thing::User);
			poly->setClosed(true);
			poly->addVertex(x, y);
			poly->addVertex(x+1, y);
			poly->addVertex(x+1, y+1);
			poly->addVertex(x, y+1);
			
			if(world->add(poly))
				currentthing = poly;
			else
				drawing = false;
		}
		break;
		
		case pmCircle:
		{
			drawing = true;
			
			Circle *circle = new Circle(type, Thing::User);
			circle->setPosition(x, y);
			
			if(world->add(circle))
				currentthing = circle;
			else
				drawing = false;
		}
		break;
		
		case pmPin:
		{			
			// We need some object at this position
			Thing *things[2] = {0, 0};
			int count = world->getThingsAt(x, y, things, 2, false);
			
			if(count > 0)
				pinthing1 = things[0];
			if(count > 1)
				pinthing2 = things[1];
			else
				pinthing2 = 0;
			
			drawing = true;
			
			Pin *pin = new Pin(x, y);
			
			if(world->add(pin))
				currentthing = pin;
			else
				drawing = false;
		}
		break;
		
		case pmMove:
		{
			Thing *thing = 0;
			int count = world->getThingsAt(x, y, &thing, 1, false);
			
			if(count > 0)
			{
				highlightthing = thing;

				if(simulation_mode)
					world->grab(thing, x, y);
				else
				{
					move_startx = x;
					move_starty = y;
					
					// Outside simulation mode, if an object is moved,
					// we have to check if it is pinned to other
					// objects and, if so, move them as well.
					move_connected_things = world->getConnectedThings(thing, &move_n_connected_things);
					
					for(int i=0; i<move_n_connected_things; ++i)
					{
						move_connected_things[i]->getPosition(&(move_things_x[i]), &(move_things_y[i]));
					}
				}
				
				drawing = true;
			}
		}
		break;
		
		case pmDelete:
		{
			drawing = true;
			
			/*
			Thing *thing = world->removeAt(x, y);
			if(thing != 0)
			{
				delete thing;
				extern Sample* smp_del;
				CommandPlaySample(smp_del, 48, 255, 0);
			}
			*/
		}
		break;
		
		default:
			break;
	}
}

void Canvas::penMove(int x, int y)
{
	switch(pen_mode)
	{
		case pmNormal:
		{
			
		}
		break;
	
		case pmPolygon:
		{
			if(!drawing)
				return;
			
			Polygon *poly = (Polygon*)currentthing;
			
			// If there's only one point yet, check if we are FIRST_DIST points away from it
			if(poly->getNVertices() == 1)
			{
				int px, py;
				poly->getVertex(0, &px, &py);
				int sqdist = (x - px) * (x - px) + (y - py) * (y - py);
			    
			    if(sqdist >= DRAW_MIN_POINT_DIST * DRAW_MIN_POINT_DIST)
			    {
			    	// Add a point
			    	poly->addVertex(x, y);
			    }
			    
			} else {
			
				// Check if the angle between the current and the last line is greater than NEW_POINT_ANGLE
				int xp, yp, xpp, ypp, n_vertices;
				n_vertices = poly->getNVertices();
				poly->getVertex(n_vertices-1, &xp, &yp);
				poly->getVertex(n_vertices-2, &xpp, &ypp);
				s32 last_dx =  xp - xpp;
				s32 last_dy =  yp - ypp;
				
				s32 dx = x - xp;
				s32 dy = y - yp;
				
				//int last_len = mysqrt(last_dx*last_dx + last_dy*last_dy);
				int last_len = nds_sqrt64(last_dx*last_dx + last_dy*last_dy);
				//int len = mysqrt(dx*dx + dy*dy);
				int len = nds_sqrt64(dx*dx + dy*dy);
				
				double angle = acos((double)(last_dx*dx + last_dy*dy) / (double)( last_len * len )) * 180.0 / M_PI ;
				
				if( len > DRAW_MIN_POINT_DIST)
				{
					if (angle < DRAW_NEW_POINT_ANGLE)
					{
						// Take the point with you
						poly->setVertex(n_vertices-1, x, y);
					}
					else if(n_vertices < MAX_POINTS - 1)
					{
					    // Add a point
						poly->addVertex(x, y);
					}
				}
			}
		}
		break;
		
		case pmBox:
		{
			if(!drawing)
				return;
			
			Polygon *poly = (Polygon*)currentthing;
			
			int cx, cy;
			poly->getVertex(0, &cx, &cy);
			
			// Make sure it's clockwise
			if( ( (x >= cx) && (y >= cy) ) || ( (x <= cx) && (y <= cy) ) )
			{
				poly->setVertex(1, x, cy);
				poly->setVertex(2, x, y);
				poly->setVertex(3, cx, y);
			}
			else
			{
				poly->setVertex(3, x, cy);
				poly->setVertex(2, x, y);
				poly->setVertex(1, cx, y);
			}
		}
		break;
		
		case pmCircle:
		{
			if(!drawing)
				return;
			
			Circle *circle = (Circle*)currentthing;
			
			int px, py;
			circle->getPosition(&px, &py);
			//int radius = mysqrt((x - px)*(x - px) + (y - py)*(y - py));
			int radius = nds_sqrt64((x - px)*(x - px) + (y - py)*(y - py));
			circle->setRadius(radius);
		}
		break;
		
		case pmPin:
		{
			if(!drawing)
				return;
			
			Pin *pin = (Pin*)currentthing;
			
			pin->setPosition(x, y);
			
			Thing *things[2] = {0, 0};
			int count = world->getThingsAt(x, y, things, 2, false);
			
			if(count > 0)
				pinthing1 = things[0];
			if(count > 1)
				pinthing2 = things[1];
			else
				pinthing2 = 0;
			
			// Snap to center of gravity
			if(count > 0)
			{
				int tx, ty;
				things[0]->getCenterOfGravity(&tx, &ty);
				int dx = tx - x;
				int dy = ty - y;
				if(dx*dx + dy*dy <= 25)
				{
					pin->setPosition(tx, ty);
				}
			}
		}
		break;
		
		case pmMove:
		{
			if(!drawing)
				return;
			
			// If simulation is not running we can just set the position. If it is running, we apply a force in
			// the direction of the pen instead
			if(!simulation_mode)
			{
				int dx = x - move_startx;
				int dy = y - move_starty;
				for(int i=0; i<move_n_connected_things; ++i)
				{
					move_connected_things[i]->setPosition(move_things_x[i] + dx, move_things_y[i] + dy);
				}
			}
			else
			{
				world->drag(x,y);
			}
		}
		break;
		
		case pmDelete:
		{
			if(!drawing)
				return;
			
			Thing *thing;
			int count = world->getThingsAt(x, y, &thing, 1, true);
			
			if(count > 0)
				highlightthing = thing;
			else
				highlightthing = 0;
		}
		break;
		
		default:
			break;
	}
}

void Canvas::penUp(int x, int y)
{
	switch(pen_mode)
		{
			case pmNormal:
			{
				
			}
			break;
		
			case pmPolygon:
			{
				if(drawing)
				{
					Polygon *poly = (Polygon*)currentthing;
					
					int n_vertices = poly->getNVertices();
					
					int fx, fy, lx, ly;
					poly->getVertex(0, &fx, &fy);
					poly->getVertex(n_vertices-1, &lx, &ly);
					s16 dx = lx - fx;
					s16 dy = ly - fy;
					
					// Close the polygon if the last point is close to the first
					if(dx*dx+dy*dy < DRAW_POLYGON_CLOSE_DIST*DRAW_POLYGON_CLOSE_DIST)
						poly->setClosed(true);
					else
						poly->setClosed(false);
					
					// Remove last point if it is too close to the first
					if(dx*dx+dy*dy < DRAW_MIN_POINT_DIST*DRAW_MIN_POINT_DIST)
					{
						poly->removeVertex(n_vertices-1);
					}
					
					// Eliminate vertices that are too close
					int cur_x, cur_y, lastx = -1, lasty = -1;
					int i=0;
					int total_len = 0;
					while(i<poly->getNVertices())
					{
						poly->getVertex(i, &cur_x, &cur_y, true);

						if(i>0)
						{
							int dx = cur_x - lastx;
							int dy = cur_y - lasty;
							int len = nds_sqrt64(dx*dx + dy*dy);
							if( len < 5 )
							{
								poly->removeVertex(i);
								printf("removing vtx\n");
								i--; // Don't advance, because the vertex with the next index now has the current index
							}
							else
								total_len += len;
						}

						lastx = cur_x;
						lasty = cur_y;

						i++;
					}
					
					n_vertices = poly->getNVertices();
					
					// Delete if too small
					if( ( n_vertices < 2 ) || (total_len < 10) )
					{
						printf("too small\n");
						world->remove(poly);
						delete poly;
					}
					else
					{
						// Recompute center
						int cx=0, cy=0, vx, vy;
						for(int i=0;i<n_vertices;++i)
						{
							poly->getVertex(i, &vx, &vy);
							cx += vx;
							cy += vy;
						}
						
						cx /= n_vertices;
						cy /= n_vertices;
						
						for(int i=0;i<n_vertices;++i)
						{
							poly->getVertex(i, &vx, &vy);
							poly->setVertex(i, vx-cx, vy-cy);
						}
						
						poly->setPosition(cx, cy);
						
						// Make physical
						bool success = world->makePhysical(poly);
						
						if(!success)
						{
							world->remove(poly);
							delete poly;
						}
					}
					currentthing = 0;
				}
				drawing = false;
			}
			break;
			
			case pmBox:
			{
				if(drawing)
				{
					Polygon *poly = (Polygon*)currentthing;
					
					// Delete if too small
					int vx, vy;
					poly->getVertex(0, &vx, &vy);
					if( (abs(x-vx) < 6) || (abs(y-vy) < 6) || ( abs(x-vx)*abs(y-vy) < 50 ) )
					{
						printf("too small\n");
						world->remove(poly);
						delete poly;
					}
					else
					{
						// Recompute center
						int cx=0, cy=0;
						for(int i=0;i<4;++i)
						{
							poly->getVertex(i, &vx, &vy);
							cx += vx;
							cy += vy;
						}
						
						cx /= 4;
						cy /= 4;
						
						for(int i=0;i<4;++i)
						{
							poly->getVertex(i, &vx, &vy);
							poly->setVertex(i, vx-cx, vy-cy);
						}
						
						poly->setPosition(cx, cy);
						
						// Make physical
						world->makePhysical(poly);
					}
					
					currentthing = 0;
				}
				
				drawing = false;
			}
			break;
			
			case pmCircle:
			{
				if(drawing)
				{
					Circle *circle = (Circle*)currentthing;
					
					// Delete if too small
					if(circle->getRadius() < 3)
					{
						printf("too small\n");
						world->remove(circle);
						delete circle;
					}
					else
					{
						world->makePhysical(circle);
					}
					
					currentthing = 0;
				}
				
				drawing = false;
			}
			break;
			
			case pmPin:
			{
				if(drawing)
				{
					Pin *pin = (Pin*)currentthing;

					Thing *things[2] = {0, 0};
					int count = world->getThingsAt(x, y, things, 2, false);

					bool pinned = false;
					if(count == 0)
					{
						printf("Nothing to pin\n");
					}
					else if(things[0]->getb2Body() == 0)
					{
						printf("weird: thing does not have a body\n");
					}
					else if(   (things[0]->getType() != Thing::Dynamic)
							&&( (count < 2) || (things[1]->getType() != Thing::Dynamic) ) )
					{
						printf("Not pinning static objects\n");
					}
					else if(count == 1)
					{
						world->pin(pin, things[0]); // Pins the object to the background
						pinned = true;
					}
					else if(count == 2)
					{
						world->pin(pin, things[0], things[1]); // Pins the objects to each other
						pinned = true;
					}
					
					if(!pinned)
					{
						world->remove(pin);
						delete pin;
					}
					
					pinthing1 = pinthing2 = 0;
					currentthing = 0;
				}
				drawing = false;
			}
			break;
			
			case pmMove:
			{
				if(drawing)
				{
					if(simulation_mode)
						world->letGo();
					else
					{
						
						for(int i=0; i<move_n_connected_things; ++i)
						{
							if(move_connected_things[i]->getShape()==Thing::Pin)
							{
								world->makeUnphysical(move_connected_things[i]);
							}
						}
						
						for(int i=0; i<move_n_connected_things; ++i)
						{
							if(move_connected_things[i]->getShape()==Thing::Pin)
							{
								world->makePhysical(move_connected_things[i]);
							}
						}
						
						free(move_connected_things);
					}
					highlightthing = 0;
				}
				drawing = false;
			}
			break;
			
			case pmDelete:
			{
				if(drawing)
				{
					Thing *thing;
					int count = world->getThingsAt(x, y, &thing, 1, true);
					
					if(count > 0)
					{
						world->remove(thing);
						
						delete thing;
						extern Sample* smp_del;
						CommandPlaySample(smp_del, 48, 255, 1);
						
						highlightthing = 0;
					}
				}
				highlightthing = 0;
				drawing = false;
			}
			break;
			
			default:
				break;
		}
}

void Canvas::drawScreenRect(int sx, int sy)
{
	drawLine(RGB15(0,0,0), sx-5,   sy-5,   sx+237, sy-5);
	drawLine(RGB15(0,0,0), sx-5,   sy+176, sx+237, sy+176);
	drawLine(RGB15(0,0,0), sx-5,   sy,     sx-5,   sy+181);
	drawLine(RGB15(0,0,0), sx+237, sy,     sx+237, sy+181);
}

void Canvas::drawLine(u16 col, int x1, int y1, int x2, int y2)
{
	static int alphaint = 2;
	alphaint++;
	if(alphaint==32)
		alphaint=2;
	
	ulSetAlpha(UL_FX_ALPHA, SHAPE_ALPHA, alphaint);
	
	// Old float code replaced by fast integer code below
	//b2Vec2 od;
	//od.x = (y1 - y2)/2;
	//od.y = (x2 - x1)/2;
	//int len = od.Length()*2;
	//od.Normalize();
	//od *= 4;
	
	int odx = y1 - y2;
	int ody = x2 - x1;
	
	//int len = mysqrt(odx*odx + ody*ody);
	//odx = 4 * (odx<<8) / len; // using 24.8 fixed point for normal calculation
	//ody = 4 * (ody<<8) / len;
	
	// Even faster than the obove using DS's hardware
	int len = nds_sqrt64(odx*odx + ody*ody);
	//int len = mysqrt(odx*odx + ody*ody);
	odx = div32(4 * (odx<<8), len); // using 24.8 fixed point for normal calculation
	ody = div32(4 * (ody<<8), len);
		
	// Alpha: Moves the line texture inside the shape, so lines don't fully overlap when
	// obejcts collide
	
	//float32 alpha = float32(1.5);
	//b2Vec2 od1 = alpha * od;
	//b2Vec2 od2 = (float32(2)-alpha) * od;
	
	int od1x = (odx * 384) >> 16; // 384 is 1.5 (alpha) in 24.8 fixed point
	int od1y = (ody * 384) >> 16;
	int od2x = ((2*odx)>>8) - od1x;
	int od2y = ((2*ody)>>8) - od1y;
	
	crayon->offsetX0=0;
	crayon->offsetY0=0;
	crayon->offsetX1=8; // <- Must be line texture height
	crayon->offsetY1=len;
	
	ulSetImageTint(crayon, col);
	
	ulDrawImageQuad(crayon,
				x1+od1x, y1+od1y,
				x2+od1x, y2+od1y,
				x2-od2x, y2-od2y,
				x1-od2x, y1-od2y);
}

void Canvas::startSimulationMode(void)
{
	simulation_mode = true;
}

void Canvas::stopSimulationMode(void)
{
	simulation_mode = false;
}