/* vim: set filetype=c ts=8 noexpandtab: */

void objectstorage_save_layer(struct OBJECTLAYER *layer);
void objectstorage_load_layer(struct OBJECTLAYER *layer);
void objectstorage_mark_layerfile_for_deletion(struct OBJECTLAYER *layer);
void objectstorage_delete_layerfiles_marked_for_deletion();
