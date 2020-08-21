/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "msgbox.h"
#include "textdraws.h"
#include "ui.h"
#include "player.h"
#include "samp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum alignment {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT,
};

struct TEXTDRAW textdraws[MAX_TEXTDRAWS];
int *textdraw_enabled;
int numtextdraws;

#define IS_VALID_INDEX_SELECTED (0 <= lst->selectedindex && lst->selectedindex < numtextdraws)

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst;
static struct UI_CHECKBOX *chk_show;
static struct UI_BUTTON *btn_edit, *btn_delete;
static struct UI_LABEL *lbl_num;
static struct RADIOBUTTONGROUP *rdbgroup_align;
static struct UI_INPUT *in_txt;
static struct UI_INPUT *in_txtcol, *in_boxcol, *in_shdcol;
static struct UI_CHECKBOX *chk_prop, *chk_outline, *chk_box;
static struct UI_INPUT *in_font;
static struct UI_INPUT *in_shadow;
static struct UI_INPUT *in_x, *in_y;
static struct UI_INPUT *in_boxx, *in_boxy;
static struct UI_INPUT *in_letterx, *in_lettery;
static int textdrawsactive;
static char lbl_num_text[35];
#define MAX_LIST_ENTRY_LEN 20
static char listnames[MAX_TEXTDRAWS * MAX_LIST_ENTRY_LEN];

static
void cb_chk_click(struct UI_CHECKBOX *chk)
{
	((struct UI_CHECKBOX*) chk->_parent._parent.userdata)->checked = 0;
	ui_chk_updatetext(chk->_parent._parent.userdata);
}

static
void cb_btn_mainmenu_textdraws(struct UI_BUTTON *btn)
{
	ui_show_window(wnd);
}

static
void cb_in_txt(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		strcpy(textdraws[lst->selectedindex].szString, in->value);
		strcpy(textdraws[lst->selectedindex].szText, in->value);
	}
}

static
void cb_lst_hover(struct UI_LIST *lst)
{
	int i;

	for (i = 0; i < numtextdraws; i++) {
		textdraw_enabled[i] = (i == lst->hoveredindex) || (lst->hoveredindex == -1);
	}
}

static
void cb_lst_item_selected(struct UI_LIST *lst)
{
	struct TEXTDRAW *td;
	char tmptxt[50];

	if (lst->selectedindex == -1) {
		btn_delete->enabled = 0;
		return;
	}
	btn_delete->enabled = 1;

	td = textdraws + lst->selectedindex;

	if (td->byteLeft) {
		ui_rdb_click_match_userdata(rdbgroup_align, (void*) ALIGN_LEFT);
	} else if (td->byteCenter) {
		ui_rdb_click_match_userdata(rdbgroup_align, (void*) ALIGN_CENTER);
	} else if (td->byteRight) {
		ui_rdb_click_match_userdata(rdbgroup_align, (void*) ALIGN_RIGHT);
	}

	ui_in_set_text(in_txt, td->szString);

	sprintf(tmptxt, "%d", td->iStyle);
	ui_in_set_text(in_font, tmptxt);

	sprintf(tmptxt, "%d", td->byteShadowSize);
	ui_in_set_text(in_shadow, tmptxt);

	sprintf(tmptxt, "%02X", td->dwLetterColor);
	ui_in_set_text(in_txtcol, tmptxt);
	sprintf(tmptxt, "%02X", td->dwShadowColor);
	ui_in_set_text(in_shdcol, tmptxt);
	sprintf(tmptxt, "%02X", td->dwBoxColor);
	ui_in_set_text(in_boxcol, tmptxt);

	sprintf(tmptxt, "%.3f", td->fX);
	ui_in_set_text(in_x, tmptxt);
	sprintf(tmptxt, "%.3f", td->fY);
	ui_in_set_text(in_y, tmptxt);
	sprintf(tmptxt, "%.3f", td->fLetterWidth);
	ui_in_set_text(in_letterx, tmptxt);
	sprintf(tmptxt, "%.3f", td->fLetterHeight);
	ui_in_set_text(in_lettery, tmptxt);
	sprintf(tmptxt, "%.3f", td->fBoxSizeX);
	ui_in_set_text(in_boxx, tmptxt);
	sprintf(tmptxt, "%.3f", td->fBoxSizeY);
	ui_in_set_text(in_boxy, tmptxt);

	chk_prop->checked = td->byteProportional;
	chk_outline->checked = td->byteOutline;
	chk_box->checked = td->byteBox;
	ui_chk_updatetext(chk_prop);
	ui_chk_updatetext(chk_outline);
	ui_chk_updatetext(chk_box);
}

