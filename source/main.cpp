/*
 *    Pocket Physics - A mechanical construction kit for Nintendo DS
 *                   Copyright 2005-2010 Tobias Weyand (me@tobw.net)
 *                            http://code.google.com/p/pocketphysics
 *
 * Pocket Physics is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Pocket Physics is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with Pocket Physics. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "Box2D.h"
#include "tobkit/tobkit.h"
#include "loaddialog.h"
#include <ulib/ulib.h>

#include <nds/arm9/ndsmotion.h>
#include <fat.h>
#include <sys/dir.h>

#include "world.h"
#include "polygon.h"
#include "canvas.h"
#include "state.h"

#include "tools.h"

#include "pocketphysics_png.h"

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
#include "icon_move_raw.h"
#include "icon_zap_raw.h"
#include "icon_save_raw.h"
#include "icon_load_raw.h"

#include "sound_click_raw.h"
#include "sound_pen_raw.h"
#include "sound_play_raw.h"
#include "sound_del_raw.h"

#define min(x,y)	((x)<(y)?(x):(y))

#define PEN_DOWN (~IPC->buttons & (1 << 6))

//#define DEBUG

#define WORLD_WIDTH		(3*256)
#define WORLD_HEIGHT	(3*192)

const bool DUAL_SCREEN = true;

u16 keysdown = 0, keysheld = 0, keysup = 0;
touchPosition touch;

bool stylus_scrolling;

int touch_was_down = 0;
int lastx, lasty;
u8 got_good_pen_reading = 0;

bool dsmotion = false;

bool init = true;

int motion_x_offset = 2048;
int motion_y_offset = 2048;

int fps = 60;
int slowfps = 60;
int currentframe = 0;
bool framesdone[60];
bool framedone = false;
int accumulated_timesteps = 0;
int passed_frames = 0;
int passed_physics_ticks = 0;
int sample_pen_id;

bool main_screen_active = false;
bool dialog_active = false; // If a dislog is active, send all pen events to the GUI
bool fat_ok = false;

std::string current_filename;



u16 *main_vram = (u16*) BG_BMP_RAM(2);
u16 *sub_vram = (u16*) BG_BMP_RAM_SUB(2);

World *world;

State state;

TobKit::GUI *gui_main, *gui_sub;

TobKit::Canvas *canvas;

TobKit::MessageBox *mb;
TobKit::Typewriter *tw;
TobKit::PPLoadDialog *load_dialog;

// <Side Bar>
//BitButton *btnflipmain, *btnflipsub;
TobKit::BitButton *btnplay, *btnstop, *btnpause, *btnload, *btnsave;
TobKit::AlternativeButton::AlternativeButtonGroup *tbbgsidebar;
TobKit::ToggleBitButton /* *tbbselect,*/*tbbdynamic, *tbbsolid, *tbbnonsolid;
// </Side Bar>

// <Bottom Bar>
TobKit::ToggleBitButton *tbbbox, *tbbpolygon, *tbbcircle, *tbbpin, *tbbdelete,
		*tbbmove;
TobKit::AlternativeButton::AlternativeButtonGroup *tbbgobjects;
TobKit::BitButton *buttonzap;
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

	if (state.simulating)
		accumulated_timesteps++;

	passed_frames++; // same as accumulated timesteps but for scrolling

	framesdone[currentframe] = framedone;

	if (!framedone)
		--fps;
	if (!framesdone[(currentframe + 1) % 60])
		++fps;

	currentframe = (currentframe + 1) % 60;

	//if(currentframe == 0)
	//	printf("%d\n",fps);

	// Adjust the timestep slowly! This buffers sudden fps increases which
	// would result in "bullet-time" effects.
	if (currentframe % 10 == 0) {
		if (slowfps < fps)
			slowfps++;
		else if (slowfps > fps)
			slowfps--;

		// Commented out because dynamic timestep adjustment makes simulation nondeterministic
		//timeStep = 2.0f / (float)slowfps;
	}

	framedone = false;
}

