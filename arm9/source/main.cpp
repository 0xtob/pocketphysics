#include "tobkit/tobkit.h"
#include <ulib/ulib.h>

#include <nds/arm9/ndsmotion.h>

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

#include "sound_click_raw.h"
#include "sound_pen_raw.h"
#include "sound_play_raw.h"
#include "sound_del_raw.h"

#define min(x,y)	((x)<(y)?(x):(y))

#define PEN_DOWN (~IPC->buttons & (1 << 6))

#define DEBUG

int scx=0;

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

// <Side Bar>
	//BitButton *btnflipmain, *btnflipsub;
	BitButton *btnplay, *btnstop, *btnpause;
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

void fpstimer()
{
	if(state.simulating)
		accumulated_timesteps++;
	
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

void switchScreens(void)
{
	lcdSwap();
	gui->switchScreens();
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
}

void pausePlay(void)
{
	CommandPlaySample(smp_play, 48, 255, 0);
	state.simulating = false;
	btnpause->hide();
	btnplay->show();
}

void stopPlay(void)
{
	CommandPlaySample(smp_play, 48, 255, 0);
	state.simulating = false;
	world->reset();
	btnpause->hide();
	btnplay->show();
}

void deleteMessageBox(void)
{
	gui->unregisterOverlayWidget(MAIN_SCREEN);
	
	delete mb;
	
	mb = 0;
	
	for(int y=0;y<171;++y)
		for(int x=0;x<232;++x)
			main_vram[256*y+x] = 0;
}

void zap(void)
{
	stopPlay();
	
	while(world->getNThings() > 0)
	{
		Thing *t = world->getThing(0);
		world->remove(t);
		delete t;
	}
	
	CommandPlaySample(smp_del, 48, 255, 0);
	deleteMessageBox();
}

void askZap(void)
{
	mb = new MessageBox(&main_vram, "destroy the world", 2, "yes", zap, "no", deleteMessageBox);
	gui->registerOverlayWidget(mb, 0, MAIN_SCREEN);
	mb->show();
	mb->pleaseDraw();
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
		
		btnplay = new BitButton(233, 171, 22, 19, &main_vram, icon_play_raw, 12, 12, 5, 1);
		btnplay->registerPushCallback(startPlay);
		
		btnstop = new BitButton(233, 151, 22, 19, &main_vram, icon_stop_raw, 12, 12, 5, 1);
		btnstop->registerPushCallback(stopPlay);
		
		btnpause = new BitButton(233, 171, 22, 19, &main_vram, icon_pause_raw, 12, 12, 5, 1);
		btnpause->registerPushCallback(pausePlay);
		
		//gui->registerWidget(tbbselect, 0, MAIN_SCREEN);
		gui->registerWidget(tbbdynamic, 0, MAIN_SCREEN);
		gui->registerWidget(tbbsolid, 0, MAIN_SCREEN);
		//gui->registerWidget(tbbnonsolid, 0, MAIN_SCREEN);
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
	// Check input
	scanKeys();
	u16 keysdown = keysDown();
	//u16 keysup = keysUp();
	u16 keysheld = keysHeld();
	touchPosition touch = touchReadXY();
	
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
			canvas->penUp(touch.px, touch.py);
			gui->penUp(touch.px, touch.py);
			CommandStopSample(0);
			
			touch_was_down = 0;
		}
		else if(touch_was_down && PEN_DOWN)
		{
			if(!got_good_pen_reading) // PenDown
			{
				if(onCanvas(touch.px, touch.py))
				{
					CommandPlaySample(smp_crayon, 48, 255, 0);
					canvas->penDown(touch.px, touch.py);
				}
				else
					gui->penDown(touch.px, touch.py);		
				
				got_good_pen_reading = 1;
			}
			
			if((abs(touch.px - lastx)>0) || (abs(touch.py - lasty)>0)) // PenMove
			{
				if(onCanvas(touch.px, touch.py))
					canvas->penMove(touch.px, touch.py);
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
	
	float32 xgrav = (float)((Xaccel - motion_x_offset) * 35) / 1638.0f;
	float32 ygrav = (float)((Yaccel - motion_y_offset) * 35) / 1638.0f;
	
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
		BLEND_Y = 31-i/2;
	
		ulStartDrawing2D();
		ulSetAlpha(UL_FX_DEFAULT, 0, 0);
		ulDrawImageXY(imgbg, 0, 0);
		drawSideBar(); drawBottomBar();
		ulEndDrawing();
		ulSyncFrame();
	}
	BLEND_Y = 0;
}

int main()
{
	ulInit(UL_INIT_ALL);
	
	ulInitGfx();
	
	videoSetMode(MODE_3_3D | DISPLAY_BG3_ACTIVE);
#ifdef DEBUG
	videoSetModeSub(MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG2_ACTIVE);
#else
	videoSetModeSub(MODE_5_2D | DISPLAY_BG2_ACTIVE);
#endif
	vramSetBankB(VRAM_B_MAIN_BG_0x06000000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	
	BG0_CR = BG_PRIORITY(1);
	
	// Main BG3: ERB
	BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(2) | BG_PRIORITY(0);
	BG3_XDX = 1 << 8;
	BG3_XDY = 0;
	BG3_YDX = 0;
	BG3_YDY = 1 << 8;
	
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
	
#ifdef DEBUG
	printf("Pocket Physics Debug build\n");
#endif
	
	lcdMainOnBottom();
	
	CommandInit();
	
	loadSamples();
#ifndef DEBUG
	showSplash();
#endif
	world = new World(231, 170);
	canvas = new Canvas(world);
	
	theme = new Theme;
	
	u16 col = RGB15(0,0,0)|BIT(15);
	u32 colcol = col | col << 16;
	swiFastCopy(&colcol, sub_vram, 192*256/2 | COPY_MODE_FILL);
	
	drawMainBg();
	setupGui();
	
	//Initialize the text part
	ulInitText();

	ul_firstPaletteColorOpaque=2;
	imgbg = ulLoadImageFilePNG((const char*)paper2_png, (int)paper2_png_size, UL_IN_VRAM, UL_PF_PAL8);
	
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
	
	irqSet(IRQ_VBLANK, fpstimer);
	irqEnable(IRQ_VBLANK);
	
	for(int i=0;i<60;++i)
		framesdone[i] = true;
	
	fadeIn();
	
	init = false;
	
	while(1)
	{
		
		ulStartDrawing2D();
		
		glTranslatef(((float)scx)/256.0f, 0.0f, 0.0f);
		
		ulSetAlpha(UL_FX_DEFAULT, 0, 0);
		
		ulDrawImageXY(imgbg, 0, 0);
		drawSideBar(); drawBottomBar();
		
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
		
		ulSetAlpha(UL_FX_ALPHA, 31, 1);
		
		canvas->draw();
		
		ulEndDrawing();
		
		handleInput();
		
		CommandProcessCommands();
		
		framedone = true;
		
		//Wait the VBlank (synchronize at 60 fps)
		ulSyncFrame();
	}

	//Program end - should never get there
	return 0;
}

