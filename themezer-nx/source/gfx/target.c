#include "gfx.h"

ShapeLinker_t *CreateSideTargetMenu(){
    ShapeLinker_t *out = CreateSideBaseMenu("Type Selection");

    ShapeLinker_t *list = NULL;
    for (int i = 0; i < 9; i++)
        ShapeLinkAdd(&list, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, targetOptions[i], NULL), ListItemType);

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 50, 400, SCREEN_H - 100), 60, COLOR_MAINBG, COLOR_CURSOR, COLOR_CURSORPRESS, COLOR_SCROLLBAR, COLOR_SCROLLBARTHUMB, LIST_CENTERLEFT, list, exitFunc, NULL, FONT_TEXT[FSize30]), ListViewType);

    ShapeLinkAdd(&out, ButtonCreate(POS(0, SCREEN_H - 50, 400, 50), COLOR_MAINBG, COLOR_RED, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Exit Themezer-NX", FONT_TEXT[FSize25], exitFunc), ButtonType);

    return out;
}

int ShowSideTargetMenu(Context_t *ctx){
    ShapeLinker_t *menu = CreateSideTargetMenu();
    Context_t menuCtx = MakeMenu(menu, ButtonHandlerBExit, NULL);

    if (menuCtx.selected->type == ListViewType && menuCtx.origin == OriginFunction){
        ListView_t *lv = menuCtx.selected->item;
        int selection = lv->highlight;
        RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
        if (rI->target != selection){
            int tempTarget = rI->target;
            int tempPage = rI->page;
            SetDefaultsRequestInfo(rI);
            rI->target = selection;
            if (selection == 8){
                rI->maxDls = 3;
                rI->limit = 8;
            }
            printf("Making request...\n");
            if (MakeRequestAsCtx(ctx, rI)){
                rI->target = tempTarget;
                rI->page = tempPage;
            }
        }
    }

    ShapeLinkDispose(&menu);
    return (menuCtx.curOffset == 8 && menuCtx.origin == OriginFunction) ? -1 : 0; 
}