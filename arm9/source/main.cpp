#include "tobkit/tobkit.h"
#include <ulib/ulib.h>

#include <nds/arm9/ndsmotion.h>
#include <fat.h>
#include <sys/dir.h>

#include "../../generic/command.h"

#include "sample.h"

#include "world.h"
#include "polygon.h"
#include "canvas.h"

#include "state.h"

#include "pocketphysics_png.h"
#include "paper2_png.h"

#include "icon_flp_raw.h"
#include "icon_dynamic_raw.h"
#include "icon_solid_raw.h"
#include "icon_nonsolid_raw.h"
#include "icon_select_raw.h"
#include "icon_play_raw.h"
#include "icon_stop_raw.h"
#include "icon_pause_raw.h"
#include "icon_box_raw.h"
#include "icon_polygon_raw.h"
#include "icon_circle_raw.h"
#include "icon_pin_raw.h"
#include "icon_delete_raw.h"
#include "icon_zap_raw.h"
#include "icon_save_raw.h"
#include "icon_load_raw.h"


#include "sound_click_raw.h"
#include "sound_pen_raw.h"
#include "sound_play_raw.h"
#include "sound_del_raw.h"
#include "tobkit/tools.h"

#define min(x,y)	((x)<(y)?(x):(y))

#define PEN_DOWN (~IPC->buttons & (1 << 6))

#define DEBUG
#define DUALSCREEN

#define WORLD_WIDTH		(3*256)
#define WORLD_HEIGHT	(3*192)

#define SCROLL_XMAX		(WORLD_WIDTH-256+24)
#define SCROLL_YMAX		(WORLD_HEIGHT-192+21)

#define SCROLL_ACCEL	1
#define	SCROLL_VMAX		8

#define KEYS_SCROLL_RIGHT	(KEY_RIGHT | KEY_A)
#define KEYS_SCROLL_LEFT		(KEY_LEFT | KEY_Y)
#define KEYS_SCROLL_UP		(KEY_UP | KEY_X)
#define KEYS_SCROLL_DOWN		(KEY_DOWN | KEY_B)

int scroll_x=0;
int scroll_y=0;

int scroll_vx=0;
int scroll_vy=0;

u16 keysdown=0, keysheld=0, keysup=0;

bool stylus_scrolling;

int touch_was_down = 0;
int lastx, lasty;
u8 got_good_pen_reading = 0;

bool dsmotion = false;

bool init = true;

int motion_x_offset = 2048;
int motion_y_offset = 2048;

int fps=60;
int slowfps=60;
int currentframe = 0;
bool framesdone[60];
bool framedone = false;
int accumulated_timesteps = 0;
int passed_frames = 0;

bool main_screen_active = false;

bool dont_draw = false;

bool dialog_active = false; // If a dislog is active, send all pen events to the GUI

bool fat_ok = false;

char *current_filename = 0;

UL_IMAGE *imgbg;

float32 timeStep = float32(1) / float32(20);

u16 *main_vram = (u16*)BG_BMP_RAM(2);
u16 *sub_vram = (u16*)BG_BMP_RAM_SUB(2);

World *world;

State state;

Theme *theme;
GUI *gui;

Canvas *canvas;

Sample *smp_crayon, *smp_play, *smp_click, *smp_del;

MessageBox *mb;

Typewriter *tw;

PPLoadDialog *load_dialog;

// <Side Bar>
	//BitButton *btnflipmain, *btnflipsub;
	BitButton *btnplay, *btnstop, *btnpause, *btnload, *btnsave;
	ToggleBitButton::ToggleBitButtonGroup *tbbgsidebar;
	ToggleBitButton /* *tbbselect,*/ *tbbdynamic, *tbbsolid, *tbbnonsolid;
// </Side Bar>

// <Bottom Bar>
	ToggleBitButton *tbbbox, *tbbpolygon, *tbbcircle, *tbbpin, *tbbdelete;
	ToggleBitButton::ToggleBitButtonGroup *tbbgobjects;
	BitButton *buttonzap;
// </Bottom Bar>

void lidSleep()
{
	__asm(".arm"); 
   __asm("mcr p15,0,r0,c7,c0,4");
   __asm("mov r0, r0");
   __asm("BX lr");
}

