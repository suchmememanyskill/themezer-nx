#include "gfx.h"

ShapeLinker_t *CreatePackDetailsMenu(ShapeLinker_t *items, RequestInfo_t *rI){
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,200), 1), RectangleType);
    ShapeLinkAdd(&out, ListGridCreate(POS(160, 100, SCREEN_W - 320, SCREEN_H - 150), 3, 260, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, (items) ? GRID_NOSIDEESC : LIST_DISABLED, items, ThemeSelect, NULL, FONT_TEXT[FSize23]), ListGridType);
    ShapeLinkAdd(&out, ButtonCreate(POS(160, 50, SCREEN_W - 320, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleTopStrip, "Back", FONT_TEXT[FSize25], exitFunc), ButtonType);
    ShapeLinkAdd(&out, rI, DataType);

    return out;
}

int ShowPackDetails(Context_t *ctx){
    ListGrid_t *gv = ShapeLinkFind(ctx->all, ListGridType)->item;
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    RequestInfo_t customRI = {3, 0, 0, 0, 0, 0, NULL, 0, 0, rI->packs[gv->highlight].themeCount, NULL, rI->packs[gv->highlight].themes, {NULL, 0, NULL, 1}, NULL};

    printf("Showing pack details...\nCount: %d\nEntry: %d\n", rI->packs[gv->highlight].themeCount, gv->highlight);

    ShapeLinker_t *items = GenListItemList(&customRI);
    if (!rI->packs[gv->highlight].isDlDone)
        AddThemeImagesToDownloadQueue(&customRI, true);

    ShapeLinker_t *menu = CreatePackDetailsMenu(items, &customRI);
    MakeMenu(menu, ButtonHandlerBExit, HandleDownloadQueue);

    rI->packs[gv->highlight].isDlDone = customRI.tInfo.finished; 

    if (!rI->packs[gv->highlight].isDlDone)
        CleanupTransferInfo(&customRI);

    ShapeLinkDispose(&menu);

    return 0;
}