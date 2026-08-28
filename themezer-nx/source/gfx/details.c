#include "gfx.h"
#include <unistd.h>

int DownloadThemeButton(Context_t *ctx);
int InstallThemeButton(Context_t *ctx);
ShapeLinker_t *CreateSelectMenu(RequestInfo_t *rI);
int EnlargePreviewImage(Context_t *ctx);
static int ShowSplashDetailsMenu(SplashInfo_t *target, ListItem_t *listItem);

static const char *GetThemeTargetLabel(const ThemeInfo_t *target){
    if (target->target < 0 || target->target >= THEME_TARGET_COUNT)
        return "Unknown";

    return targetOptions[target->target + 1];
}

static ShapeLinker_t *CreateQuickIdLoadingMenu(){
    ShapeLinker_t *render = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&render, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&render, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,200), 1), RectangleType);
    ShapeLinkAdd(&render, TextCenteredCreate(POS(0, 0, SCREEN_W, SCREEN_H), "Looking up Quick ID...", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);
    ShapeLinkAdd(&render, TextCenteredCreate(POS(0, SCREEN_H - 90, SCREEN_W, 40), "Press B to cancel", COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);

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
    customRI.contentType = RequestContentThemes;

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

static const char *GetRemoteInstallTypeLabel(const RemoteInstallInfo_t *target){
    return (target->kind == RemoteInstallKindSplash) ? "Hekate Splash" : "Switch Theme";
}

static int ShowDownloadResult(DownloadProgressContext_t *progress, int res){
    if (!res)
        return 0;

    ShapeLinkAdd(&progress->menu, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), COLOR(0, 0, 0, 0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    free(progress->message->text.text);
    progress->message->text.text = CopyTextUtil(progress->cancelled ? "Download cancelled!" : "Download failed!");
    MakeMenu(progress->menu, ButtonHandlerBExit, NULL);

    return res;
}

static int DownloadRemoteInstallToPath(RemoteInstallInfo_t *target, char *path){
    DownloadProgressContext_t progress = {0};
    ShapeLinker_t *render = CreateDownloadProgressMenu((target->kind == RemoteInstallKindSplash) ? "Downloading Splash..." : "Downloading Theme...", &progress);
    RenderShapeLinkList(render);
    int res = DownloadThemeFromUrl(CopyTextUtil(target->downloadLink), path, &progress);
    ShowDownloadResult(&progress, res);

    ShapeLinkDispose(&render);
    return res;
}

static int DownloadRemoteInstallButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    RemoteInstallInfo_t *target = rI->remoteInstall;
    char *path = GetRemoteInstallPath(target);
    int res = DownloadRemoteInstallToPath(target, path);
    free(path);
    return res;
}

static int InstallRemoteInstallAtPath(RemoteInstallInfo_t *target, char *path){
    int res = 0;

    if (access(path, F_OK) == -1)
        res = DownloadRemoteInstallToPath(target, path);

    if (!res){
        int installSlot = (target->kind == RemoteInstallKindSplash) ? SPLASH_INSTALL_SLOT : target->target;
        SetInstallSlot(installSlot, path);

        const char *title = (target->kind == RemoteInstallKindSplash) ? "Splash Queued!" : "Theme Queued!";
        const char *message = (target->kind == RemoteInstallKindSplash) ?
            "The remote Hekate splash is queued for NXThemes Installer.\nExit the app to apply it.\nYou can exit the app by pressing the + button." :
            "The remote theme is queued for NXThemes Installer.\nExit the app to apply it.\nYou can exit the app by pressing the + button.";
        ShapeLinker_t *out = CreateBaseMessagePopup((char *)title, (char *)message);
        ShapeLinkAdd(&out, RectangleCreate(POS(250, 470, 780, 50), COLOR_CARDCURSOR, 1), RectangleType);
        ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
        ShapeLinkAdd(&out, TextCenteredCreate(POS(250, 470, 780, 50), "Got it!", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);
        MakeMenu(out, ButtonHandlerBExit, NULL);
        ShapeLinkDispose(&out);
    }

    free(path);
    return 0;
}

static int InstallRemoteInstallButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    RemoteInstallInfo_t *target = rI->remoteInstall;
    return InstallRemoteInstallAtPath(target, GetRemoteInstallPath(target));
}

static int InstallRemoteInstallOnlyButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    RemoteInstallInfo_t *target = rI->remoteInstall;
    return InstallRemoteInstallAtPath(target, GetTemporaryRemoteInstallPath(target));
}

static ShapeLinker_t *CreateRemoteSelectMenu(RequestInfo_t *rI){
    ShapeLinker_t *out = NULL;
    RemoteInstallInfo_t *target = rI->remoteInstall;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,170), 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(150, 120, SCREEN_W - 300, 440), COLOR_MAINBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(150, 70, SCREEN_W - 350, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(155, 72, 0, 50), target->name, COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(SCREEN_W - 200, 70, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARCURSOR, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(SCREEN_W - 200, 70, 50, 50), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(190, 150, 280, 60), COLOR_INSTALLBTN, COLOR_INSTALLBTNPRS, COLOR_WHITE, COLOR_INSTALLBTNSEL, GetInstallButtonState() ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install & Save", FONT_TEXT[FSize30], InstallRemoteInstallButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(490, 150, 280, 60), COLOR_INSTALLONLYBTN, COLOR_INSTALLONLYBTNPRS, COLOR_WHITE, COLOR_INSTALLONLYBTNSEL, GetInstallButtonState() ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install Only", FONT_TEXT[FSize30], InstallRemoteInstallOnlyButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(790, 150, 280, 60), COLOR_DOWNLOADBTN, COLOR_DOWNLOADBTNPRS, COLOR_WHITE, COLOR_DOWNLOADBTNSEL, 0, ButtonStyleFlat, "Download Only", FONT_TEXT[FSize30], DownloadRemoteInstallButton), ButtonType);

    char *created = CopyTextUtil(target->lastUpdated);
    char *info = NULL;
    if (target->kind == RemoteInstallKindSplash){
        info = CopyTextArgsUtil("By %s\n\nCreated: %s\n\nQuick ID: %s\n\nType: %s", target->creator, strtok(created, "T"), target->quickId, GetRemoteInstallTypeLabel(target));
    }
    else {
        info = CopyTextArgsUtil("By %s\n\nCreated: %s\n\nQuick ID: %s\n\nType: %s\n\nMenu: %s", target->creator, strtok(created, "T"), target->quickId, GetRemoteInstallTypeLabel(target), GetInstallSlotLabel(target->target));
    }
    ShapeLinkAdd(&out, TextCenteredCreate(POS(190, 250, 900, 250), info, COLOR_WHITE, FONT_TEXT[FSize28]), TextBoxType);
    free(info);
    free(created);

    ShapeLinkAdd(&out, rI, DataType);

    return out;
}

