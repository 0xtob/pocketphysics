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
#define DRAW_POLYGON_CLOSE_DIST	7 //15
#define DRAW_NEW_POINT_ANGLE    20

#define N_CIRCLE_SEGMENTS		16

Canvas::Canvas(World *_world):
	world(_world), drawing(false)
{
	crayon = ulLoadImageFilePNG((const char*)crayon_png, (int)crayon_png_size, UL_IN_VRAM, UL_PF_PAL3_A5);
}

void Canvas::draw(void)
{
	int n_things = world->getNThings();
	for(int i=0;i<n_things;++i)
	{
		Thing *thing = world->getThing(i);
		
		u16 col=0;
		if( (thing->getShape() == Thing::Polygon) || (thing->getShape() == Thing::Circle) )
		{
			if(thing->getType() == Thing::Dynamic)
				col = RGB15(0,0,31);
			else if(thing->getType() == Thing::Solid)
				col = RGB15(31,0,0);
			else if(thing->getType() == Thing::NonSolid)
				col = RGB15(0,31,0);
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
				int vecx = ( (radius<<12) * COS_bin[(angle * 512 / 360) % 512] ) >> 12;
				int vecy = ( (radius<<12) * SIN_bin[(angle * 512 / 360) % 512] ) >> 12;
				
				int lastx=0, lasty=0;
				for(int i=0;i<=N_CIRCLE_SEGMENTS;++i)
				{
					vecx = ( (radius<<12) * COS_bin[(angle + 512 * i / N_CIRCLE_SEGMENTS) % 512]) >> 12;
					vecy = ( (radius<<12) * SIN_bin[(angle + 512 * i / N_CIRCLE_SEGMENTS) % 512]) >> 12;
					
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
				int px, py;
				Pin *pin = (Pin*)thing;
				pin->getPosition(&px, &py);
				u16 pincol = RGB15(0,31,0);
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
			
			world->add(poly);
			
			currentthing = poly;
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
			
			world->add(poly);
			
			currentthing = poly;
		}
		break;
		
		case pmCircle:
		{
			drawing = true;
			
			Circle *circle = new Circle(type, Thing::User);
			circle->setPosition(x, y);
			
			world->add(circle);
			
			currentthing = circle;
		}
		break;
		
		case pmPin:
		{
			// We need some object at this position
			Thing *things[2] = {0, 0};
			int count = world->getThingsAt(x, y, things, 2);
			
			if(count > 0)
			{
				if(things[0]->getb2Body() == 0)
				{
					printf("weird: thing does not have a body\n");
					return;
				}
				if(   (things[0]->getType() != Thing::Dynamic)
					&&( (count < 2) || (things[1]->getType() != Thing::Dynamic) ) )
				{
					printf("Not pinning static objects\n");
					return;
				}
				Pin *pin = new Pin(x, y);
				if(count == 1)
					world->pin(pin, things[0]); // Pins the object to the background
				else if(count == 2)
					world->pin(pin, things[0], things[1]); // Pins the objects to each other
				world->add(pin);
			}
			else
				printf("nothing picked!\n");
			break;
		}
		
		case pmDelete:
		{
			Thing *thing = world->removeAt(x, y);
			if(thing != 0)
			{
				delete thing;
				extern Sample* smp_del;
				CommandPlaySample(smp_del, 48, 255, 0);
			}
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
				
				double last_len = sqrt((double)(last_dx*last_dx + last_dy*last_dy));
				double len = sqrt((double)(dx*dx + dy*dy));
				
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
			int radius = (int)b2Sqrt(float32((x - px)*(x - px) + (y - py)*(y - py)));
			circle->setRadius(radius);
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
					
					
					// Delete if too small
					if( n_vertices < 2 )
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
					if( (abs(x-vx) < 3) || (abs(y-vy) < 3) )
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
			
			default:
				break;
		}
}

void Canvas::drawLine(u16 col, int x1, int y1, int x2, int y2)
{
	static int alphaint = 2;
	alphaint++;
	if(alphaint==32)
		alphaint=2;
	
	ulSetAlpha(UL_FX_ALPHA, 20, alphaint);
	
	// Old float code replaced by fast integer code below
	//b2Vec2 od;
	//od.x = (y1 - y2)/2;
	//od.y = (x2 - x1)/2;
	//int len = od.Length()*2;
	//od.Normalize();
	//od *= 4;
	
	int odx = y1 - y2;
	int ody = x2 - x1;
	int len = mysqrt(odx*odx + ody*ody);
	odx = 4 * (odx<<8) / len; // using 24.8 fixed point for normal calculation
	ody = 4 * (ody<<8) / len;
	
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
