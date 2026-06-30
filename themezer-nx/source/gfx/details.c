#include "gfx.h"
#include <unistd.h>

int DownloadThemeButton(Context_t *ctx);
int InstallThemeButton(Context_t *ctx);
ShapeLinker_t *CreateSelectMenu(RequestInfo_t *rI);

static const char *GetThemeTargetLabel(const ThemeInfo_t *target){
    if (target->target < 0 || target->target >= 7)
        return "Unknown";

    return targetOptions[target->target + 1];
}

static ShapeLinker_t *CreateQuickIdLoadingMenu(){
    ShapeLinker_t *render = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&render, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&render, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,200), 1), RectangleType);
    ShapeLinkAdd(&render, TextCenteredCreate(POS(0, 0, SCREEN_W, SCREEN_H), "Looking up Quick ID...", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);

    return render;
}

static int ShowQuickIdMessage(const char *title, const char *message){
    ShapeLinker_t *menu = CreateBaseMessagePopup((char *)title, (char *)message);
    ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, 780, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Ok", FONT_TEXT[FSize28], exitFunc), ButtonType);
    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

static int ShowThemeDetailsMenu(ThemeInfo_t *target, ListItem_t *listItem){
    int update = 0;

    if (target->preview != NULL){
        int w, h;
        SDL_QueryTexture(target->preview, NULL, NULL, &w, &h);
        update = (w < 1000 || h < 700);
    }

    RequestInfo_t customRI = {0};
    customRI.maxDls = 1;
    customRI.curPageItemCount = 1;
    customRI.themes = target;

    if (update)
        AddThemeImagesToDownloadQueue(&customRI, false);

    ShapeLinker_t *menu = CreateSelectMenu(&customRI);
    MakeMenu(menu, ButtonHandlerBExit, update ? HandleDownloadQueue : NULL);
    ShapeLinkDispose(&menu);

    if (update){
        CleanupTransferInfo(&customRI);
        if (listItem != NULL && listItem->leftImg != target->preview){
            SDL_DestroyTexture(listItem->leftImg);
            listItem->leftImg = target->preview;
        }
    }

    return 0;
}

static ShapeLinker_t *CreateRemoteSelectMenu(RequestInfo_t *rI){
    ShapeLinker_t *out = NULL;
    ThemeInfo_t *target = rI->themes;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,170), 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(150, 120, SCREEN_W - 300, 440), COLOR_MAINBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(150, 70, SCREEN_W - 350, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(155, 72, 0, 50), target->name, COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(SCREEN_W - 200, 70, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARCURSOR, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(SCREEN_W - 200, 70, 50, 50), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(190, 150, 420, 60), COLOR_INSTALLBTN, COLOR_INSTALLBTNPRS, COLOR_WHITE, COLOR_INSTALLBTNSEL, GetInstallButtonState() ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install", FONT_TEXT[FSize30], InstallThemeButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(670, 150, 420, 60), COLOR_DOWNLOADBTN, COLOR_DOWNLOADBTNPRS, COLOR_WHITE, COLOR_DOWNLOADBTNSEL, 0, ButtonStyleFlat, "Download Only", FONT_TEXT[FSize30], DownloadThemeButton), ButtonType);

    char *info = CopyTextArgsUtil("By %s\n\nCreated: %s\n\nQuick ID: %s\n\nMenu: %s", target->creator, strtok(target->lastUpdated, "T"), target->id, GetThemeTargetLabel(target));
    ShapeLinkAdd(&out, TextCenteredCreate(POS(190, 250, 900, 250), info, COLOR_WHITE, FONT_TEXT[FSize28]), TextBoxType);
    free(info);

    ShapeLinkAdd(&out, rI, DataType);

    return out;
}

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

    char *path = GetThemePath(target, GetThemeTargetLabel(target));
    int res = DownloadThemeFromUrl(CopyTextUtil(target->downloadLink), path);

    if (res){
        ShapeLinkAdd(&render, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
        free(text->text.text);
        text->text.text = CopyTextUtil("Download failed!");
        MakeMenu(render, ButtonHandlerBExit, NULL);
    }

    ShapeLinkDispose(&render);

    free(path);

    return res;
}

int InstallThemeButton(Context_t *ctx){
    ShapeLinker_t *out = CreateBaseMessagePopup("Install Queued!", "Install Queued. Exit the app to apply the theme.\nYou can exit the app by pressing the + button.");

    ShapeLinkAdd(&out, RectangleCreate(POS(250, 470, 780, 50), COLOR_CARDCURSOR, 1), RectangleType);
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(250, 470, 780, 50), "Got it!", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);

    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    ThemeInfo_t *target = rI->themes;
    char *path = GetThemePath(target, GetThemeTargetLabel(target));

    int res = !(access(path, F_OK) != -1);

    if (res)
        res = DownloadThemeButton(ctx);
    
    if (!res){
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

    ShapeLinkAdd(&out, RectangleCreate(POS(50, 100, SCREEN_W - 100, SCREEN_H - 150), COLOR_MAINBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(50, 50, SCREEN_W - 150, 50), COLOR_TOPBAR, 1), RectangleType);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(55, 52, 0 /* 0 width left alligns it */, 50), target->name, COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);

    ShapeLinkAdd(&out, ButtonCreate(POS(SCREEN_W - 100, 50, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARCURSOR, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);

    ShapeLinkAdd(&out, ButtonCreate(POS(50, 100, 860, 488), COLOR_MAINBG, COLOR_CARDCURSORPRESS, COLOR_WHITE, COLOR_CARDCURSOR, (target->preview == NULL) ? BUTTON_DISABLED : 0, ButtonStyleFlat, NULL, NULL, EnlargePreviewImage), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(target->preview, POS(55, 105, 850, 478), 0), ImageType);

    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(SCREEN_W - 100, 50, 50, 50), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(915, 110, SCREEN_W - 980, 60), COLOR_INSTALLBTN, COLOR_INSTALLBTNPRS, COLOR_WHITE, COLOR_INSTALLBTNSEL, (GetInstallButtonState()) ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install", FONT_TEXT[FSize30], InstallThemeButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(915, 180, SCREEN_W - 980, 60), COLOR_DOWNLOADBTN, COLOR_DOWNLOADBTNPRS, COLOR_WHITE, COLOR_DOWNLOADBTNSEL, 0, ButtonStyleFlat, "Download Only", FONT_TEXT[FSize30], DownloadThemeButton), ButtonType);

    char *info = CopyTextArgsUtil("By %s\n\nLast Updated: %s\n\nID: %s\nDownloads: %d\nSaves: %d\n\nMenu: %s", target->creator, strtok(target->lastUpdated, "T"), target->id, target->dlCount, target->likeCount, GetThemeTargetLabel(target));
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 250, SCREEN_W - 990, 420), info, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);
    if (target->description != NULL && target->description[0]) {
        ShapeLinkAdd(&out, TextCenteredCreate(POS(60, 590, SCREEN_W - 120, 82), target->description, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);
    }

    free(info);
    //ShapeLinkAdd()

    ShapeLinkAdd(&out, rI, DataType);

    return out;
}

int ThemeSelect(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ListGrid_t *gv = ShapeLinkFind(all, ListGridType)->item;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;
    ThemeInfo_t *target = &rI->themes[gv->highlight];
    
    if (rI->target == 0)
        return ShowPackDetails(ctx);

    ListItem_t *li = ShapeLinkOffset(gv->text, gv->highlight)->item;
    ShowThemeDetailsMenu(target, li);
        
    return 0;
}

int ShowQuickIdLookup(Context_t *ctx){
    char *quickId = showKeyboard("Input Quick ID (find this on themezer.net)", NULL, 64);
    if (quickId == NULL)
        return 0;

    if (isStringNullOrEmpty(quickId)){
        free(quickId);
        return 0;
    }

    ShapeLinker_t *loadingMenu = CreateQuickIdLoadingMenu();
    RenderShapeLinkList(loadingMenu);

    RequestInfo_t lookupRI = {0};
    QuickIdLookupType_t lookupType = QuickIdLookupNone;
    int res = LookupByQuickId(quickId, &lookupRI, &lookupType);

    ShapeLinkDispose(&loadingMenu);

    if (res == 0){
        if (lookupType == QuickIdLookupTheme){
            ShowThemeDetailsMenu(lookupRI.themes, NULL);
        }
        else if (lookupType == QuickIdLookupPack){
            RequestInfo_t customRI = {0};
            customRI.maxDls = 12;
            customRI.target = -1;
            customRI.curPageItemCount = lookupRI.packs[0].themeCount;
            customRI.themes = lookupRI.packs[0].themes;

            ShapeLinker_t *items = GenListItemList(&customRI);
            AddThemeImagesToDownloadQueue(&customRI, true);

            ShapeLinker_t *menu = CreatePackDetailsMenu(items, &customRI);
            MakeMenu(menu, ButtonHandlerBExit, HandleDownloadQueue);
            CleanupTransferInfo(&customRI);
            ShapeLinkDispose(&menu);
        }
        else if (lookupType == QuickIdLookupRemoteTheme){
            ShapeLinker_t *menu = CreateRemoteSelectMenu(&lookupRI);
            MakeMenu(menu, ButtonHandlerBExit, NULL);
            ShapeLinkDispose(&menu);
        }
    }
    else if (res == 1){
        char *message = CopyTextArgsUtil("No result found for Quick ID: %s", quickId);
        ShowQuickIdMessage("Quick ID Not Found", message);
        free(message);
    }
    else if (res < 0 && res != -4){
        ShowQuickIdMessage("Quick ID Lookup Failed", "The quick ID lookup could not be completed.");
    }

    FreeThemes(&lookupRI);
    free(quickId);

    return 0;
}
