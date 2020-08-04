/* vim: set filetype=c ts=8 noexpandtab: */

#define MAX_GANG_ZONES 1024

struct GANG_ZONE {
	float minx;
	float maxy;
	float maxx;
	float miny;
	int color; // 0xAABBGGRR
	int altcolor; // 0xAABBGGRR
};

extern struct GANG_ZONE gangzone_data[MAX_GANG_ZONES];
extern int *gangzone_enable;
extern int numgangzones;

void gangzone_init();
void gangzone_dispose();
void gangzone_frame_update();
void gangzone_on_active_window_changed(struct UI_WINDOW *_wnd);
int gangzone_on_background_element_just_clicked();
