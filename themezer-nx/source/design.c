#include "design.h"
#include <JAGL.h>
#include "utils.h"
#include <unistd.h>
#include <switch.h>
#include <stdio.h>
#include <sys/stat.h>
#include "curl.h"
#include "libs/cJSON.h"

int exitFunc(Context_t *ctx){
    return -1;
}

int ButtonHandlerBExit(Context_t *ctx){
    if (ctx->kHeld & KEY_B)
        return -1;

    return 0;
}
SDL_Texture *menuIcon, *searchIcon, *setIcon, *sortIcon, *arrowLIcon, *arrowRIcon, *LeImg; 

int lennify(Context_t *ctx){
    static int lenny = false;
    if (!lenny){
        ShapeLinkAdd(&ctx->all, ImageCreate(LeImg, POS(555, 0, 170, 60), 0), ImageType);
        lenny = true;
    }
    return 0;
}

ShapeLinker_t *CreateMainMenu(ShapeLinker_t *listItems) { 
    ShapeLinker_t *out = NULL;

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, 60), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), "X.X There seem to be no themes here!", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);


    // MenuButton
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN1, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(menuIcon, POS(30, 0, 60, 60), 0), ImageType);

    // SearchButton
    ShapeLinkAdd(&out, ButtonCreate(POS(120, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN2, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(searchIcon, POS(150, 0, 60, 60), 0), ImageType);

    // SettingsButton
    ShapeLinkAdd(&out, ButtonCreate(POS(240, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN3, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(setIcon, POS(270, 0, 60, 60), 0), ImageType);

    // SortButton
    ShapeLinkAdd(&out, ButtonCreate(POS(360, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN4, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(sortIcon, POS(390, 0, 60, 60), 0), ImageType);

    // LeftArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(800, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN5, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowLIcon, POS(830, 0, 60, 60), 0), ImageType);

    // Easter egg
    ShapeLinkAdd(&out, ButtonCreate(POS(480, 0, 320, 60), COLOR_TOPBARBUTTONS, COLOR_TOPBARBUTTONS, COLOR_WHITE, COLOR_TOPBARBUTTONS, BUTTON_NOJOYSEL, ButtonStyleFlat, NULL, NULL, lennify), ButtonType);

    // Text inbetween arrows
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 0, 240, 60), "<Not implemented>", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);

    // RightArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(1160, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN5, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowRIcon, POS(1190, 0, 60, 60), 0), ImageType);

    ShapeLinkAdd(&out, ListGridCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), 4, 260, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, (listItems) ? 0 : LIST_DISABLED, listItems, NULL, NULL, FONT_TEXT[FSize28]), ListGridType);
    // 4, 260
        
    return out;
}

void InitDesign(){
    menuIcon = LoadImageSDL("romfs:/menu.png");
    searchIcon = LoadImageSDL("romfs:/search.png");
    setIcon = LoadImageSDL("romfs:/set.png");
    arrowLIcon = LoadImageSDL("romfs:/arrowL.png");
    arrowRIcon = LoadImageSDL("romfs:/arrowR.png");
    sortIcon = LoadImageSDL("romfs:/sort.png");
    LeImg = LoadImageSDL("romfs:/lenny.png");
}

void ExitDesign(){
    SDL_DestroyTexture(menuIcon);
    SDL_DestroyTexture(searchIcon);
    SDL_DestroyTexture(setIcon);
    SDL_DestroyTexture(arrowLIcon);
    SDL_DestroyTexture(arrowRIcon);
    SDL_DestroyTexture(sortIcon);
    SDL_DestroyTexture(LeImg);
}