#include "gfx.h"

enum {
    BOOT_MENU_ACTION_NONE = 0,
    BOOT_MENU_ACTION_BROWSE,
};

static int bootMenuAction = BOOT_MENU_ACTION_NONE;

static int OpenBrowseMenu(Context_t *ctx){
    (void)ctx;
    bootMenuAction = BOOT_MENU_ACTION_BROWSE;
    return -1;
}

static int OpenQuickIdFromBoot(Context_t *ctx){
    return ShowQuickIdLookup(ctx);
}

static void AddTiledBackground(ShapeLinker_t **out){
    ShapeLinkAdd(out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_MAINBG, 1), RectangleType);

    if (bgTile){
        SizeInfo_t tileSize = GetTextureSize(bgTile);
        int tileW = (SCREEN_W + 2) / 3;
        int tileH = (tileSize.w > 0) ? (tileW * tileSize.h / tileSize.w) : tileW;

        if (tileH <= 0)
            tileH = tileW;

        for (int y = 0; y < SCREEN_H; y += tileH){
            for (int x = 0; x < SCREEN_W; x += tileW){
                ShapeLinkAdd(out, ImageCreate(bgTile, POS(x, y, tileW, tileH), 0), ImageType);
            }
        }
    }
}

void ResetBootMenuAction(void){
    bootMenuAction = BOOT_MENU_ACTION_NONE;
}

int ConsumeBootMenuAction(void){
    int action = bootMenuAction;
    bootMenuAction = BOOT_MENU_ACTION_NONE;
    return action;
}

bool ConsumeBootMenuBrowseRequested(void){
    return ConsumeBootMenuAction() == BOOT_MENU_ACTION_BROWSE;
}

int ButtonHandlerBootMenu(Context_t *ctx){
    if (ctx->kDown & HidNpadButton_Plus)
        return -1;

    return 0;
}

ShapeLinker_t *CreateBootMenu(void){
    ShapeLinker_t *out = NULL;
    SDL_Color browseAccent = COLOR_MAIN_TOPBAR_THEME;
    SDL_Color quickIdAccent = COLOR_MAIN_TOPBAR_PACK;
    const SDL_Color buttonBase = COLOR(36, 36, 38, 230);
    const SDL_Rect browseCard = POS(166, 270, 420, 270);
    const SDL_Rect quickIdCard = POS(694, 270, 420, 270);

    AddTiledBackground(&out);

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0, 0, 0, 110), 1), RectangleType);
    ShapeLinkAdd(&out, ImageCreate(logo, POS((SCREEN_W - 267) / 2, 78, 267, 80), 0), ImageType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 194, SCREEN_W, 46), "What do you want to do?", COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);

    ShapeLinkAdd(&out, ButtonCreate(browseCard, buttonBase, browseAccent, COLOR_WHITE, browseAccent, 0, ButtonStyleFlat, NULL, NULL, OpenBrowseMenu), ButtonType);
    ShapeLinkAdd(&out, RectangleCreate(POS(browseCard.x + 24, browseCard.y + 24, browseCard.w - 48, browseCard.h - 48), COLOR(255, 255, 255, 26), 1), RectangleType);
    ShapeLinkAdd(&out, ImageCreate(browseIcon, POS(browseCard.x + (browseCard.w - 80) / 2, browseCard.y + 58, 80, 80), 0), ImageType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(browseCard.x, browseCard.y + 158, browseCard.w, 52), "Browse", COLOR_WHITE, FONT_TEXT[FSize35]), TextCenteredType);

    ShapeLinkAdd(&out, ButtonCreate(quickIdCard, buttonBase, quickIdAccent, COLOR_WHITE, quickIdAccent, 0, ButtonStyleFlat, NULL, NULL, OpenQuickIdFromBoot), ButtonType);
    ShapeLinkAdd(&out, RectangleCreate(POS(quickIdCard.x + 24, quickIdCard.y + 24, quickIdCard.w - 48, quickIdCard.h - 48), COLOR(255, 255, 255, 26), 1), RectangleType);
    ShapeLinkAdd(&out, ImageCreate(quickIdIcon, POS(quickIdCard.x + (quickIdCard.w - 80) / 2, quickIdCard.y + 58, 80, 80), 0), ImageType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(quickIdCard.x + 24, quickIdCard.y + 158, quickIdCard.w - 48, 52), "Install by Quick ID", COLOR_WHITE, FONT_TEXT[FSize35]), TextCenteredType);

    ShapeLinkAdd(&out, GlyphCreate(596, SCREEN_H - 44, BUTTON_PLUS, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(620, SCREEN_H - 50, 70, 30), "Exit", COLOR_WHITE, FONT_TEXT[FSize20]), TextCenteredType);

    return out;
}
