/* vim: set filetype=c ts=8 noexpandtab: */

#define VEHICLE_MODEL_TOTAL (611-400+1)

struct CARCOLDATA {
	char amount;
	short position;
};

extern struct CARCOLDATA carcoldata[VEHICLE_MODEL_TOTAL];
extern char carcols[1912];
extern int actualcarcols[128];
