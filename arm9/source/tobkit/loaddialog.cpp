#include "loaddialog.h"

#include <fat.h>
#include <sys/types.h>
#include <sys/dir.h>

#include <tinyxml.h>

#include "tools.h"

#include "icon_play_raw.h"
#include "icon_back_raw.h"
#include "icon_delete_file_raw.h"

#include "../../../generic/command.h"

#define LOAD_DIALOG_WIDTH	219
#define LOAD_DIALOG_HEIGHT	162

#define POLAROID_WIDTH	68
#define POLAROID_HEIGHT	65

extern Sample *smp_click, *smp_del;

/* ===================== PUBLIC ===================== */

PPLoadDialog::PPLoadDialog(uint16 **_vram):
	Widget((SCREEN_WIDTH-LOAD_DIALOG_WIDTH)/2, (SCREEN_HEIGHT-LOAD_DIALOG_HEIGHT)/2,
			LOAD_DIALOG_WIDTH, LOAD_DIALOG_HEIGHT, _vram),
	page(0), showfwd(false), showback(false), onOk(0), resultname(0), ask_delete_confirm(false), file_to_delete(0)
{
	buttoncancel = new Button(x+85, y+138, 53, 19, _vram);
	buttoncancel->setCaption("cancel");
	
	thumbnails = (u16**)calloc(1, sizeof(u16*)*6);
	names = (char**)calloc(1, sizeof(char*)*6);
	
	datadir = "";
	if(diropen("/data/pocketphysics/sketches"))
		datadir = "/data/pocketphysics/sketches";
	else if(diropen("/pocketphysics/sketches"))
		datadir = "/pocketphysics/sketches";
	
	loadThumbnails();
}

PPLoadDialog::~PPLoadDialog()
{
	delete buttoncancel;
	freeThumbnails();
	free(thumbnails);
	free(names);
}

// Drawing request
void PPLoadDialog::pleaseDraw(void)
{
	draw();
}

// Event calls
void PPLoadDialog::penDown(u8 px, u8 py)
{
	if(!ask_delete_confirm)
	{
		// On the cancel button?
		if( (px>=x+85) && (px<=x+85+53) && (py>=y+138) && (py<=y+138+19) )
		{
			CommandPlaySample(smp_click, 48, 255, 0);
			buttoncancel->penDown(px, py);
		}
		
		// On the back button?
		if(showback && (px>=x+5) && (px<=x+5+22) && (py>=y+138) && (py<=y+138+19) )
		{
			page--;
			if(page == 0)
				showback = false;
			CommandPlaySample(smp_click, 48, 255, 0);
			loadThumbnails();
			draw();
		}

		// On the fwd button?
		if(showfwd && (px>=x+189) && (px<=x+189+22) && (py>=y+138) && (py<=y+138+19) )
		{
			page++;
			showback = true;
			CommandPlaySample(smp_click, 48, 255, 0);
			loadThumbnails();
			draw();
		}

		// On a thumbnail?
		if( (px > x+8) && (px < x+214) && (py > y+5) && (py < y+136))
		{
			int xidx = (px - x - 8) / 69;
			int yidx = (py - y - 5) / 66;
			int thumb = 3*yidx+xidx;
			if(names[thumb])
			{
				// On the delete icon?
				int relx = (px - x - 8) % 69;
				int rely = (py - y - 5) % 66;
				if( (relx >= POLAROID_WIDTH-12) && (relx <= POLAROID_WIDTH) && (rely < 9) )
				{
					// Yes, ask if the user wants to delete the file
					ask_delete_confirm = true;
					file_to_delete = thumb;
					draw();
					CommandPlaySample(smp_click, 48, 255, 0);
				}
				else if(onOk)
				{
					// No, load the file
					resultname = names[thumb];
					CommandPlaySample(smp_click, 48, 255, 0);
					onOk();
				}
			}
		}
	}
	else
	{
		// Delete dialog
		
		// No button
		if(isInRect(px-x, py-y, width / 2 - 40, 115, width / 2 - 40 + 32, 115 + 14))
		{
			ask_delete_confirm = false;
			CommandPlaySample(smp_click, 48, 255, 0);
			draw();
		}
		
		// Yes button
		if(isInRect(px-x, py-y, width / 2 + 40 - 32, 115, width / 2 + 40, 115 + 14))
		{
			char *f = (char*)calloc(1, 255);
			sprintf(f, "%s/%s", datadir, names[file_to_delete]);
			printf("deleting %s", f);
			remove(f);
			free(f);
			
			CommandPlaySample(smp_del, 48, 255, 0);
			loadThumbnails();
			ask_delete_confirm = false;
			draw();
		}
	}
}

void PPLoadDialog::penUp(u8 px, u8 py)
{
	if(!ask_delete_confirm)
	{
		// On the button?
		if( (px>=x+85) && (px<=x+85+53) && (py>=y+138) && (py<=y+138+19) )
			buttoncancel->penUp(px, py);
	}
}

// Callback registration
void PPLoadDialog::registerOkCallback(void (*onOk_)(void))
{
	onOk = onOk_;
}

void PPLoadDialog::registerCancelCallback(void (*onCancel_)(void))
{
	buttoncancel->registerPushCallback(onCancel_);
}
		
void PPLoadDialog::show(void)
{
	buttoncancel->show();
	
	if(!visible)
	{
		visible = true;
		pleaseDraw();
	}
}

void PPLoadDialog::setTheme(Theme *theme_)
{
	buttoncancel->setTheme(theme_);
		
	theme = theme_;
}

char *PPLoadDialog::getFilename(void)
{
	return resultname;
}

/* ===================== PRIVATE ===================== */