void mearureFps()
{
	main_screen_active = !main_screen_active;
	
	if(state.simulating)
		accumulated_timesteps++;
	
	passed_frames++; // same as accumulated timesteps but for scrolling
	
	framesdone[currentframe] = framedone;
	
	if(!framedone)
		--fps;
	if(!framesdone[(currentframe+1)%60])
		++fps;
	
	currentframe = (currentframe + 1) % 60;
	
	//if(currentframe == 0)
	//	printf("%d\n",fps);
	
	// Adjust the timestep slowly! This buffers sudden fps increases which
	// would result in "bullet-time" effects.
	if(currentframe%10==0)
	{
		if(slowfps < fps)
			slowfps++;
		else if(slowfps > fps)
			slowfps--;
		
		// Commented out because dynamic timestep adjustment makes simulation nondeterministic
		//timeStep = 2.0f / (float)slowfps;
	}
	
	framedone = false;
}

void drawBgBox(void)
{
	ulSetAlpha(UL_FX_ALPHA, 10, 1);
	ulDrawFillRect(16, 15, 16+219, 15+162, RGB15(0,0,0));
}

void drawSideBar(void)
{
	ulSetAlpha(UL_FX_ALPHA, 10, 1);
	ulDrawFillRect(232, 0, 256, 192, RGB15(0,0,0));
}

void drawBottomBar(void)
{
	ulSetAlpha(UL_FX_ALPHA, 10, 1);
	ulDrawFillRect(0, 171, 232, 192, RGB15(0,0,0));
}

void draw()
{
	ulEndFrame();
	
	ulStartDrawing2D();
#ifdef DUALSCREEN
	if (ulGetMainLcd()) // Bottom Screen
	{
		videoSetMode(MODE_3_3D);
#endif
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrthof32(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -4090, 1);
		glMatrixMode(GL_MODELVIEW);
		
		glTranslate3f32(-scroll_x, 0, 0);
		glTranslate3f32(0, -scroll_y, 0);
		
		ulSetAlpha(UL_FX_DEFAULT, 0, 0);
		ulDrawImageXY(imgbg, 0, 0);
		ulSetAlpha(UL_FX_ALPHA, 31, 1);
		
		if(!dont_draw)
		canvas->draw();
		
		glLoadIdentity();
		if(!dialog_active)
		{
			drawSideBar();
			drawBottomBar();
		}
		else
		{
			drawBgBox();
		}
#ifdef DUALSCREEN
	}
	else // Top Screen
	{
		videoSetMode(MODE_3_3D |  DISPLAY_BG1_ACTIVE | DISPLAY_BG3_ACTIVE);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrthof32(0, WORLD_WIDTH, WORLD_HEIGHT, 0, -4090, 1);
		glMatrixMode(GL_MODELVIEW);
					
		ulSetAlpha(UL_FX_DEFAULT, 0, 0);
		ulDrawImageXY(imgbg, 0, 0);
		ulSetAlpha(UL_FX_ALPHA, 31, 1);
		if(!dont_draw)
			canvas->draw();
		if(state.draw_window)
			canvas->drawScreenRect(scroll_x, scroll_y);
	}
#endif
	
	ulEndDrawing();
	
	framedone = true;
}

void handleScrolling(void)
{
	touchPosition touch = touchReadXY();
	
	if( (keysheld & KEY_TOUCH) && ( (keysheld & KEY_L) || (keysheld & KEY_R) ) )
	{
		stylus_scrolling = true;
		scroll_vx = (touch.px - 120) / 5;
		scroll_vy = (touch.py - 90) / 5;
	}
	else
	{
		stylus_scrolling = false;

		if( (keysheld & KEYS_SCROLL_RIGHT) && (scroll_vx < SCROLL_VMAX) )
			scroll_vx ++;
		else if( (keysheld & KEYS_SCROLL_LEFT) && (scroll_vx > -SCROLL_VMAX) )
			scroll_vx --;
		else if(scroll_vx > 0) {
			scroll_vx --; if(scroll_vx < 0) scroll_vx = 0;
		} else if(scroll_vx < 0) {
			scroll_vx ++; if(scroll_vx > 0) scroll_vx = 0;
		} if( (keysheld & KEYS_SCROLL_DOWN) && (scroll_vy < SCROLL_VMAX) )
			scroll_vy ++;
		else if( (keysheld & KEYS_SCROLL_UP) && (scroll_vy > -SCROLL_VMAX) )
			scroll_vy --;
		else if(scroll_vy > 0) {
			scroll_vy --; if(scroll_vy < 0) scroll_vy = 0;
		} else if(scroll_vy < 0) {
			scroll_vy ++; if(scroll_vy > 0) scroll_vy = 0;
		}
	}
	
	if(scroll_vx != 0)
	{
		scroll_x += scroll_vx;

		if(scroll_x < 0)
			scroll_x = 0;
		if(scroll_x > SCROLL_XMAX)
			scroll_x = SCROLL_XMAX;
	}

	if(scroll_vy != 0)
	{
		scroll_y += scroll_vy;

		if(scroll_y < 0)
			scroll_y = 0;
		if(scroll_y > SCROLL_YMAX)
			scroll_y = SCROLL_YMAX;
	}
}

