/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "msgbox.h"
#include "gangzone.h"
#include "ui.h"
#include "player.h"
#include "samp.h"
#include "vk.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

struct GANG_ZONE gangzone_data[MAX_GANG_ZONES];
int *gangzone_enable;
int numgangzones;

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst;
static struct UI_BUTTON *btn_edit, *btn_delete, *btn_bulkadd;
static struct UI_LABEL *lbl_num;
static struct UI_INPUT *in_color;
static struct UI_WINDOW *wnd_palette, *wnd_bulkadd;
static float zone_z = 0.0f;
static char lbl_num_text[35];
#define GANG_ZONE_TEXT_LEN 16
static char listnames[MAX_GANG_ZONES * GANG_ZONE_TEXT_LEN];
static int gangzonesactive;
static int was_lmb_down;
static int dont_update_color_textbox;
static int bulk_amount = 10;
static float bulk_direction = 45.0f, bulk_off = 10.0f;

/*this is actually size/2*/
#define HANDLE_SIZE (7.5f)

struct RwV3D handles[4];
int dragging_handle;

int last_handle_snapped_to = 0;

static struct IM2DVERTEX verts[] = {
	{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
	{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
	{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
	{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
};

#define IS_VALID_INDEX_SELECTED (0 <= lst->selectedindex && lst->selectedindex < numgangzones)

static
void abgr_rgba(int *col)
{
	unsigned int v = *((unsigned int*) col);

	v = ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v & 0xFF0000) >> 8) | (v >> 24);
	*col = *((int*) &v);
}

static
int altcolor(int col)
{
	return (col & 0xFF00FF00) | (0xFF - (col & 0xFF)) |
		((0xFF - ((col & 0xFF) >> 8)) << 8) |
		((0xFF - ((col & 0xFF0000) >> 16)) << 16);
}

static
void bulkedit_apply_to_zone(struct GANG_ZONE *master, struct GANG_ZONE *to, int number, int altcolor)
{
	double dir;

	dir = bulk_direction * M_PI / 180.0;
	to->color = master->color;
	to->altcolor = altcolor;
	to->maxx = master->maxx + (float) (bulk_off * number * cos(dir));
	to->minx = master->minx + (float) (bulk_off * number * cos(dir));
	to->maxy = master->maxy + (float) (bulk_off * number * sin(dir));
	to->miny = master->miny + (float) (bulk_off * number * sin(dir));
}

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
		btn_bulkadd->enabled = 0;
		return;
	}
	btn_delete->enabled = 1;
	btn_bulkadd->enabled = 1;

	if (!dont_update_color_textbox) {
		col = gangzone_data[lst->selectedindex].color;
		abgr_rgba(&col);
		sprintf(buf, "%08x", col);
		ui_in_set_text(in_color, buf);
	}
	gangzone_data[lst->selectedindex].altcolor = altcolor(gangzone_data[lst->selectedindex].color);
}