static int DownloadSplashToPath(SplashInfo_t *target, char *path){
    DownloadProgressContext_t progress = {0};
    ShapeLinker_t *render = CreateDownloadProgressMenu("Downloading Splash...", &progress);
    RenderShapeLinkList(render);
    int res = DownloadThemeFromUrl(CopyTextUtil(target->downloadLink), path, &progress);
    ShowDownloadResult(&progress, res);

    ShapeLinkDispose(&render);
    return res;
}

static int DownloadSplashButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    SplashInfo_t *target = rI->splashes;
    char *path = GetSplashPath(target);
    int res = DownloadSplashToPath(target, path);
    free(path);
    return res;
}

static int InstallSplashAtPath(SplashInfo_t *target, char *path){
    int res = 0;

    if (access(path, F_OK) == -1)
        res = DownloadSplashToPath(target, path);

    if (!res){
        SetInstallSlot(SPLASH_INSTALL_SLOT, path);

        ShapeLinker_t *out = CreateBaseMessagePopup("Splash Queued!", "The Hekate splash is queued for NXThemes Installer.\nExit the app to apply it.\nYou can exit the app by pressing the + button.");
        ShapeLinkAdd(&out, RectangleCreate(POS(250, 470, 780, 50), COLOR_CARDCURSOR, 1), RectangleType);
        ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
        ShapeLinkAdd(&out, TextCenteredCreate(POS(250, 470, 780, 50), "Got it!", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);
        MakeMenu(out, ButtonHandlerBExit, NULL);
        ShapeLinkDispose(&out);
    }

    free(path);
    return 0;
}

static int InstallSplashButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    return InstallSplashAtPath(rI->splashes, GetSplashPath(rI->splashes));
}

static int InstallSplashOnlyButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    return InstallSplashAtPath(rI->splashes, GetTemporarySplashPath(rI->splashes));
}