void updateInput(void)
{
	// Check input
	scanKeys();
	keysdown |= keysDown();
	keysup |= keysUp();
	keysheld = keysHeld();
}

void VBlankHandler()
{
	mearureFps();
	updateInput();
	if(!dialog_active)
		handleScrolling();
	draw();
}

void switchScreens(void)
{
	lcdSwap();
	gui->switchScreens();
}

void drawMainBg()
{
	drawSideBar();
	drawBottomBar();
}

void tbbgsidebarChanged(s8 item)
{
	if(!init)
		CommandPlaySample(smp_click, 48, 255, 0);
	
	switch(item)
	{
		//case 0: // Select
		//	canvas->setPenMode(Canvas::pmNormal);
		//	break;
			
		case 0: // Dynamic
			canvas->setObjectMode(Canvas::omDynamic);
			break;
			
		case 1: // Solid
			canvas->setObjectMode(Canvas::omSolid);
			break;
			
		case 2: // Non-Solid
			canvas->setObjectMode(Canvas::omNonSolid);
			break;
			
		default:
			printf("Unhandled sidebar icon?!\n");
			break;
	}
}

void penModeChanged(s8 item)
{
	if(!init)
		CommandPlaySample(smp_click, 48, 255, 0);
	
	switch(item)
	{
		case -1:
			canvas->setPenMode(Canvas::pmNormal);
			break;
			
		case 0:
			canvas->setPenMode(Canvas::pmPolygon);
			break;
		
		case 1:
			canvas->setPenMode(Canvas::pmBox);
			break;
		
		case 2:
			canvas->setPenMode(Canvas::pmCircle);
			break;
		
		case 3:
			canvas->setPenMode(Canvas::pmPin);
			break;
			
		case 4:
			canvas->setPenMode(Canvas::pmDelete);
			break;
			
		default:
			printf("mäh.\n");
	}
}

void startPlay(void)
{
	CommandPlaySample(smp_play, 48, 255, 0);
	state.simulating = true;
	btnplay->hide();
	btnpause->show();
	canvas->hidePins();
}

void pausePlay(void)
{
	CommandPlaySample(smp_play, 48, 255, 0);
	state.simulating = false;
	btnpause->hide();
	btnplay->show();
	canvas->showPins();
}

void stopPlay(void)
{
	dont_draw = true;
	CommandPlaySample(smp_play, 48, 255, 0);
	state.simulating = false;
	world->reset();
	btnpause->hide();
	btnplay->show();
	canvas->showPins();
	dont_draw = false;
}

void clearGui(void)
{
	u16 col = RGB15(0,0,0);
	u32 colcol = col | col << 16;
	swiFastCopy(&colcol, main_vram, 192*256/2 | COPY_MODE_FILL);
}

void redrawGui(void)
{
	clearGui();
	gui->draw();
}

void deleteMessageBox(void)
{
	gui->unregisterOverlayWidget(MAIN_SCREEN);
	delete mb;
	mb = 0;
	redrawGui();
}

void zap(void)
{
	stopPlay();
	dont_draw = true;
	
	while(world->getNThings() > 0)
	{
		Thing *t = world->getThing(0);
		world->remove(t);
		delete t;
	}
	
	CommandPlaySample(smp_del, 48, 255, 0);
	deleteMessageBox();
	
	dont_draw = false;
}

void askZap(void)
{
	mb = new MessageBox(&main_vram, "destroy the world", 2, "yes", zap, "no", deleteMessageBox);
	gui->registerOverlayWidget(mb, 0, MAIN_SCREEN);
	mb->show();
	mb->pleaseDraw();
}

