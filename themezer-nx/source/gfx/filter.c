#include "gfx.h"

int SideMenuSortSelection(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ListView_t *lv = ShapeLinkOffset(all, 14)->item;
    FilterOptions_t *options = ShapeLinkFind(all, DataType)->item;
    int selection = lv->highlight;

    if (selection != options->sort){
        // Remove active color
        ListItem_t *previousItem = ShapeLinkOffset(lv->text, options->sort)->item;
        previousItem->leftColor = COLOR_WHITE;
        SetInactiveColorTexture(sortIcons[options->sort]);

        options->sort = selection;

        // Set active color
        ListItem_t *newItem = ShapeLinkOffset(lv->text, options->sort)->item;
        newItem->leftColor = COLOR_FILTERACTIVE;
        SetActiveColorTexture(sortIcons[options->sort]);
    }

    return 0;
}

int SideMenuOrderSelection(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ListView_t *lv = ShapeLinkOffset(all, 17)->item;
    FilterOptions_t *options = ShapeLinkFind(all, DataType)->item;
    int selection = lv->highlight;

    if (selection != options->order){
        // Remove active color
        ListItem_t *previousItem = ShapeLinkOffset(lv->text, options->order)->item;
        previousItem->leftColor = COLOR_WHITE;
        SetInactiveColorTexture(orderIcons[options->order]);

        options->order = selection;

        // Set active color
        ListItem_t *newItem = ShapeLinkOffset(lv->text, options->order)->item;
        newItem->leftColor = COLOR_FILTERACTIVE;
        SetActiveColorTexture(orderIcons[options->order]);
    }

    return 0;
}

int SideMenuClearSearch(Context_t *ctx){
    FilterOptions_t *options = ShapeLinkFind(ctx->all, DataType)->item;
    TextCentered_t *text = ShapeLinkOffset(ctx->all, 9)->item;
    if (options->search != NULL && options->search[0]){
        free(options->search);
        options->search = CopyTextUtil("");
        free(text->text.text);
        text->text.text = CopyTextUtil("Search");
    }

    return 0;
}

int SideMenuSetSearch(Context_t *ctx){
    FilterOptions_t *options = ShapeLinkFind(ctx->all, DataType)->item;
    TextCentered_t *text = ShapeLinkOffset(ctx->all, 9)->item;

    char *out = showKeyboard("Input search terms. Max 100 characters", options->search, 100);

    if (out == NULL)
        return 0;
    
    if (!isStringNullOrEmpty(out)){
        if (options->search != NULL)
            free(options->search);

        options->search = SanitizeString(out);
        free(text->text.text);
        text->text.text = CopyTextArgsUtil("Search: %s", options->search);
    }

    free(out);
    return 0;
}

ShapeLinker_t *CreateSideFilterMenu(FilterOptions_t *options){
    ShapeLinker_t *out = CreateSideBaseMenu("Search & Filters");

    ShapeLinkAdd(&out, options, DataType);

    char *search = options->search[0] ? CopyTextArgsUtil("Search: %s", (options->search)) : CopyTextUtil("Search");
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 60, 400, 50), COLOR_BTN2, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 60, 400, 50), search, COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(200, 110, 200, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleFlat, "Clear", FONT_TEXT[FSize28], SideMenuClearSearch), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 110, 200, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleFlat, "Set", FONT_TEXT[FSize28], SideMenuSetSearch), ButtonType);
    free(search);

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 200, 400, 50), COLOR_BTN2, 1), RectangleType);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 200, 400, 50), "Sort By", COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);

    ShapeLinker_t *sortList = NULL;
    for (int i = 0; i < 4; i++) {
        if (i == options->sort) {
            SetActiveColorTexture(sortIcons[i]);
        } else {
            SetInactiveColorTexture(sortIcons[i]);
        }
        ShapeLinkAdd(&sortList, ListItemCreate((i == options->sort) ? COLOR_FILTERACTIVE : COLOR_WHITE, COLOR_WHITE, sortIcons[i], sortOptions[i], NULL), ListItemType);
    }

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 250, 400, 200), 50, COLOR_MAINBG, COLOR_CURSOR, COLOR_CURSORPRESS, COLOR_SCROLLBAR, COLOR_SCROLLBARTHUMB, LIST_CENTERLEFT, sortList, SideMenuSortSelection, NULL, FONT_TEXT[FSize28]), ListViewType);


    ShapeLinkAdd(&out, RectangleCreate(POS(0, 490, 400, 50), COLOR_BTN2, 1), RectangleType);

    char *order = CopyTextUtil("Order");
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 490, 400, 50), order, COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);
    free(order);

    ShapeLinker_t *orderList = NULL;
    for (int i = 0; i < 2; i++) {
        if (i == options->order) {
            SetActiveColorTexture(orderIcons[i]);
        } else {
            SetInactiveColorTexture(orderIcons[i]);
        }
        ShapeLinkAdd(&orderList, ListItemCreate((i == options->order) ? COLOR_FILTERACTIVE : COLOR_WHITE, COLOR_WHITE, orderIcons[i], orderOptions[i], NULL), ListItemType);
    }

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 540, 400, 100), 50, COLOR_MAINBG, COLOR_CURSOR, COLOR_CURSORPRESS, COLOR_SCROLLBAR, COLOR_SCROLLBARTHUMB, LIST_CENTERLEFT, orderList, SideMenuOrderSelection, NULL, FONT_TEXT[FSize28]), ListViewType);

    ShapeLinkAdd(&out, ButtonCreate(POS(0, SCREEN_H - 50, 400, 50), COLOR_MAINBG, COLOR_CARDCURSOR, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Apply", FONT_TEXT[FSize28], exitFunc), ButtonType);

    return out;
}

int ShowSideFilterMenu(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    FilterOptions_t options = {rI->sort, rI->order, CopyTextUtil(rI->search)};
    ShapeLinker_t *menu = CreateSideFilterMenu(&options);
    Context_t menuCtx = MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);
    
    if (menuCtx.curOffset == 18 && menuCtx.origin == OriginFunction){
        if (rI->search != NULL)
            free(rI->search);

        rI->search = options.search;
        rI->order = options.order;
        rI->sort = options.sort;
        rI->page = 1;

        if (MakeRequestAsCtx(ctx, rI)){
            if (rI->search != NULL)
                free(rI->search);

            rI->search = CopyTextUtil("");
        }
    }
    else if (options.search != NULL)
        free(options.search);

    return 0;
}