void drawBgBox(void)
{
	ulSetAlpha(UL_FX_ALPHA, 10, 1);
	ulDrawFillRect(16, 15, 16 + 219, 15 + 162, RGB15(0,0,0));
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

    if (ulGetMainLcd() || (!DUAL_SCREEN)) // Bottom Screen
    {
    	if(DUAL_SCREEN) {
    		videoSetMode(MODE_3_3D);
    	}

    	canvas->draw();

        glLoadIdentity();
        if(!dialog_active) {
            drawSideBar();
            drawBottomBar();
        } else {
            drawBgBox();
        }
    }
    else if(DUAL_SCREEN) // Top Screen
    {
        videoSetMode(MODE_3_3D | DISPLAY_BG1_ACTIVE | DISPLAY_BG3_ACTIVE);
        canvas->draw();
    }

	ulEndDrawing();
	framedone = true;
}

void updateInput(void)
{
	touchRead(&touch);
	scanKeys();
	keysdown = keysDown();
	keysup = keysUp();
	keysheld = keysHeld();
}

void VBlankHandler()
{
	// And now for something completely ugly:
	// Save the values of the div and sqrt registers, because the vblank might interrupt
	// a division/sqrt operation
	while (REG_SQRTCNT & SQRT_BUSY) {
	};
	while (REG_DIVCNT & DIV_BUSY) {
	};
	u16 divcr = REG_DIVCNT;
	int64 divnum64 = REG_DIV_NUMER;
	int64 divdenom64 = REG_DIV_DENOM;

	u16 sqrtcr = REG_SQRTCNT;
	int64 sqrtparam64 = REG_SQRT_PARAM;

	mearureFps();
	updateInput();

	draw();

	// Now restore the div/sqrt registers
	REG_DIVCNT = divcr;
	while (REG_DIVCNT & DIV_BUSY) {
	};
	REG_DIV_NUMER = divnum64;
	REG_DIV_DENOM = divdenom64;

	REG_SQRTCNT = sqrtcr;
	while (REG_SQRTCNT & SQRT_BUSY) {
	};
	REG_SQRT_PARAM = sqrtparam64;

	while (REG_SQRTCNT & SQRT_BUSY) {
	};
	while (REG_DIVCNT & DIV_BUSY) {
	};
}

void switchScreens(void)
{
	lcdSwap();
}

void drawMainBg()
{
	drawSideBar();
	drawBottomBar();
}

void tbbgsidebarChanged(int item)
{
	if (!init) {
		soundPlaySample(sound_click_raw, SoundFormat_16Bit,
				sound_click_raw_size, 16381, 127, 64, false, 0);
	}

	switch (item) {
		//case 0: // Select
		//	canvas->setPenMode(Canvas::pmNormal);
		//	break;

		case 0: // Dynamic
			canvas->setObjectMode(TobKit::Canvas::omDynamic);
			break;

		case 1: // Solid
			canvas->setObjectMode(TobKit::Canvas::omSolid);
			break;

		case 2: // Non-Solid
			canvas->setObjectMode(TobKit::Canvas::omNonSolid);
			break;

		default:
			printf("Unhandled sidebar icon?!\n");
			break;
	}
}

void penModeChanged(s8 item)
{
	if (!init) {
		soundPlaySample(sound_click_raw, SoundFormat_16Bit,
				sound_click_raw_size, 16381, 127, 64, false, 0);
	}

	switch (item) {
		case -1:
			canvas->setPenMode(TobKit::Canvas::pmNormal);
			break;

		case 0:
			canvas->setPenMode(TobKit::Canvas::pmPolygon);
			break;

		case 1:
			canvas->setPenMode(TobKit::Canvas::pmBox);
			break;

		case 2:
			canvas->setPenMode(TobKit::Canvas::pmCircle);
			break;

		case 3:
			canvas->setPenMode(TobKit::Canvas::pmPin);
			break;

		case 4:
			canvas->setPenMode(TobKit::Canvas::pmMove);
			break;

		case 5:
			canvas->setPenMode(TobKit::Canvas::pmDelete);
			break;

		default:
			printf("maeh.\n");
			break;
	}
}

void startPlay(void)
{
	soundPlaySample(sound_play_raw, SoundFormat_16Bit, sound_play_raw_size,
			16381, 127, 64, false, 0);
	state.simulating = true;
	btnplay->hide();
	btnpause->show();
	canvas->startSimulationMode();
}