inline u16 avgcol(u16 col1, u16 col2, u16 col3, u16 col4)
{
	return  ( (    (col1 & 31)      + (col2 & 31)       + (col3 & 31)       + (col4 & 31) )      / 4 )
          | ( ( ( ((col1>>5) & 31)  + ((col2>>5) & 31)  + ((col3>>5) & 31)  + ((col4>>5) & 31))  / 4 ) << 5 )
          | ( ( ( ((col1>>10) & 31) + ((col2>>10) & 31) + ((col3>>10) & 31) + ((col4>>10) & 31)) / 4 ) << 10);
}

char *b64screenshot(void)
{
	bool draw_window_tmp = state.draw_window;
	state.draw_window = false;
	
	// Wait until a full frame (top and bootom screens) was drawn and
	// capturing is finished
	while(ulGetMainLcd());
	while(!ulGetMainLcd());
	while(ulGetMainLcd());
	while(REG_CAPTURE & BIT(31));
	
	// Copy VRAM_C, becase this will take longer than a frame
	u16 *screen = (u16*)malloc(sizeof(u16*)*256*192);
	dmaCopy(VRAM_C, screen, 256*192*2);
	
	state.draw_window = draw_window_tmp;
	
	u16 linebuffer[4][64] = {{0}};
	
	u16 *screen_scaled = (u16*)malloc(2*64*48);
	
	// Scale the image using some cheap box filter. Looks decent IMO.
	int bufline = 0;
	for(int y=0; y<192; ++y)
	{
		for(int x=0; x<64; ++x)
		{
			linebuffer[bufline][x] = avgcol( screen[256*y+(4*x)+0],
					screen[256*y+(4*x)+1],
					screen[256*y+(4*x)+2],
					screen[256*y+(4*x)+3]);
		}
		
		bufline++;
		
		if(bufline == 4)
		{
			for(int x=0; x<64; ++x)
			{
				screen_scaled[64*(y/4)+x] = avgcol( linebuffer[0][x],
												linebuffer[1][x],
												linebuffer[2][x],
												linebuffer[3][x]) | BIT(15);
			}
			
			bufline = 0;
		}
	}
	
	char *b64screen;
	b64encode((u8*)screen_scaled, 2*64*48, &b64screen);
	
	free(screen);
	free(screen_scaled);
	
	return b64screen;
}

void save(void)
{
	char *thumbnail = b64screenshot();
	world->save(current_filename, thumbnail);
	free(thumbnail);
}

void showTypewriter(const char *prompt, const char *str, void (*okCallback)(void), void (*cancelCallback)(void))
{
	dialog_active = true;
	
	clearGui();
	
	tw = new Typewriter(prompt, (u16*)CHAR_BASE_BLOCK(1),
		(u16*)SCREEN_BASE_BLOCK(12), 3, &main_vram, &BG1_X0, &BG1_Y0);
	
	tw->setText(str);
	gui->registerOverlayWidget(tw, KEY_LEFT|KEY_RIGHT, MAIN_SCREEN);
	if(okCallback!=0) {
		tw->registerOkCallback(okCallback);
	}
	if(cancelCallback != 0) {
		tw->registerCancelCallback(cancelCallback);
	}
	tw->show();
	tw->pleaseDraw();
}

void deleteTypewriter(void)
{
	gui->unregisterOverlayWidget(MAIN_SCREEN);
	delete tw;
	
	redrawGui();
	
	dialog_active = false;
}

void overwriteFile(void)
{
	save();
	deleteMessageBox();
}

void handleSaveDialogOk(void)
{
	char *text = tw->getText();
	
	// Append extension if neccessary
	if(strcmp(text,"") != 0)
	{
		char *name;
		if( strcmp(text+strlen(text)-3, ".pp") != 0 ) {
			// Append extension
			name = (char*)malloc(strlen(text)+3+1);
			strcpy(name,text);
			strcpy(name+strlen(name),".pp");
		} else {
			// Leave as is
			name = (char*)malloc(strlen(text)+1);
			strcpy(name,text);
		}

		deleteTypewriter();
		
		printf("Saving %s\n", name);
		
		strncpy(current_filename, name, 256);
		
		// Check if the file already exists
		char* fullname = (char*)calloc(1,256);
		sprintf(fullname, "%s/%s", "pocketphysics/sketches", name);
		
		FILE* f = fopen(fullname,"r");

		if( f != 0 )
		{
			mb = new MessageBox(&main_vram, "overwrite file", 2, "yes", overwriteFile, "no", deleteMessageBox);
			gui->registerOverlayWidget(mb, 0, MAIN_SCREEN);
			mb->show();
			mb->pleaseDraw();
		}
		else
		{
			fclose(f);
			save();
		}
		
		free(fullname);
		free(name);
	}
}

