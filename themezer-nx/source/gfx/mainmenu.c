#include "gfx.h"

int lennify(Context_t *ctx){
    static int lenny = false;
    if (!lenny){
        ShapeLinkAdd(&ctx->all, ImageCreate(LeImg, POS(386, 0, 170, 60), 0), ImageType);
        lenny = true;
    }
    return 0;
}

int NextPageButton(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;

    if (rI->page >= rI->pageCount){
        return 0;
    }

    rI->page++;

    if (MakeRequestAsCtx(ctx,rI))
        rI->page--;

    return 0;
}

int PrevPageButton(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;

    if (rI->page <= 1){
        return 0;
    }

    rI->page--;

    if (MakeRequestAsCtx(ctx,rI))
        rI->page++;

    return 0;
}

int ButtonHandlerMainMenu(Context_t *ctx){
    if (ctx->kHeld & KEY_R)
        return NextPageButton(ctx);
    if (ctx->kHeld & KEY_L)
        return PrevPageButton(ctx);
    if (ctx->kHeld & KEY_Y)
        return ShowSideFilterMenu(ctx);
    if (ctx->kHeld & KEY_X)
        return ShowSideTargetMenu(ctx);
    if (ctx->kHeld & KEY_MINUS)
        return ShowSideQueueMenu(ctx);

    return 0;
}

ShapeLinker_t *CreateMainMenu(ShapeLinker_t *listItems, RequestInfo_t *rI) { 
    ShapeLinker_t *out = NULL;

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), COLOR_MAINBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, 60), COLOR_TOPBAR, 1), RectangleType);

    // Text inbetween arrows
    char *temp = CopyTextArgsUtil("%d/%d (%d)", rI->page, rI->pageCount, rI->itemCount);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 0, 240, 60), temp, COLOR_WHITE, FONT_TEXT[FSize25]), TextCenteredType);
    free(temp);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), "X.X There seem to be no themes here!", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);

    // MenuButton
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN1, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideTargetMenu), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(menuIcon, POS(30, 0, 60, 60), 0), ImageType);

    // SearchButton
    ShapeLinkAdd(&out, ButtonCreate(POS(120, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN2, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideFilterMenu), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(searchIcon, POS(150, 0, 60, 60), 0), ImageType);

    if (GetInstallButtonState()){
        // SettingsButton
        ShapeLinkAdd(&out, ButtonCreate(POS(240, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN3, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideQueueMenu), ButtonType);
        ShapeLinkAdd(&out, ImageCreate(setIcon, POS(270, 0, 60, 60), 0), ImageType);
    }

    // SortButton
    //ShapeLinkAdd(&out, ButtonCreate(POS(360, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN4, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideFilterMenu), ButtonType);
    //ShapeLinkAdd(&out, ImageCreate(sortIcon, POS(390, 0, 60, 60), 0), ImageType);

    // LeftArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(800, 0, 120, 60), COLOR_TOPBARBUTTONS, rI->page == 1 ? COLOR_TOPBARBUTTONS : COLOR_BTNPAGINATION, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, PrevPageButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowLIcon, POS(830, 0, 60, 60), 0), ImageType);

    // Easter egg
    ShapeLinkAdd(&out, ButtonCreate(POS(386, 0, 170, 60), COLOR_TOPBARBUTTONS, COLOR_TOPBARBUTTONS, COLOR_WHITE, COLOR_TOPBARBUTTONS, BUTTON_NOJOYSEL, ButtonStyleFlat, NULL, NULL, lennify), ButtonType);

    // RightArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(1160, 0, 120, 60), COLOR_TOPBARBUTTONS, rI->page == rI->pageCount ? COLOR_TOPBARBUTTONS : COLOR_BTNPAGINATION, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, NextPageButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowRIcon, POS(1190, 0, 60, 60), 0), ImageType);

    ShapeLinkAdd(&out, ListGridCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), 4, 260, COLOR_MAINBG, COLOR_CARDCURSOR, COLOR_CARDCURSORPRESS, COLOR_SCROLLBAR, COLOR_SCROLLBARTHUMB, (listItems) ? GRID_NOSIDEESC : LIST_DISABLED, listItems, ThemeSelect, NULL, FONT_TEXT[FSize23]), ListGridType);
    // 4, 260

    ShapeLinkAdd(&out, rI, DataType);

    // Logo
    ShapeLinkAdd(&out, ImageCreate(logo, POS(584, 0, 60, 60), 0), ImageType);

    // Glyphs
    ShapeLinkAdd(&out, GlyphCreate(97, 2, BUTTON_X, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    ShapeLinkAdd(&out, GlyphCreate(217, 2, BUTTON_Y, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    ShapeLinkAdd(&out, GlyphCreate(337, 2, BUTTON_MINUS, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    ShapeLinkAdd(&out, GlyphCreate(804, 2, BUTTON_L, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    ShapeLinkAdd(&out, GlyphCreate(1256, 2, BUTTON_R, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);

    return out;
}