void pausePlay(void)
{
	soundPlaySample(sound_play_raw, SoundFormat_16Bit, sound_play_raw_size,
			16381, 127, 64, false, 0);
	state.simulating = false;
	btnpause->hide();
	btnplay->show();
	canvas->stopSimulationMode();
}

void stopPlay(void)
{
	canvas->disableDrawing();
	soundPlaySample(sound_play_raw, SoundFormat_16Bit, sound_play_raw_size,
			16381, 127, 64, false, 0);
	state.simulating = false;
	world->reset(!dsmotion);
	btnpause->hide();
	btnplay->show();
	canvas->stopSimulationMode();
	canvas->enableDrawing();
}

void deleteMessageBox(void)
{
	delete mb;
	mb = 0;
}

void zap(void)
{
	stopPlay();
	canvas->disableDrawing();

	while (world->getNThings() > 0) {
		Thing *t = world->getThing(0);
		world->remove(t);
		delete t;
	}

	soundPlaySample(sound_del_raw, SoundFormat_16Bit, sound_del_raw_size, 16381,
			127, 64, false, 0);
	deleteMessageBox();

	canvas->enableDrawing();
}

void askZap(void)
{
	mb = new TobKit::MessageBox(gui_main, "destroy the world", 2, "yes", "no");
	mb->getSignal(0).connect(sigc::ptr_fun(zap));
	mb->getSignal(1).connect(sigc::ptr_fun(deleteMessageBox));
}

inline u16 avgcol(u16 col1, u16 col2, u16 col3, u16 col4)
{
	return (((col1 & 31) + (col2 & 31) + (col3 & 31) + (col4 & 31)) / 4)
			| (((((col1 >> 5) & 31) + ((col2 >> 5) & 31) + ((col3 >> 5) & 31)
					+ ((col4 >> 5) & 31)) / 4) << 5)
			| (((((col1 >> 10) & 31) + ((col2 >> 10) & 31) + ((col3 >> 10) & 31)
					+ ((col4 >> 10) & 31)) / 4) << 10);
}

char *b64screenshot(void)
{
	bool draw_window_tmp = canvas->getDrawWindow();
	canvas->setDrawWindow(false);

	// Wait until a full frame (top and bootom screens) was drawn and
	// capturing is finished
	while (!ulGetMainLcd())
		;
	while (ulGetMainLcd())
		;
	while (!ulGetMainLcd())
		;
	while (ulGetMainLcd())
		;
	while (REG_DISPCAPCNT & BIT(31))
		;

	// Copy VRAM_C, becase this will take longer than a frame
	u16 *screen = (u16*) malloc(sizeof(u16*) * 256 * 192);
	dmaCopy(VRAM_C, screen, 256 * 192 * 2);

	canvas->setDrawWindow(draw_window_tmp);

	u16 linebuffer[4][64] = { { 0 } };

	u16 *screen_scaled = (u16*) malloc(2 * 64 * 48);

	// Scale the image using some cheap box filter. Looks decent IMO.
	int bufline = 0;
	for (int y = 0; y < 192; ++y) {
		for (int x = 0; x < 64; ++x) {
			linebuffer[bufline][x] = avgcol(screen[256 * y + (4 * x) + 0],
					screen[256 * y + (4 * x) + 1],
					screen[256 * y + (4 * x) + 2],
					screen[256 * y + (4 * x) + 3]);
		}

		bufline++;

		if (bufline == 4) {
			for (int x = 0; x < 64; ++x) {
				screen_scaled[64 * (y / 4) + x] = avgcol(linebuffer[0][x],
						linebuffer[1][x], linebuffer[2][x],
						linebuffer[3][x]) | BIT(15);
			}

			bufline = 0;
		}
	}

	char *b64screen;
	b64encode((u8*) screen_scaled, 2 * 64 * 48, &b64screen);

	free(screen);
	free(screen_scaled);

	return b64screen;
}

void save(void)
{
	char *thumbnail = 0;
#ifndef SCLITE
	thumbnail = b64screenshot();
#endif
	world->save(current_filename.c_str(), thumbnail);
	free(thumbnail);
}