void deleteLoadDialog(void)
{
	gui->unregisterOverlayWidget(MAIN_SCREEN);
	delete load_dialog;

	redrawGui();

	dialog_active = false;
}

void handleLoadDialogOk(void)
{
	printf("Loading %s\n", load_dialog->getFilename());
	world->load(load_dialog->getFilename());
	deleteLoadDialog();
}

void showLoadDialog(void)
{
	printf("Load Dialog\n");
	
	load_dialog = new PPLoadDialog(&main_vram);
	dialog_active = true;
	clearGui();
	
	gui->registerOverlayWidget(load_dialog, 0, MAIN_SCREEN);
	load_dialog->registerOkCallback(handleLoadDialogOk);
	load_dialog->registerCancelCallback(deleteLoadDialog);
	
	load_dialog->show();
}

void showSaveDialog(void)
{
	printf("Save\n");
	showTypewriter("save as", current_filename, handleSaveDialogOk, deleteTypewriter);
}

void setupGui(void)
{
	gui = new GUI;
	gui->setTheme(theme);
	
	// <Side Bar>
		tbbgsidebar = new ToggleBitButton::ToggleBitButtonGroup();
		
		//tbbselect = new ToggleBitButton(233, 22, 22, 19, &main_vram, icon_select_raw, 18, 15, 1, 2, tbbgsidebar);
		tbbdynamic = new ToggleBitButton(233, 1, 22, 19, &main_vram, icon_dynamic_raw, 18, 15, 1, 2, tbbgsidebar);
		tbbsolid = new ToggleBitButton(233, 21, 22, 19, &main_vram, icon_solid_raw, 18, 15, 1, 2, tbbgsidebar);
		//tbbnonsolid = new ToggleBitButton(233, 41, 22, 19, &main_vram, icon_nonsolid_raw, 18, 15, 1, 2, tbbgsidebar);
		
		tbbgsidebar->registerChangeCallback(tbbgsidebarChanged);
		
		btnload = new BitButton(233, 101, 22, 19, &main_vram, icon_load_raw, 18, 15, 2, 1);
		btnload->registerPushCallback(showLoadDialog);
		
		btnsave = new BitButton(233, 121, 22, 19, &main_vram, icon_save_raw, 18, 15, 2, 1);
		btnsave->registerPushCallback(showSaveDialog);
		
		btnstop = new BitButton(233, 151, 22, 19, &main_vram, icon_stop_raw, 12, 12, 5, 1);
		btnstop->registerPushCallback(stopPlay);
		
		btnplay = new BitButton(233, 171, 22, 19, &main_vram, icon_play_raw, 12, 12, 5, 1);
		btnplay->registerPushCallback(startPlay);
		
		btnpause = new BitButton(233, 171, 22, 19, &main_vram, icon_pause_raw, 12, 12, 5, 1);
		btnpause->registerPushCallback(pausePlay);
		
		//gui->registerWidget(tbbselect, 0, MAIN_SCREEN);
		gui->registerWidget(tbbdynamic, 0, MAIN_SCREEN);
		gui->registerWidget(tbbsolid, 0, MAIN_SCREEN);
		//gui->registerWidget(tbbnonsolid, 0, MAIN_SCREEN);
#ifndef DEBUG
		if(fat_ok)
		{
#endif
			gui->registerWidget(btnload, 0, MAIN_SCREEN);
			gui->registerWidget(btnsave, 0, MAIN_SCREEN);
#ifndef DEBUG
		}
#endif
		gui->registerWidget(btnpause, 0, MAIN_SCREEN);
		gui->registerWidget(btnplay, 0, MAIN_SCREEN);
		gui->registerWidget(btnstop, 0, MAIN_SCREEN);
	// </Side Bar>
		
	// <Bottom Bar>
		tbbgobjects = new ToggleBitButton::ToggleBitButtonGroup();
		
		tbbpolygon = new ToggleBitButton(1, 172, 22, 19, &main_vram, icon_polygon_raw, 18, 15, 2, 2, tbbgobjects);
		tbbbox = new ToggleBitButton(24, 172, 22, 19, &main_vram, icon_box_raw, 18, 15, 2, 2, tbbgobjects);
		tbbcircle = new ToggleBitButton(47, 172, 22, 19, &main_vram, icon_circle_raw, 18, 15, 2, 2, tbbgobjects);
		tbbpin = new ToggleBitButton(70, 172, 22, 19, &main_vram, icon_pin_raw, 18, 15, 2, 2, tbbgobjects);
		
		tbbdelete = new ToggleBitButton(175, 172, 22, 19, &main_vram, icon_delete_raw, 18, 15, 2, 2, tbbgobjects);
		
		tbbgobjects->registerChangeCallback(penModeChanged);
		
		buttonzap = new BitButton(198, 172, 22, 19, &main_vram, icon_zap_raw, 18, 15, 2, 2);
		buttonzap->registerPushCallback(askZap);
		
		gui->registerWidget(tbbpolygon, 0, MAIN_SCREEN);
		gui->registerWidget(tbbbox, 0, MAIN_SCREEN);
		gui->registerWidget(tbbcircle, 0, MAIN_SCREEN);
		gui->registerWidget(tbbpin, 0, MAIN_SCREEN);
		gui->registerWidget(tbbdelete, 0, MAIN_SCREEN);
		gui->registerWidget(buttonzap, 0, MAIN_SCREEN);
	// </Bottom Bar>
		
	gui->showAll();
	btnpause->hide();
	gui->draw();
	
	tbbgsidebar->setActive(0);
	tbbgobjects->setActive(0);
}

