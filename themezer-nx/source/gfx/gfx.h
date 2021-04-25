#pragma once
#include <JAGL.h>
#include "colors.h"
#include <switch.h>
#include "../model.h"
#include "../utils.h"
#include "../curl.h"

extern SDL_Texture *menuIcon, *searchIcon, *setIcon, *arrowLIcon, *arrowRIcon, *LeImg, *XIcon, *loadingScreen, *logo;
extern SDL_Texture *targetIcons[];
extern SDL_Texture *sortIcons[];
extern SDL_Texture *orderIcons[];

// textures.c
void InitTextures();
void DestroyTextures();
void SetActiveColorTexture(SDL_Texture *texture);
void SetInactiveColorTexture(SDL_Texture *texture);

// menuutils.c
int MakeRequestAsCtx(Context_t *ctx, RequestInfo_t *rI);
ShapeLinker_t *CreateBaseMessagePopup(char *title, char *message);
ShapeLinker_t *CreateSideBaseMenu(char *menuName);
int ButtonHandlerBExit(Context_t *ctx);
int exitFunc(Context_t *ctx);
int GetInstallButtonState();
void SetInstallButtonState(int state);
int ShowCurlError(Context_t *ctx);

// packdetails.c
int ShowPackDetails(Context_t *ctx);

// target.c
int ShowSideTargetMenu(Context_t *ctx);

// filter.c
int ShowSideFilterMenu(Context_t *ctx);

// queue.c
int ShowSideQueueMenu(Context_t *ctx);

// details.c
int ThemeSelect(Context_t *ctx);

// mainmenu.c
int ButtonHandlerMainMenu(Context_t *ctx);
ShapeLinker_t *CreateMainMenu(ShapeLinker_t *listItems, RequestInfo_t *rI);