static
void update_list()
{
	int i, selected_index, col;
	char *names[MAX_GANG_ZONES];

	if (IS_VALID_INDEX_SELECTED) {
		gangzone_data[lst->selectedindex].altcolor = gangzone_data[lst->selectedindex].color;
	}

	selected_index = lst->selectedindex;
	for (i = 0; i < numgangzones; i++) {
		names[i] = listnames + i * GANG_ZONE_TEXT_LEN;
		col = gangzone_data[i].color;
		abgr_rgba(&col);
		sprintf(names[i], "%04d:_%p", i, col);
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
int snap_cursor_to_handle(struct GANG_ZONE *zone, int handle_index)
{
	struct RwV3D vin, vout;

	switch (handle_index) {
	case 0:
		vin.x = zone->minx;
		vin.y = zone->maxy;
		break;
	case 1:
		vin.x = zone->maxx;
		vin.y = zone->maxy;
		break;
	case 2:
		vin.x = zone->maxx;
		vin.y = zone->miny;
		break;
	case 3:
		vin.x = zone->minx;
		vin.y = zone->miny;
	}
	vin.z = zone_z;
	game_WorldToScreen(&vout, &vin);
	if (vout.z > 0.0f && vout.x > 0.0f && vout.x < fresx && vout.y > 0.0f && vout.y < fresy) {
		cursorx = vout.x;
		cursory = vout.y;
		return 1;
	}
	return 0;
}

static
int snap_cursor_to_next_handle(struct GANG_ZONE *zone)
{
	int i;

	for (i = 0; i < 4; i++) {
		last_handle_snapped_to = (last_handle_snapped_to + 1) % 4;
		if (snap_cursor_to_handle(zone, last_handle_snapped_to)) {
			return 1;
		}
	}
	return 0;
}

static
void add(int relative_index_to_select_0_or_1)
{
	int i, index_of_new_zone, force_under_camera;
	double facingAngle, udrotation, camera_dist_to_z_plane;
	struct RwV3D *lv;

	if (numgangzones >= MAX_GANG_ZONES) {
		msg_message = "Limit_reached!";
		msg_title = "Gangzones";
		msg_btn1text = "Ok";
		msg_show(cb_msg_limitreached);
		return;
	}

	if (IS_VALID_INDEX_SELECTED) {
		for (i = numgangzones; i > lst->selectedindex; i--) {
			gangzone_data[i] = gangzone_data[i - 1];
		}
		index_of_new_zone = lst->selectedindex + relative_index_to_select_0_or_1;
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
		index_of_new_zone = numgangzones * relative_index_to_select_0_or_1;
	}

	last_handle_snapped_to = (last_handle_snapped_to + 3) % 4; /*since snap_cursor_to_next_handle will go to the next handle, and we want to keep the same after cloning*/
	force_under_camera = !snap_cursor_to_next_handle(gangzone_data + index_of_new_zone);
	if (force_under_camera || !numgangzones) {
		if (!numgangzones) {
			gangzone_data[0].color = 0xFF000000;
		}
		lv = &camera->lookVector;
		zone_z = (float) fabs(camera->position.z - zone_z);
		zone_z = lv->z > 0.0f ? camera->position.z + zone_z : camera->position.z - zone_z;
		facingAngle = atan2(lv->y, lv->x);
		udrotation = atan2(lv->z, sqrt(lv->x * lv->x + lv->y * lv->y));
		camera_dist_to_z_plane = (zone_z - camera->position.z) / tan(udrotation);
		gangzone_data[index_of_new_zone].minx = camera->position.x - 10.0f + (float) (camera_dist_to_z_plane * cos(facingAngle));
		gangzone_data[index_of_new_zone].maxx = camera->position.x + 10.0f + (float) (camera_dist_to_z_plane * cos(facingAngle));
		gangzone_data[index_of_new_zone].miny = camera->position.y - 10.0f + (float) (camera_dist_to_z_plane * sin(facingAngle));
		gangzone_data[index_of_new_zone].maxy = camera->position.y + 10.0f + (float) (camera_dist_to_z_plane * sin(facingAngle));
		for (i = 0; i < 4 && !snap_cursor_to_handle(gangzone_data + index_of_new_zone, i); i++);
	}

	gangzone_enable[numgangzones] = 1;
	numgangzones++;
	update_list();
	ui_lst_set_selected_index(lst, index_of_new_zone);
	cb_lst_item_selected(lst);
}

static
void cb_btn_add(struct UI_BUTTON *btn)
{
	add(((int) btn->_parent.userdata) & 1);
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
		abgr_rgba(&col);
		gangzone_data[lst->selectedindex].color = col;
		dont_update_color_textbox = 1;
		update_list();
		dont_update_color_textbox = 0;
	}
}

static
void cb_btn_use_palette_color(struct UI_BUTTON *btn)
{
	if (IS_VALID_INDEX_SELECTED) {
		gangzone_data[lst->selectedindex].color = (int) btn->_parent.userdata;
		update_list();
	}
	ui_show_window(wnd);
	ui_wnd_dispose(wnd_palette);
	wnd_palette = NULL;
}

static
void cb_lbl_palette_draw(struct UI_LABEL *lbl)
{
	ui_element_draw_background((void*) lbl, (int) lbl->_parent.userdata);
}

static
void add_palette_color(int rgba)
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;
	char colbuf[10];
	int abgr;

	sprintf(colbuf, "%p:", rgba);
	btn = ui_btn_make(colbuf, cb_btn_use_palette_color);
	abgr = rgba;
	abgr_rgba(&abgr);
	btn->_parent.userdata = (void*) abgr;
	ui_wnd_add_child(wnd_palette, btn);
	lbl = ui_lbl_make("___");
	lbl->_parent.proc_draw = (ui_method*) cb_lbl_palette_draw;
	lbl->_parent.userdata = (void*) (0xFF000000 | (rgba >> 8));
	ui_wnd_add_child(wnd_palette, lbl);
}