static ShapeLinker_t *CreateSplashSelectMenu(RequestInfo_t *rI){
    SplashInfo_t *target = rI->splashes;
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,170), 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(50, 100, SCREEN_W - 100, SCREEN_H - 150), COLOR_MAINBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(50, 50, SCREEN_W - 150, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(55, 52, 0, 50), target->name, COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(SCREEN_W - 100, 50, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARCURSOR, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);

    ShapeLinkAdd(&out, ButtonCreate(POS(50, 100, 860, 488), COLOR_MAINBG, COLOR_CARDCURSORPRESS, COLOR_WHITE, COLOR_CARDCURSOR, (target->preview == NULL) ? BUTTON_DISABLED : 0, ButtonStyleFlat, NULL, NULL, EnlargePreviewImage), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(target->preview, POS(55, 105, 850, 478), 0), ImageType);
    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(SCREEN_W - 100, 50, 50, 50), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(915, 110, SCREEN_W - 980, 60), COLOR_INSTALLBTN, COLOR_INSTALLBTNPRS, COLOR_WHITE, COLOR_INSTALLBTNSEL, GetInstallButtonState() ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install & Save", FONT_TEXT[FSize30], InstallSplashButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(915, 180, SCREEN_W - 980, 60), COLOR_INSTALLONLYBTN, COLOR_INSTALLONLYBTNPRS, COLOR_WHITE, COLOR_INSTALLONLYBTNSEL, GetInstallButtonState() ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install Only", FONT_TEXT[FSize30], InstallSplashOnlyButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(915, 250, SCREEN_W - 980, 60), COLOR_DOWNLOADBTN, COLOR_DOWNLOADBTNPRS, COLOR_WHITE, COLOR_DOWNLOADBTNSEL, 0, ButtonStyleFlat, "Download Only", FONT_TEXT[FSize30], DownloadSplashButton), ButtonType);

    char *created = CopyTextUtil(target->createdAt);
    char *updated = CopyTextUtil(target->lastUpdated);
    char *createdDate = strtok(created, "T");
    char *updatedDate = strtok(updated, "T");
    char *info = CopyTextArgsUtil("By %s\n\nCreated: %s\nUpdated: %s\n\nQuick ID: %s\n\nType: Hekate Splash", target->creator, createdDate, updatedDate, target->quickId);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 320, SCREEN_W - 990, 260), info, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);
    free(info);
    free(created);
    free(updated);

    if (target->description != NULL && target->description[0])
        ShapeLinkAdd(&out, TextCenteredCreate(POS(60, 590, SCREEN_W - 120, 82), target->description, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);

    ShapeLinkAdd(&out, rI, DataType);
    return out;
}

