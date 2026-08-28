#include "gfx.h"

static bool mainMenuReturnToBoot = false;
static bool mainMenuLoaded = false;

enum {
    MAIN_MENU_LOAD_NOT_STARTED = 0,
    MAIN_MENU_LOAD_RUNNING,
    MAIN_MENU_LOAD_DONE,
    MAIN_MENU_LOAD_ERROR,
};

static volatile int mainMenuLoadState = MAIN_MENU_LOAD_NOT_STARTED;
static volatile bool mainMenuLoadCancelRequested = false;
static int mainMenuLoadResult = 0;
static Thread mainMenuLoadThread;
static bool mainMenuLoadThreadCreated = false;

static void AddMainMenuBackground(ShapeLinker_t **out, RequestInfo_t *rI);
static int LoadMainMenuData(RequestInfo_t *rI);
static void ShowMainMenuLoadError(int res);

void ResetMainMenuReturnToBoot(void){
    mainMenuReturnToBoot = false;
}

bool ConsumeMainMenuReturnToBoot(void){
    bool returnToBoot = mainMenuReturnToBoot;
    mainMenuReturnToBoot = false;
    return returnToBoot;
}

static int LoadMainMenuData(RequestInfo_t *rI){
    return MakeJsonRequestCancelable(GenLink(rI), &rI->response, &mainMenuLoadCancelRequested);
}

static void LoadMainMenuDataThread(void *arg){
    mainMenuLoadResult = LoadMainMenuData(arg);
    mainMenuLoadState = (mainMenuLoadResult == 0) ? MAIN_MENU_LOAD_DONE : MAIN_MENU_LOAD_ERROR;
}

static int StartMainMenuLoad(RequestInfo_t *rI){
    if (mainMenuLoaded || mainMenuLoadState == MAIN_MENU_LOAD_RUNNING)
        return 0;

    if (mainMenuLoadThreadCreated && mainMenuLoadState != MAIN_MENU_LOAD_RUNNING){
        threadWaitForExit(&mainMenuLoadThread);
        threadClose(&mainMenuLoadThread);
        mainMenuLoadThreadCreated = false;
    }

    mainMenuLoadResult = 0;
    mainMenuLoadCancelRequested = false;
    mainMenuLoadState = MAIN_MENU_LOAD_RUNNING;
    Result res = threadCreate(&mainMenuLoadThread, LoadMainMenuDataThread, rI, NULL, 0x40000, 0x2B, -2);
    if (R_FAILED(res)){
        mainMenuLoadResult = (int)res;
        mainMenuLoadState = MAIN_MENU_LOAD_ERROR;
        return mainMenuLoadResult;
    }

    res = threadStart(&mainMenuLoadThread);
    if (R_FAILED(res)){
        threadClose(&mainMenuLoadThread);
        mainMenuLoadResult = (int)res;
        mainMenuLoadState = MAIN_MENU_LOAD_ERROR;
        return mainMenuLoadResult;
    }

    mainMenuLoadThreadCreated = true;
    return 0;
}

static void CloseFinishedMainMenuLoadThread(void){
    if (!mainMenuLoadThreadCreated || mainMenuLoadState == MAIN_MENU_LOAD_RUNNING)
        return;

    threadWaitForExit(&mainMenuLoadThread);
    threadClose(&mainMenuLoadThread);
    mainMenuLoadThreadCreated = false;
}

void CleanupMainMenuLoad(void){
    if (!mainMenuLoadThreadCreated)
        return;

    if (mainMenuLoadState == MAIN_MENU_LOAD_RUNNING)
        mainMenuLoadCancelRequested = true;

    threadWaitForExit(&mainMenuLoadThread);
    threadClose(&mainMenuLoadThread);
    mainMenuLoadThreadCreated = false;

    if (mainMenuLoadResult == CURLE_ABORTED_BY_CALLBACK){
        mainMenuLoadResult = 0;
        mainMenuLoadState = MAIN_MENU_LOAD_NOT_STARTED;
    }
}

static void ShowMainMenuLoadError(int res){
    if (res > 0){
        ShowConnErrMenu(res);
        return;
    }

    char *message = CopyTextArgsUtil("Loading browse data failed. Error Code: %d", res);
    ShapeLinker_t *menu = CreateBaseMessagePopup("Browse Load Failed", message);
    ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, 780, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Ok", FONT_TEXT[FSize28], exitFunc), ButtonType);
    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);
    free(message);
}