void showTypewriter(const char *prompt, const char *str,
		void (*okCallback)(void), void (*cancelCallback)(void))
{
	dialog_active = true;

	tw = new TobKit::Typewriter(gui_main, prompt, str);

	if (okCallback != 0) {
		tw->signal_ok.connect(sigc::ptr_fun(okCallback));
	}
	if (cancelCallback != 0) {
		tw->signal_cancel.connect(sigc::ptr_fun(cancelCallback));
	}
	tw->show();
}

void deleteTypewriter(void)
{
	delete tw;
	dialog_active = false;
}

void overwriteFile(void)
{
	save();
	deleteMessageBox();
}

void handleSaveDialogOk(void)
{
	std::string text = tw->getText();

	if (text != "") {
		std::string name;
		// Append extension if neccessary
		if (text.substr(text.length() - 3, 3) != ".pp") {
			name = text + ".pp";
		} else {
			// Leave as is
			name = text;
		}

		deleteTypewriter();

		printf("Saving %s\n", name.c_str());

		current_filename = name;

		// Check if the file already exists
		std::string fullname = "pocketphysics/sketches/" + name;
		FILE* f = fopen(fullname.c_str(), "r");
		if (f != 0) {
			mb = new TobKit::MessageBox(gui_main, "Overwrite file?", 2, "yes",
					"no");
			mb->getSignal(0).connect(sigc::ptr_fun(overwriteFile));
			mb->getSignal(1).connect(sigc::ptr_fun(deleteMessageBox));
		} else {
			fclose(f);
			save();
		}
	}
}

void deleteLoadDialog(void)
{
	delete load_dialog;
	dialog_active = false;
}

void handleLoadDialogOk(void)
{
	printf("Loading %s\n", load_dialog->getFilename().c_str());
	world->load(load_dialog->getFilename().c_str());
	current_filename = load_dialog->getFilename();
	deleteLoadDialog();
}

void showLoadDialog(void)
{
	soundPlaySample(sound_play_raw, SoundFormat_16Bit, sound_play_raw_size,
			16381, 127, 64, false, 0);

	load_dialog = new TobKit::PPLoadDialog(gui_main);
	dialog_active = true;

	load_dialog->signal_ok.connect(sigc::ptr_fun(handleLoadDialogOk));
	load_dialog->signal_cancel.connect(sigc::ptr_fun(deleteLoadDialog));

	load_dialog->show();
}

void showSaveDialog(void)
{
	soundPlaySample(sound_play_raw, SoundFormat_16Bit, sound_play_raw_size,
			16381, 127, 64, false, 0);

	showTypewriter("save as", current_filename.c_str(), handleSaveDialogOk,
			deleteTypewriter);
}

