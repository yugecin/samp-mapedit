/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "msgbox.h"
#include "gangzone.h"
#include "ui.h"
#include "player.h"
#include "samp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

struct GANG_ZONE gangzone_data[MAX_GANG_ZONES];
int *gangzone_enable;
int numgangzones;

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst;
static struct UI_BUTTON *btn_edit, *btn_delete;
static struct UI_LABEL *lbl_num;
static struct UI_INPUT *in_color;
static float zone_z;
static char lbl_num_text[35];
#define GANG_ZONE_TEXT_LEN 16
static char listnames[MAX_GANG_ZONES * GANG_ZONE_TEXT_LEN];
static int gangzonesactive;
static int was_lmb_down;
static int dont_update_color_textbox;

/*this is actually size/2*/
#define HANDLE_SIZE (7.5f)

struct RwV3D handles[4];
int dragging_handle;

static struct IM2DVERTEX verts[] = {
	{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
	{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
	{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
	{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
};

#define IS_VALID_INDEX_SELECTED (0 <= lst->selectedindex && lst->selectedindex < numgangzones)

static
void cb_chk_click(struct UI_CHECKBOX *chk)
{
	((struct UI_CHECKBOX*) chk->_parent._parent.userdata)->checked = 0;
	ui_chk_updatetext(chk->_parent._parent.userdata);
}

static
void cb_btn_mainmenu_gangzones(struct UI_BUTTON *btn)
{
	ui_show_window(wnd);
}

static
void cb_lst_item_selected(struct UI_LIST *lst)
{
	char buf[20];
	int col;
	int i;

	for (i = 0; i < numgangzones; i++) {
		gangzone_data[i].altcolor = gangzone_data[i].color;
	}

	if (IS_VALID_INDEX_SELECTED) {
		gangzone_data[lst->selectedindex].altcolor = gangzone_data[lst->selectedindex].color;
	}

	if (lst->selectedindex == -1) {
		btn_delete->enabled = 0;
		return;
	}
	btn_delete->enabled = 1;

	if (!dont_update_color_textbox) {
		sprintf(buf, "%08x", gangzone_data[lst->selectedindex].color);
		ui_in_set_text(in_color, buf);
	}
	col = gangzone_data[lst->selectedindex].color;
	col = (col & 0xFF00FF00) | (0xFF - (col & 0xFF)) |
		((0xFF - ((col & 0xFF) >> 8)) << 8) |
		((0xFF - ((col & 0xFF0000) >> 16)) << 16);
	gangzone_data[lst->selectedindex].altcolor = col;
}

static
void update_list()
{
	int i, selected_index;
	char *names[MAX_GANG_ZONES];

	if (IS_VALID_INDEX_SELECTED) {
		gangzone_data[lst->selectedindex].altcolor = gangzone_data[lst->selectedindex].color;
	}

	selected_index = lst->selectedindex;
	for (i = 0; i < numgangzones; i++) {
		names[i] = listnames + i * GANG_ZONE_TEXT_LEN;
		sprintf(names[i], "%04d:_%p", i, gangzone_data[i].color);
	}
	ui_lst_set_data(lst, names, numgangzones);
	ui_lst_set_selected_index(lst, selected_index);
	cb_lst_item_selected(lst);
}

static
void cb_msg_limitreached(int choice)
{
	ui_show_window(wnd);
}

static
void cb_btn_add(struct UI_BUTTON *btn)
{
	int i, index_to_select, relative_index_to_select_0_or_1;

	if (numgangzones >= MAX_GANG_ZONES) {
		msg_message = "Limit_reached!";
		msg_title = "Gangzones";
		msg_btn1text = "Ok";
		msg_show(cb_msg_limitreached);
		return;
	}

	relative_index_to_select_0_or_1 = ((int) btn->_parent.userdata) & 1;

	if (IS_VALID_INDEX_SELECTED) {
		for (i = numgangzones; i > lst->selectedindex; i--) {
			gangzone_data[i] = gangzone_data[i - 1];
		}
		index_to_select = lst->selectedindex + relative_index_to_select_0_or_1;
	} else {
		if (numgangzones > 0) {
			if (relative_index_to_select_0_or_1) {
				gangzone_data[numgangzones] = gangzone_data[numgangzones - 1];
			} else {
				for (i = numgangzones; i > 0; i--) {
					gangzone_data[i] = gangzone_data[i - 1];
				}
			}
		}
		index_to_select = numgangzones * relative_index_to_select_0_or_1;
	}
	if (!numgangzones) {
		gangzone_data[0].color = 0xFF000000;
		gangzone_data[0].minx = camera->position.x - 50.0f;
		gangzone_data[0].maxx = camera->position.x + 50.0f;
		gangzone_data[0].miny = camera->position.y - 50.0f;
		gangzone_data[0].maxy = camera->position.y + 50.0f;
	}
	gangzone_enable[numgangzones] = 1;
	numgangzones++;
	update_list();
	ui_lst_set_selected_index(lst, index_to_select);
	cb_lst_item_selected(lst);
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	int i;

	if (IS_VALID_INDEX_SELECTED) {
		gangzone_data[lst->selectedindex].altcolor = gangzone_data[lst->selectedindex].color;
	}

	numgangzones--;
	gangzone_enable[numgangzones] = 0;
	for (i = lst->selectedindex; i < numgangzones; i++) {
		gangzone_data[i] = gangzone_data[i + 1];
	}
	i = lst->selectedindex;
	update_list();
	ui_lst_set_selected_index(lst, i < numgangzones ? i : numgangzones - 1);
	cb_lst_item_selected(lst);
}

static
void in_cb_color(struct UI_INPUT *in)
{
	int col;

	if (IS_VALID_INDEX_SELECTED && common_col(in->value, &col)) {
		gangzone_data[lst->selectedindex].color = col;
		dont_update_color_textbox = 1;
		update_list();
		dont_update_color_textbox = 0;
	}
};

int gangzone_on_background_element_just_clicked()
{
	if (gangzonesactive) {
		return 0;
	}
	return 1;
}

/*http://geomalgorithms.com/a03-_inclusion.html*/
static
float wn_isleft(float x1, float y1, float x2, float y2)
{
	return (x2 - x1) * (cursory - y1) - (cursorx - x1) * (y2 - y1);
}

static
int wn(struct RwV3D a, struct RwV3D b)
{
	if (a.y <= cursory) {
		if (b.y > cursory) {
			if (wn_isleft(a.x, a.y, b.x, b.y) > 0.0f) {
				return 1;
			}
		}
	} else {
		if (b.y <= cursory) {
			if (wn_isleft(a.x, a.y, b.x, b.y) < 0.0f) {
				return -1;
			}
		}
	}
	return 0;
}

static
void try_set_clicked_gangzone_active()
{
	struct RwV3D in, a, b, c, d;
	int i;

	/*count down, because the most specific zones are at the end*/
	for (i = numgangzones - 1; i >= 0; i--) {
		in.x = gangzone_data[i].minx;
		in.y = gangzone_data[i].maxy;
		in.z = zone_z;
		game_WorldToScreen(&a, &in);
		in.x = gangzone_data[i].maxx;
		game_WorldToScreen(&b, &in);
		in.y = gangzone_data[i].miny;
		game_WorldToScreen(&c, &in);
		in.x = gangzone_data[i].minx;
		game_WorldToScreen(&d, &in);
		if (a.z > 0.0f && b.z > 0.0f && c.z > 0.0f && d.z > 0.0f) {
			if (wn(a, b) + wn(b, c) + wn(c, d) + wn(d, a)) {
				if (lst->selectedindex != i) {
					ui_lst_set_selected_index(lst, i);
					cb_lst_item_selected(lst);
				}
				return;
			}
		}
	}
}

void gangzone_frame_update()
{
	struct RwV3D pos;
	struct RwV3D a, b, c, d, in;
	float *minx, *maxx, *miny, *maxy;
	int col, i, j;
	int alpha;

	if (!gangzonesactive) {
		return;
	}

	game_RwIm2DPrepareRender();
	for (i = 0; i < numgangzones; i++) {
		in.x = gangzone_data[i].minx;
		in.y = gangzone_data[i].maxy;
		in.z = zone_z;
		game_WorldToScreen(&a, &in);
		in.x = gangzone_data[i].maxx;
		game_WorldToScreen(&b, &in);
		in.y = gangzone_data[i].miny;
		game_WorldToScreen(&c, &in);
		in.x = gangzone_data[i].minx;
		game_WorldToScreen(&d, &in);
		if(a.z > 0.0f && b.z > 0.0f && c.z > 0.0f && d.z > 0.0f) {
			verts[0].x = a.x; verts[0].y = a.y;
			verts[1].x = b.x; verts[1].y = b.y;
			verts[2].x = c.x; verts[2].y = c.y;
			verts[3].x = d.x; verts[3].y = d.y;
			col = gangzone_data[i].color;
			col = (col & 0xFF00FF00) | ((col & 0xFF) << 16) | ((col & 0xFF0000) >> 16);
			if (lst->selectedindex != i) {
				col = (col & 0xFFFFFF) | 0x70000000;
			}
			verts[0].col = verts[1].col = verts[2].col = verts[3].col = col;
			game_RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, 4);
		}
	}
	if (IS_VALID_INDEX_SELECTED) {
		alpha = 0xFF & (int) (55 + (0xFF - 55) * (1.0f - fabs(sinf(*timeInGame * 0.004f))));
		col = (alpha << 24) | 0xFF00FF;
		verts[0].col = verts[1].col = verts[2].col = verts[3].col = col;
		in.x = gangzone_data[lst->selectedindex].minx;
		in.y = gangzone_data[lst->selectedindex].maxy;
		in.z = zone_z;
		game_WorldToScreen(handles + 0, &in);
		in.x = gangzone_data[lst->selectedindex].maxx;
		game_WorldToScreen(handles + 1, &in);
		in.y = gangzone_data[lst->selectedindex].miny;
		game_WorldToScreen(handles + 2, &in);
		in.x = gangzone_data[lst->selectedindex].minx;
		game_WorldToScreen(handles + 3, &in);
		for (j = 0; j < 4; j++) {
			if (handles[j].z > 0.0f) {
				verts[0].x = handles[j].x - HANDLE_SIZE;
				verts[0].y = handles[j].y - HANDLE_SIZE;
				verts[1].x = handles[j].x + HANDLE_SIZE;
				verts[1].y = handles[j].y - HANDLE_SIZE;
				verts[2].x = handles[j].x + HANDLE_SIZE;
				verts[2].y = handles[j].y + HANDLE_SIZE;
				verts[3].x = handles[j].x - HANDLE_SIZE;
				verts[3].y = handles[j].y + HANDLE_SIZE;
				game_RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, 4);
			}
		}
	}

	player_position = camera->position;
	if (activeMouseState->lmb) {
		if (!was_lmb_down) {
			if (!ui_is_cursor_hovering_any_window()) {
				was_lmb_down = 1;
				if (IS_VALID_INDEX_SELECTED) {
					dragging_handle = -1;
					for (j = 0; j < 4; j++) {
						if (handles[j].x - HANDLE_SIZE < cursorx &&
							cursorx < handles[j].x + HANDLE_SIZE &&
							handles[j].y - HANDLE_SIZE < cursory &&
							cursory < handles[j].y + HANDLE_SIZE)
						{
							dragging_handle = j;
							goto isdragging;
						}
					}
				}
				try_set_clicked_gangzone_active();
isdragging:
				;
			}
		} else {
			ui_get_entity_pointed_at(&clicked_entity, &clicked_colpoint);
			if (clicked_entity) {
				pos = clicked_colpoint.pos;
			} else {
				game_ScreenToWorld(&pos, cursorx, cursory, 50.0f);
			}
			player_position = pos;

			if (dragging_handle != -1) {
				zone_z = pos.z;
				minx = &gangzone_data[lst->selectedindex].minx;
				maxx = &gangzone_data[lst->selectedindex].maxx;
				miny = &gangzone_data[lst->selectedindex].miny;
				maxy = &gangzone_data[lst->selectedindex].maxy;
handle_changed:
				switch(dragging_handle) {
				case 0: /*minx maxy*/
					if (pos.x > *maxx) {
						dragging_handle = 1;
						goto handle_changed;
					}
					if (pos.y < *miny) {
						dragging_handle = 3;
						goto handle_changed;
					}
					*minx = pos.x;
					*maxy = pos.y;
					break;
				case 1: /*maxx maxy*/
					if (pos.x < *minx) {
						dragging_handle = 0;
						goto handle_changed;
					}
					if (pos.y < *miny) {
						dragging_handle = 2;
						goto handle_changed;
					}
					*maxx = pos.x;
					*maxy = pos.y;
					break;
				case 2: /*maxx miny*/
					if (pos.x < *minx) {
						dragging_handle = 3;
						goto handle_changed;
					}
					if (pos.y > *maxy) {
						dragging_handle = 1;
						goto handle_changed;
					}
					*maxx = pos.x;
					*miny = pos.y;
					break;
				case 3: /*minx miny*/
					if (pos.x > *maxx) {
						dragging_handle = 2;
						goto handle_changed;
					}
					if (pos.y > *maxy) {
						dragging_handle = 0;
						goto handle_changed;
					}
					*minx = pos.x;
					*miny = pos.y;
					break;
				}
			}
		}
	} else {
		was_lmb_down = 0;
		dragging_handle = -1;
	}
	player_position.z += 100.0f;

	sprintf(lbl_num_text, "num_zones:_%d", numgangzones);
}

void gangzone_on_active_window_changed(struct UI_WINDOW *_wnd)
{
	if (_wnd == wnd) {
		gangzonesactive = 1;
		was_lmb_down = 0;
		// update list
		update_list();
	} else {
		if (IS_VALID_INDEX_SELECTED) {
			gangzone_data[lst->selectedindex].altcolor = gangzone_data[lst->selectedindex].color;
		}
		gangzonesactive = 0;
	}
}

void gangzone_init()
{
	struct UI_BUTTON *btn;
	int poolsaddr;
	int gangzonesaddr;
	int **ptr;
	int i;

	poolsaddr = *(int*) (*(int*)(samp_handle + /*samp_info_offset*/0x26EA0C) + /*pools*/0x3DE);
	gangzonesaddr = *(int*) (poolsaddr + 0x18);
	gangzone_enable = (int*) (gangzonesaddr + MAX_GANG_ZONES * sizeof(int*));
	ptr = (int**) gangzonesaddr;
	for (i = 0; i < MAX_GANG_ZONES; i++) {
		*ptr = (int*) (gangzone_data + i);
		ptr++;
	}

	btn = ui_btn_make("Gangzones", cb_btn_mainmenu_gangzones);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	wnd = ui_wnd_make(9000.0f, 300.0f, "Gangzones");
	wnd->columns = 4;

	lbl_num = ui_lbl_make(lbl_num_text);
	lbl_num->_parent.span = 4;
	ui_wnd_add_child(wnd, lbl_num);
	lst = ui_lst_make(35, cb_lst_item_selected);
	lst->_parent.span = 4;
	ui_wnd_add_child(wnd, lst);
	btn = ui_btn_make("Clone before", cb_btn_add);
	btn->_parent.userdata = (void*) 0;
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Clone after", cb_btn_add);
	btn->_parent.userdata = (void*) 1;
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn_delete = ui_btn_make("Delete", cb_btn_delete);
	btn_delete->enabled = 0;
	btn_delete->_parent.span = 4;
	ui_wnd_add_child(wnd, btn_delete);
	ui_wnd_add_child(wnd, ui_lbl_make("colABGR"));
	in_color = ui_in_make(in_cb_color);
	in_color->_parent.span = 3;
	ui_wnd_add_child(wnd, in_color);
}

void gangzone_dispose()
{
	ui_wnd_dispose(wnd);
	// this crashes when exiting the game.. hmm..
	//memset(gangzone_enable, 0, sizeof(int) * MAX_GANG_ZONES);
}
