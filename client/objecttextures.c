/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ide.h"
#include "msgbox.h"
#include "objects.h"
#include "objbrowser.h"
#include "bulkedit.h"
#include "bulkeditui.h"
#include "objectseditor.h"
#include "ui.h"
#include "sockets.h"
#include "../shared/serverlink.h"

#include "game_texturelist.c"

static int model_txdidx[MAX_MODELS];
/*
int txd_hashes[]
char *txd_names[]
char *txd_textures[]
int txd_textures_offset[]
char txd_numtextures[]
*/

static struct UI_WINDOW *wnd;
static struct RADIOBUTTONGROUP *rdbgroup_materialindex;
static struct RADIOBUTTONGROUP *rdbgroup_materialsize;
static struct RADIOBUTTONGROUP *rdbgroup_align;
static struct UI_INPUT *in_font, *in_text, *in_fontsize, *in_fgcol;
static struct UI_INPUT *in_bgcol, *in_matcol, *in_modelid, *in_txd, *in_texture;
static struct UI_CHECKBOX *chk_bold;

static struct UI_WINDOW *wnd_textures;
#define NUM_TEXTURES_BUTTONS 63
static struct UI_BUTTON *btn_textures[NUM_TEXTURES_BUTTONS];

int fgcol = -1, bgcol = 0xFF000000, matcol = 0;

static struct OBJECT *object;

static
int nfsu2_cshash(char *input)
{
	unsigned int result;
	int j;

	result = -1;
	j = 0;
	while (input[j]) {
		result *= 33;
		result += input[j];
		j++;
	}
	return result;
}

void objecttextures_associate_model_txd(int model, char *txd)
{
	int hash, i;

	hash = nfsu2_cshash(txd);
	for (i = 0; i < MAX_MODELS; i++) {
		if (txd_hashes[i] == hash) {
			model_txdidx[model] = i + 1;
			break;
		}
	}
}