static
void cb_rdb_alignment(struct UI_RADIOBUTTON *rdb)
{
	if (IS_VALID_INDEX_SELECTED) {
		switch ((int) rdb->_parent._parent.userdata) {
		case ALIGN_LEFT:
			textdraws[lst->selectedindex].byteLeft = 1;
			textdraws[lst->selectedindex].byteCenter = 0;
			textdraws[lst->selectedindex].byteRight = 0;
			break;
		case ALIGN_CENTER:
			textdraws[lst->selectedindex].byteLeft = 0;
			textdraws[lst->selectedindex].byteCenter = 1;
			textdraws[lst->selectedindex].byteRight = 0;
			break;
		case ALIGN_RIGHT:
			textdraws[lst->selectedindex].byteLeft = 0;
			textdraws[lst->selectedindex].byteCenter = 0;
			textdraws[lst->selectedindex].byteRight = 1;
			break;
		}
	}
}

static
void cb_in_font(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].iStyle = atoi(in->value);
		if (textdraws[lst->selectedindex].iStyle != 5) {
			textdraws[lst->selectedindex].__unk4 = -1;
		}
	}
}

static
void cb_in_shadow(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].byteShadowSize = atoi(in->value);
	}
}

static
void cb_in_x(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].fX = (float) atof(in->value);
	}
}

static
void cb_in_y(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].fY = (float) atof(in->value);
	}
}

static
void cb_in_letterx(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].fLetterWidth = (float) atof(in->value);
	}
}

static
void cb_in_lettery(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].fLetterHeight = (float) atof(in->value);
	}
}

static
void cb_in_boxx(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].fBoxSizeX = (float) atof(in->value);
	}
}

static
void cb_in_boxy(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].fBoxSizeY = (float) atof(in->value);
	}
}

static
void cb_in_col(struct UI_INPUT *in)
{
	int col;
	int c;
	int j;

	if (IS_VALID_INDEX_SELECTED && in->valuelen == 8) {
		for (j = 0; j < 8; j++) {
			c = in->value[j] - '0';
			if (c < 0 || 9 < c) {
				c = in->value[j] - 'A' + 10;
				if (c < 10 || 15 < c) {
					c = in->value[j] - 'a' + 10;
					if (c < 10 || 15 < c) {
						return;
					}
				}
			}
			col |= c << ((8 - j) * 4);
		}
		*(int*) ((int) &textdraws[lst->selectedindex] + (int) in->_parent.userdata) = col;
	}
}

static
void cb_chk_prop(struct UI_CHECKBOX *chk)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].byteProportional = chk->checked;
	}
}

static
void cb_chk_outline(struct UI_CHECKBOX *chk)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].byteOutline = chk->checked;
	}
}

static
void cb_chk_box(struct UI_CHECKBOX *chk)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].byteBox = chk->checked;
	}
}

static
void update_list()
{
	int i, selected_index, j;
	char *names[MAX_TEXTDRAWS];
	char tmpchar;

	selected_index = lst->selectedindex;
	for (i = 0; i < numtextdraws; i++) {
		names[i] = listnames + i * MAX_LIST_ENTRY_LEN;
		for (j = 0; j < MAX_LIST_ENTRY_LEN; j++) {
			tmpchar = textdraws[i].szString[j];
			if (tmpchar == ' ') {
				names[i][j] = '_';
			} else {
				names[i][j] = tmpchar;
			}
		}
		names[i][MAX_LIST_ENTRY_LEN] = 0;
	}
	ui_lst_set_data(lst, names, numtextdraws);
	ui_lst_set_selected_index(lst, selected_index);
	cb_lst_item_selected(lst);
}