static
int cb_wnd_palette_close(struct UI_WINDOW *_)
{
	ui_hide_window();
	ui_wnd_dispose(wnd_palette);
	wnd_palette = NULL;
	ui_show_window(wnd);
	return 1;
}

static
void open_wnd_palette()
{
	struct UI_LABEL *lbl;
	int listedcolors[MAX_GANG_ZONES];
	int numlistedcolors;
	int i, j, col;

	wnd_palette = ui_wnd_make(300.0f, 300.0f, "Gangzones");
	wnd_palette->columns = 8;
	wnd_palette->proc_close = (ui_method*) cb_wnd_palette_close;

	lbl = ui_lbl_make("Colors currently used");
	lbl->_parent.span = 8;
	ui_wnd_add_child(wnd_palette, lbl);
	numlistedcolors = 0;
	for (i = 0; i < numgangzones; i++) {
		col = gangzone_data[i].color;
		for (j = 0; j < numlistedcolors; j++) {
			if (listedcolors[j] == col) {
				goto skip;
			}
		}
		listedcolors[numlistedcolors++] = col;
		abgr_rgba(&col);
		add_palette_color(col);
skip:
		;
	}
	if (numlistedcolors % 8) {
		// fill columns until we're at 0 again
		lbl = ui_lbl_make("_");
		lbl->_parent.span = 8 - (numlistedcolors % 8);
		ui_wnd_add_child(wnd_palette, lbl);
	}
	lbl = ui_lbl_make("Default colors");
	lbl->_parent.span = 8;
	ui_wnd_add_child(wnd_palette, lbl);
	add_palette_color(0x989898FF);
	add_palette_color(0x909898FF);
	add_palette_color(0x686868FF);
	add_palette_color(0x595753FF);
	add_palette_color(0x000000FF);
	add_palette_color(0xFFFFFFFF);
	add_palette_color(0x988870FF);
	add_palette_color(0xE0C060FF);
	add_palette_color(0x386830FF);
	add_palette_color(0x708838FF);

	ui_wnd_center(wnd_palette);
	ui_show_window(wnd_palette);
}

static
int wnd_accept_keyup(struct UI_WINDOW *_wnd, int vk)
{
	if (IS_VALID_INDEX_SELECTED) {
		if (vk == VK_TAB) {
			if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
				last_handle_snapped_to = (last_handle_snapped_to + 2) % 4; /*skip two from current so we will effectively go in reverse direction*/
			}
			return snap_cursor_to_next_handle(gangzone_data + lst->selectedindex);
		} 
	}
	// the "bulk add" window has this same listener, so only do the following if we're actually in the main window
	if (_wnd == wnd) {
		if (vk == VK_C) {
			add(1);
			return 1;
		} else if (IS_VALID_INDEX_SELECTED && vk == VK_V) {
			open_wnd_palette();
			return 1;
		}
	}
	return 0;
}

static
void bulk_update()
{
	struct GANG_ZONE *master;
	int i, idx;

	master = gangzone_data + lst->selectedindex;
	for (idx = MAX_GANG_ZONES - bulk_amount, i = 1; idx < MAX_GANG_ZONES; idx++, i++) {
		gangzone_enable[idx] = 1;
		bulkedit_apply_to_zone(master, gangzone_data + idx, i, altcolor(master->color));
	}
}

