#include "gfx.h"

int EnlargePreviewImage(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    ThemeInfo_t *target = rI->themes;

    int w, h;
    SDL_QueryTexture(target->preview, NULL, NULL, &w, &h);

    if (w < 1000 || h < 700)
        return 0;

    ShapeLinker_t *menu = NULL;
    ShapeLinkAdd(&menu, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&menu, ImageCreate(target->preview, POS(0, 0, SCREEN_W, SCREEN_H), 0), ImageType);

    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

int DownloadThemeButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    ThemeInfo_t *target = rI->themes;

    ShapeLinker_t *render = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&render, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&render, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,200), 1), RectangleType);
    TextCentered_t *text = TextCenteredCreate(POS(0, 0, SCREEN_W, SCREEN_H), "Downloading Theme...", COLOR_WHITE, FONT_TEXT[FSize45]);
    ShapeLinkAdd(&render, text, TextCenteredType);

    RenderShapeLinkList(render);

    char *path = GetThemePath(target->creator, target->name, targetOptions[target->target]);
    int res = DownloadThemeFromID(target->id, path);

    if (res){
        ShapeLinkAdd(&render, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
        free(text->text.text);
        text->text.text = CopyTextUtil("Download failed! Press A to return");
        MakeMenu(render, ButtonHandlerBExit, NULL);
    }

    ShapeLinkDispose(&render);

    free(path);

    return res;
}

int InstallThemeButton(Context_t *ctx){
    ShapeLinker_t *out = CreateBaseMessagePopup("Install Queued!", "Install Queued. Exit the app to apply the theme.\nYou can exit the app by pressing the + button or on the bottom in the target menu selecting the 'Exit Themezer-NX' option.\nPress A to return");

    ShapeLinkAdd(&out, RectangleCreate(POS(250, 470, 780, 50), COLOR_CENTERLISTSELECTION, 1), RectangleType);
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(250, 470, 780, 50), "Got it!", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);

    int res = DownloadThemeButton(ctx);
    if (!res){
        RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
        ThemeInfo_t *target = rI->themes;
        
        char *path = GetThemePath(target->creator, target->name, targetOptions[target->target]);

        SetInstallSlot(target->target, path);

        MakeMenu(out, ButtonHandlerBExit, NULL);
    }

    ShapeLinkDispose(&out);

    return 0;
}

ShapeLinker_t *CreateSelectMenu(RequestInfo_t *rI){
    ShapeLinker_t *out = NULL;
    ThemeInfo_t *target = rI->themes;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,170), 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(50, 100, SCREEN_W - 100, SCREEN_H - 150), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(50, 50, SCREEN_W - 150, 50), COLOR_TOPBAR, 1), RectangleType);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(55, 50, 0 /* 0 width left alligns it */, 50), target->name, COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);

    ShapeLinkAdd(&out, ButtonCreate(POS(SCREEN_W - 100, 50, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);

    ShapeLinkAdd(&out, ButtonCreate(POS(50, 100, 860, 488), COLOR_CENTERLISTBG, COLOR_WHITE, COLOR_WHITE, COLOR_CENTERLISTSELECTION, 0, ButtonStyleFlat, NULL, NULL, EnlargePreviewImage), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(target->preview, POS(55, 105, 850, 478), 0), ImageType);

    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(SCREEN_W - 100, 50, 50, 50), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(915, 110, SCREEN_W - 980, 60), COLOR_INSTBTN, COLOR_INSTBTNPRS, COLOR_WHITE, COLOR_INSTBTNSEL, (GetInstallButtonState()) ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install Theme", FONT_TEXT[FSize30], InstallThemeButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(915, 180, SCREEN_W - 980, 60), COLOR_DLBTN, COLOR_DLBTNPRS, COLOR_WHITE, COLOR_DLBTNSEL, 0, ButtonStyleFlat, "Download Theme", FONT_TEXT[FSize30], DownloadThemeButton), ButtonType);

    char *info = CopyTextArgsUtil("Creator: %s\n\nLast Updated: %s\n\nDownload Count: %d\nLike Count: %d\nID: t%s\nTarget: %s", target->creator, target->lastUpdated, target->dlCount, target->likeCount, target->id, targetOptions[target->target]);
    char *desc = CopyTextArgsUtil("Description: %s", target->description);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 250, SCREEN_W - 990, 420), info, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(60, 588, SCREEN_W - 120, 82), desc, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);

    free(info);
    free(desc);
    //ShapeLinkAdd()

    ShapeLinkAdd(&out, rI, DataType);

    return out;
}

int ThemeSelect(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ListGrid_t *gv = ShapeLinkFind(all, ListGridType)->item;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;
    ThemeInfo_t *target = &rI->themes[gv->highlight];

    if (target->preview == NULL)
        return 0;

    int w, h, update = 1;
    SDL_QueryTexture(target->preview, NULL, NULL, &w, &h);

    if (w >= 1000 && h >= 700)
        update = 0;

    RequestInfo_t customRI = {0};
    customRI.curPageItemCount = 1;
    customRI.themes = target;

    if (update)
        AddThemeImagesToDownloadQueue(&customRI, false);

    ShapeLinker_t *menu = CreateSelectMenu(&customRI);
    MakeMenu(menu, ButtonHandlerBExit, (update) ? HandleDownloadQueue : NULL);
    ShapeLinkDispose(&menu);

    if (update){
        CleanupTransferInfo(&customRI);
        ListItem_t *li = ShapeLinkOffset(gv->text, gv->highlight)->item;
        if (li->leftImg != target->preview){
            SDL_DestroyTexture(li->leftImg);
            li->leftImg = target->preview;
        }
    }
        
    return 0;
}