static
void col(char *text, int *out)
{
	int j, col, c;
	char unused[30];

	col = 0;
	for (j = 0; j < 8; j++) {
		c = text[j] - '0';
		if (c < 0 || 9 < c) {
			c = text[j] - 'A' + 10;
			if (c < 10 || 15 < c) {
				c = text[j] - 'a' + 10;
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
	*out = col;
}

static
void update_material()
{
	int modelid;
	int materialIndex;

	materialIndex = (int) rdbgroup_materialindex->activebutton->_parent._parent.userdata;
	modelid = atoi(in_modelid->value);
	objects_set_material(
		object,
		modelid,
		materialIndex,
		in_txd->value,
		in_texture->value,
		matcol);
}

static
void cb_modelid(struct UI_INPUT *in)
{
	update_material();
}

static
void cb_in_txd(struct UI_INPUT *in)
{
	update_material();
}

static
void cb_in_texture(struct UI_INPUT *in)
{
	update_material();
}

static
void cb_in_matcol(struct UI_INPUT *in)
{
	col(in->value, &matcol);
	update_material();
}

static
void update_material_text()
{
	int materialIndex;
	int materialSize;
	int fontsize;
	char bold;
	char align;

	bold = chk_bold->checked;
	fontsize = atoi(in_fontsize->value);
	materialIndex = (int) rdbgroup_materialindex->activebutton->_parent._parent.userdata;
	materialSize = (int) rdbgroup_materialsize->activebutton->_parent._parent.userdata;
	align = (int) rdbgroup_align->activebutton->_parent._parent.userdata;
	objects_set_material_text(object,
		materialIndex,
		materialSize, in_font->value, fontsize, bold, fgcol, bgcol, align, in_text->value);
}

static
void cb_rdb_materialindex(struct UI_RADIOBUTTON *rdb)
{
	update_material_text(); /*TODO: may be texture*/
}

static
void cb_rdb_materialsize(struct UI_RADIOBUTTON *rdb)
{
	update_material_text();
}

static
int wnd_textures_proc_close(struct UI_WINDOW *_wnd)
{
	ui_show_window(wnd);
	return 1;
}

static
void cb_msg_no_textures_for_model(int opt)
{
	ui_show_window(wnd);
}

static
void cb_btn_texture(struct UI_BUTTON *btn)
{
	ui_in_set_text(in_texture, btn->text);
	update_material();
}

static
void cb_objbrowser_model_chosen(int modelid)
{
	int idx, i;
	int offset;
	char str[100];

	idx = model_txdidx[modelid] - 1;
	if (idx == -1) {
		msg_title = "Textures";
		msg_message = "No_textures_found?";
		msg_btn1text = "Ok";
		msg_show(cb_msg_no_textures_for_model);
		return;
	}

	sprintf(str, "%d", modelid);
	ui_in_set_text(in_modelid, str);
	ui_in_set_text(in_txd, txd_names[idx]);
	offset = txd_textures_offset[idx];
	for (i = 0; i < NUM_TEXTURES_BUTTONS; i++) {
		if (btn_textures[i] == NULL) {
			break;
		}
		ui_wnd_remove_child(wnd_textures, btn_textures[i]);
		free(btn_textures[i]);
		btn_textures[i] = NULL;
	}
	for (i = 0; i < txd_numtextures[idx] && i < NUM_TEXTURES_BUTTONS; i++) {
		btn_textures[i] = ui_btn_make(txd_textures[offset + i], cb_btn_texture);
		ui_wnd_add_child(wnd_textures, btn_textures[i]);
	}

	ui_show_window(wnd_textures);
}

static
void cb_btn_pick_object(struct UI_BUTTON *btn)
{
	struct RwV3D pos;

	game_ObjectGetPos(object->sa_object, &pos);
	ui_hide_window();
	objbrowser_highlight_model(atoi(in_modelid->value));
	objbrowser_show(&pos);
	objbrowser_never_create = 1;
	objbrowser_cb = cb_objbrowser_model_chosen;
}

static
void cb_in_text(struct UI_INPUT *in)
{
	update_material_text();
}

static
void cb_in_fgcol(struct UI_INPUT *in)
{
	col(in->value, &fgcol);
	update_material_text();
}

static
void cb_in_bgcol(struct UI_INPUT *in)
{
	col(in->value, &bgcol);
	update_material_text();
}

static
void cb_in_font(struct UI_INPUT *in)
{
	update_material_text();
}

static
void cb_in_fontsize(struct UI_INPUT *in)
{
	update_material_text();
}

static
void cb_chk_bold(struct UI_CHECKBOX *chk)
{
	update_material_text();
}

static
void cb_rdbgroup_align(struct UI_RADIOBUTTON *rdb)
{
	update_material_text();
}

void objecttextures_show(struct OBJECT *obj)
{
	object = obj;
	ui_show_window(wnd);
}

void objecttextures_init()
{
	struct UI_BUTTON *btn;
	struct UI_RADIOBUTTON *rdb;
	struct UI_LABEL *lbl;

	wnd_textures = ui_wnd_make(300.0f, 300.0f, "Textures");
	wnd_textures->columns = 5;
	ui_wnd_add_child(wnd_textures, lbl = ui_lbl_make("THIS_LIST_IS_INCOMPLETE"));
	lbl->_parent.span = 5;
	ui_wnd_add_child(wnd_textures, lbl = ui_lbl_make("(beach_sfs.txd_misses_grass_128hv etc)"));
	lbl->_parent.span = 5;
	wnd_textures->proc_close = wnd_textures_proc_close;

	wnd = ui_wnd_make(5000.0f, 300.0f, "Textures");
	wnd->columns = 5;

	rdbgroup_materialindex = ui_rdbgroup_make(cb_rdb_materialindex);
	ui_wnd_add_child(wnd, ui_lbl_make("Material_index:"));
	rdb = ui_rdb_make("0", rdbgroup_materialindex, 1);
	rdb->_parent._parent.userdata = (void*) 0;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("1", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 1;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("2", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 2;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("3", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 3;
	ui_wnd_add_child(wnd, rdb);
	ui_wnd_add_child(wnd, NULL);
	rdb = ui_rdb_make("4", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 4;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("5", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 5;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("6", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 6;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("7", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 7;
	ui_wnd_add_child(wnd, rdb);
	ui_wnd_add_child(wnd, NULL);
	rdb = ui_rdb_make("8", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 8;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("9", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 9;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("10", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 10;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("11", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 11;
	ui_wnd_add_child(wnd, rdb);
	ui_wnd_add_child(wnd, NULL);
	rdb = ui_rdb_make("12", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 12;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("13", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 13;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("14", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 14;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("15", rdbgroup_materialindex, 0);
	rdb->_parent._parent.userdata = (void*) 15;
	ui_wnd_add_child(wnd, rdb);

	ui_wnd_add_child(wnd, lbl = ui_lbl_make("Set_material"));
	ui_wnd_add_child(wnd, lbl = ui_lbl_make("==============================="));
	lbl->_parent.span = 4;
	ui_wnd_add_child(wnd, ui_lbl_make("model:"));
	ui_wnd_add_child(wnd, in_modelid = ui_in_make(cb_modelid));
	in_modelid->_parent.span = 4;
	ui_wnd_add_child(wnd, ui_lbl_make("txd:"));
	ui_wnd_add_child(wnd, in_txd = ui_in_make(cb_in_txd));
	in_txd->_parent.span = 4;
	ui_wnd_add_child(wnd, ui_lbl_make("texture:"));
	ui_wnd_add_child(wnd, in_texture = ui_in_make(cb_in_texture));
	in_texture->_parent.span = 4;
	ui_wnd_add_child(wnd, ui_lbl_make("colARGB:"));
	ui_wnd_add_child(wnd, in_matcol = ui_in_make(cb_in_matcol));
	in_matcol->_parent.span = 4;
	ui_wnd_add_child(wnd, ui_lbl_make("From_object:"));
	ui_wnd_add_child(wnd, btn = ui_btn_make("pick_object", cb_btn_pick_object));
	btn->_parent.span = 4;

	ui_wnd_add_child(wnd, lbl = ui_lbl_make("Set_text"));
	ui_wnd_add_child(wnd, lbl = ui_lbl_make("==============================="));
	lbl->_parent.span = 4;
	ui_wnd_add_child(wnd, ui_lbl_make("Text:"));
	ui_wnd_add_child(wnd, in_text = ui_in_make(cb_in_text));
	in_text->_parent.span = 4;
	ui_wnd_add_child(wnd, ui_lbl_make("Font:"));
	ui_wnd_add_child(wnd, in_font = ui_in_make(cb_in_font));
	in_font->_parent.span = 4;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_lbl_make("Size:"));
	ui_wnd_add_child(wnd, in_fontsize = ui_in_make(cb_in_fontsize));
	in_fontsize->_parent.pref_width = 20;
	ui_wnd_add_child(wnd, chk_bold = ui_chk_make("Bold", 0, cb_chk_bold));
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_lbl_make("Align:"));
	rdbgroup_align = ui_rdbgroup_make(cb_rdbgroup_align);
	rdb = ui_rdb_make("<", rdbgroup_align, 1);
	rdb->_parent._parent.userdata = (void*) 0;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("i", rdbgroup_align, 0);
	rdb->_parent._parent.userdata = (void*) 1;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make(">", rdbgroup_align, 0);
	rdb->_parent._parent.userdata = (void*) 2;
	ui_wnd_add_child(wnd, rdb);
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_lbl_make("FgARGB:"));
	ui_wnd_add_child(wnd, in_fgcol = ui_in_make(cb_in_fgcol));
	in_fgcol->_parent.span = 4;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_lbl_make("BgARGB:"));
	ui_wnd_add_child(wnd, in_bgcol = ui_in_make(cb_in_bgcol));
	in_bgcol->_parent.span = 4;

	rdbgroup_materialsize = ui_rdbgroup_make(cb_rdb_materialsize);
	ui_wnd_add_child(wnd, ui_lbl_make("Texture_size:"));
	rdb = ui_rdb_make("32x32", rdbgroup_materialsize, 1);
	rdb->_parent._parent.userdata = (void*) 10;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("64x32", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 20;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("64x64", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 30;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("128x32", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 40;
	ui_wnd_add_child(wnd, rdb);
	ui_wnd_add_child(wnd, NULL);
	rdb = ui_rdb_make("128x64", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 50;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("128x128", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 60;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("256x32", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 70;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("256x64", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 80;
	ui_wnd_add_child(wnd, rdb);
	ui_wnd_add_child(wnd, NULL);
	rdb = ui_rdb_make("256x128", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 90;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("256x256", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 100;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("512x64", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 110;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("512x128", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 120;
	ui_wnd_add_child(wnd, rdb);
	ui_wnd_add_child(wnd, NULL);
	rdb = ui_rdb_make("512x256", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 130;
	ui_wnd_add_child(wnd, rdb);
	rdb = ui_rdb_make("512x512", rdbgroup_materialsize, 0);
	rdb->_parent._parent.userdata = (void*) 140;
	ui_wnd_add_child(wnd, rdb);
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, NULL);
}

void objecttextures_dispose()
{
	ui_wnd_dispose(wnd);
}