bool onCanvas(int px, int py)
{
	return ((px>=0)&&(px<231)&&(py>=0)&&(py<170))&&(mb==0);
}

void calibrate_motion(void)
{
	  iprintf("OK, calibrating\n");
	  
	  s32 xmean=0, ymean=0;
	  for(int i=0;i<30;++i)
	  {
	    xmean += motion_read_x(); // this returns a value between 0 and 4095
	    ymean += motion_read_y(); // this returns a value between 0 and 4095
	    swiWaitForVBlank();
	  }
	  xmean /= 30;
	  ymean /= 30;
	  
	  motion_x_offset = xmean;
	  motion_y_offset = ymean;
	  
	  
	  printf("x %d y %d\n", motion_x_offset, motion_y_offset);
	  printf("Done\n");
	  
	  deleteMessageBox();
}

void request_calibration()
{
	mb = new MessageBox(&main_vram, "please put the ds on a table", 1, "calibrate dsmotion!", calibrate_motion);
	gui->registerOverlayWidget(mb, 0, MAIN_SCREEN);
	mb->show();
	mb->pleaseDraw();
}

void handleInput(void)
{
	touchPosition touch = touchReadXY();
	
	// Prevent drawing while scrolling with pen
	if(keysdown & (KEY_L | KEY_R))
	{
		canvas->penUp(touch.px, touch.py);
		CommandStopSample(0);
	}
	
	if(keysdown && dialog_active) // Send keys to the gui (for left/right navigating in keyboard)
	{
		gui->buttonPress(keysdown);
	}
	
	if(keysup && dialog_active)
	{
		gui->buttonRelease(keysup);
	}
	
	if(keysdown & KEY_SELECT)
		state.draw_window = !state.draw_window;
	
	// clear keysdown
	keysdown = 0;

	if(!touch_was_down && PEN_DOWN)
	{
		got_good_pen_reading = 0; // Wait one frame until passing the event
		lastx = touch.px;
		lasty = touch.py;
		touch_was_down = 1;
	}
	else
	{
		if(touch_was_down && !PEN_DOWN) // PenUp
		{
			canvas->penUp(touch.px + scroll_x, touch.py + scroll_y);
			gui->penUp(touch.px, touch.py);
			CommandStopSample(0);
			
			touch_was_down = 0;
		}
		else if(touch_was_down && PEN_DOWN)
		{
			if(!got_good_pen_reading) // PenDown
			{
				if(!stylus_scrolling && onCanvas(touch.px, touch.py) && !dialog_active)
				{
					CommandPlaySample(smp_crayon, 48, 255, 0);
					canvas->penDown(touch.px + scroll_x, touch.py + scroll_y);
				}
				else
					gui->penDown(touch.px, touch.py);		
				
				got_good_pen_reading = 1;
			}
			
			if((abs(touch.px - lastx)>0) || (abs(touch.py - lasty)>0)) // PenMove
			{
				if(onCanvas(touch.px, touch.py) && !dialog_active)
					canvas->penMove(touch.px + scroll_x, touch.py + scroll_y);
				else
					gui->penMove(touch.px, touch.py);
				
				lastx = touch.px;
				lasty = touch.py;
			}
		}
	}

	// DSMotion shortcut for swappers
	if( (keysheld & KEY_L) && (keysheld & KEY_R) )
	{
		if(motion_init() != 0)
		{
			dsmotion = true;
		  
			request_calibration();
		}
		else
		{
			dsmotion = false;
		
			iprintf("Get a DSMotion. They are fun!\n");
		}
	}
	
	passed_frames = 0;
	
	// Special case: User holds down the pen and scrolls => generate penmove event
	if( ( (scroll_vx != 0) || (scroll_vy != 0) )
			&& (touch_was_down && PEN_DOWN)
			&& (onCanvas(touch.px, touch.py) ) )
		canvas->penMove(touch.px + scroll_x, touch.py + scroll_y);
}

