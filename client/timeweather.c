/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "sockets.h"
#include "timeweather.h"
#include "../shared/serverlink.h"
#include <stdio.h>
#include <string.h>

#define MAX_WEATHER 22

static struct UI_BUTTON *btn_main_menu;
static struct UI_WINDOW *wnd;
static struct UI_INPUT *in_time;
static struct RADIOBUTTONGROUP *weathergroup;

static int weather;
static int hour;

static
void timeweather_synctime()
{
	timeweather_set_time(hour);
}

static
void cb_weathergroup_changed(struct UI_RADIOBUTTON *btn)
{
	timeweather_set_weather(weather = (int) btn->_parent._parent.userdata);
}

void timeweather_set_time(int time)
{
	struct MSG_NC nc;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SetWorldTime;
	nc.params.asint[1] = time;
	sockets_send((char*) &nc, sizeof(nc));
}

void timeweather_set_weather(int weatherid)
{
	struct MSG_NC nc;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SetWeather;
	nc.params.asint[1] = weatherid;
	sockets_send((char*) &nc, sizeof(nc));
}

void timeweather_resync()
{
	struct UI_RADIOBUTTON *btn;

	timeweather_synctime();
	if ((btn = weathergroup->activebutton) != NULL) {
		timeweather_set_weather((int) btn->_parent._parent.userdata);
	}
}

static
void cb_btn_main_menu(struct UI_BUTTON *btn)
{
	ui_show_window(wnd);
}

static
void cb_time_input_changed(struct UI_INPUT *in)
{
	hour = atoi(in->value);
	if (hour < 0 || 23 < hour) {
		hour = 0;
	}
	timeweather_synctime();
}

void timeweather_init()
{
	struct UI_RADIOBUTTON *btn;

	/*main menu entry*/
	btn_main_menu = ui_btn_make("Time/Weather", cb_btn_main_menu);
	btn_main_menu->enabled = 0;
	btn_main_menu->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn_main_menu);

	/*window*/
	wnd = ui_wnd_make(500.0f, 200.0f, "Time/Weather");
	wnd->columns = 2;

	ui_wnd_add_child(wnd, ui_lbl_make("Time:"));
	ui_wnd_add_child(wnd, in_time = ui_in_make(cb_time_input_changed));

	ui_wnd_add_child(wnd, ui_lbl_make("Weather:"));
	weathergroup = ui_rdbgroup_make(cb_weathergroup_changed);
#define WEATHERBUTTON(NAME,ID) \
	btn = ui_rdb_make(NAME, weathergroup, 0);\
	btn->_parent._parent.userdata = (void*) ID;\
	btn->_parent.alignment = LEFT;\
	ui_wnd_add_child(wnd, btn);

	WEATHERBUTTON("(0)_LS_Extra_Sunny", 0);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(1)_LS_Sunny", 1);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(2)_LS_Extra_Sunny_Smog", 2);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(3)_LS_Sunny_Smog", 3);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(4)_LS_Cloudy", 4);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(5)_SF_Sunny", 5);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(6)_SF_Extra_Sunny", 6);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(7)_SF_Cloudy", 7);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(8)_SF_Rainy", 8);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(9)_SF_Foggy", 9);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(10)_LV_Sunny", 10);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(11)_LV_Extra_Sunny", 11);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(12)_LV_Cloudy", 12);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(13)_CS_Extra_Sunny", 13);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(14)_CS_Sunny", 14);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(15)_CS_Cloudy", 15);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(16)_CS_Rainy", 16);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(17)_DE_Extra_Sunny", 17);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(18)_DE_Sunny", 18);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(19)_DE_Sandstorm", 19);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(20)_Underwater", 20);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(21)_Extracolours_1", 21);
	ui_wnd_add_child(wnd, NULL);
	WEATHERBUTTON("(22)_Extracolours_2", 22);
#undef WEATHERBUTTON
}

void timeweather_dispose()
{
	ui_wnd_dispose(wnd);
}

void timeweather_prj_save(FILE *f, char *buf)
{
	_asm push f
	_asm push 1
	_asm push 0
	_asm push buf
	sprintf(buf, "hour %d\n", hour);
	_asm mov [esp+0x4], eax
	_asm call fwrite
	sprintf(buf, "weather %d\n", weather);
	_asm mov [esp+0x4], eax
	_asm call fwrite
	_asm add esp, 0x10
}

int timeweather_prj_load_line(char *buf)
{
	if (strncmp("hour ", buf, 5) == 0) {
		hour = atoi(buf + 5);
		return 1;
	} else if (strncmp("weather ", buf, 8) == 0) {
		weather = atoi(buf + 8);
		return 1;
	}
	return 0;
}

void timeweather_prj_preload()
{
	hour = 12;
	weather = 0;
}

void timeweather_prj_postload()
{
	char timetextvalue[11];

	if (weather > MAX_WEATHER) {
		weather = 0;
	}
	sprintf(timetextvalue, "%d", hour);
	ui_in_set_text(in_time, timetextvalue);
	/*following statement will sync weather (due to rdbgroup listeners)*/
	ui_rdb_click_match_userdata(weathergroup, (void*) weather);
	timeweather_synctime();
	btn_main_menu->enabled = 1;
}
