#include "gfx.h"

ShapeLinker_t *CreateSideTargetMenu(RequestInfo_t *rI){
    ShapeLinker_t *out = CreateSideBaseMenu("Type");

    ShapeLinker_t *list = NULL;
    for (int i = 0; i < TARGET_OPTION_COUNT; i++) {
        if (rI->target == i) {
            SetActiveColorTexture(targetIcons[i]);
        } else {
            SetInactiveColorTexture(targetIcons[i]);
        }
        ShapeLinkAdd(&list, ListItemCreate((rI->target == i) ? COLOR_FILTERACTIVE : COLOR_WHITE, COLOR_WHITE, targetIcons[i], targetOptions[i], NULL), ListItemType);
    }

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 50, 400, SCREEN_H - 100), 60, COLOR_MAINBG, COLOR_CURSOR, COLOR_CURSORPRESS, COLOR_SCROLLBAR, COLOR_SCROLLBARTHUMB, LIST_CENTERLEFT, list, exitFunc, NULL, FONT_TEXT[FSize30]), ListViewType);

    ShapeLinkAdd(&out, ButtonCreate(POS(0, SCREEN_H - 50, 400, 50), COLOR_MAINBG, COLOR_RED, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Exit Themezer-NX", FONT_TEXT[FSize25], exitFunc), ButtonType);
    ShapeLinkAdd(&out, GlyphCreate(376, SCREEN_H - 48, BUTTON_PLUS, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);

    return out;
}

int ShowSideTargetMenu(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    ShapeLinker_t *menu = CreateSideTargetMenu(rI);
    Context_t menuCtx = MakeMenu(menu, ButtonHandlerBXExit, NULL);

    if (menuCtx.selected->type == ListViewType && menuCtx.origin == OriginFunction){
        ListView_t *lv = menuCtx.selected->item;
        int selection = lv->highlight;
        if (rI->target != selection){
            int tempTarget = rI->target;
            int tempPage = rI->page;
            int tempSort = rI->sort;
            int tempOrder = rI->order;
            char *tempSearch = CopyTextUtil(rI->search ? rI->search : "");
            RequestContentType_t tempContentType = rI->contentType;
            SetDefaultsRequestInfo(rI);
            rI->target = selection;
            rI->sort = tempSort;
            rI->order = tempOrder;
            NNFREE(rI->search);
            rI->search = tempSearch;
            rI->contentType = (selection == 0) ? RequestContentPacks :
                (selection == SPLASH_TARGET_INDEX) ? RequestContentSplashes : RequestContentThemes;
            printf("Making request...\n");
            if (MakeRequestAsCtx(ctx, rI)){
                rI->target = tempTarget;
                rI->page = tempPage;
                rI->contentType = tempContentType;
            }
        }
    }

    ShapeLinkDispose(&menu);
    return (menuCtx.curOffset == 8 && menuCtx.origin == OriginFunction) ? -1 : 0;
}