void loadSamples()
{
	u32 n_samples = sound_pen_raw_size/2;
	u32 loopstart = n_samples / 10;
	smp_crayon = new Sample((void*)sound_pen_raw, n_samples, 16381);
	smp_crayon->setLoop(PING_PONG_LOOP);
	smp_crayon->setLoopStartAndLength(loopstart, n_samples - loopstart);
	
	smp_play = new Sample((void*)sound_play_raw, sound_play_raw_size/2, 16381);
	smp_click = new Sample((void*)sound_click_raw, sound_click_raw_size/2, 16381);
	smp_del = new Sample((void*)sound_del_raw, sound_del_raw_size/2, 16381);
}


void updateGravity()
{
	int Xaccel = motion_read_x(); // this returns a value between 0 and 4095
	int Yaccel = motion_read_y(); // this returns a value between 0 and 4095
	
	float32 xgrav = (float)((Xaccel - motion_x_offset) * 5) / 1638.0f;
	float32 ygrav = (float)((Yaccel - motion_y_offset) * 5) / 1638.0f;
	
	world->setGravity(-xgrav, ygrav);
}

void showSplash(void)
{
	UL_IMAGE *imgpp = ulLoadImageFilePNG((const char*)pocketphysics_png, (int)pocketphysics_png_size, UL_IN_VRAM, UL_PF_PAL8);
	for(int i=0;i<60;++i)
	{
		BLEND_Y = 31-i/2;
		ulStartDrawing2D();
		
		ulDrawImageXY(imgpp, 0, 0);
		
		ulEndDrawing();
		ulSyncFrame();
	}
	scanKeys();
	while(!keysDown())
	{
		BLEND_Y = 0;
		ulStartDrawing2D();
				
		ulDrawImageXY(imgpp, 0, 0);
		
		ulEndDrawing();
		ulSyncFrame();
		
		scanKeys();
	}
	for(int i=60;i>0;--i)
	{
		BLEND_Y = 31-i/2;
		ulStartDrawing2D();
		
		ulDrawImageXY(imgpp, 0, 0);
		
		ulEndDrawing();
		ulSyncFrame();
	}
}

void fadeIn()
{
	for(int i=0;i<60;++i)
	{
		ulSyncFrame();
		
		BLEND_Y = 31-i/2;
		
		ulStartDrawing2D();
	
		if (ulGetMainLcd()) // Bottom Screen
		{
			videoSetMode(MODE_3_3D);
	
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrthof32(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -4090, 1);
			glMatrixMode(GL_MODELVIEW);
			
			glTranslate3f32(-scroll_x, 0, 0);
			glTranslate3f32(0, -scroll_y, 0);
			
			ulSetAlpha(UL_FX_DEFAULT, 0, 0);
			ulDrawImageXY(imgbg, 0, 0);
			ulSetAlpha(UL_FX_ALPHA, 31, 1);
			
			canvas->draw();
			
			glLoadIdentity();
			drawSideBar(); drawBottomBar();
	
		}
		else // Top Screen
		{
			videoSetMode(MODE_3_3D | DISPLAY_BG3_ACTIVE);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrthof32(0, WORLD_WIDTH, WORLD_HEIGHT, 0, -4090, 1);
			glMatrixMode(GL_MODELVIEW);
						
			ulSetAlpha(UL_FX_DEFAULT, 0, 0);
			
			ulDrawImageXY(imgbg, 0, 0);
			
			ulSetAlpha(UL_FX_ALPHA, 31, 1);
			
			canvas->draw();
			canvas->drawScreenRect(scroll_x, scroll_y);
		}
	
		
		ulEndDrawing();
	}
	BLEND_Y = 0;
	SUB_BLEND_Y = 0;
}

