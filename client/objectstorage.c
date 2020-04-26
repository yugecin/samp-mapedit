/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "objectstorage.h"
#include "project.h"
#include "ui.h"
#include <string.h>
#include <stdio.h>

#pragma pack(push,1)
union MAPDATA {
	char buf[200];
	char c;
	short s;
	int i;
	struct {
		int model;
		struct RwV3D pos, rot;
		float drawdistance;
	} object;
	struct {
		int model;
		struct RwV3D pos;
		float radius;
	} remove;
};
#pragma pack(pop)

/**
<projectname>_layer_<layername>.map follows basdon's .map file format
<projectname>_layer_<layername>.met contains metadata for removes:
  every remove has an entry, in the same order as in the .map file:
    haslod BYTE
    descriptionlen BYTE
    description (variable)
*/

enum fopen_mode {
	MODE_READ,
	MODE_READ_META,
	MODE_WRITE,
};

static
FILE *layer_fopen(struct OBJECTLAYER *layer, char *ext, enum fopen_mode mode)
{
	FILE *file;
	char fname[100], *prj;

	prj = prj_get_name();
	sprintf(fname, "samp-mapedit\\%s_layer_%s.%s", prj, layer->name, ext);
	file = fopen(fname, (mode == MODE_WRITE) ? "wb" : "rb");

	if (!file && mode != MODE_READ_META) {
		sprintf(debugstring, "failed to read/write file %s", fname);
		ui_push_debug_string();
	}

	return file;
}

#define write(X) fwrite(data.buf,X,1,f)
void objectstorage_save_layer(struct OBJECTLAYER *layer)
{
	struct OBJECT *object;
	struct REMOVEDBUILDING *remove;
	FILE *f, *meta;
	union MAPDATA data;
	int i;
	int totalremoves;

	if (!(f = layer_fopen(layer, "map", MODE_WRITE)) ||
		!(meta = layer_fopen(layer, "met", MODE_WRITE)))
	{
		return;
	}

	totalremoves = 0;
	for (i = 0; i < layer->numremoves; i++) {
		if (layer->removes[i].lodmodel) {
			totalremoves += 2;
		} else {
			totalremoves += 1;
		}
	}

	data.i = 1; /*spec version*/
	write(sizeof(int));
	data.i = totalremoves;
	write(sizeof(int));
	data.i = layer->numobjects;
	write(sizeof(int));
	for (i = 0; i < layer->numremoves; i++) {
		remove = layer->removes + i;
		data.remove.model = -remove->model;
		data.remove.pos = remove->origin;
		data.remove.radius = (float) sqrt(remove->radiussq);
		write(sizeof(data.remove));
		if (remove->lodmodel != 0) {
			data.remove.model = -remove->lodmodel;
			write(sizeof(data.remove));
		}

		data.c = remove->lodmodel != 0;
		fwrite(data.buf, sizeof(char), 1, meta);
		if (remove->description == NULL) {
			data.i = 0;
			fwrite(data.buf, sizeof(char), 1, meta);
		} else {
			data.i = strlen(remove->description);
			fwrite(data.buf, sizeof(char), 1, meta);
			fwrite(remove->description, data.i, 1, meta);
		}
	}
	for (i = 0; i < layer->numobjects; i++) {
		object = layer->objects + i;
		data.object.model = object->model;
		game_ObjectGetPos(object->sa_object, &data.object.pos);
		game_ObjectGetRot(object->sa_object, &data.object.rot);
		data.object.drawdistance = 500.0f; /*TODO drawdistance*/
		write(sizeof(data.object));
	}

	fclose(f);
	if (meta) {
		fclose(meta);
	}
}
#undef write

#define read(X) fread(data.buf,X,1,f)
void objectstorage_load_layer(struct OBJECTLAYER *layer)
{
	struct OBJECT *object;
	struct REMOVEDBUILDING *remove;
	FILE *f, *meta;
	union MAPDATA data;
	int entriestoread;
	float rad;

	if (!(f = layer_fopen(layer, "map", MODE_READ))) {
		return;
	}
	meta = layer_fopen(layer, "met", MODE_READ_META);

	read(sizeof(int));
	if (data.i != 1) { /*spec version*/
		sprintf(debugstring, "wrong map file version %d", data.i);
		ui_push_debug_string();
		return;
	}
	read(sizeof(int));
	layer->numremoves = data.i;
	read(sizeof(int));
	layer->numobjects = data.i;
	remove = layer->removes;
	object = layer->objects;
	entriestoread = layer->numremoves + layer->numobjects;
	while (entriestoread-- > 0) {
		read(sizeof(int));
		fseek(f, -(int) sizeof(int), SEEK_CUR);
		if (data.i < 0) { /*model*/
			read(sizeof(data.remove));
			rad = data.remove.radius;
			remove->model = -data.remove.model;
			remove->origin = data.remove.pos;
			remove->radiussq = rad * rad;
			if (!meta) {
				remove->description = NULL;
				remove++;
				continue;
			}

			fread(data.buf, sizeof(char), 1, meta);
			if (data.c) { /*lod model*/
				read(sizeof(data.remove));
				remove->lodmodel = -data.remove.model;
				layer->numremoves--;
				entriestoread--;
			}
			fread(data.buf, sizeof(char), 1, meta);
			if (data.c) {
				remove->description = malloc(INPUT_TEXTLEN + 1);
				fread(remove->description, data.c, 1, meta);
				remove->description[data.c] = 0;
			} else {
				remove->description = NULL;
			}
			remove++;
		} else {
			read(sizeof(data.object));
			object->model = data.object.model;
			objbase_mkobject(object, &data.object.pos);
			object->rot = malloc(sizeof(struct RwV3D));
			*object->rot = data.object.rot;
			object++;
		}
	}

	fclose(f);
	if (meta) {
		fclose(meta);
	}
}
#undef read