void setupGui(void)
{
	// <Side Bar>
	tbbgsidebar = new TobKit::AlternativeButton::AlternativeButtonGroup();

	//tbbselect = new ToggleBitButton(233, 22, 22, 19, &main_vram, icon_select_raw, 18, 15, 1, 2, tbbgsidebar);
	tbbdynamic = new TobKit::ToggleBitButton(gui_main, icon_dynamic_raw, 233, 1,
			tbbgsidebar, 22, 19, 18, 15, 1, 2);
	//
	tbbsolid = new TobKit::ToggleBitButton(gui_main, icon_solid_raw, 233, 21,
			tbbgsidebar, 22, 19, 18, 15, 1, 2);

	//tbbnonsolid = new ToggleBitButton(233, 41, 22, 19, &main_vram, icon_nonsolid_raw, 18, 15, 1, 2, tbbgsidebar);

	tbbgsidebar->signal_changed.connect(sigc::ptr_fun(tbbgsidebarChanged));

	btnstop = new TobKit::BitButton(gui_main, icon_stop_raw, 233, 152, 22, 19,
			12, 12, 5, 1);
	btnstop->signal_pushed.connect(sigc::ptr_fun(stopPlay));

	btnplay = new TobKit::BitButton(gui_main, icon_play_raw, 233, 172, 22, 19,
			12, 12, 5, 1);
	btnplay->signal_pushed.connect(sigc::ptr_fun(startPlay));

	btnpause = new TobKit::BitButton(gui_main, icon_pause_raw, 233, 172, 22, 19,
			12, 12, 5, 1, 0, false);
	btnpause->signal_pushed.connect(sigc::ptr_fun(pausePlay));

#ifndef DEBUG
	if(fat_ok)
	{
#endif
	btnload = new TobKit::BitButton(gui_main, icon_load_raw, 233, 101, 22, 19,
			18, 15, 2, 1);
	btnload->signal_pushed.connect(sigc::ptr_fun(showLoadDialog));

	btnsave = new TobKit::BitButton(gui_main, icon_save_raw, 233, 121, 22, 19,
			18, 15, 2, 1);
	btnsave->signal_pushed.connect(sigc::ptr_fun(showSaveDialog));
#ifndef DEBUG
}
#endif
	// </Side Bar>

	// <Bottom Bar>
	tbbgobjects = new TobKit::AlternativeButton::AlternativeButtonGroup();

	tbbpolygon = new TobKit::ToggleBitButton(gui_main, icon_polygon_raw, 1, 172,
			tbbgobjects, 22, 19, 18, 15, 2, 2);
	tbbbox = new TobKit::ToggleBitButton(gui_main, icon_box_raw, 24, 172,
			tbbgobjects, 22, 19, 18, 15, 2, 2);
	tbbcircle = new TobKit::ToggleBitButton(gui_main, icon_circle_raw, 47, 172,
			tbbgobjects, 22, 19, 18, 15, 2, 2);
	tbbpin = new TobKit::ToggleBitButton(gui_main, icon_pin_raw, 70, 172,
			tbbgobjects, 22, 19, 18, 15, 2, 2);

	tbbmove = new TobKit::ToggleBitButton(gui_main, icon_move_raw, 152, 172,
			tbbgobjects, 22, 19, 18, 15, 2, 2);
	tbbdelete = new TobKit::ToggleBitButton(gui_main, icon_delete_raw, 175, 172,
			tbbgobjects, 22, 19, 18, 15, 2, 2);

	tbbgobjects->signal_changed.connect(sigc::ptr_fun(penModeChanged));

	buttonzap = new TobKit::BitButton(gui_main, icon_zap_raw, 198, 172, 22, 19,
			18, 15, 2, 2);
	buttonzap->signal_pushed.connect(sigc::ptr_fun(askZap));
	// </Bottom Bar>

	tbbgsidebar->setChecked(0);
	tbbgobjects->setChecked(0);
}

