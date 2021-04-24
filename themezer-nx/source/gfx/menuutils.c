#include "gfx.h"
#include "colors.h"
#include <switch.h>
#include "../model.h"
#include "../utils.h"
#include "../curl.h"

int InstallButtonState = 0;

void SetInstallButtonState(int state){
    InstallButtonState = state;
}

int GetInstallButtonState(){
    return InstallButtonState;
}

int exitFunc(Context_t *ctx){
    return -1;
}

int ButtonHandlerBExit(Context_t *ctx){
    if (ctx->kHeld & KEY_B)
        return -1;

    return 0;
}

ShapeLinker_t *CreateSideBaseMenu(char *menuName){ // Count: 7
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 50, 400, SCREEN_H - 50), COLOR_MAINBG, 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, 350, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 0, 400, 50), menuName, COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(350, 0, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARCURSOR, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(350, 0, 50, 50), 0), ImageType);
    ShapeLinkAdd(&out, ButtonCreate(POS(400, 0, SCREEN_W - 400, SCREEN_H), COLOR(0,0,0,170), COLOR(0,0,0,170), COLOR(0,0,0,170), COLOR(0,0,0,170), BUTTON_NOJOYSEL, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);

    return out;
}

ShapeLinker_t *CreateBaseMessagePopup(char *title, char *message){ // Other code needs to add buttons at the bottom at Y 470 H 50, Count: 6
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0, 0, 0, 170), 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(250, 200, SCREEN_W - 500, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(260, 200, 0, 50), title, COLOR_WHITE, FONT_TEXT[FSize25]), TextCenteredType);
    ShapeLinkAdd(&out, RectangleCreate(POS(250, 250, SCREEN_W - 500, 220), COLOR_MAINBG, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(260, 260, SCREEN_W - 520, 200), message, COLOR_WHITE, FONT_TEXT[FSize28]), TextBoxType);

    return out;
}

int ShowCurlError(Context_t *ctx){
    ShapeLinker_t *menu = NULL;

    ShapeLinkAdd(&menu, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_MAINBG, COLOR_MAINBG, COLOR_WHITE, COLOR_MAINBG, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&menu, RectangleCreate(POS(0, 0, SCREEN_W, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&menu, TextCenteredCreate(POS(0, 0, SCREEN_W, 50), "Back", COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);

    ShapeLinkAdd(&menu, TextCenteredCreate(POS(10, 60, SCREEN_W - 20, SCREEN_H - 70), cURLErrBuff, COLOR_WHITE, FONT_TEXT[FSize25]), TextBoxType);

    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

int ShowConnErrMenu(int res){
    char *message = CopyTextArgsUtil("Something went wrong when connecting to the themezer server! Error Code: %d", res);
    ShapeLinker_t *menu = CreateBaseMessagePopup("Connection Error!", message);
    free(message);

    bool showDetails = (cURLErrBuff[0] != '\0');

    ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, (showDetails) ? 390 : 780, 50), COLOR_MAINBG, COLOR_RED, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Alright", FONT_TEXT[FSize28], exitFunc), ButtonType);
    if (showDetails)
        ShapeLinkAdd(&menu, ButtonCreate(POS(640, 470, 390, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Show Details", FONT_TEXT[FSize28], ShowCurlError), ButtonType);

    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

int MakeRequestAsCtx(Context_t *ctx, RequestInfo_t *rI){
    ShapeLinker_t *items = NULL;
    int res = -1;

    CleanupTransferInfo(rI);

    printf("Making JSON request...\n");
    if (!(res = MakeJsonRequest(GenLink(rI), &rI->response))){
        printf("JSON request got! parsing...\n");
        if (!(res = GenThemeArray(rI))){
            printf("JSON data parsed!\n");
            items = GenListItemList(rI);
            AddThemeImagesToDownloadQueue(rI, true);

            ShapeLinker_t *all = ctx->all;
            ListGrid_t *gv = ShapeLinkFind(all, ListGridType)->item;
            TextCentered_t *pageText = ShapeLinkFind(all, TextCenteredType)->item;
            if (gv->text)
                ShapeLinkDispose(&gv->text);
            
            gv->text = items;
            SETBIT(gv->options, LIST_DISABLED, !items);
            gv->highlight = 0;
            free(pageText->text.text);
            pageText->text.text = CopyTextArgsUtil("Page %d/%d", rI->page, rI->pageCount);
            
        }
    }
    else {
        ShowConnErrMenu(res);
    }

    printf("Res: %d\n", res);
    return res;
}