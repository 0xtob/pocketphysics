#include "Theme.h"

Theme::Theme(void)
{
	// Set default colors
	col_dark_bg             = RGB15(4,6,15)|BIT(15);
	col_medium_bg           = RGB15(9,11,17)|BIT(15);
	col_light_bg            = RGB15(16,18,24)|BIT(15);
	col_lighter_bg          = RGB15(23,25,31) | BIT(15);
	col_light_ctrl          = RGB15(31,31,0)|BIT(15); // RGB15(26,26,26)|BIT(15)
	col_dark_ctrl           = RGB15(31,18,0)|BIT(15); // RGB15(31,31,31)|BIT(15)
	col_darker_ctrl         = RGB15(22,12,0)|BIT(15);
	col_darkest_ctrl        = RGB15(17,8,0)|BIT(15);
	col_light_ctrl_disabled = col_light_bg;
	col_dark_ctrl_disabled  = col_medium_bg;
	col_list_highlight1     = RGB15(28,15,0)|BIT(15);
	col_list_highlight2     = RGB15(28,28,0)|BIT(15);
	col_outline             = RGB15(0,0,0)|BIT(15);
	col_sepline             = RGB15(31,31,0)|BIT(15);
	col_icon                = RGB15(0,0,0)|BIT(15);
	col_text                = RGB15(0,0,0)|BIT(15);
	col_signal              = RGB15(31,0,0)|BIT(15);
}
/*
theme->col_dark_bg
theme->col_medium_bg
theme->col_light_bg
theme->col_lighter_bg
theme->col_light_ctrl
theme->col_dark_ctrl
theme->col_light_ctrl_disabled
theme->col_dark_ctrl_disabled
theme->col_list_highlight1
theme->col_list_highlight2
theme->col_outline
theme->col_sepline
theme->col_icon
theme->col_text
theme->col_signal
*/