static
void cb_in_bulk_direction(struct UI_INPUT *in)
{
	bulk_direction = (float) atof(in->value);
	bulk_update();
}

static
void cb_in_bulk_off(struct UI_INPUT *in)
{
	bulk_off = (float) atof(in->value);
	bulk_update();
}

static
void cb_in_bulk_amount(struct UI_INPUT *in)
{
	int i;

	for (i = 1; i <= bulk_amount; i++) {
		gangzone_enable[MAX_GANG_ZONES - i] = 0;
	}
	bulk_amount = atoi(in->value);
	if (bulk_amount > MAX_GANG_ZONES - numgangzones - 2) { // -2 is just a "safe margin", no specific meaning
		bulk_amount = MAX_GANG_ZONES - numgangzones - 2;
		if (bulk_amount < 0) {
			bulk_amount = 0;
		}
	}
	bulk_update();
}

static
int cb_wnd_bulkadd_close()
{
	int i;

	for (i = 0; i < bulk_amount; i++) {
		gangzone_enable[MAX_GANG_ZONES - 1 - i] = 0;
	}
	if (IS_VALID_INDEX_SELECTED) {
		gangzone_data[lst->selectedindex].altcolor = altcolor(gangzone_data[lst->selectedindex].color);
	}
	ui_hide_window();
	ui_wnd_dispose(wnd_bulkadd);
	wnd_bulkadd = NULL;
	ui_show_window(wnd);
	return 1;
}

static
void cb_btn_bulk_commit(struct UI_BUTTON *btn)
{
	struct GANG_ZONE *master;
	int from, to, at, n;

	for (from = MAX_GANG_ZONES - 1, n = 0; n < bulk_amount; from--, n++) {
		gangzone_enable[from] = 0;
	}
	if (IS_VALID_INDEX_SELECTED && bulk_amount) {
		{
			from = lst->selectedindex + 1;
			to = numgangzones;
			for (n = 0; n < bulk_amount; n++) {
				gangzone_enable[to] = 1;
				gangzone_data[to++] = gangzone_data[from++];
			}
			numgangzones += bulk_amount;
		}
		master = gangzone_data + lst->selectedindex;
		at = lst->selectedindex + 1;
		for (n = 1; n <= bulk_amount; n++) {
			bulkedit_apply_to_zone(master, gangzone_data + at++, n, master->color);
		}
	}

	n = bulk_amount;
	bulk_amount = 0; /* so that the undo code in cb_wnd_bulkadd_close doesn't undo anything, given that
			    we just committed new gangzones which may now overlap with the beginning of the
			    "temp uncommitted bulkadd preview" zones */
	cb_wnd_bulkadd_close();
	bulk_amount = n; /* and restore it or the input field in the window next time we open it will be 0 */
}