void PPLoadDialog::draw(void)
{
	drawFullBox(0, 0, LOAD_DIALOG_WIDTH, LOAD_DIALOG_HEIGHT, 0);
	//drawBorder();
	
	bool empty = true;
	for(int i=0;i<6;++i)
		if(names[i])
			empty = false;
	
	if(empty)
		drawString("nothing here yet", 65, LOAD_DIALOG_HEIGHT/2-5, 100, theme->col_text);
	else if(ask_delete_confirm)
	{
		int strx = (width - getStringWidth("delete this sketch")) / 2;
		drawString("delete this sketch", strx, 20, 255, RGB15(31,31,31)|BIT(15));
		drawPolaroid((width - POLAROID_WIDTH)/2, 40, file_to_delete, false);
		
		// yes and no buttons
		drawGradient(theme->col_dark_ctrl, theme->col_light_ctrl, width / 2 - 40, 115, 32, 14);
		drawBox(width / 2 - 40, 115, 32, 14, theme->col_outline);
		drawString("no", width / 2 - 42 + 12, 117);
		
		drawGradient(theme->col_dark_ctrl, theme->col_light_ctrl, width / 2 + 40 - 32, 115, 32, 14);
		drawBox(width / 2 + 40 - 32, 115, 32, 14, theme->col_outline);
		drawString("yes", width / 2 + 40 + 5 - 32, 117);
	}
	else
	{
		for(int by=0; by<2; ++by)
		{
			for(int bx=0; bx<3; ++bx)
			{
				int idx = 3*by+bx;
				if(names[idx])
					drawPolaroid(5+(POLAROID_WIDTH+1)*bx, 5+(POLAROID_HEIGHT+1)*by, idx);
			}
		}
		
		buttoncancel->pleaseDraw();
		
		// fwd and back buttons
		if(showback)
		{
			drawGradient(theme->col_dark_ctrl, theme->col_light_ctrl, 5, 138, 22, 19);
			drawBox(5, 138, 22, 19, theme->col_outline);
			drawMonochromeIcon(5+5, 138+1, 12, 12, icon_back_raw);
		}
		
		if(showfwd)
		{
			drawGradient(theme->col_dark_ctrl, theme->col_light_ctrl, 189, 138, 22, 19);
			drawBox(189, 138, 22, 19, theme->col_outline);
			drawMonochromeIcon(189+5, 138+1, 12, 12, icon_play_raw);
		}
	}
}

void PPLoadDialog::drawPolaroid(u8 px, u8 py, u8 thumb, bool del_icon)
{
	drawFullBox(px, py, POLAROID_WIDTH, POLAROID_HEIGHT, RGB15(31,31,31)|BIT(15));
	drawBox(px, py, POLAROID_WIDTH, POLAROID_HEIGHT, RGB15(0,0,0)|BIT(15));
	if(names[thumb])
	{
		if(thumbnails[thumb] != 0)
			drawImage(px+2, py+2, 64, 48, thumbnails[thumb]);
		drawString(names[thumb], px+3, py+51, 64, theme->col_text);
	}
	
	if(del_icon)
		drawMonochromeIcon(px+POLAROID_WIDTH-9, py+2, 7, 7, icon_delete_file_raw, RGB15(31,0,0)|BIT(15));
}

void PPLoadDialog::loadThumbnail(char *filename, int idx)
{
	// Load
	char *f = (char*)calloc(1, 255);
	sprintf(f, "%s/%s", datadir, filename);
	TiXmlDocument doc(f);
	free(f);
	
	if( !doc.LoadFile() )
		return;
	
	TiXmlNode *root;
	if( doc.FirstChildElement("ppsketch") != 0 )
		root = doc.FirstChildElement("ppsketch");
	else // No root element => old version
		root = &doc;
	
	TiXmlElement *imageelement = root->FirstChildElement("image");
	if(imageelement == 0)
	{
		thumbnails[idx] = 0;
		return;
	}
		
	const char *b64screenshot = imageelement->GetText();
	
	char *cscreenshot;
	b64decode(b64screenshot, &cscreenshot);
	u16 *screenshot = (u16*)cscreenshot;
	
	thumbnails[idx] = screenshot;
}

void PPLoadDialog::loadThumbnails(void)
{
	freeThumbnails();
	
	char *filename = (char*)calloc(1, 256);
	
	DIR_ITER *dir;
	struct stat filestats;
	
	if(((dir = diropen(datadir)) == NULL))
	{
		iprintf("Dir read error!\n");
		return;
	}
	
	int find_result = dirnext(dir, filename, &filestats);
	
	if(find_result == -1)
	{
		iprintf("No files found!\n");
		return;
	}
	
	lowercase(filename);
	
	showfwd = false;
	int filenr = 0;
	while(find_result != -1)
	{
		if( (filename[0] != '.') && (!(filestats.st_mode & S_IFDIR)) 
				&& (strcmp(filename+strlen(filename)-3, ".pp") == 0) ) // Hidden files and directories
		{
			if( filenr / 6 == page)
			{
				int index = filenr % 6;
				char *name = (char*)calloc(1, 256);
				strcpy(name, filename);
				names[index] = name;
				printf("%d %s\n", index, filename);
				loadThumbnail(filename, index);
			}
			if(filenr / 6 > page)
				showfwd = true;
			
			filenr++;
		}
		
		find_result = dirnext(dir, filename, &filestats);
		lowercase(filename);
	}
	
	free(filename);
	dirclose(dir);
}

void PPLoadDialog::freeThumbnails(void)
{
	for(int i=0; i<6; ++i)
	{
		if(names[i] != 0)
		{
			free(names[i]);
			names[i] = 0;
		}
		if(thumbnails[i] != 0)
		{
			free(thumbnails[i]);
			thumbnails[i] = 0;
		}
	}
}
