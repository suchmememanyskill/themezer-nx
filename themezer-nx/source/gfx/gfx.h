#pragma once
#include <JAGL.h>
#include "colors.h"
#include <switch.h>
#include "../model.h"
#include "../utils.h"
#include "../curl.h"

extern SDL_Texture *menuIcon, *searchIcon, *setIcon, *arrowLIcon, *arrowRIcon, *LeImg, *XIcon, *loadingScreen, *logo;

// textures.c
void InitTextures();
void DestroyTextures();

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