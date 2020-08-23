#include "gfx.h"

ShapeLinker_t *CreatePackDetailsMenu(ShapeLinker_t *items, RequestInfo_t *rI){
    ShapeLinker_t *out = NULL;

    ShapeLinkAdd(&out, ListGridCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), 4, 260, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, (items) ? GRID_NOSIDEESC : LIST_DISABLED, items, ThemeSelect, NULL, FONT_TEXT[FSize23]), ListGridType);
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, 60), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleTopStrip, "Back", FONT_TEXT[FSize30], exitFunc), ButtonType);
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