static int ShowSplashDetailsMenu(SplashInfo_t *target, ListItem_t *listItem){
    int update = 0;

    if (target->preview != NULL){
        int w, h;
        SDL_QueryTexture(target->preview, NULL, NULL, &w, &h);
        update = (w < 1000 || h < 700);
    }

    RequestInfo_t customRI = {0};
    customRI.maxDls = 1;
    customRI.curPageItemCount = 1;
    customRI.splashes = target;
    customRI.contentType = RequestContentSplashes;

    if (update)
        AddThemeImagesToDownloadQueue(&customRI, false);

    ShapeLinker_t *menu = CreateSplashSelectMenu(&customRI);
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

int EnlargePreviewImage(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    SDL_Texture *preview = (rI->contentType == RequestContentSplashes) ? rI->splashes[0].preview : rI->themes[0].preview;

    int w, h;
    SDL_QueryTexture(preview, NULL, NULL, &w, &h);

    if (w < 1000 || h < 700)
        return 0;

    ShapeLinker_t *menu = NULL;
    ShapeLinkAdd(&menu, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&menu, ImageCreate(preview, POS(0, 0, SCREEN_W, SCREEN_H), 0), ImageType);

    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

static int DownloadThemeToPath(ThemeInfo_t *target, char *path){
    DownloadProgressContext_t progress = {0};
    ShapeLinker_t *render = CreateDownloadProgressMenu("Downloading Theme...", &progress);
    RenderShapeLinkList(render);

    int res = DownloadThemeFromUrl(CopyTextUtil(target->downloadLink), path, &progress);
    ShowDownloadResult(&progress, res);

    ShapeLinkDispose(&render);
    return res;
}

int DownloadThemeButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    ThemeInfo_t *target = rI->themes;
    char *path = GetThemePath(target, GetThemeTargetLabel(target));
    int res = DownloadThemeToPath(target, path);
    free(path);
    return res;
}

static int InstallThemeAtPath(ThemeInfo_t *target, char *path){
    int res = 0;

    if (access(path, F_OK) == -1)
        res = DownloadThemeToPath(target, path);

    if (!res){
        SetInstallSlot(target->target, path);

        ShapeLinker_t *out = CreateBaseMessagePopup("Install Queued!", "Install Queued. Exit the app to apply the theme.\nYou can exit the app by pressing the + button.");
        ShapeLinkAdd(&out, RectangleCreate(POS(250, 470, 780, 50), COLOR_CARDCURSOR, 1), RectangleType);
        ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
        ShapeLinkAdd(&out, TextCenteredCreate(POS(250, 470, 780, 50), "Got it!", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);
        MakeMenu(out, ButtonHandlerBExit, NULL);
        ShapeLinkDispose(&out);
    }

    free(path);
    return 0;
}

int InstallThemeButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    return InstallThemeAtPath(rI->themes, GetThemePath(rI->themes, GetThemeTargetLabel(rI->themes)));
}

static int InstallThemeOnlyButton(Context_t *ctx){
    RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
    return InstallThemeAtPath(rI->themes, GetTemporaryThemePath(rI->themes));
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

    ShapeLinkAdd(&out, ButtonCreate(POS(915, 110, SCREEN_W - 980, 60), COLOR_INSTALLBTN, COLOR_INSTALLBTNPRS, COLOR_WHITE, COLOR_INSTALLBTNSEL, (GetInstallButtonState()) ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install & Save", FONT_TEXT[FSize30], InstallThemeButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(915, 180, SCREEN_W - 980, 60), COLOR_INSTALLONLYBTN, COLOR_INSTALLONLYBTNPRS, COLOR_WHITE, COLOR_INSTALLONLYBTNSEL, (GetInstallButtonState()) ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install Only", FONT_TEXT[FSize30], InstallThemeOnlyButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(915, 250, SCREEN_W - 980, 60), COLOR_DOWNLOADBTN, COLOR_DOWNLOADBTNPRS, COLOR_WHITE, COLOR_DOWNLOADBTNSEL, 0, ButtonStyleFlat, "Download Only", FONT_TEXT[FSize30], DownloadThemeButton), ButtonType);

    char *created = CopyTextUtil(target->createdAt);
    char *updated = CopyTextUtil(target->lastUpdated);
    char *createdDate = strtok(created, "T");
    char *updatedDate = strtok(updated, "T");
    char *info = CopyTextArgsUtil("By %s\n\nCreated: %s\nUpdated: %s\n\nID: %s\nDownloads: %d\nSaves: %d\n\nMenu: %s", target->creator, createdDate, updatedDate, target->id, target->dlCount, target->likeCount, GetThemeTargetLabel(target));
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 320, SCREEN_W - 990, 260), info, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);
    if (target->description != NULL && target->description[0]) {
        ShapeLinkAdd(&out, TextCenteredCreate(POS(60, 590, SCREEN_W - 120, 82), target->description, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);
    }

    free(info);
    free(created);
    free(updated);
    //ShapeLinkAdd()

    ShapeLinkAdd(&out, rI, DataType);

    return out;
}

int ThemeSelect(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ListGrid_t *gv = ShapeLinkFind(all, ListGridType)->item;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;

    if (rI->target == 0)
        return ShowPackDetails(ctx);

    if (rI->contentType == RequestContentSplashes){
        ListItem_t *li = ShapeLinkOffset(gv->text, gv->highlight)->item;
        return ShowSplashDetailsMenu(&rI->splashes[gv->highlight], li);
    }

    ThemeInfo_t *target = &rI->themes[gv->highlight];

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
            customRI.contentType = RequestContentThemes;

            ShapeLinker_t *items = GenListItemList(&customRI);
            AddThemeImagesToDownloadQueue(&customRI, true);

            ShapeLinker_t *menu = CreatePackDetailsMenu(items, &customRI);
            MakeMenu(menu, ButtonHandlerBExit, HandleDownloadQueue);
            CleanupTransferInfo(&customRI);
            ShapeLinkDispose(&menu);
        }
        else if (lookupType == QuickIdLookupRemoteInstall){
            ShapeLinker_t *menu = CreateRemoteSelectMenu(&lookupRI);
            MakeMenu(menu, ButtonHandlerBExit, NULL);
            ShapeLinkDispose(&menu);
        }
        else if (lookupType == QuickIdLookupSplash){
            ShowSplashDetailsMenu(lookupRI.splashes, NULL);
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

    FreeRequestContent(&lookupRI);
    free(quickId);

    return 0;
}
