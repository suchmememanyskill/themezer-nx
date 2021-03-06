#include "gfx.h"

int SideMenuSortSelection(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ListView_t *lv = ShapeLinkOffset(all, 14)->item;
    TextCentered_t *text = ShapeLinkOffset(all, 13)->item;
    FilterOptions_t *options = ShapeLinkFind(all, DataType)->item;
    int selection = lv->highlight;

    if (selection != options->sort){
        if (text->text.text != NULL)
            free(text->text.text);

        options->sort = selection;
        text->text.text = CopyTextArgsUtil("Sort selection: %s", sortOptions[options->sort]);
    }

    return 0;
}

int SideMenuOrderSelection(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ListView_t *lv = ShapeLinkOffset(all, 17)->item;
    TextCentered_t *text = ShapeLinkOffset(all, 16)->item;
    FilterOptions_t *options = ShapeLinkFind(all, DataType)->item;
    int selection = lv->highlight;

    if (selection != options->order){
        if (text->text.text != NULL)
            free(text->text.text);

        options->order = selection;
        text->text.text = CopyTextArgsUtil("Order selection: %s", orderOptions[options->order]);
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
        text->text.text = CopyTextUtil("Search: None");
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
    ShapeLinker_t *out = CreateSideBaseMenu("Select filters:");

    ShapeLinkAdd(&out, options, DataType);

    char *search = CopyTextArgsUtil("Search: %s", (options->search[0]) ? options->search : "None");
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 60, 400, 50), COLOR_BTN2, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 60, 400, 50), search, COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(200, 110, 200, 50), COLOR_CENTERLISTBG, COLOR_CENTERLISTPRESS, COLOR_WHITE, COLOR_CENTERLISTSELECTION, 0, ButtonStyleFlat, "Clear Search", FONT_TEXT[FSize28], SideMenuClearSearch), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 110, 200, 50), COLOR_CENTERLISTBG, COLOR_CENTERLISTPRESS, COLOR_WHITE, COLOR_CENTERLISTSELECTION, 0, ButtonStyleFlat, "Set Search", FONT_TEXT[FSize28], SideMenuSetSearch), ButtonType);
    free(search);

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 200, 400, 50), COLOR_BTN2, 1), RectangleType);

    char *sort = CopyTextArgsUtil("Sort selection: %s", sortOptions[options->sort]);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 200, 400, 50), sort, COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);
    free(sort);

    ShapeLinker_t *sortList = NULL;
    for (int i = 0; i < 4; i++)
        ShapeLinkAdd(&sortList, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, sortOptions[i], NULL), ListItemType);

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 250, 400, 200), 50, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, LIST_CENTERLEFT, sortList, SideMenuSortSelection, NULL, FONT_TEXT[FSize28]), ListViewType);


    ShapeLinkAdd(&out, RectangleCreate(POS(0, 490, 400, 50), COLOR_BTN2, 1), RectangleType);

    char *order = CopyTextArgsUtil("Order selection: %s", orderOptions[options->order]);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 490, 400, 50), order, COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);
    free(order);

    ShapeLinker_t *orderList = NULL;
    for (int i = 0; i < 2; i++)
        ShapeLinkAdd(&orderList, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, orderOptions[i], NULL), ListItemType);

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 540, 400, 100), 50, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, LIST_CENTERLEFT, orderList, SideMenuOrderSelection, NULL, FONT_TEXT[FSize28]), ListViewType);

    ShapeLinkAdd(&out, ButtonCreate(POS(0, SCREEN_H - 50, 400, 50), COLOR_CENTERLISTBG, COLOR_BTN2, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, "Apply filters", FONT_TEXT[FSize28], exitFunc), ButtonType);

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