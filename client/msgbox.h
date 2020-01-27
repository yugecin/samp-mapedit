/* vim: set filetype=c ts=8 noexpandtab: */

extern char *msg_title, *msg_message;

void msg_init();
void msg_dispose();
/**
Set msg_message and msg_title before invoking.

@param hiddenwindow the window this msg hides, to reshow after, may be NULL
*/
void msg_show(struct UI_WINDOW *hiddenwindow);
