/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objects.h"
#include "objectstorage.h"
#include "project.h"
#include "ui.h"
#include <string.h>
#include <stdio.h>

#pragma pack(push,1)
union MAPDATA {
	char buf[100];
	struct {
		int version;
		int numremoves;
		int numobjects;
		int numzones;
		float streamin;
		float streamout;
		float drawdistance;
	} header;
	struct {
		int model;
		struct RwV3D pos;
		struct RwV3D rot;
	} object;
	struct {
		int model;
		struct RwV3D pos;
		float radius;
	} remove;
	struct {
		float west, south, east, north;
		int color;
	} zone;
};
#pragma pack(pop)

struct LAYERTODELETE {
	char name[INPUT_TEXTLEN + 1];
	struct LAYERTODELETE *next;
};

static struct LAYERTODELETE *layer_to_delete;

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
void layer_filename(char *buf, char *layername, char *ext)
{
	char *prjname;

	prjname = prj_get_name();
	sprintf(buf, "samp-mapedit\\%s_layer_%s.%s", prjname, layername, ext);
}

static
FILE *layer_fopen(struct OBJECTLAYER *layer, char *ext, enum fopen_mode mode)
{
	FILE *file;
	char fname[100];

	layer_filename(fname, layer->name, ext);
	file = fopen(fname, (mode == MODE_WRITE) ? "wb" : "rb");

	if (!file && mode != MODE_READ_META) {
		sprintf(debugstring, "failed to read/write file %s", fname);
		ui_push_debug_string();
	}

	return file;
}

void objectstorage_save_layer(struct OBJECTLAYER *layer)
{
	struct OBJECT *object;
	struct REMOVEDBUILDING *remove;
	FILE *f, *meta;
	union MAPDATA data;
	int i;
	char haslod;
	char descriptionlen;

	if (!(f = layer_fopen(layer, "map", MODE_WRITE))) {
		return;
	}
	if (layer->numremoves) {
		meta = layer_fopen(layer, "met", MODE_WRITE);
	} else {
		meta = NULL;
	}

	data.header.version = 0x0250414D;
	data.header.numremoves = 0;
	for (i = 0; i < layer->numremoves; i++) {
		if (layer->removes[i].lodmodel) {
			data.header.numremoves += 2;
		} else {
			data.header.numremoves += 1;
		}
	}
	data.header.numobjects = layer->numobjects;
	data.header.numzones = 0; // TODO
	data.header.streamin = layer->stream_in_radius;
	data.header.streamout = layer->stream_out_radius;
	data.header.drawdistance = layer->drawdistance;
	fwrite(&data.header, sizeof(data.header), 1, f);

	for (i = 0; i < layer->numremoves; i++) {
		remove = layer->removes + i;

		data.remove.model = remove->model;
		data.remove.pos = remove->origin;
		data.remove.radius = (float) sqrt(remove->radiussq);
		fwrite(&data.remove, sizeof(data.remove), 1, f);

		if (remove->lodmodel != 0) {
			data.remove.model = remove->lodmodel;
			fwrite(&data.remove, sizeof(data.remove), 1, f);
		}

		haslod = remove->model != -1 && remove->lodmodel != 0;
		fwrite(&haslod, sizeof(char), 1, meta);

		if (remove->description == NULL) {
			descriptionlen = 0;
		} else {
			descriptionlen = strlen(remove->description);
			fwrite(remove->description, descriptionlen, 1, meta);
		}
		fwrite(&descriptionlen, sizeof(char), 1, meta);
	}

	for (i = 0; i < layer->numobjects; i++) {
		object = layer->objects + i;
		data.object.model = object->model;
		if (object->sa_object) {
			game_ObjectGetPos(object->sa_object, &data.object.pos);
			game_ObjectGetRot(object->sa_object, &data.object.rot);
		} else {
			data.object.pos = object->pos;
			data.object.rot = object->rot;
		}
		fwrite(&data.object, sizeof(data.object), 1, f);
	}

	fclose(f);
	if (meta) {
		fclose(meta);
	}
}

