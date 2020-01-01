/* vim: set filetype=c ts=8 noexpandtab: */

/*
- thanks to the ones that helped make the idb that I have (I can't remember
  where I found it)
- thanks to Seemann, DjDeji69 and others involved with
  SannyBuilder, CLEO & gtagmodding opcode database
- thanks to plugin-sdk maintainers https://github.com/DK22Pac/plugin-sdk/
*/

#define GAME_RESOLUTION_X *(int*)(0xC17044)
#define GAME_RESOLUTION_Y *(int*)(0xC17048)
#define CPAD 0xB73458

#define TEXT_ALIGN_CENTER 0
#define TEXT_ALIGN_LEFT 1
#define TEXT_ALIGN_RIGHT 2

#pragma pack(push,1)
struct Rect {
	float left;
	float bottom;
	float right;
	float top;
};
EXPECT_SIZE(struct Rect, 0x10);

struct CMouseState {
	char lmb;
	char rmb;
	char mmb;
	char wheelUp;
	char wheelDown;
	char xmb1;
	char xmb2;
	char __pad0;
	float z;
	float x;
	float y;
};
EXPECT_SIZE(struct CMouseState, 0x14);
#pragma pack(pop)

extern unsigned int *fontColorABGR;
extern unsigned char *enableHudByOpcode;
extern struct CMouseState *activeMouseState;
extern struct CMouseState *currentMouseState;
extern struct CMouseState *prevMouseState;

void game_FreezePlayer(char flag);
/**
sdk CFont::GetStringWidth
*/
float game_TextGetSizeX(char *text, char unk1, char unk2);
/**
sdk CFont::GetTextRect
*/
int game_TextGetSizeXY(struct Rect *size, float sizex, float sizey, char *text);
/**
sdk CFont::PrintChar
*/
int game_TextPrintChar(float x, float y, char character);
/**
idb _drawText
sdk CFont::PrintString
*/
int game_TextPrintString(float x, float y, char *text);
/**
sdk CFont::PrintStringFromBottom
*/
int game_TextPrintStringFromBottom(float x, float y, char *text);
/**
smp TextDrawAlignment
idb CText__SetTextAlignment
sdk CFont::SetOrientation
scm ? 03E4: set_text_draw_align_right 0
*/
int game_TextSetAlign(char align);
/**
idb dummy_719500
sdk CFont::SetAlphaFade
*/
int game_TextSetAlpha(float alpha);
/**
smp TextDrawUseBox
idb CText__SetTextBackground
sdk CFont::SetBackground
scm 0345: enable_text_draw_background 0
*/
int game_TextSetBackground(char flag, char includewrap);
/**
smp TextDrawBoxColor
idb dummy_7195E0
sdk CFont::SetBackgroundColor
*/
int game_TextSetBackgroundColor(int abgr);
/**
smp TextDrawColor
idb CText__SetTextColour
sdk CFont::SetColor
scm 0340: set_text_draw_RGBA 180 180 180 255
*/
int game_TextSetColor(int abgr);
/**
smp TextDrawFont
idb CText__SetFont
sdk CFont::SetFontStyle
scm 0349: set_text_draw_font 3
*/
int game_TextSetFont(char font);
/**
smp TextDrawFont
sdk CFont::SetJustify
scm 0341: set_text_draw_align_justify 0
*/
int game_TextSetJustify(char flag);
/**
smp TextDrawLetterSize
idb CText__SetTextLetterSize
sdk CFont::SetScale
scm 033F: set_text_draw_letter_size 1.3 3.36
*/
int game_TextSetLetterSize(float x, float y);
/**
See game_TextSetProportional
*/
int game_TextSetMonospace(char monospace);
/**
smp TextDrawSetOutline
idb CText__SetTextOutline
sdk CFont::SetEdge
scm 081C: draw_text_outline 2 RGBA 0 0 0 255
*/
int game_TextSetOutline(char outlinesize);
/**
Inverse monospace

smp TextDrawSetProportional
idb CText__SetTextUseProportionalValues
sdk CFont::SetProportional
scm 0348: enable_text_draw_proportional 1
*/
int game_TextSetProportional(char proportional);
/**
smp TextDrawSetShadow
idb CText__SetTextShadow
sdk CFont::SetDropShadowPosition
scm 060D: draw_text_shadow 0 rgba 0 0 0 255
*/
int game_TextSetShadow(char shadowsize);
/**
smp TextDrawBackgroundColor
idb CText__SetBorderEffectRGBA
sdk CFont::SetDropColor
scm 060D: draw_text_shadow 0 rgba 0 0 0 255
*/
int game_TextSetShadowColor(int abgr);
/**
idk dummy_7194D0
sdk CFont::SetWrapx
*/
int game_TextSetWrapX(float value);
/**
?
*/
int game_TextSetWrapY(float value);
