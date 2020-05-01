/* vim: set filetype=c ts=8 noexpandtab: */

struct ENTITYCOLORINFO {
	void *entity;
	int lit_flag_mask[2]; /*one for entity + one for LOD*/
};

void entity_render_exclusively(void *entity);
/**
NOP when given entity is already the colored entity in given info
*/
void entity_color(struct ENTITYCOLORINFO *info, struct CEntity* e, int color);
/**
col is ARGB
*/
void entity_draw_bound_rect(struct CEntity *entity, int col);
void entity_init();

extern struct CEntity *exclusiveEntity;
extern struct CEntity *lastCarSpawned;
