/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"
#include "msgbox.h"
#include <string.h>

char *msg_title, *msg_message, *msg_message2 = "";
char *msg_btn1text = NULL, *msg_btn2text = NULL, *msg_btn3text = NULL;

static struct UI_WINDOW *msg_wnd;
static struct UI_LABEL *msg_lbl, *msg_lbl2;
static struct UI_BUTTON *msg_btn1, *msg_btn2, *msg_btn3;

static msgboxcb *result_cb;

static
void cb_btn(struct UI_BUTTON *btn)
{
	ui_hide_window();
	msg_message2 = "";
	msg_btn1text = NULL;
	msg_btn2text = NULL;
	msg_btn3text = NULL;
	if (result_cb != NULL) {
		result_cb((int) btn->_parent.userdata);
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
	msg_lbl2 = ui_lbl_make(NULL);
	msg_lbl2->_parent.span = 3;
	ui_wnd_add_child(msg_wnd, msg_lbl2);
	msg_btn1 = ui_btn_make("a", cb_btn);
	msg_btn1->_parent.userdata = (void*) MSGBOX_RESULT_1;
	ui_wnd_add_child(msg_wnd, msg_btn1);
	msg_btn2 = ui_btn_make("b", cb_btn);
	msg_btn2->_parent.userdata = (void*) MSGBOX_RESULT_2;
	ui_wnd_add_child(msg_wnd, msg_btn2);
	msg_btn3 = ui_btn_make("c", cb_btn);
	msg_btn3->_parent.userdata = (void*) MSGBOX_RESULT_3;
	ui_wnd_add_child(msg_wnd, msg_btn3);
}

void msg_dispose()
{
	TRACE("msg_dispose");
	ui_wnd_dispose(msg_wnd);
}

void msg_show(msgboxcb *cb)
{
	struct UI_ELEMENT *e;

	result_cb = cb;
	msg_lbl->text = msg_message;
	msg_lbl->_parent.proc_recalc_size(msg_lbl);
	msg_lbl2->text = msg_message2;
	msg_lbl2->_parent.proc_recalc_size(msg_lbl2);
	msg_wnd->title = msg_title;
	msg_btn1->text = msg_btn1text;
	msg_btn2->text = msg_btn2text;
	msg_btn3->text = msg_btn3text;
	msg_btn1->enabled = msg_btn1text != NULL;
	msg_btn2->enabled = msg_btn2text != NULL;
	msg_btn3->enabled = msg_btn3text != NULL;
	msg_btn1->_parent.proc_recalc_size(msg_btn1);
	msg_btn2->_parent.proc_recalc_size(msg_btn2);
	msg_btn3->_parent.proc_recalc_size(msg_btn3);
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
