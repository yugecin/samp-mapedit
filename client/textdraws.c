/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "msgbox.h"
#include "textdraws.h"
#include "ui.h"
#include "player.h"
#include "samp.h"
#include "sockets.h"
#include "../shared/serverlink.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum alignment {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT,
};

struct TEXTDRAW textdraws[MAX_TEXTDRAWS];
char textdraw_text[MAX_TEXTDRAWS][800];
char textdraw_name[MAX_TEXTDRAWS][100];
int *textdraw_enabled;
int numtextdraws;

#define IS_VALID_INDEX_SELECTED (0 <= lst->selectedindex && lst->selectedindex < numtextdraws)

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst;
static struct UI_CHECKBOX *chk_show;
static struct UI_BUTTON *btn_edit, *btn_delete;
static struct UI_LABEL *lbl_num;
static struct RADIOBUTTONGROUP *rdbgroup_align;
static struct UI_INPUT *in_name, *in_txt;
static struct UI_INPUT *in_txtcol, *in_boxcol, *in_shdcol;
static struct UI_CHECKBOX *chk_prop, *chk_sel, *chk_box;
static struct UI_INPUT *in_font;
static struct UI_INPUT *in_shadow, *in_outline;
static struct UI_INPUT *in_x, *in_y;
static struct UI_INPUT *in_boxx, *in_boxy;
static struct UI_INPUT *in_letterx, *in_lettery;
static struct UI_INPUT *in_model, *in_col1, *in_col2, *in_rx, *in_ry, *in_rz, *in_zoom;
static struct UI_INPUT *in_massmove_x, *in_massmove_y, *in_widen;
static int textdrawsactive;
static char txtlen[10];
static char lbl_num_text[35];
#define MAX_LIST_ENTRY_LEN 20
static char listnames[MAX_TEXTDRAWS * MAX_LIST_ENTRY_LEN];

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

	ui_in_set_text(in_name, textdraw_name[td - textdraws]);
	ui_in_set_text(in_txt, textdraw_text[td - textdraws]);
	sprintf(txtlen, "%d", strlen(textdraw_text[td - textdraws]));

	sprintf(tmptxt, "%d", td->iStyle);
	ui_in_set_text(in_font, tmptxt);

	sprintf(tmptxt, "%d", td->byteShadowSize);
	ui_in_set_text(in_shadow, tmptxt);

	sprintf(tmptxt, "%d", td->byteOutline);
	ui_in_set_text(in_outline, tmptxt);

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
	chk_sel->checked = td->selectable;
	chk_box->checked = td->byteBox;
	ui_chk_updatetext(chk_prop);
	ui_chk_updatetext(chk_sel);
	ui_chk_updatetext(chk_box);

	sprintf(tmptxt, "%d", td->sModel);
	ui_in_set_text(in_model, tmptxt);
	sprintf(tmptxt, "%d", td->sColor[0]);
	ui_in_set_text(in_col1, tmptxt);
	sprintf(tmptxt, "%d", td->sColor[1]);
	ui_in_set_text(in_col2, tmptxt);
	sprintf(tmptxt, "%.3f", td->fRot[0]);
	ui_in_set_text(in_rx, tmptxt);
	sprintf(tmptxt, "%.3f", td->fRot[1]);
	ui_in_set_text(in_ry, tmptxt);
	sprintf(tmptxt, "%.3f", td->fRot[2]);
	ui_in_set_text(in_rz, tmptxt);
	sprintf(tmptxt, "%.3f", td->fZoom);
	ui_in_set_text(in_zoom, tmptxt);
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
			tmpchar = textdraw_name[i][j];
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
}

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
void cb_in_name(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		strcpy(textdraw_name[lst->selectedindex], in->value);
		update_list();
	}
}

