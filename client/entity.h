/* vim: set filetype=c ts=8 noexpandtab: */

#define BBOX_ALPHA_ANIM_VALUE \
	(char) (55 * (1.0f - fabs(sinf(*timeInGame * 0.004f))))

struct ENTITYCOLORINFO {
	void *entity;
	int lit_flag_mask[2]; /*one for entity + one for LOD*/
};

/**
NOP when given entity is already the colored entity in given info
*/
void entity_color(struct ENTITYCOLORINFO *info, struct CEntity* e, int color);
void entity_draw_bound_rect(struct CEntity *entity, int col);
void entity_init();