int lennify(Context_t *ctx){
    static int lenny = false;
    if (!lenny){
        ShapeLinkAdd(&ctx->all, ImageCreate(LeImg, POS(644, 0, 156, 60), 0), ImageType);
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
    ShowLoadingPageUI(ctx, rI);

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
    ShowLoadingPageUI(ctx, rI);

    if (MakeRequestAsCtx(ctx,rI))
        rI->page++;

    return 0;
}

static int BackToBootButton(Context_t *ctx){
    (void)ctx;
    mainMenuLoadCancelRequested = true;
    mainMenuReturnToBoot = true;
    return -1;
}

static bool IsMainMenuReady(void){
    return mainMenuLoaded;
}

static int ShowSideTargetMenuIfReady(Context_t *ctx){
    return IsMainMenuReady() ? ShowSideTargetMenu(ctx) : 0;
}

static int ShowSideFilterMenuIfReady(Context_t *ctx){
    return IsMainMenuReady() ? ShowSideFilterMenu(ctx) : 0;
}

static int ShowSideQueueMenuIfReady(Context_t *ctx){
    return IsMainMenuReady() ? ShowSideQueueMenu(ctx) : 0;
}

int ButtonHandlerMainMenu(Context_t *ctx){
    if (ctx->kDown & HidNpadButton_B){
        mainMenuLoadCancelRequested = true;
        mainMenuReturnToBoot = true;
        return -1;
    }
    if (ctx->kHeld & HidNpadButton_R)
        return NextPageButton(ctx);
    if (ctx->kHeld & HidNpadButton_L)
        return PrevPageButton(ctx);
    if (IsMainMenuReady() && ctx->kHeld & HidNpadButton_Y)
        return ShowSideFilterMenu(ctx);
    if (IsMainMenuReady() && ctx->kHeld & HidNpadButton_X)
        return ShowSideTargetMenu(ctx);
    if (IsMainMenuReady() && ctx->kHeld & HidNpadButton_Minus)
        return ShowSideQueueMenu(ctx);

    return 0;
}

static int HandleMainMenuFrame(Context_t *ctx){
    if (mainMenuLoadState == MAIN_MENU_LOAD_DONE){
        RequestInfo_t *rI = ShapeLinkFind(ctx->all, DataType)->item;
        CloseFinishedMainMenuLoadThread();
        mainMenuLoadResult = GenThemeArray(rI);
        if (mainMenuLoadResult != 0){
            mainMenuLoadState = MAIN_MENU_LOAD_ERROR;
            return HandleMainMenuFrame(ctx);
        }

        ShapeLinker_t *items = GenListItemList(rI);
        AddThemeImagesToDownloadQueue(rI, true);
        UpdateMainMenuUI(ctx, rI, items);
        mainMenuLoaded = true;
        mainMenuLoadState = MAIN_MENU_LOAD_NOT_STARTED;
    }
    else if (mainMenuLoadState == MAIN_MENU_LOAD_ERROR){
        int res = mainMenuLoadResult;
        CloseFinishedMainMenuLoadThread();
        mainMenuLoadState = MAIN_MENU_LOAD_NOT_STARTED;
        if (res == CURLE_ABORTED_BY_CALLBACK){
            mainMenuReturnToBoot = true;
            return -1;
        }
        ShowMainMenuLoadError(res);
        mainMenuReturnToBoot = true;
        return -1;
    }

    return HandleDownloadQueue(ctx);
}

static SDL_Rect FitThumbHashBackground(SDL_Rect area){
    int width = area.w;
    int height = width * 9 / 16;

    if (height > area.h){
        height = area.h;
        width = height * 16 / 9;
    }

    return POS(area.x + (area.w - width) / 2, area.y + (area.h - height) / 2, width, height);
}

static void AddMainMenuBackground(ShapeLinker_t **out, RequestInfo_t *rI){
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

    SDL_Texture *thumbHashBackground = (rI->target == 0) ? packBgThumbHash : themeBgThumbHash;
    if (thumbHashBackground){
        ShapeLinkAdd(out, ImageCreate(thumbHashBackground, FitThumbHashBackground(POS(0, 0, SCREEN_W, SCREEN_H)), 0), ImageType);
    }
}

ShapeLinker_t *CreateMainMenu(ShapeLinker_t *listItems, RequestInfo_t *rI) { 
    ShapeLinker_t *out = NULL;
    int backButtonX = 0;
    int targetButtonX = 120;
    int searchButtonX = 240;
    int queueButtonX = 360;
    SDL_Color accentColor = GetMainMenuAccentColor(rI);

    AddMainMenuBackground(&out, rI);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, 60), COLOR_MAIN_TOPBAR, 1), RectangleType);

    // Text inbetween arrows
    char *temp = CopyTextArgsUtil("%d/%d (%d)", rI->page, rI->pageCount, rI->itemCount);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 0, 240, 60), temp, COLOR_WHITE, FONT_TEXT[FSize25]), TextCenteredType);
    free(temp);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), listItems ? " " : "Loading...", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);
    ShapeLinkAdd(&out, ImageCreate(moodDown, POS(0, 0, 0, 0), 0), ImageType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 460, SCREEN_W, 80), " ", COLOR_WHITE, FONT_TEXT[FSize35]), TextCenteredType);

    // BackButton
    ShapeLinkAdd(&out, ButtonCreate(POS(backButtonX, 0, 120, 60), COLOR_MAIN_TOPBARBUTTONS, accentColor, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, BackToBootButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowLIcon, POS(backButtonX + 28, 0, 60, 60), 0), ImageType);

    // MenuButton
    ShapeLinkAdd(&out, ButtonCreate(POS(targetButtonX, 0, 120, 60), COLOR_MAIN_TOPBARBUTTONS, accentColor, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideTargetMenuIfReady), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(menuIcon, POS(targetButtonX + 30, 0, 60, 60), 0), ImageType);

    // SearchButton
    ShapeLinkAdd(&out, ButtonCreate(POS(searchButtonX, 0, 120, 60), COLOR_MAIN_TOPBARBUTTONS, accentColor, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideFilterMenuIfReady), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(searchIcon, POS(searchButtonX + 30, 0, 60, 60), 0), ImageType);

    // QueueButton
    ShapeLinkAdd(&out, ButtonCreate(POS(queueButtonX, 0, 120, 60), COLOR_MAIN_TOPBARBUTTONS, accentColor, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideQueueMenuIfReady), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(queueIcon, POS(queueButtonX + 30, 0, 60, 60), 0), ImageType);

    // LeftArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(800, 0, 120, 60), COLOR_MAIN_TOPBARBUTTONS, rI->page > 1 ? accentColor : COLOR_MAIN_TOPBARBUTTONS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, PrevPageButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowLIcon, POS(830, 0, 60, 60), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(644, 0, 156, 60), COLOR_MAIN_TOPBARBUTTONS, COLOR_MAIN_TOPBARBUTTONS, COLOR_WHITE, COLOR_MAIN_TOPBARBUTTONS, BUTTON_NOJOYSEL, ButtonStyleFlat, NULL, NULL, lennify), ButtonType);

    // RightArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(1160, 0, 120, 60), COLOR_MAIN_TOPBARBUTTONS, rI->page < rI->pageCount ? accentColor : COLOR_MAIN_TOPBARBUTTONS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, NULL, NULL, NextPageButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowRIcon, POS(1190, 0, 60, 60), 0), ImageType);

    ShapeLinkAdd(&out, ListGridCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), 4, 260, COLOR_FROM_RGBA(COLOR_TRANSPARENT_RGBA), accentColor, accentColor, COLOR_SCROLLBARBG, accentColor, (listItems) ? GRID_NOSIDEESC : LIST_DISABLED, listItems, ThemeSelect, NULL, FONT_TEXT[FSize23]), ListGridType);
    // 4, 260

    ShapeLinkAdd(&out, rI, DataType);

    // Logo
    ShapeLinkAdd(&out, ImageCreate(icon, POS(584, 0, 60, 60), 0), ImageType);

    // Glyphs
    ShapeLinkAdd(&out, GlyphCreate(100, 2, BUTTON_B, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    ShapeLinkAdd(&out, GlyphCreate(217, 2, BUTTON_X, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    ShapeLinkAdd(&out, GlyphCreate(337, 2, BUTTON_Y, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    ShapeLinkAdd(&out, GlyphCreate(457, 2, BUTTON_MINUS, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);

    Glyph_t *leftButtonIcon = GlyphCreate(804, 2, BUTTON_L, COLOR_WHITE, FONT_BTN[FSize20]);
    Glyph_t *rightButtonIcon = GlyphCreate(1256, 2, BUTTON_R, COLOR_WHITE, FONT_BTN[FSize20]);

    if (rI->page == 1) {
        SETBIT(leftButtonIcon->options, TEXT_GLYPH_NO_RENDER, 1);
    }
    if (rI->page == rI->pageCount) {
        SETBIT(rightButtonIcon->options, TEXT_GLYPH_NO_RENDER, 1);
    }

    ShapeLinkAdd(&out, leftButtonIcon, GlyphType);
    ShapeLinkAdd(&out, rightButtonIcon, GlyphType);

    return out;
}

bool RunMainMenu(RequestInfo_t *rI){
    if (!mainMenuLoaded && mainMenuLoadState == MAIN_MENU_LOAD_DONE){
        CloseFinishedMainMenuLoadThread();
        int res = GenThemeArray(rI);
        if (res != 0){
            mainMenuLoadState = MAIN_MENU_LOAD_NOT_STARTED;
            ShowMainMenuLoadError(res);
            return true;
        }

        AddThemeImagesToDownloadQueue(rI, true);
        mainMenuLoaded = true;
        mainMenuLoadState = MAIN_MENU_LOAD_NOT_STARTED;
    }
    else if (!mainMenuLoaded && mainMenuLoadState == MAIN_MENU_LOAD_ERROR){
        int res = mainMenuLoadResult;
        CloseFinishedMainMenuLoadThread();
        mainMenuLoadState = MAIN_MENU_LOAD_NOT_STARTED;
        if (res != CURLE_ABORTED_BY_CALLBACK)
            ShowMainMenuLoadError(res);
        return true;
    }

    if (!mainMenuLoaded)
        StartMainMenuLoad(rI);

    ResetMainMenuReturnToBoot();
    ShapeLinker_t *items = mainMenuLoaded ? GenListItemList(rI) : NULL;
    ShapeLinker_t *mainMenu = CreateMainMenu(items, rI);
    MakeMenu(mainMenu, ButtonHandlerMainMenu, HandleMainMenuFrame);
    ShapeLinkDispose(&mainMenu);

    bool returnToBoot = ConsumeMainMenuReturnToBoot();
    if (returnToBoot)
        CleanupMainMenuLoad();

    return returnToBoot;
}