int main()
{
	ulInit(UL_INIT_ALL);
	
	ulInitGfx();
	
	videoSetMode(MODE_3_3D | DISPLAY_BG1_ACTIVE | DISPLAY_BG3_ACTIVE);
#ifdef DEBUG
	videoSetModeSub(MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG2_ACTIVE);
#else
	videoSetModeSub(MODE_5_2D | DISPLAY_BG2_ACTIVE);
#endif
	vramSetBankB(VRAM_B_MAIN_BG_0x06000000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	
	// Main BG0: 3D
	BG0_CR = BG_PRIORITY(2);
	
	// Main BG3: ERB
	BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(2) | BG_PRIORITY(1);
	BG3_XDX = 1 << 8;
	BG3_XDY = 0;
	BG3_YDX = 0;
	BG3_YDY = 1 << 8;
	
	// Main BG1: Keyboard
	BG1_CR = BG_COLOR_16 | BG_32x32 | BG_MAP_BASE(12) | BG_TILE_BASE(1);
	
	// Sub BG0: Text
	SUB_BG0_CR = BG_MAP_BASE(4) | BG_TILE_BASE(0) | BG_PRIORITY(0);
	BG_PALETTE_SUB[255] = RGB15(31,31,31); //by default font will be rendered with color 255
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(4), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);
	
	// Sub BG2: ERB
	SUB_BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(2) | BG_PRIORITY(1);
	SUB_BG2_XDX = 1 << 8;
	SUB_BG2_XDY = 0;
	SUB_BG2_YDX = 0;
	SUB_BG2_YDY = 1 << 8;
	
	BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG3;
	SUB_BLEND_CR = BLEND_FADE_BLACK | BLEND_SRC_BG2;
	
#ifdef DEBUG
	printf("Pocket Physics Debug build\n");
#endif
	
	lcdMainOnBottom();
	
	CommandInit();
	
	loadSamples();
#ifndef DEBUG
	showSplash();
#endif
	world = new World(WORLD_WIDTH, WORLD_HEIGHT);
	canvas = new Canvas(world);
	
	theme = new Theme;
	
	u16 col = RGB15(0,0,0)|BIT(15);
	u32 colcol = col | col << 16;
	swiFastCopy(&colcol, sub_vram, 192*256/2 | COPY_MODE_FILL);
	
	drawMainBg();
	setupGui();
	
	ul_firstPaletteColorOpaque=2;
	imgbg = ulLoadImageFilePNG((const char*)paper2_png, (int)paper2_png_size, UL_IN_VRAM, UL_PF_PAL8);
	imgbg->stretchX = WORLD_WIDTH+24;
	imgbg->stretchY = WORLD_HEIGHT+21;
	
	//Initialize the text part
	ulInitText();
#ifdef DUALSCREEN
	ulInitDualScreenMode();
#endif
	videoSetMode(MODE_3_3D | DISPLAY_BG1_ACTIVE | DISPLAY_BG3_ACTIVE);
	
	current_filename = (char*)calloc(1, 256);
	
	fat_ok = fatInitDefault();
	
	if(motion_init() != 0)
	{
		dsmotion = true;
	  
		request_calibration();
	}
	else
	{
		dsmotion = false;
	
		iprintf("Get a DSMotion. They are fun!\n");
	}
	
	irqEnable(IRQ_VBLANK);
	
	for(int i=0;i<60;++i)
		framesdone[i] = true;
	
#ifndef DEBUG
	fadeIn();
#endif
	
	irqSet(IRQ_VBLANK, VBlankHandler);
	
	BLEND_Y = 0;
	SUB_BLEND_Y = 0;
	
	init = false;
	
	while(1)
	{
		if(state.simulating)
		{
			for(int i=0; i<min(2, accumulated_timesteps); ++i)
			{
				if(dsmotion)
					updateGravity();
				world->step(timeStep);
			}
		}
		accumulated_timesteps = 0;
		
		handleInput();
		
		CommandProcessCommands();
		
#ifdef DUALSCREEN
		swiWaitForVBlank();
#else
		ulSyncFrame();
#endif
	}

	//Program end - should never get there
	return 0;
}

