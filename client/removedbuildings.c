/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "removedbuildings.h"

#define VISIBLE_FLAG 0x00000080
#define COLLISION_FLAG 0x00000001
#define REMOVED_MASK (~(VISIBLE_FLAG | COLLISION_FLAG))

/*people shouldn't go too crazy with removes anyways*/
#define MAX_ALL_REMOVES 1000

/*assume all removed buildings has the visibility flag*/
static struct {
	struct CEntity *entities[MAX_ALL_REMOVES];
	char hadcollision[MAX_ALL_REMOVES];
} removedata;
static int numallremoves;

#include "ui.h"
/**
@return 0 when all remove slots are taken
*/
static
int rb_process_entity(struct CEntity *entity)
{
	struct REMOVEDBUILDING *remove;
	struct RwV3D pos;
	int layer, layerremoves;
	float dx, dy, dz;

	/*TODO: figure out sector coordinates so whole sectors can be culled
	if they're not within reach*/

	game_ObjectGetPos(entity, &pos);
	for (layer = 0; layer < numlayers; layer++) {
		layerremoves = layers[layer].numremoves;
		remove = layers[layer].removes - 1; /*cuz ++ below*/
		while (layerremoves > 0) {
			layerremoves--;
			remove++;
			if (remove->model == -1 ||
				remove->model == entity->model)
			{
				dx = pos.x - remove->origin.x;
				dy = pos.y - remove->origin.y;
				dx = dx * dx + dy * dy;
				if (dx > remove->radiussq) {
					continue;
				}
				dz = pos.z - remove->origin.z;
				if (dx + dz * dz > remove->radiussq) {
					continue;
				}
				removedata.entities[numallremoves] = entity;
				removedata.hadcollision[numallremoves] =
					(entity->flags & 1);
				numallremoves++;
				entity->flags &= REMOVED_MASK;
				if (numallremoves == MAX_ALL_REMOVES) {
					return 0;
				}
				entity = entity->lod;
				if (entity <= 0 ||
					(remove->model != -1 &&
					entity->model != remove->lodmodel))
				{
					/*TODO: if the model itself is not
					being removed, but the LOD is, then
					it will not work because the code
					won't get here.*/
					return 1;
				}
				removedata.entities[numallremoves] = entity;
				removedata.hadcollision[numallremoves] =
					(entity->flags & 1);
				numallremoves++;
				entity->flags &= REMOVED_MASK;
				return MAX_ALL_REMOVES - numallremoves;
			}
		}
	}
	return 1;
}

void rb_do_all()
{
	struct CSector *sector;
	struct CDoubleLinkListNode *node;
	int sectorindex;

	/*TODO: this does not contain dynamic objects such as: hay,
	lampposts, building lights, fences, ..*/

	sector = worldSectors;
	sectorindex = MAX_SECTORS;
	while (--sectorindex >= 0) {
		node = sector->buildings;
		while (node != NULL) {
			if (!rb_process_entity(node->item)) {
				return;
			}
			node = node->next;
		}
		node = sector->dummies;
		while (node != NULL) {
			if (!rb_process_entity(node->item)) {
				return;
			}
			node = node->next;
		}
		sector++;
	}
}

void rb_undo_all()
{
	while (numallremoves) {
		numallremoves--;
		removedata.entities[numallremoves]->flags |=
			(VISIBLE_FLAG | removedata.hadcollision[numallremoves]);
	}
}
