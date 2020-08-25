/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

void ui_chk_updatetext(struct UI_CHECKBOX *chk)
{
	chk->_parent.text[1] = '_' + ('x' - '_') * chk->checked;
}

static
int ui_chk_mouseup(struct UI_CHECKBOX *chk)
{
	if (ui_element_being_clicked == chk) {
		if (ui_element_is_hovered(&chk->_parent._parent)) {
			chk->checked = !chk->checked;
			ui_chk_updatetext(chk);
			if (chk->cb) {
				chk->cb(chk);
			}
		}
		return 1;
	}
	return 0;
}

struct UI_CHECKBOX *ui_chk_make(char *text, int check, chkcb *cb)
{
	struct UI_CHECKBOX *chk;
	int textlenandzero;

	textlenandzero = 3 + strlen(text) + 1;
	chk = malloc(sizeof(struct UI_CHECKBOX));
	ui_elem_init(chk, UIE_RADIOBUTTON);
	chk->_parent._parent.proc_dispose = (ui_method*) ui_btn_dispose;
	/*draw for radiobutton can be the button proc*/
	chk->_parent._parent.proc_draw = (ui_method*) ui_btn_draw;
	/*mousedown for radiobutton can be the button proc*/
	chk->_parent._parent.proc_mousedown = (ui_method*) ui_btn_mousedown;
	chk->_parent._parent.proc_mouseup = (ui_method*) ui_chk_mouseup;
	/*recalcsize for radiobutton can be the button proc*/
	chk->_parent._parent.proc_recalc_size = (ui_method*) ui_btn_recalc_size;
	chk->_parent.alignment = CENTER;
	chk->_parent.text = malloc(sizeof(char) * textlenandzero);
	chk->_parent.text[0] = '[';
	chk->_parent.text[1] = '_' + ('x' - '_') * check;
	chk->_parent.text[2] = ']';
	memcpy(chk->_parent.text + 3, text, textlenandzero - 3);
	chk->_parent.foregroundABGR = -1;
	chk->_parent.enabled = 1;
	chk->checked = check;
	chk->cb = cb;
	ui_btn_recalc_size((struct UI_BUTTON*) chk);
	return chk;
}