static
void cb_btn_add(struct UI_BUTTON *btn)
{
	int i, index_to_select;

	if (IS_VALID_INDEX_SELECTED) {
		for (i = numtextdraws; i > lst->selectedindex; i--) {
			textdraws[i] = textdraws[i - 1];
		}
		index_to_select = lst->selectedindex + 1;
	} else {
		if (numtextdraws > 0) {
			textdraws[numtextdraws] = textdraws[numtextdraws - 1];
		} else {
			sprintf(textdraws[0].szString, "hi hi");
			sprintf(textdraws[0].szText, "hi hi");
			textdraws[0].fLetterHeight = 1.0f;
			textdraws[0].fLetterWidth = 1.0f;
			textdraws[0].dwLetterColor = -1;
			textdraws[0].fX = 200.0f;
			textdraws[0].fY = 200.0f;
			textdraws[0].iStyle = 1;
			textdraws[0].fBoxSizeX = 600.0f;
			textdraws[0].fBoxSizeY = 900.0f;
			textdraws[0].dwBoxColor = 0x66FF0000;
			textdraws[0].byteLeft = 1;
			textdraws[0].byteOutline = 1;
			textdraws[0].byteShadowSize = 1;
			textdraws[0].dwShadowColor = 0xFF00FFFF;
			textdraws[0].byteProportional = 1;
			textdraws[0].byteBox = 1;
			textdraws[0].__unk2 = -1;
			textdraws[0].__unk3 = -1;
			textdraws[0].__unk4 = -1;
		}
		index_to_select = numtextdraws;
	}
	textdraw_enabled[numtextdraws] = 1;
	numtextdraws++;
	update_list();
	ui_lst_set_selected_index(lst, index_to_select);
	cb_lst_item_selected(lst);
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	int i;

	numtextdraws--;
	textdraw_enabled[numtextdraws] = 0;
	for (i = lst->selectedindex; i < numtextdraws; i++) {
		textdraws[i] = textdraws[i + 1];
	}
	i = lst->selectedindex;
	update_list();
	if (i < numtextdraws) {
		ui_lst_set_selected_index(lst, i);
	}
	cb_lst_item_selected(lst);
}

int textdraws_on_background_element_just_clicked()
{
	if (textdrawsactive) {
		return 0;
	}
	return 1;
}

void textdraws_frame_update()
{
	if (!textdrawsactive) {
		return;
	}

	sprintf(lbl_num_text, "num_textdraws:_%d", numtextdraws);
}

void textdraws_on_active_window_changed(struct UI_WINDOW *_wnd)
{
	int i;

	if (_wnd == wnd) {
		textdrawsactive = 1;
		// update list
		update_list();
		*hudScaleX = originalHudScaleX;
		*hudScaleY = originalHudScaleY;
		for (i = 0; i < numtextdraws; i++) {
			textdraw_enabled[i] = 1;
		}
	} else if (textdrawsactive) {
		textdrawsactive = 0;
		*hudScaleX = NEW_HUD_SCALE_X;
		*hudScaleY = NEW_HUD_SCALE_Y;
		for (i = 0; i < numtextdraws; i++) {
			textdraw_enabled[i] = 0;
		}
	}
}

void textdraws_ui_activated()
{
	int i;

	if (textdrawsactive) {
		*hudScaleX = originalHudScaleX;
		*hudScaleY = originalHudScaleY;
	}
	for (i = 0; i < numtextdraws; i++) {
		textdraw_enabled[i] = textdrawsactive;
	}
}

void textdraws_ui_deactivated()
{
	int i;

	for (i = 0; i < numtextdraws; i++) {
		textdraw_enabled[i] = chk_show->checked;
	}
}

