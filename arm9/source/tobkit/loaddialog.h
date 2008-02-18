#ifndef LOADDIALOG_H_
#define LOADDIALOG_H_

#include "widget.h"
#include "gui.h"
#include "button.h"

class PPLoadDialog : public Widget
{
	public:
		PPLoadDialog(uint16 **_vram);
		~PPLoadDialog();
	
		// Drawing request
		void pleaseDraw(void);
		
		// Event calls
		void penDown(u8 px, u8 py);
		void penUp(u8 px, u8 py);
		
		// Callback registration
		void registerOkCallback(void (*onOk_)(void));
		void registerCancelCallback(void (*onCancel_)(void));
				
		void show(void);
		void setTheme(Theme *theme_);
		
		char *getFilename(void);
		
	private:
		void draw(void);
		
		void drawPolaroid(u8 px, u8 py, u8 thumb);
		
		void loadThumbnail(char *filename, int idx);
		void loadThumbnails(void);
		void freeThumbnails(void);
		
		u8 page;
		bool showfwd, showback;
		void (*onOk)(void);
		Button *buttoncancel;
		u16 **thumbnails;
		char **names;
		char *resultname;
		char *datadir;
};

#endif /*LOADDIALOG_H_*/