void objectstorage_load_layer(struct OBJECTLAYER *layer)
{
	struct OBJECT *object;
	struct REMOVEDBUILDING *remove;
	FILE *f, *meta;
	union MAPDATA data;
	int i;
	char descriptionlen;
	char haslod;

	if (!(f = layer_fopen(layer, "map", MODE_READ))) {
		return;
	}
	meta = layer_fopen(layer, "met", MODE_READ_META);

	if (!(fread(&data.header, sizeof(data.header), 1, f))) {
		goto corrupted;
	}

	if (data.header.version != 0x0250414D) {
		sprintf(debugstring, "wrong map file version %p", data.header.version);
		ui_push_debug_string();
		goto ret;
	}
	layer->numremoves = data.header.numremoves;
	layer->numobjects = data.header.numobjects;
	layer->stream_in_radius = data.header.streamin;
	layer->stream_out_radius = data.header.streamout;
	layer->drawdistance = data.header.drawdistance;

	remove = layer->removes;
	object = layer->objects;

	for (i = 0; i < layer->numremoves; i++) {
		if (!(fread(&data.remove, sizeof(data.remove), 1, f))) {
			goto corrupted;
		}

		remove->model = data.remove.model;
		remove->origin = data.remove.pos;
		remove->radiussq = data.remove.radius * data.remove.radius;
		remove->description = NULL;

		if (meta) {
			fread(&haslod, sizeof(char), 1, meta);
			if (haslod) { /*lod model*/
				if (!(fread(&data.remove, sizeof(data.remove), 1, f))) {
					goto corrupted;
				}
				remove->lodmodel = data.remove.model;
				layer->numremoves--;
			}
			fread(&descriptionlen, sizeof(char), 1, meta);
			if (descriptionlen) {
				remove->description = malloc(INPUT_TEXTLEN + 1);
				fread(remove->description, descriptionlen, 1, meta);
				remove->description[descriptionlen] = 0;
			}
		}
		remove++;
	}

	for (i = 0; i < layer->numobjects; i++) {
		if (!(fread(&data.object, sizeof(data.object), 1, f))) {
			sprintf(debugstring, "missing %d objects", layer->numobjects - i - 1);
			ui_push_debug_string();
			goto corrupted;
		}
		object->model = data.object.model;
		object->pos = data.object.pos;
		object->rot = data.object.rot;
		if (layer->show) {
			objects_mkobject(object);
		} else {
			objects_mkobject_dontcreate(object);
		}
		object++;
	}

	// TODO: zones

ret:
	fclose(f);
	if (meta) {
		fclose(meta);
	}
	return;
corrupted:
	sprintf(debugstring, "layer corrupted: %s", layer->name);
	ui_push_debug_string();
	goto ret;
}

void objectstorage_mark_layerfile_for_deletion(struct OBJECTLAYER *layer)
{
	struct LAYERTODELETE *deletion, *ptr;

	deletion = malloc(sizeof(struct LAYERTODELETE));
	strcpy(deletion->name, layer->name);
	deletion->next = NULL;

	if (layer_to_delete == NULL) {
		layer_to_delete = deletion;
		return;
	}

	ptr = layer_to_delete;
	while (ptr->next != NULL) {
		ptr = ptr->next;
	}
	ptr->next = deletion;
}

void objectstorage_delete_layerfiles_marked_for_deletion()
{
	struct LAYERTODELETE *deletion, *next;
	int layeridx;
	char buf[100];

	deletion = layer_to_delete;
	layer_to_delete = NULL;

	while (deletion != NULL) {
		for (layeridx = 0; layeridx < numlayers; layeridx++) {
			if (!strcmp(layers[layeridx].name, deletion->name)) {
				goto dontremove;
			}
		}
		layer_filename(buf, deletion->name, "map");
		remove(buf);
		layer_filename(buf, deletion->name, "met");
		remove(buf);
dontremove:
		next = deletion->next;
		free(deletion);
		deletion = next;
	}
}
