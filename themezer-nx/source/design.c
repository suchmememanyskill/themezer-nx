#include "design.h"
#include <JAGL.h>
#include "utils.h"
#include <unistd.h>
#include <switch.h>
#include <stdio.h>
#include <sys/stat.h>
#include "curl.h"
#include "libs/cJSON.h"
#include <sys/stat.h> 

int exitFunc(Context_t *ctx){
    return -1;
}

int ButtonHandlerBExit(Context_t *ctx){
    if (ctx->kHeld & KEY_B)
        return -1;

    return 0;
}
SDL_Texture *menuIcon, *searchIcon, *setIcon, *sortIcon, *arrowLIcon, *arrowRIcon, *LeImg, *XIcon, *loadingScreen; 

int lennify(Context_t *ctx){
    static int lenny = false;
    if (!lenny){
        ShapeLinkAdd(&ctx->all, ImageCreate(LeImg, POS(555, 0, 170, 60), 0), ImageType);
        lenny = true;
    }
    return 0;
}

int EnlargePreviewImage(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ThemeInfo_t *target = ShapeLinkFind(all, DataType)->item;

    ShapeLinker_t *menu = NULL;
    ShapeLinkAdd(&menu, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&menu, ImageCreate(target->preview, POS(0, 0, SCREEN_W, SCREEN_H), 0), ImageType);

    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

int DownloadThemeButton(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ThemeInfo_t *target = ShapeLinkFind(all, DataType)->item;

    ShapeLinker_t *render = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&render, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&render, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,200), 1), RectangleType);
    ShapeLinkAdd(&render, TextCenteredCreate(POS(0, 0, SCREEN_W, SCREEN_H), "Downloading Theme...", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);

    RenderShapeLinkList(render);
    ShapeLinkDispose(&render);

    mkdir("/Themes/", 0777);

    char *creatorPath = CopyTextArgsUtil("/Themes/%s", target->creator);
    mkdir(creatorPath, 0777);

    char *path = CopyTextArgsUtil("%s/%s.nxtheme", creatorPath, target->name);

    DownloadThemeFromID(target->id, path);

    free(path);
    free(creatorPath);

    return 0;
}

ShapeLinker_t *CreateSelectMenu(ThemeInfo_t *target){
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,170), 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(50, 100, SCREEN_W - 100, SCREEN_H - 150), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(50, 50, SCREEN_W - 150, 50), COLOR_TOPBAR, 1), RectangleType);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(55, 50, 0 /* 0 width left alligns it */, 50), target->name, COLOR_WHITE, FONT_TEXT[FSize33]), TextCenteredType);

    ShapeLinkAdd(&out, ButtonCreate(POS(SCREEN_W - 100, 50, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(SCREEN_W - 100, 50, 50, 50), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(50, 100, 860, 488), COLOR_CENTERLISTBG, COLOR_WHITE, COLOR_WHITE, COLOR_CENTERLISTSELECTION, 0, ButtonStyleFlat, NULL, NULL, EnlargePreviewImage), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(target->preview, POS(55, 105, 850, 478), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(915, 110, SCREEN_W - 980, 60), COLOR_INSTBTN, COLOR_GREEN, COLOR_WHITE, COLOR_INSTBTNSEL, 0, ButtonStyleFlat, "Install Theme", FONT_TEXT[FSize33], NULL), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(915, 180, SCREEN_W - 980, 60), COLOR_DLBTN, COLOR_GREEN, COLOR_WHITE, COLOR_DLBTNSEL, 0, ButtonStyleFlat, "Download Theme", FONT_TEXT[FSize33], DownloadThemeButton), ButtonType);

    char *info = CopyTextArgsUtil("Creator: %s\n\nLast Updated: %s\n\nDownload Count: %d\nLike Count: %d\nID: t%s", target->creator, target->lastUpdated, target->dlCount, target->likeCount, target->id);
    char *desc = CopyTextArgsUtil("Description: %s", target->description);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 250, SCREEN_W - 990, 420), info, COLOR_WHITE, FONT_TEXT[FSize25]), TextBoxType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(60, 593, SCREEN_W - 120, 80), desc, COLOR_WHITE, FONT_TEXT[FSize25]), TextBoxType);

    free(info);
    free(desc);
    //ShapeLinkAdd()

    ShapeLinkAdd(&out, target, DataType);

    return out;
}

