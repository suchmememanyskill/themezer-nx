#pragma once
#include <JAGL.h>
#include "colors.h"
#include <switch.h>
#include "../model.h"
#include "../utils.h"
#include "../curl.h"

extern SDL_Texture *menuIcon, *searchIcon, *queueIcon, *arrowLIcon, *arrowRIcon, *LeImg, *XIcon, *logo, *icon, *banner, *bgTile, *themeBgThumbHash, *packBgThumbHash, *moodDown, *browseIcon, *quickIdIcon;
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
void UpdateMainMenuUI(Context_t *ctx, RequestInfo_t *rI, ShapeLinker_t *items);
void ShowLoadingPageUI(Context_t *ctx, RequestInfo_t *rI);
void SetMainMenuEmptyMessage(ShapeLinker_t *all, char *emptyMessage);
void SetMainMenuNoContentState(ShapeLinker_t *all, bool visible);
ShapeLinker_t *CreateBaseMessagePopup(char *title, char *message);
ShapeLinker_t *CreateDownloadProgressMenu(const char *message, DownloadProgressContext_t *progress);
ShapeLinker_t *CreateSideBaseMenu(char *menuName);
int ButtonHandlerBExit(Context_t *ctx);
int ButtonHandlerBXExit(Context_t *ctx);
int ButtonHandlerBYExit(Context_t *ctx);
int ButtonHandlerBMinusExit(Context_t *ctx);
int exitFunc(Context_t *ctx);
int GetInstallButtonState();
void SetInstallButtonState(int state);
SDL_Color GetMainMenuAccentColor(RequestInfo_t *rI);
int ShowCurlError(Context_t *ctx);
int ShowConnErrMenu(int res);

// packdetails.c
ShapeLinker_t *CreatePackDetailsMenu(ShapeLinker_t *items, RequestInfo_t *rI);
int ShowPackDetails(Context_t *ctx);

// target.c
int ShowSideTargetMenu(Context_t *ctx);

// filter.c
int ShowSideFilterMenu(Context_t *ctx);

// queue.c
int ShowSideQueueMenu(Context_t *ctx);

// details.c
int ThemeSelect(Context_t *ctx);
int ShowQuickIdLookup(Context_t *ctx);

// mainmenu.c
int ButtonHandlerMainMenu(Context_t *ctx);
int ButtonHandlerBootMenu(Context_t *ctx);
ShapeLinker_t *CreateBootMenu(void);
ShapeLinker_t *CreateMainMenu(ShapeLinker_t *listItems, RequestInfo_t *rI);
bool RunMainMenu(RequestInfo_t *rI);
void CleanupMainMenuLoad(void);
void ResetBootMenuAction(void);
int ConsumeBootMenuAction(void);
bool ConsumeBootMenuBrowseRequested(void);
void ResetMainMenuReturnToBoot(void);
bool ConsumeMainMenuReturnToBoot(void);
