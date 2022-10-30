/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "gangzone.h"
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
		int objectdata_size;
		int numzones;
		float streamin;
		float streamout;
		float drawdistance;
	} header;
	struct {
		short objectdata_size;
		int model;
		struct RwV3D pos;
		struct RwV3D rot;
		float drawdistance;
		char no_camera_col;
		short attached_object_id;
		short attached_vehicle_id;
		char num_materials;
	} object;
	struct {
		int model;
		struct RwV3D pos;
		float radius;
	} remove;
	struct {
		float west, south, east, north;
		int color;
	} gangzone;
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
	int i, j;
	char haslod;
	char descriptionlen;
	int dogangzones;

	dogangzones = layer == layers;

	if (!(f = layer_fopen(layer, "map", MODE_WRITE))) {
		return;
	}
	if (layer->numremoves) {
		meta = layer_fopen(layer, "met", MODE_WRITE);
	} else {
		meta = NULL;
	}

	data.header.version = 0x0350414D;
	data.header.numremoves = 0;
	for (i = 0; i < layer->numremoves; i++) {
		if (layer->removes[i].lodmodel) {
			data.header.numremoves += 2;
		} else {
			data.header.numremoves += 1;
		}
	}
	data.header.numobjects = layer->numobjects;
	data.header.objectdata_size = sizeof(data.object) * layer->numobjects;
	for (i = 0; i < layer->numobjects; i++) {
		object = layer->objects + i;
		for (j = 0; j < object->num_materials; j++) {
			if (object->material_type[j] == 1) {
				data.header.objectdata_size += 1 + 1 + 2 + 1 + 1 + 4;
				data.header.objectdata_size += object->material_texture[j].txdname_len;
				data.header.objectdata_size += object->material_texture[j].texture_len;
			}
		}
	}
	data.header.numzones = dogangzones ? numgangzones : 0;
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

	if (dogangzones) {
		for (i = 0; i < numgangzones; i++) {
			data.gangzone.west = gangzone_data[i].minx;
			data.gangzone.south = gangzone_data[i].miny;
			data.gangzone.east = gangzone_data[i].maxx;
			data.gangzone.north = gangzone_data[i].maxy;
			data.gangzone.color = gangzone_data[i].color;
			fwrite(&data.gangzone, sizeof(data.gangzone), 1, f);
		}
	}

	data.object.drawdistance = data.header.drawdistance;
	data.object.no_camera_col = 0;
	data.object.attached_object_id = -1;
	data.object.attached_vehicle_id = -1;
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
		data.object.num_materials = object->num_materials;
		data.object.objectdata_size = sizeof(data.object);
		for (j = 0; j < object->num_materials; j++) {
			if (object->material_type[j] == 1) {
				data.object.objectdata_size += 1 + 1 + 2 + 1 + 1 + 4;
				data.object.objectdata_size += object->material_texture[j].txdname_len;
				data.object.objectdata_size += object->material_texture[j].texture_len;
			}
		}
		fwrite(&data.object, sizeof(data.object), 1, f);
		for (j = 0; j < object->num_materials; j++) {
			fwrite(&object->material_type[j], 1, 1, f);
			fwrite(&object->material_index[j], 1, 1, f);
			fwrite(&object->material_texture[j].model, 2, 1, f);
			fwrite(&object->material_texture[j].txdname_len, 1, 1, f);
			fwrite(&object->material_texture[j].txdname,
				object->material_texture[j].txdname_len, 1, f);
			fwrite(&object->material_texture[j].texture_len, 1, 1, f);
			fwrite(&object->material_texture[j].texture,
				object->material_texture[j].texture_len, 1, f);
			fwrite(&object->material_texture[j].color, 4, 1, f);
		}
	}

	fclose(f);
	if (meta) {
		fclose(meta);
	}
}

static
void objectstorage_do_hardcoded_gangzone_creation()
{
#if 0
	int i;
	// code to generate more gangzones:
	float minx = gangzone_data[153].minx;
	float maxx = gangzone_data[153].maxx;
	float miny = gangzone_data[153].miny;
	float maxy = gangzone_data[153].maxy;
	int color = gangzone_data[153].color;
	float dx = (6962.0f - 7098.0f) / 18.0f;
	float dy = (3818.0f - 4055.0f) / 18.0f;
	for (i = 0; i < 18; ) {
		i++;
		gangzone_data[numgangzones].minx = minx + dx * i;
		gangzone_data[numgangzones].maxx = maxx + dx * i;
		gangzone_data[numgangzones].miny = miny + dy * i;
		gangzone_data[numgangzones].maxy = maxy + dy * i;
		gangzone_data[numgangzones].color = color;
		gangzone_data[numgangzones].altcolor = color;
		gangzone_enable[numgangzones] = 1;
		numgangzones++;
	}
#endif
}