static
void cb_in_txt(struct UI_INPUT *in)
{
	int i;
	int tildes;
	char v;

	if (IS_VALID_INDEX_SELECTED) {
		sprintf(txtlen, "%d", strlen(in->value));
		i = 0;
		tildes = 0;
		for (;;) {
			v = in->value[i];
			if (v == '~') {
				tildes++;
			}
			textdraw_text[lst->selectedindex][i] = v;
			if (v == 0) {
				if (tildes % 2) {
					strcpy(textdraw_text[lst->selectedindex], "unmatched tilde");
				}
				break;
			}
			i++;
		}
		/*Normally I shouldn't have to do this and SA-MP does it, but somehow it doesn't work?*/
		strcpy(textdraws[lst->selectedindex].szText, textdraw_text[lst->selectedindex]);
		game_TextConvertKeybindings(textdraws[lst->selectedindex].szText);
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
void cb_in_float(struct UI_INPUT *in)
{
	if (IS_VALID_INDEX_SELECTED) {
		*(float*)((int) &textdraws[lst->selectedindex] + (int) in->_parent.userdata) = (float) atof(in->value);
		/*would this cause memleaks? if it works by creating textures, maybe doing this doesn't
		free earlier made textures.*/
		if (textdraws[lst->selectedindex].iStyle == 5) {
			textdraws[lst->selectedindex].probablyTextureIdForPreview = -1;
		}
	}
}

static
void cb_in_int(struct UI_INPUT *in)
{
	int offset;

	if (IS_VALID_INDEX_SELECTED) {
		offset = (int) in->_parent.userdata;
		*(int*)((int) &textdraws[lst->selectedindex] + offset) = (int) atoi(in->value);
	}
}

static
void cb_in_byte(struct UI_INPUT *in)
{
	int offset;

	if (IS_VALID_INDEX_SELECTED) {
		offset = (int) in->_parent.userdata;
		*(char*)((int) &textdraws[lst->selectedindex] + offset) = (char) atoi(in->value);
		if (offset == (int) &textdraws[0].byteShadowSize - (int) &textdraws[0]) {
			textdraws[lst->selectedindex].byteOutline = 0;
			ui_in_set_text(in_outline, "0");
		}
		if (offset == (int) &textdraws[0].byteOutline - (int) &textdraws[0]) {
			textdraws[lst->selectedindex].byteShadowSize = 0;
			ui_in_set_text(in_shadow, "0");
		}
	}
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
			textdraws[lst->selectedindex].probablyTextureIdForPreview = -1;
		}
	}
}