int ThemeSelect(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ListGrid_t *gv = ShapeLinkFind(all, ListGridType)->item;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;
    ThemeInfo_t *target = &rI->themes[gv->highlight];

    if (target->preview == NULL)
        return 0;

    ShapeLinker_t *menu = CreateSelectMenu(target);
    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

int MakeRequestAsCtx(Context_t *ctx, RequestInfo_t *rI){
    ShapeLinker_t *items = NULL;
    int res = -1;

    CleanupTransferInfo(rI);

    printf("Making JSON request...\n");
    if (!MakeJsonRequest(GenLink(rI), &rI->response)){
        printf("JSON request got! parsing...\n");
        if (!(res = GenThemeArray(rI))){
            printf("JSON data parsed!\n");
            items = GenListItemList(rI);
            AddThemeImagesToDownloadQueue(rI);

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

    printf("Res: %d\n", res);
    return res;
}

int NextPageButton(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;

    if (rI->page >= rI->pageCount){
        return 0;
    }

    rI->page++;

    MakeRequestAsCtx(ctx,rI);
    return 0;
}

int PrevPageButton(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;

    if (rI->page <= 1){
        return 0;
    }

    rI->page--;

    MakeRequestAsCtx(ctx,rI);
    return 0;
}

ShapeLinker_t *CreateSideTargetMenu(){
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,170), 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, 350, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 0, 350, 50), "Select a target:", COLOR_WHITE, FONT_TEXT[FSize25]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(350, 0, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(350, 0, 50, 50), 0), ImageType);

    ShapeLinker_t *list = NULL;
    ShapeLinkAdd(&list, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Home Menu", NULL), ListItemType);
    ShapeLinkAdd(&list, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Lock Screen", NULL), ListItemType);
    ShapeLinkAdd(&list, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "All Apps", NULL), ListItemType);
    ShapeLinkAdd(&list, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Player Select", NULL), ListItemType);

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 50, 400, SCREEN_H - 50), 75, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, LIST_CENTERLEFT, list, exitFunc, NULL, FONT_TEXT[FSize30]), ListViewType);

    return out;
}

int SideTargetMenu(Context_t *ctx){
    ShapeLinker_t *menu = CreateSideTargetMenu();
    Context_t menuCtx = MakeMenu(menu, ButtonHandlerBExit, NULL);

    if (menuCtx.selected->type == ListViewType && menuCtx.origin == OriginFunction){
        ListView_t *lv = menuCtx.selected->item;
        int selection = lv->highlight;
        RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
        if (rI->target != selection){
            SetDefaultsRequestInfo(rI);
            rI->target = selection;
            printf("Making request...\n");
            MakeRequestAsCtx(ctx, rI);
        }
    }

    ShapeLinkDispose(&menu);
    return 0;
} 

ShapeLinker_t *CreateMainMenu(ShapeLinker_t *listItems, RequestInfo_t *rI) { 
    ShapeLinker_t *out = NULL;

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, 60), COLOR_TOPBAR, 1), RectangleType);

    // Text inbetween arrows
    char *temp = CopyTextArgsUtil("Page %d/%d", rI->page, rI->pageCount);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 0, 240, 60), temp, COLOR_WHITE, FONT_TEXT[FSize25]), TextCenteredType);
    free(temp);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), "X.X There seem to be no themes here!", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);


    // MenuButton
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN1, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, SideTargetMenu), ButtonType);
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
    ShapeLinkAdd(&out, ButtonCreate(POS(800, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN5, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, PrevPageButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowLIcon, POS(830, 0, 60, 60), 0), ImageType);

    // Easter egg
    ShapeLinkAdd(&out, ButtonCreate(POS(480, 0, 320, 60), COLOR_TOPBARBUTTONS, COLOR_TOPBARBUTTONS, COLOR_WHITE, COLOR_TOPBARBUTTONS, BUTTON_NOJOYSEL, ButtonStyleFlat, NULL, NULL, lennify), ButtonType);

    // RightArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(1160, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN5, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NextPageButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowRIcon, POS(1190, 0, 60, 60), 0), ImageType);

    ShapeLinkAdd(&out, ListGridCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), 4, 260, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, (listItems) ? 0 : LIST_DISABLED, listItems, ThemeSelect, NULL, FONT_TEXT[FSize25]), ListGridType);
    // 4, 260
        
    ShapeLinkAdd(&out, rI, DataType);

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
    XIcon = LoadImageSDL("romfs:/x.png");
    loadingScreen = LoadImageSDL("romfs:/loading.png");
}

void ExitDesign(){
    SDL_DestroyTexture(menuIcon);
    SDL_DestroyTexture(searchIcon);
    SDL_DestroyTexture(setIcon);
    SDL_DestroyTexture(arrowLIcon);
    SDL_DestroyTexture(arrowRIcon);
    SDL_DestroyTexture(sortIcon);
    SDL_DestroyTexture(LeImg);
    SDL_DestroyTexture(XIcon);
    SDL_DestroyTexture(loadingScreen);
}