void calibrate_motion(void)
{
	iprintf("OK, calibrating\n");

	s32 xmean = 0, ymean = 0;
	for (int i = 0; i < 30; ++i) {
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
	mb = new TobKit::MessageBox(gui_main, "please put the ds on a table", 1,
			"calibrate dsmotion!");
	mb->getSignal(0).connect(sigc::ptr_fun(calibrate_motion));
}

void handleInput(void)
{
	// Prevent drawing while scrolling with pen
	/*
	 if(keysdown & (KEY_L | KEY_R)) {
	 canvas->penUp(touch.px, touch.py);
	 soundKill(sample_pen_id);
	 }

	 if(keysdown && dialog_active) { // Send keys to the gui (for left/right navigating in keyboard)
	 gui_main->buttonPress(keysdown);
	 }

	 if(keysup && dialog_active)	{
	 gui_main->buttonRelease(keysup);
	 }
	 */
	if (keysdown & KEY_SELECT) {
		canvas->setDrawWindow(!canvas->getDrawWindow());
	}

	gui_main->handleInput(keysdown, keysup, keysheld, touch);
	/*
	 if(!touch_was_down && PEN_DOWN)	{
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
	 soundKill(sample_pen_id);

	 touch_was_down = 0;
	 }
	 else if(touch_was_down && PEN_DOWN)
	 {
	 if(!got_good_pen_reading) // PenDown
	 {
	 if(!stylus_scrolling && onCanvas(touch.px, touch.py) && !dialog_active)
	 {
	 sample_pen_id = soundPlaySample(sound_pen_raw, SoundFormat_16Bit, sound_pen_raw_size, 16381, 127, 64, true, sound_pen_raw_size/10);
	 canvas->penDown(touch.px + scroll_x, touch.py + scroll_y);
	 } else {
	 gui->penDown(touch.px, touch.py);
	 }

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
	 */

	// DSMotion shortcut for swappers
	if ((keysheld & KEY_L) && (keysheld & KEY_R)) {
		if (motion_init() != 0) {
			dsmotion = true;
			world->reset(false);

			request_calibration();
		} else {
			dsmotion = false;
			world->reset(true);

			iprintf("Get a DSMotion. They are fun!\n");
		}
	}

	passed_frames = 0;

	/*
	 * TODO: move this to canvas
	 // Special case: User holds down the pen and scrolls => generate penmove event
	 if( ( (scroll_vx != 0) || (scroll_vy != 0) )
	 && (touch_was_down && (keysdown & KEY_TOUCH))
	 && (onCanvas(touch.px, touch.py) ) )
	 canvas->penMove(touch.px + scroll_x, touch.py + scroll_y);
	 */
}
/*
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
 */

void updateGravity()
{
	int Xaccel = motion_read_x(); // this returns a value between 0 and 4095
	int Yaccel = motion_read_y(); // this returns a value between 0 and 4095

	float32 xgrav = (float) ((Xaccel - motion_x_offset) * 5) / 1638.0f;
	float32 ygrav = (float) ((Yaccel - motion_y_offset) * 5) / 1638.0f;

	world->setGravity(-xgrav, ygrav);
}

void showSplash(void)
{
	videoSetModeSub(MODE_5_2D);

	UL_IMAGE *imgpp = ulLoadImageFilePNG((const char*) pocketphysics_png,
			(int) pocketphysics_png_size, UL_IN_VRAM, UL_PF_PAL8);
	for (int i = 0; i < 60; ++i) {
		REG_BLDY = 31 - i / 2;
		ulStartDrawing2D();

		ulDrawImageXY(imgpp, 0, 0);

		ulEndDrawing();
		ulSyncFrame();
	}
	scanKeys();
	while (!keysDown()) {
		REG_BLDY = 0;
		ulStartDrawing2D();

		ulDrawImageXY(imgpp, 0, 0);

		ulEndDrawing();
		ulSyncFrame();

		scanKeys();
	}
	for (int i = 60; i > 0; --i) {
		REG_BLDY = 31 - i / 2;
		ulStartDrawing2D();

		ulDrawImageXY(imgpp, 0, 0);

		ulEndDrawing();
		ulSyncFrame();
	}
	ulDeleteImage(imgpp);
}

// TODO: Instead of copying code from draw, fade in by just tweaking te blend registers while vblank is alrady running
void fadeIn()
{
//	for (int i = 0; i < 60; ++i) {
//		ulSyncFrame();
//
//		REG_BLDY = 31 - i / 2;
//
//		ulStartDrawing2D();
//
//		if (ulGetMainLcd()) // Bottom Screen
//		{
//			videoSetMode(MODE_3_3D);
//
//			glMatrixMode(GL_PROJECTION);
//			glLoadIdentity();
//			glOrthof32(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -4090, 1);
//			glMatrixMode(GL_MODELVIEW);
//
//			glTranslate3f32(-scroll_x, 0, 0);
//			glTranslate3f32(0, -scroll_y, 0);
//
//			ulSetAlpha(UL_FX_DEFAULT, 0, 0);
//			ulDrawImageXY(imgbg, 0, 0);
//			ulSetAlpha(UL_FX_ALPHA, 31, 1);
//
//			canvas->draw();
//
//			glLoadIdentity();
//			drawSideBar();
//			drawBottomBar();
//
//		} else // Top Screen
//		{
//			videoSetMode(MODE_3_3D | DISPLAY_BG3_ACTIVE);
//			glMatrixMode(GL_PROJECTION);
//			glLoadIdentity();
//			glOrthof32(0, WORLD_WIDTH, WORLD_HEIGHT, 0, -4090, 1);
//			glMatrixMode(GL_MODELVIEW);
//
//			ulSetAlpha(UL_FX_DEFAULT, 0, 0);
//
//			ulDrawImageXY(imgbg, 0, 0);
//
//			ulSetAlpha(UL_FX_ALPHA, 31, 1);
//
//			canvas->draw();
//			canvas->drawScreenRect(scroll_x, scroll_y);
//		}
//
//		ulEndDrawing();
//	}
//	REG_BLDY = 0;
//	REG_BLDY_SUB = 0;
}

int main()
{
	ulInit(UL_INIT_LIBONLY);
	ulInitGfx();

	soundEnable();

#ifdef DEBUG
	defaultExceptionHandler();
#endif

	// Video configuration of Pocket Physics:
	//
	// VRAM:
	//       Bank  Screen  Mapping     From    Use
	//
	//       A     Main    Texture     uLib    3D
	//       B     Main    0x06000000  TobKit  GUI
	//       C     Sub     0x06000000  uLib    Capture (dual-screen 3D)
	//       D     Sub     Sprite      uLib    Capture (dual-screen 3D)
	//
	// Backgrounds:
	//
	//   Main Screen: Mode_3_3D
	//
	//     BG  Mode            Use
	//     0   3D
	//     1   Tiles           Console
	//     3   ERB 16 Bit BMP  TobKit GUI
	//
	//   Sub Screen: Mode_5_2D
	//
	//     BG       Mode            Use
	//     2        ERB 16 Bit BMP  Not Sure
	//     Sprites                  3D on both screens, showing captured image using 4 sprites

	TobKit::Theme theme_trans_bg;
	theme_trans_bg.col_bg = 0; // Make the background transparent

	// Sets up main screen to MODE_3_2D and sets up bg layer 3 in 16 bit bitmap mode
	gui_main = TobKit::GUI::instance(TobKit::MAIN_SCREEN, theme_trans_bg);

#ifdef DEBUG
	// Sets up sub screen bg layer 1 in console mode
	gui_main->setupConsole();
	printf("Pocket Physics Debug build\n");
#endif

#ifndef DEBUG
	videoSetMode(MODE_3_3D | DISPLAY_BG3_ACTIVE);
	REG_BLDCNT = BLEND_FADE_BLACK | BLEND_SRC_BG0 | BLEND_SRC_BG3;
	REG_BLDCNT_SUB = BLEND_FADE_BLACK | BLEND_SRC_BG2;
	showSplash();
#endif

	if (DUAL_SCREEN) {
		ulInitDualScreenMode();
	}
	videoSetMode(MODE_3_3D | DISPLAY_BG1_ACTIVE | DISPLAY_BG3_ACTIVE);
	bgSetPriority(0, 2); // Put the 3D layer in the background
	bgSetPriority(3, 1); // Put the 2D layer in the foreground
	bgSetPriority(1, 0); // Put the tile layer (keyboard) on top

	lcdMainOnBottom();

	world = new World(WORLD_WIDTH, WORLD_HEIGHT);
	canvas = new TobKit::Canvas(gui_main, world, 0, 0, 231, 170,
			KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN, DUAL_SCREEN);

	printf("Initializing FAT\n");

	fat_ok = fatInitDefault();
#ifdef DEBUG
	if (fat_ok)
		printf("OK\n");
	else
		printf("Fail!\n");
#endif

	setupGui();

	ul_firstPaletteColorOpaque = 2;


	if (motion_init() != 0) {
		dsmotion = true;

		world->reset(false);
		request_calibration();
	} else {
		dsmotion = false;
		world->reset(true);

		iprintf("Get a DSMotion. They are fun!\n");
	}

	irqEnable(IRQ_VBLANK);

	memset(framesdone, 1, sizeof(bool) * 60);

#ifndef DEBUG
	fadeIn();
#endif

	irqSet(IRQ_VBLANK, VBlankHandler);

	REG_BLDCNT = 0;
	REG_BLDCNT_SUB = 0;

	// TODO: What the hell's this for?
	init = false;

	while (true) {
		if (state.simulating) {
			if (accumulated_timesteps < 1) {
				swiWaitForVBlank();
			}
			accumulated_timesteps = 0;

			if (dsmotion) {
				updateGravity();
			}
			world->step();
		} else {
			swiWaitForVBlank();
		}

		handleInput();
	}

	//Program end - should never get there
	return 0;
}
