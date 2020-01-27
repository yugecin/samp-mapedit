/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"
#include "msgbox.h"
#include <string.h>

char *msg_title, *msg_message;

static struct UI_WINDOW *msg_wnd;
static struct UI_LABEL *msg_lbl;
static struct UI_BUTTON *msg_btn1, *msg_btn2, *msg_btn3;
static struct UI_WINDOW *result_wnd;

static
void cb_btn(struct UI_BUTTON *btn)
{
	if (result_wnd != NULL) {
		ui_show_window(result_wnd);
	}
}

void msg_init()
{
	msg_wnd = ui_wnd_make(0.0f, 0.0f, NULL);
	msg_wnd->columns = 3;
	msg_wnd->closeable = 0;

	msg_lbl = ui_lbl_make(NULL);
	msg_lbl->_parent.span = 3;
	ui_wnd_add_child(msg_wnd, msg_lbl);
	msg_btn1 = ui_btn_make("a", cb_btn);
	ui_wnd_add_child(msg_wnd, msg_btn1);
	msg_btn2 = ui_btn_make("b", cb_btn);
	ui_wnd_add_child(msg_wnd, msg_btn2);
	msg_btn3 = ui_btn_make("c", cb_btn);
	ui_wnd_add_child(msg_wnd, msg_btn3);
}

void msg_dispose()
{
	free(msg_wnd);
}

void msg_show(struct UI_WINDOW *hiddenwindow)
{
	struct UI_ELEMENT *e;

	result_wnd = hiddenwindow;
	msg_lbl->text = msg_message;
	msg_lbl->_parent.proc_recalc_size(msg_lbl);
	msg_wnd->title = msg_title;
	/*force layout now to get the size, to position in center*/
	msg_wnd->_parent.need_layout = 1;
	e = (struct UI_ELEMENT*) msg_wnd;
	e->proc_update(e);
	e->x = (fresx - e->width) / 2.0f;
	e->y = (fresy - e->height) / 2.0f;
	/*still ask layout so the children's positions can be adjusted*/
	msg_wnd->_parent.need_layout = 1;
	ui_show_window(msg_wnd);
}