static
void open_wnd_bulkadd()
{
	struct UI_INPUT *in_direction, *in_off, *in_amount;
	struct UI_BUTTON *btn_commit;
	char buf[100];

	if (IS_VALID_INDEX_SELECTED) {
		gangzone_data[lst->selectedindex].altcolor = gangzone_data[lst->selectedindex].color;
	}

	wnd_bulkadd = ui_wnd_make(fresx, fresy / 2.0f, "Bulk add");
	wnd_bulkadd->columns = 2;
	wnd_bulkadd->proc_close = (ui_method*) cb_wnd_bulkadd_close;
	wnd->_parent._parent.proc_accept_keyup = (ui_method1*) wnd_accept_keyup; /*to still have the TAB action*/

	ui_wnd_add_child(wnd_bulkadd, ui_lbl_make("direction_(deg):"));
	ui_wnd_add_child(wnd_bulkadd, (in_direction = ui_in_make(cb_in_bulk_direction)));
	ui_wnd_add_child(wnd_bulkadd, ui_lbl_make("offset:"));
	ui_wnd_add_child(wnd_bulkadd, (in_off = ui_in_make(cb_in_bulk_off)));
	ui_wnd_add_child(wnd_bulkadd, ui_lbl_make("amount:"));
	ui_wnd_add_child(wnd_bulkadd, (in_amount = ui_in_make(cb_in_bulk_amount)));
	ui_wnd_add_child(wnd_bulkadd, (btn_commit = ui_btn_make("commit", cb_btn_bulk_commit)));

	sprintf(buf, "%.2f", bulk_direction);
	ui_in_set_text(in_color, buf);
	sprintf(buf, "%.2f", bulk_off);
	ui_in_set_text(in_off, buf);
	sprintf(buf, "%d", bulk_amount);
	ui_in_set_text(in_amount, buf);
	btn_commit->_parent.span = 2;

	ui_show_window(wnd_bulkadd);
	bulk_update();
}

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
	double lvhang;
	int col, i, j;
	int alpha;

	if (!gangzonesactive) {
		return;
	}

	game_RwIm2DPrepareRender();
	for (i = 0;; i++) {
		if (i == numgangzones) {
			i = MAX_GANG_ZONES - bulk_amount;
		}
		if (i >= numgangzones && (ui_get_active_window() != wnd_bulkadd || i >= MAX_GANG_ZONES)) {
			break;
		}
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
							last_handle_snapped_to = dragging_handle;
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
				zone_z = pos.z;
			} else {
				game_ScreenToWorld(&pos, cursorx, cursory, 50.0f);
			}
			player_position = pos;

			if (dragging_handle != -1) {
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
				if (wnd_bulkadd && ui_get_active_window() == wnd_bulkadd) {
					bulk_update();
				}
			}
		}
	} else {
		was_lmb_down = 0;
		dragging_handle = -1;
	}
	player_position.z += 100.0f;
	// offset the player a little so the player blip on the minimap is not hiding the gangzone being edited
	lvhang = atan2(camera->lookVector.y, camera->lookVector.x);
	player_position.x -= (float) (75.0f * cos(lvhang));
	player_position.y -= (float) (75.0f * sin(lvhang));

	sprintf(lbl_num_text, "num_zones:_%d", numgangzones);
}

void gangzone_on_active_window_changed(struct UI_WINDOW *_wnd)
{
	if (wnd_bulkadd && _wnd == wnd_bulkadd) {
		return; // pretend we're still in the gangzone main window
	}
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
	struct UI_LABEL *lbl;
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
	wnd->_parent._parent.proc_accept_keyup = (ui_method1*) wnd_accept_keyup;

	lbl_num = ui_lbl_make(lbl_num_text);
	lbl_num->_parent.span = 4;
	ui_wnd_add_child(wnd, lbl_num);
	lbl = ui_lbl_make("press ~r~TAB~w~ to snap cursor");
	lbl->_parent.span = 4;
	ui_wnd_add_child(wnd, lbl);
	lst = ui_lst_make(25, cb_lst_item_selected);
	lst->_parent.span = 4;
	ui_wnd_add_child(wnd, lst);
	btn = ui_btn_make("Clone before", cb_btn_add);
	btn->_parent.userdata = (void*) 0;
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Clone after (C)", cb_btn_add);
	btn->_parent.userdata = (void*) 1;
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn_delete = ui_btn_make("Delete", cb_btn_delete);
	btn_delete->enabled = 0;
	btn_delete->_parent.span = 2;
	ui_wnd_add_child(wnd, btn_delete);
	btn_bulkadd = ui_btn_make("Bulkadd", (ui_method*) open_wnd_bulkadd);
	btn_bulkadd->enabled = 0;
	btn_bulkadd->_parent.span = 2;
	ui_wnd_add_child(wnd, btn_bulkadd);
	ui_wnd_add_child(wnd, ui_lbl_make("colRGBA"));
	in_color = ui_in_make(in_cb_color);
	in_color->_parent.span = 2;
	in_color->_parent.pref_width = 100.0f;
	ui_wnd_add_child(wnd, in_color);
	ui_wnd_add_child(wnd, ui_btn_make("palette (V)", (ui_method*) open_wnd_palette));

	wnd_palette = NULL;
}

void gangzone_dispose()
{
	ui_wnd_dispose(wnd);
	// this crashes when exiting the game.. hmm..
	//memset(gangzone_enable, 0, sizeof(int) * MAX_GANG_ZONES);
}