void textdraws_init()
{
	struct UI_RADIOBUTTON *rdb;
	struct UI_BUTTON *btn;
	int poolsaddr;
	int textdrawpool;
	int textdrawsaddr;
	int **ptr;
	int i;

	sprintf(debugstring, "%p", textdraws);
	ui_push_debug_string();
	ui_push_debug_string();
	ui_push_debug_string();
	ui_push_debug_string();
	poolsaddr = *(int*) (*(int*)(samp_handle + /*samp_info_offset*/0x26EA0C) + /*pools*/0x3DE);
	textdrawpool = *(int*) (poolsaddr + 0x20);
	textdraw_enabled = (void*) textdrawpool;
	textdrawsaddr = textdrawpool + MAX_TEXTDRAWS * sizeof(int) + MAX_PLAYER_TEXTDRAWS * sizeof(int);
	ptr = (int**) textdrawsaddr;
	for (i = 0; i < MAX_TEXTDRAWS; i++) {
		*ptr = (int*) (textdraws + i);
		ptr++;
	}

	btn = ui_btn_make("Textdraws", cb_btn_mainmenu_textdraws);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	wnd = ui_wnd_make(9000.0f, 300.0f, "Textdraws");
	wnd->columns = 3;

	lbl_num = ui_lbl_make(lbl_num_text);
	lbl_num->_parent.span = 2;
	ui_wnd_add_child(wnd, chk_show = ui_chk_make("show", 0, NULL));
	ui_wnd_add_child(wnd, lbl_num);
	lst = ui_lst_make(8, cb_lst_item_selected);
	lst->hovercb = cb_lst_hover;
	lst->_parent.span = 3;
	lst->_parent.pref_width = 100.0f;
	ui_wnd_add_child(wnd, lst);

	btn = ui_btn_make("Copy_selected/Add", cb_btn_add);
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn_delete = ui_btn_make("Delete", cb_btn_delete);
	btn_delete->_parent.span = 1;
	btn_delete->enabled = 0;
	ui_wnd_add_child(wnd, btn_delete);

	in_txt = ui_in_make(cb_in_txt);
	in_txt->_parent.span = 3;
	ui_wnd_add_child(wnd, in_txt);

	ui_wnd_add_child(wnd, ui_lbl_make("font"));
	in_font = ui_in_make(cb_in_font);
	in_font->_parent.span = 2;
	ui_wnd_add_child(wnd, in_font);

	ui_wnd_add_child(wnd, ui_lbl_make("shadow"));
	in_shadow = ui_in_make(cb_in_shadow);
	in_shadow->_parent.span = 2;
	ui_wnd_add_child(wnd, in_shadow);

	ui_wnd_add_child(wnd, ui_lbl_make("x"));
	in_x = ui_in_make(cb_in_x);
	in_x->_parent.span = 2;
	ui_wnd_add_child(wnd, in_x);

	ui_wnd_add_child(wnd, ui_lbl_make("y"));
	in_y = ui_in_make(cb_in_y);
	in_y->_parent.span = 2;
	ui_wnd_add_child(wnd, in_y);

	ui_wnd_add_child(wnd, ui_lbl_make("lsx"));
	in_letterx = ui_in_make(cb_in_letterx);
	in_letterx->_parent.span = 2;
	ui_wnd_add_child(wnd, in_letterx);

	ui_wnd_add_child(wnd, ui_lbl_make("lsy"));
	in_lettery = ui_in_make(cb_in_lettery);
	in_lettery->_parent.span = 2;
	ui_wnd_add_child(wnd, in_lettery);

	ui_wnd_add_child(wnd, ui_lbl_make("boxsx"));
	in_boxx = ui_in_make(cb_in_boxx);
	in_boxx->_parent.span = 2;
	ui_wnd_add_child(wnd, in_boxx);

	ui_wnd_add_child(wnd, ui_lbl_make("boxsy"));
	in_boxy = ui_in_make(cb_in_boxy);
	in_boxy->_parent.span = 2;
	ui_wnd_add_child(wnd, in_boxy);

	ui_wnd_add_child(wnd, ui_lbl_make("txtcol"));
	in_txtcol = ui_in_make(cb_in_col);
	in_txtcol->_parent.span = 2;
	in_txtcol->_parent.userdata = (void*) ((int) &textdraws[0].dwLetterColor - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_txtcol);

	ui_wnd_add_child(wnd, ui_lbl_make("shdcol"));
	in_shdcol = ui_in_make(cb_in_col);
	in_shdcol->_parent.span = 2;
	in_shdcol->_parent.userdata = (void*) ((int) &textdraws[0].dwShadowColor - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_shdcol);

	ui_wnd_add_child(wnd, ui_lbl_make("boxcol"));
	in_boxcol = ui_in_make(cb_in_col);
	in_boxcol->_parent.span = 2;
	in_boxcol->_parent.userdata = (void*) ((int) &textdraws[0].dwBoxColor - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_boxcol);

	rdbgroup_align = ui_rdbgroup_make(cb_rdb_alignment);
	rdb = ui_rdb_make("left", rdbgroup_align, 1);
	rdb->_parent._parent.userdata = (void*) ALIGN_LEFT;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("center", rdbgroup_align, 0);
	rdb->_parent._parent.userdata = (void*) ALIGN_CENTER;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("right", rdbgroup_align, 0);
	rdb->_parent._parent.userdata = (void*) ALIGN_RIGHT;
	ui_wnd_add_child(wnd, rdb);

	ui_wnd_add_child(wnd, chk_prop = ui_chk_make("prop", 1, cb_chk_prop));
	ui_wnd_add_child(wnd, chk_outline = ui_chk_make("outline", 1, cb_chk_outline));
	ui_wnd_add_child(wnd, chk_box = ui_chk_make("box", 1, cb_chk_box));
}

void textdraws_dispose()
{
	ui_wnd_dispose(wnd);
	// this crashes when exiting the game.. hmm..
	//memset(textdraw_enabled, 0, sizeof(int) * MAX_TEXTDRAWS);
}
