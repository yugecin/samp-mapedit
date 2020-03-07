/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (msgboxcb)(int);

#define MSGBOX_RESULT_1 1
#define MSGBOX_RESULT_2	2
#define MSGBOX_RESULT_3	3

extern char *msg_title, *msg_message, *msg_message2;
/**
NULL to disable the button
*/
extern char *msg_btn1text, *msg_btn2text, *msg_btn3text;

void msg_init();
void msg_dispose();
/**
Set msg_ values before invoking.

@param hiddenwindow the window this msg hides, to reshow after, may be NULL
*/
void msg_show(msgboxcb *cb);