static
void cb_in_col(struct UI_INPUT *in)
{
	char unused[20];
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
			/*this sprintf is unused, but removing it results in wrong values*/
			/*99000000 would deviate into 99000007 or something*/
			/*no idea why, optimizations are even disabled*/
			sprintf(unused, "%p", c << ((7 - j) * 4));
			col |= c << ((7 - j) * 4);
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
void cb_chk_sel(struct UI_CHECKBOX *chk)
{
	if (IS_VALID_INDEX_SELECTED) {
		textdraws[lst->selectedindex].selectable = chk->checked;
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
void cb_btn_testselection(struct UI_BUTTON *btn)
{
	struct MSG_NC nc;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SelectTextDraw;
	nc.params.asint[1] = 0;
	nc.params.asint[2] = 0xFFFF0000;
	sockets_send(&nc, sizeof(nc));
}

static
void cb_btn_add(struct UI_BUTTON *btn)
{
	int i, index_to_select;

	if (IS_VALID_INDEX_SELECTED) {
		for (i = numtextdraws; i > lst->selectedindex; i--) {
			textdraws[i] = textdraws[i - 1];
			strcpy(textdraw_name[i], textdraw_name[i - 1]);
			strcpy(textdraw_text[i], textdraw_text[i - 1]);
			strcpy(textdraws[i].szText, textdraw_text[i]);
			game_TextConvertKeybindings(textdraws[i].szText);
		}
		index_to_select = lst->selectedindex + 1;
	} else {
		if (numtextdraws > 0) {
			textdraws[numtextdraws] = textdraws[numtextdraws - 1];
		} else {
			sprintf(textdraw_name[0], "textdraw");
			sprintf(textdraw_text[0], "hi hi");
			strcpy(textdraws[0].szText, textdraw_text[0]);
			textdraws[0].fLetterHeight = 1.0f;
			textdraws[0].fLetterWidth = .25f;
			textdraws[0].dwLetterColor = -1;
			textdraws[0].fX = 200.0f;
			textdraws[0].fY = 200.0f;
			textdraws[0].iStyle = 1;
			textdraws[0].fBoxSizeX = 500.0f;
			textdraws[0].fBoxSizeY = 200.0f;
			textdraws[0].dwBoxColor = 0x66000000;
			textdraws[0].byteLeft = 1;
			textdraws[0].byteOutline = 1;
			textdraws[0].byteShadowSize = 0;
			textdraws[0].dwShadowColor = 0xFF000000;
			textdraws[0].byteProportional = 1;
			textdraws[0].byteBox = 1;
			textdraws[0].__unk2 = -1;
			textdraws[0].__unk3 = -1;
			textdraws[0].probablyTextureIdForPreview = -1;
			textdraws[0].fZoom = 2.0f;
			textdraws[0].sModel = 411;
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
void cb_btn_up(struct UI_BUTTON *btn)
{
	struct TEXTDRAW tmp;
	char tmptext[TEXTDRAW_TEXT_LEN];
	char tmpname[TEXTDRAW_TEXT_LEN];

	if (IS_VALID_INDEX_SELECTED && lst->selectedindex > 0) {
		tmp = textdraws[lst->selectedindex - 1];
		textdraws[lst->selectedindex - 1] = textdraws[lst->selectedindex];
		textdraws[lst->selectedindex] = tmp;
		strcpy(tmptext, textdraw_text[lst->selectedindex - 1]);
		strcpy(textdraw_text[lst->selectedindex - 1], textdraw_text[lst->selectedindex]);
		strcpy(textdraw_text[lst->selectedindex], tmptext);
		strcpy(tmpname, textdraw_name[lst->selectedindex - 1]);
		strcpy(textdraw_name[lst->selectedindex - 1], textdraw_name[lst->selectedindex]);
		strcpy(textdraw_name[lst->selectedindex], tmpname);
		lst->selectedindex--;
		update_list();
	}
}

static
void cb_btn_down(struct UI_BUTTON *btn)
{
	struct TEXTDRAW tmp;
	char tmptext[TEXTDRAW_TEXT_LEN];
	char tmpname[TEXTDRAW_TEXT_LEN];

	if (IS_VALID_INDEX_SELECTED && lst->selectedindex < numtextdraws - 1) {
		tmp = textdraws[lst->selectedindex + 1];
		textdraws[lst->selectedindex + 1] = textdraws[lst->selectedindex];
		textdraws[lst->selectedindex] = tmp;
		strcpy(tmptext, textdraw_text[lst->selectedindex + 1]);
		strcpy(textdraw_text[lst->selectedindex + 1], textdraw_text[lst->selectedindex]);
		strcpy(textdraw_text[lst->selectedindex], tmptext);
		strcpy(tmpname, textdraw_name[lst->selectedindex + 1]);
		strcpy(textdraw_name[lst->selectedindex + 1], textdraw_name[lst->selectedindex]);
		strcpy(textdraw_name[lst->selectedindex], tmpname);
		lst->selectedindex++;
		update_list();
	}
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	int i;

	numtextdraws--;
	textdraw_enabled[numtextdraws] = 0;
	for (i = lst->selectedindex; i < numtextdraws; i++) {
		textdraws[i] = textdraws[i + 1];
		strcpy(textdraw_text[i], textdraw_text[i + 1]);
		strcpy(textdraw_name[i], textdraw_name[i + 1]);
	}
	i = lst->selectedindex;
	update_list();
	if (i < numtextdraws) {
		ui_lst_set_selected_index(lst, i);
	}
	cb_lst_item_selected(lst);
}

static
void cb_btn_massmove(struct UI_BUTTON *btn)
{
	float x, y;
	int i;

	x = (float) atof(in_massmove_x->value);
	y = (float) atof(in_massmove_y->value);

	for (i = 0; i < numtextdraws; i++) {
		textdraws[i].fX += x;
		textdraws[i].fY += y;
		textdraws[i].fBoxSizeX += x;
	}
	cb_lst_item_selected(lst);
}

static
void cb_btn_widen(struct UI_BUTTON *btn)
{
	float x;
	int i;

	x = (float) atof(in_widen->value);

	for (i = 0; i < numtextdraws; i++) {
		textdraws[i].fBoxSizeX += x;
		if (textdraws[i].byteCenter) {
			textdraws[i].fBoxSizeY += x;
		}
	}
	cb_lst_item_selected(lst);
}

static int dragging;
static float origCursorX, origCursorY;
static float *dragging_prop_x;
static float *dragging_prop_y;

static
void dragging_lbl_mousedown(struct UI_ELEMENT *elem)
{
	int off_x, off_y;

	if (ui_element_is_hovered(elem) && IS_VALID_INDEX_SELECTED) {
		dragging = 1;
		origCursorX = cursorx;
		origCursorY = cursory;
		off_x = (int) elem->userdata & 0xFFFF;
		off_y = (unsigned int) elem->userdata >> 16;
		if (off_x == 0) {
			dragging_prop_x = 0;
		} else {
			dragging_prop_x = (float*) ((int) &textdraws[lst->selectedindex] + off_x);
		}
		if (off_y == 0) {
			dragging_prop_y = 0;
		} else {
			dragging_prop_y = (float*) ((int) &textdraws[lst->selectedindex] + off_y);
		}
	}
}

static
struct UI_LABEL* mk_dragging_lbl(char *text, void *ptr1, void *ptr2)
{
	struct UI_LABEL *lbl;

	lbl = ui_lbl_make(text);
	lbl->_parent.userdata =
		(void*) (((int) ptr1 - (int) &textdraws[0]) | ((((int) ptr2 - (int) &textdraws[0])) << 16));
	lbl->_parent.proc_mousedown = (ui_method*) dragging_lbl_mousedown;

	return lbl;
}

void textdraws_frame_update()
{
	float diff;

	if (!textdrawsactive) {
		return;
	}

	if (!activeMouseState->lmb) {
		dragging = 0;
	}
	if (dragging) {
		cursorx = origCursorX;
		cursory = origCursorY;
		if (dragging_prop_x) {
			diff = activeMouseState->x;
			if (activeKeyState->lshift) {
				diff /= 10.0f;
			}
			*dragging_prop_x += diff;
		}
		if (dragging_prop_y) {
			/*TODO is if off (copied from ui_do_cursor_movement)*/
			diff = -activeMouseState->y * fresx / fresy;
			if (activeKeyState->lshift) {
				diff /= 10.0f;
			}
			*dragging_prop_y += diff;
		}
		cb_lst_item_selected(lst);
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
		update_list();
		cb_lst_item_selected(lst);
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
	struct UI_ELEMENT *elem;
	struct UI_RADIOBUTTON *rdb;
	struct UI_BUTTON *btn;
	int poolsaddr;
	int textdrawpool;
	int textdrawsaddr;
	int **ptr;
	int i;

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
	btn->_parent.span = 3;
	ui_wnd_add_child(wnd, btn);

	ui_wnd_add_child(wnd, ui_btn_make("Up", cb_btn_up));
	ui_wnd_add_child(wnd, ui_btn_make("Down", cb_btn_down));
	btn_delete = ui_btn_make("Delete", cb_btn_delete);
	btn_delete->enabled = 0;
	ui_wnd_add_child(wnd, btn_delete);

	in_name = ui_in_make(cb_in_name);
	in_name->_parent.span = 3;
	ui_wnd_add_child(wnd, in_name);

	ui_wnd_add_child(wnd, ui_lbl_make(txtlen));
	in_txt = ui_in_make(cb_in_txt);
	in_txt->_parent.span = 2;
	ui_wnd_add_child(wnd, in_txt);

	ui_wnd_add_child(wnd, ui_lbl_make("font"));
	in_font = ui_in_make(cb_in_font);
	in_font->_parent.span = 2;
	ui_wnd_add_child(wnd, in_font);

	ui_wnd_add_child(wnd, ui_lbl_make("shadow"));
	in_shadow = ui_in_make(cb_in_byte);
	in_shadow->_parent.span = 2;
	in_shadow->_parent.userdata = (void*) ((int) &textdraws[0].byteShadowSize - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_shadow);

	ui_wnd_add_child(wnd, ui_lbl_make("outline"));
	in_outline = ui_in_make(cb_in_byte);
	in_outline->_parent.span = 2;
	in_outline->_parent.userdata = (void*) ((int) &textdraws[0].byteOutline - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_outline);

	ui_wnd_add_child(wnd, mk_dragging_lbl("x", &textdraws[0].fX, &textdraws[0].fY));
	in_x = ui_in_make(cb_in_float);
	in_x->_parent.span = 2;
	in_x->_parent.userdata = (void*) ((int) &textdraws[0].fX - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_x);

	ui_wnd_add_child(wnd, mk_dragging_lbl("y", &textdraws[0].fX, &textdraws[0].fY));
	in_y = ui_in_make(cb_in_float);
	in_y->_parent.span = 2;
	in_y->_parent.userdata = (void*) ((int) &textdraws[0].fY - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_y);

	ui_wnd_add_child(wnd, mk_dragging_lbl("lsx", &textdraws[0].fLetterWidth, &textdraws[0].fLetterHeight));
	in_letterx = ui_in_make(cb_in_float);
	in_letterx->_parent.span = 2;
	in_letterx->_parent.userdata = (void*) ((int) &textdraws[0].fLetterWidth - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_letterx);

	ui_wnd_add_child(wnd, mk_dragging_lbl("lsy", &textdraws[0].fLetterWidth, &textdraws[0].fLetterHeight));
	in_lettery = ui_in_make(cb_in_float);
	in_lettery->_parent.span = 2;
	in_lettery->_parent.userdata = (void*) ((int) &textdraws[0].fLetterHeight - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_lettery);

	ui_wnd_add_child(wnd, mk_dragging_lbl("boxsx", &textdraws[0].fBoxSizeX, &textdraws[0].fBoxSizeY));
	in_boxx = ui_in_make(cb_in_float);
	in_boxx->_parent.span = 2;
	in_boxx->_parent.userdata = (void*) ((int) &textdraws[0].fBoxSizeX - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_boxx);

	ui_wnd_add_child(wnd, mk_dragging_lbl("boxsy", &textdraws[0].fBoxSizeX, &textdraws[0].fBoxSizeY));
	in_boxy = ui_in_make(cb_in_float);
	in_boxy->_parent.span = 2;
	in_boxy->_parent.userdata = (void*) ((int) &textdraws[0].fBoxSizeY - (int) &textdraws[0]);
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
	ui_wnd_add_child(wnd, chk_sel = ui_chk_make("sel", 1, cb_chk_sel));
	ui_wnd_add_child(wnd, chk_box = ui_chk_make("box", 1, cb_chk_box));

	in_model = ui_in_make(cb_in_int);
	in_model->_parent.userdata = (void*) ((int) &textdraws[0].sModel - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_model);
	in_col1 = ui_in_make(cb_in_int);
	in_col1->_parent.userdata = (void*) ((int) &textdraws[0].sColor[0] - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_col1);
	in_col2 = ui_in_make(cb_in_int);
	in_col2->_parent.userdata = (void*) ((int) &textdraws[0].sColor[1] - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_col2);

	in_rx = ui_in_make(cb_in_float);
	in_rx->_parent.userdata = (void*) ((int) &textdraws[0].fRot[0] - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_rx);
	in_ry = ui_in_make(cb_in_float);
	in_ry->_parent.userdata = (void*) ((int) &textdraws[0].fRot[1] - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_ry);
	in_rz = ui_in_make(cb_in_float);
	in_rz->_parent.userdata = (void*) ((int) &textdraws[0].fRot[2] - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_rz);

	ui_wnd_add_child(wnd, ui_lbl_make("zoom"));
	in_zoom = ui_in_make(cb_in_float);
	in_zoom->_parent.span = 2;
	in_zoom->_parent.userdata = (void*) ((int) &textdraws[0].fZoom - (int) &textdraws[0]);
	ui_wnd_add_child(wnd, in_zoom);

	ui_wnd_add_child(wnd, ui_btn_make("massmove", cb_btn_massmove));
	ui_wnd_add_child(wnd, in_massmove_x = ui_in_make(NULL));
	ui_wnd_add_child(wnd, in_massmove_y = ui_in_make(NULL));

	ui_wnd_add_child(wnd, ui_btn_make("widen", cb_btn_widen));
	ui_wnd_add_child(wnd, in_widen = ui_in_make(NULL));
	ui_wnd_add_child(wnd, NULL);

	btn = ui_btn_make("test_selection", cb_btn_testselection);
	btn->_parent.span = 3;
	ui_wnd_add_child(wnd, btn);

	for (i = 0; i < wnd->_parent.childcount; i++) {
		elem = wnd->_parent.children[i];
		if (elem && elem->type == UIE_INPUT) {
			elem->pref_width = 20.0f;
		}
	}
}

void textdraws_dispose()
{
	ui_wnd_dispose(wnd);
	// this crashes when exiting the game.. hmm..
	//memset(textdraw_enabled, 0, sizeof(int) * MAX_TEXTDRAWS);
}
