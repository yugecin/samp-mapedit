/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ide.h"
#include "ui.h"
#include "objecttextures.h"
#include <string.h>
#include <stdio.h>

char *modelNames[MAX_MODELS];
static char names[MAX_MODELS * 20];
static char *nameptr;
static char *files[] = {
	"data/maps/country/countn2.ide",
	"data/maps/country/countrye.ide",
	"data/maps/country/countryN.ide",
	"data/maps/country/countryS.ide",
	"data/maps/country/countryW.ide",
	"data/maps/country/counxref.ide",
	"data/maps/generic/barriers.ide",
	"data/maps/generic/dynamic.ide",
	"data/maps/generic/dynamic2.ide",
	"data/maps/generic/multiobj.ide",
	"data/maps/generic/procobj.ide",
	"data/maps/generic/vegepart.ide",
	"data/maps/interior/gen_int1.ide",
	"data/maps/interior/gen_int2.ide",
	"data/maps/interior/gen_int3.ide",
	"data/maps/interior/gen_int4.ide",
	"data/maps/interior/gen_int5.ide",
	"data/maps/interior/gen_intb.ide",
	"data/maps/interior/int_cont.ide",
	"data/maps/interior/int_LA.ide",
	"data/maps/interior/int_SF.ide",
	"data/maps/interior/int_veg.ide",
	"data/maps/interior/propext.ide",
	"data/maps/interior/props.ide",
	"data/maps/interior/props2.ide",
	"data/maps/interior/savehous.ide",
	"data/maps/interior/stadint.ide",
	"data/maps/LA/LAe.ide",
	"data/maps/LA/LAe2.ide",
	"data/maps/LA/LAhills.ide",
	"data/maps/LA/LAn.ide",
	"data/maps/LA/LAn2.ide",
	"data/maps/LA/LAs.ide",
	"data/maps/LA/LAs2.ide",
	"data/maps/LA/LAw.ide",
	"data/maps/LA/LAw2.ide",
	"data/maps/LA/LaWn.ide",
	"data/maps/LA/LAxref.ide",
	"data/maps/leveldes/leveldes.ide",
	"data/maps/leveldes/levelmap.ide",
	"data/maps/leveldes/levelxre.ide",
	"data/maps/leveldes/seabed.ide",
	"data/maps/SF/SFe.ide",
	"data/maps/SF/SFn.ide",
	"data/maps/SF/SFs.ide",
	"data/maps/SF/SFSe.ide",
	"data/maps/SF/SFw.ide",
	"data/maps/SF/SFxref.ide",
	"data/maps/txd.ide",
	"data/maps/vegas/vegasE.ide",
	"data/maps/vegas/VegasN.ide",
	"data/maps/vegas/VegasS.ide",
	"data/maps/vegas/VegasW.ide",
	"data/maps/vegas/vegaxref.ide",
	"data/maps/veh_mods/veh_mods.ide",
	"SAMP/SAMP.ide",
	"SAMP/CUSTOM.ide",
	"",
};

static
int ide_load_file(char *filename)
{
	FILE *f;
	char buf[512];
	char inobj;
	int pos;
	int modelid;
	int amount;
	char *txdptr;

	amount = 0;
	if (f = fopen(filename, "r")) {
		inobj = 0;
nextline:
		if (fgets(buf, sizeof(buf), f) == NULL) {
			goto done;
		}
		if (buf[0] == '#' || buf[0] == 0 || buf[0] == ' ') {
			;
		} else if (buf[0] == 'o' && buf[1] == 'b' && buf[2] == 'j') {
			inobj = 1;
		} else if (buf[0] == 't' && buf[1] == 'o' &&
			buf[2] == 'b' && buf[3] == 'j')
		{
			inobj = 1;
		} else if (buf[0] == 'a' && buf[1] == 'n' &&
			buf[2] == 'i' && buf[3] == 'm')
		{
			inobj = 1;
		} else if (buf[0] == 'e' && buf[1] == 'n' && buf[2] == 'd') {
			inobj = 0;
		} else if (inobj) {
			amount++;
			modelid = atoi(buf);
			if (modelid < 0 || MAX_MODELS <= modelid) {
				sprintf(debugstring,
					"model id %d in file %s oob",
					modelid,
					filename);
				ui_push_debug_string();
				goto nextline;
			}
			if (nameptr - names > sizeof(names) - 30) {
				sprintf(debugstring, "ide name pool depleted");
				ui_push_debug_string();
				goto nextline;
			}
			pos = 0;
			while (buf[pos++] != ',');
			while (buf[pos] == ' ') pos++;
			modelNames[modelid] = nameptr;
			sprintf(nameptr, "%05d:_", modelid);
			nameptr += 7;
nextchr:
			*nameptr = buf[pos];
			if (*nameptr == ',' || *nameptr == ' ') {
				*nameptr = 0;
				nameptr++;
			} else {
				nameptr++;
				pos++;
				goto nextchr;
			}
			pos++;
			if (buf[pos] == ' ') {
				pos++;
			}
			txdptr = buf + pos;
			while (buf[pos] && buf[pos] != ',') {
				pos++;
			}
			buf[pos] = 0;
			objecttextures_associate_model_txd(modelid, txdptr);
		}
		goto nextline;
done:
		fclose(f);
	} else {
		sprintf(debugstring, "~r~failed to read %s", filename);
		ui_push_debug_string();
	}
	return amount;
}

void ide_load()
{
	int i, num;

	memset(modelNames, 0, sizeof(modelNames));
	nameptr = names;
	i = num = 0;
	while (files[i][0]) {
		num += ide_load_file(files[i++]);
	}
	/*sprintf(debugstring,
		"loaded %d modelnames, %dB/%dB used",
		num,
		nameptr - names,
		sizeof(names));
	ui_push_debug_string();*/
}
