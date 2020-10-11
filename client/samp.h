/* vim: set filetype=c ts=8 noexpandtab: */

#pragma pack(push,1)
struct RPCParameters {
	void *data;
	int dataBitLength;
};

struct PRCDATA_SetPlayerObjectMaterialText {
	short objectid;
	char materialType; /*1 material 2 text*/
	char materialindex;
	short modelid;
	/*char txdnamelen;
	char txdname[];
	char texturenamelen;
	char texturename[];
	int materialcolor*/
};
EXPECT_SIZE(struct PRCDATA_SetPlayerObjectMaterialText, 2 + 1 + 1 + 2);
#pragma pack(pop)

void samp_init();
void samp_dispose();
void samp_break_chat_bar();
void samp_restore_chat_bar();
void samp_hide_ui_f7();
void samp_restore_ui_f7();
void samp_SetPlayerObjectMaterial(struct RPCParameters *rpc_parameters);
void samp_patchObjectMaterialReadText(char *text);
void samp_unpatchObjectMaterialReadText();

extern int samp_handle;