void objectstorage_load_layer(struct OBJECTLAYER *layer)
{
	struct OBJECT *object;
	struct REMOVEDBUILDING *remove;
	FILE *f, *meta;
	union MAPDATA data;
	int i, j;
	char descriptionlen;
	char haslod;
	int dogangzones;
	int actual_num_gangzones_in_file;

	dogangzones = layer == layers;

	if (!(f = layer_fopen(layer, "map", MODE_READ))) {
		return;
	}
	meta = layer_fopen(layer, "met", MODE_READ_META);

	if (!(fread(&data.header, sizeof(data.header), 1, f))) {
		goto corrupted;
	}

	if (data.header.version != 0x0350414D) {
		sprintf(debugstring, "wrong map file version %p", data.header.version);
		ui_push_debug_string();
		goto ret;
	}
	layer->numremoves = data.header.numremoves;
	layer->numobjects = data.header.numobjects;
	actual_num_gangzones_in_file = data.header.numzones;
	if (dogangzones) {
		numgangzones = data.header.numzones;
	}
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

	if (dogangzones) {
		memset(gangzone_enable, 0, sizeof(int) * MAX_GANG_ZONES);
	} else if (actual_num_gangzones_in_file) {
		sprintf(debugstring, "non-first layer has gangzones! will be wiped on save");
		ui_push_debug_string();
	}
	for (i = 0; i < actual_num_gangzones_in_file; i++) {
		if (!(fread(&data.gangzone, sizeof(data.gangzone), 1, f))) {
			sprintf(debugstring, "missing %d gangzones", numgangzones - i - 1);
			ui_push_debug_string();
			goto corrupted;
		}
		if (dogangzones) {
			gangzone_data[i].minx = data.gangzone.west;
			gangzone_data[i].miny = data.gangzone.south;
			gangzone_data[i].maxx = data.gangzone.east;
			gangzone_data[i].maxy = data.gangzone.north;
			gangzone_data[i].color = gangzone_data[i].altcolor = data.gangzone.color;
			gangzone_enable[i] = 1;
		}
	}
	if (dogangzones) {
		objectstorage_do_hardcoded_gangzone_creation();
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
		object->num_materials = data.object.num_materials;
		for (j = 0; j < object->num_materials; j++) {
			if (!fread(&object->material_type[j], 1, 1, f)) {
				sprintf(debugstring, "failed to read object %d material %d (type)", i, j);
				ui_push_debug_string();
				goto corrupted;
			}
			if (object->material_type[j] != 1) {
				sprintf(debugstring, "unsupported object %d material %d type %d", i, j, object->material_type[j]);
				ui_push_debug_string();
				goto corrupted;
			}
			if (!fread(&object->material_index[j], 1, 1, f)) {
				sprintf(debugstring, "failed to read object %d material %d (idx)", i, j);
				ui_push_debug_string();
				goto corrupted;
			}
			if (!fread(&object->material_texture[j].model, 2, 1, f)) {
				sprintf(debugstring, "failed to read object %d material %d (model)", i, j);
				ui_push_debug_string();
				goto corrupted;
			}
			if (!fread(&object->material_texture[j].txdname_len, 1, 1, f)) {
				sprintf(debugstring, "failed to read object %d material %d (txdname len)", i, j);
				ui_push_debug_string();
				goto corrupted;
			}
			if (!fread(object->material_texture[j].txdname,
				object->material_texture[j].txdname_len, 1, f))
			{
				sprintf(debugstring, "failed to read object %d material %d (txdname)", i, j);
				ui_push_debug_string();
				goto corrupted;
			}
			object->material_texture[j].txdname[object->material_texture[j].txdname_len] = 0;
			if (!fread(&object->material_texture[j].texture_len, 1, 1, f)) {
				sprintf(debugstring, "failed to read object %d material %d (texture_len)", i, j);
				ui_push_debug_string();
				goto corrupted;
			}
			if (!fread(object->material_texture[j].texture,
				object->material_texture[j].texture_len, 1, f))
			{
				sprintf(debugstring, "failed to read object %d material %d (texture)", i, j);
				ui_push_debug_string();
				goto corrupted;
			}
			object->material_texture[j].texture[object->material_texture[j].texture_len] = 0;
			if (!fread(&object->material_texture[j].color, 4, 1, f)) {
				sprintf(debugstring, "failed to read object %d material %d (color)", i, j);
				ui_push_debug_string();
				goto corrupted;
			}
		}
		if (layer->show) {
			objects_mkobject(object);
		} else {
			objects_mkobject_dontcreate(object);
		}
		object++;
	}

	if (fread(&data, 1, 1, f)) {
		sprintf(debugstring, "extra data at the end of the file");
		ui_push_debug_string();
		layer->numobjects = 0; /*for safety*/
		goto corrupted;
	}

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
