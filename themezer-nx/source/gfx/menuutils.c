#include "gfx.h"
#include "colors.h"
#include <switch.h>
#include "../model.h"
#include "../utils.h"
#include "../curl.h"

int InstallButtonState = 0;

void SetInstallButtonState(int state){
    InstallButtonState = state;
}

int GetInstallButtonState(){
    return InstallButtonState;
}

static bool SameRect(SDL_Rect a, SDL_Rect b){
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

static TextCentered_t *FindTextCenteredByRect(ShapeLinker_t *all, SDL_Rect pos){
    for (ShapeLinker_t *iter = all; iter != NULL; iter = iter->next){
        if (iter->type == TextCenteredType){
            TextCentered_t *text = iter->item;
            SDL_Rect textPos = POS(text->text.x, text->text.y, text->w, text->h);
            if (SameRect(textPos, pos))
                return text;
        }
    }

    return NULL;
}

static Image_t *FindImageByTexture(ShapeLinker_t *all, SDL_Texture *texture){
    for (ShapeLinker_t *iter = all; iter != NULL; iter = iter->next){
        if (iter->type == ImageType){
            Image_t *image = iter->item;
            if (image->texture == texture)
                return image;
        }
    }

    return NULL;
}

static Button_t *FindButtonByRect(ShapeLinker_t *all, SDL_Rect pos){
    for (ShapeLinker_t *iter = all; iter != NULL; iter = iter->next){
        if (iter->type == ButtonType){
            Button_t *button = iter->item;
            if (SameRect(button->pos, pos))
                return button;
        }
    }

    return NULL;
}

static Glyph_t *FindGlyphByPosition(ShapeLinker_t *all, int x, int y){
    for (ShapeLinker_t *iter = all; iter != NULL; iter = iter->next){
        if (iter->type == GlyphType){
            Glyph_t *glyph = iter->item;
            if (glyph->x == x && glyph->y == y)
                return glyph;
        }
    }

    return NULL;
}

static Image_t *FindMainMenuThumbHashBackground(ShapeLinker_t *all){
    for (ShapeLinker_t *iter = all; iter != NULL; iter = iter->next){
        if (iter->type == ImageType){
            Image_t *image = iter->item;
            if (image->texture == themeBgThumbHash || image->texture == packBgThumbHash)
                return image;
        }
    }

    return NULL;
}

static SDL_Texture *GetMainMenuThumbHashBackground(RequestInfo_t *rI){
    return (rI->target == 0) ? packBgThumbHash : themeBgThumbHash;
}

SDL_Color GetMainMenuAccentColor(RequestInfo_t *rI){
    return (rI->target == 0) ? COLOR_MAIN_TOPBAR_PACK : COLOR_MAIN_TOPBAR_THEME;
}

static void SetButtonSecondaryByRect(ShapeLinker_t *all, SDL_Rect pos, SDL_Color color){
    Button_t *button = FindButtonByRect(all, pos);
    if (button)
        button->secondary = color;
}

static void UpdateMainMenuAccentColor(RequestInfo_t *rI, ShapeLinker_t *all){
    SDL_Color accentColor = GetMainMenuAccentColor(rI);
    ShapeLinker_t *gridLink = ShapeLinkFind(all, ListGridType);
    ListGrid_t *grid = gridLink ? gridLink->item : NULL;

    SetButtonSecondaryByRect(all, POS(0, 0, 120, 60), accentColor);
    SetButtonSecondaryByRect(all, POS(120, 0, 120, 60), accentColor);
    SetButtonSecondaryByRect(all, POS(240, 0, 120, 60), accentColor);
    SetButtonSecondaryByRect(all, POS(360, 0, 120, 60), accentColor);
    SetButtonSecondaryByRect(all, POS(800, 0, 120, 60), rI->page > 1 ? accentColor : COLOR_MAIN_TOPBARBUTTONS);
    SetButtonSecondaryByRect(all, POS(644, 0, 156, 60), COLOR_MAIN_TOPBARBUTTONS);
    SetButtonSecondaryByRect(all, POS(1160, 0, 120, 60), rI->page < rI->pageCount ? accentColor : COLOR_MAIN_TOPBARBUTTONS);

    if (grid){
        grid->selected = accentColor;
        grid->pressed = accentColor;
        grid->scrollbarBg = COLOR_SCROLLBARBG;
        grid->scrollbarThumb = accentColor;
    }
}

static void UpdateMainMenuBackground(RequestInfo_t *rI, ShapeLinker_t *all){
    Image_t *thumbHashBackground = FindMainMenuThumbHashBackground(all);
    if (thumbHashBackground)
        thumbHashBackground->texture = GetMainMenuThumbHashBackground(rI);

    UpdateMainMenuAccentColor(rI, all);
}

void SetMainMenuEmptyMessage(ShapeLinker_t *all, char *emptyMessage){
    TextCentered_t *emptyText = FindTextCenteredByRect(all, POS(0, 60, SCREEN_W, SCREEN_H - 60));
    if (!emptyText)
        return;

    free(emptyText->text.text);
    emptyText->text.text = CopyTextUtil(emptyMessage);
}

void SetMainMenuNoContentState(ShapeLinker_t *all, bool visible){
    Image_t *emptyImage = FindImageByTexture(all, moodDown);
    TextCentered_t *emptyCaption = FindTextCenteredByRect(all, POS(0, 460, SCREEN_W, 80));

    if (emptyImage)
        emptyImage->pos = visible ? POS(576, 246, 128, 128) : POS(0, 0, 0, 0);

    if (emptyCaption){
        free(emptyCaption->text.text);
        emptyCaption->text.text = CopyTextUtil(visible ? "Nothing to see here..." : " ");
    }
}

int exitFunc(Context_t *ctx){
    return -1;
}

int ButtonHandlerBExit(Context_t *ctx){
    if (ctx->kHeld & HidNpadButton_B)
        return -1;

    return 0;
}

static int ButtonHandlerExitOnMask(Context_t *ctx, u64 buttonMask){
    if (ctx->kDown & buttonMask)
        return -1;

    return 0;
}

int ButtonHandlerBXExit(Context_t *ctx){
    return ButtonHandlerExitOnMask(ctx, HidNpadButton_B | HidNpadButton_X);
}

int ButtonHandlerBYExit(Context_t *ctx){
    return ButtonHandlerExitOnMask(ctx, HidNpadButton_B | HidNpadButton_Y);
}

int ButtonHandlerBMinusExit(Context_t *ctx){
    return ButtonHandlerExitOnMask(ctx, HidNpadButton_B | HidNpadButton_Minus);
}

ShapeLinker_t *CreateSideBaseMenu(char *menuName){ // Count: 7
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 50, 400, SCREEN_H - 50), COLOR_MAINBG, 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, 350, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 0, 400, 50), menuName, COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(350, 0, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARCURSOR, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(350, 0, 50, 50), 0), ImageType);
    ShapeLinkAdd(&out, ButtonCreate(POS(400, 0, SCREEN_W - 400, SCREEN_H), COLOR(0,0,0,170), COLOR(0,0,0,170), COLOR(0,0,0,170), COLOR(0,0,0,170), BUTTON_NOJOYSEL, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);

    return out;
}

ShapeLinker_t *CreateBaseMessagePopup(char *title, char *message){ // Other code needs to add buttons at the bottom at Y 470 H 50, Count: 6
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0, 0, 0, 170), 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(250, 200, SCREEN_W - 500, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(260, 200, 0, 50), title, COLOR_WHITE, FONT_TEXT[FSize25]), TextCenteredType);
    ShapeLinkAdd(&out, RectangleCreate(POS(250, 250, SCREEN_W - 500, 220), COLOR_MAINBG, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(260, 260, SCREEN_W - 520, 200), message, COLOR_WHITE, FONT_TEXT[FSize28]), TextBoxType);

    return out;
}

static void AddDownloadCancelHint(ShapeLinker_t **out){
    const char *prefix = "Hold";
    const char *suffix = "to cancel";
    const int hintY = 460;
    const int hintH = 50;
    const int gap = 8;

    SizeInfo_t prefixSize = GetTextSizeSDL(FONT_TEXT[FSize23], prefix);
    SizeInfo_t suffixSize = GetTextSizeSDL(FONT_TEXT[FSize23], suffix);
    int glyphMinX, glyphMinY, glyphMaxX, glyphMaxY, glyphAdvance;
    if (TTF_GlyphMetrics(FONT_BTN[FSize20], BUTTON_B, &glyphMinX, &glyphMaxX, &glyphMinY, &glyphMaxY, &glyphAdvance) != 0 || glyphAdvance < 0)
        glyphAdvance = 0;

    int x = (SCREEN_W - prefixSize.w - gap - glyphAdvance - gap - suffixSize.w) / 2;
    int textY = hintY + (hintH - prefixSize.h) / 2;
    int glyphY = hintY + (hintH - TTF_FontHeight(FONT_BTN[FSize20])) / 2;

    ShapeLinkAdd(out, TextCreate(x, textY, prefix, COLOR_WHITE, FONT_TEXT[FSize23]), TextType);
    x += prefixSize.w + gap;
    ShapeLinkAdd(out, GlyphCreate(x, glyphY, BUTTON_B, COLOR_WHITE, FONT_BTN[FSize20]), GlyphType);
    x += glyphAdvance + gap;
    ShapeLinkAdd(out, TextCreate(x, textY, suffix, COLOR_WHITE, FONT_TEXT[FSize23]), TextType);
}

ShapeLinker_t *CreateDownloadProgressMenu(const char *message, DownloadProgressContext_t *progress){
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0, 0, 0, 200), 1), RectangleType);

    TextCentered_t *progressMessage = TextCenteredCreate(POS(0, 250, SCREEN_W, 100), message, COLOR_WHITE, FONT_TEXT[FSize45]);
    ShapeLinkAdd(&out, progressMessage, TextCenteredType);
    AddDownloadCancelHint(&out);

    ProgressBar_t *progressBar = ProgressBarCreate(POS(220, 400, SCREEN_W - 440, 35), COLOR_DOWNLOADBTN, COLOR_DARKGREY, ProgressBarStyleBorder, 0);
    ShapeLinkAdd(&out, progressBar, ProgressBarType);

    if (progress){
        progress->menu = out;
        progress->message = progressMessage;
        progress->bar = progressBar;
        progress->cancelled = false;
        progress->finalResponseReady = false;
    }

    return out;
}

int ShowCurlError(Context_t *ctx){
    ShapeLinker_t *menu = NULL;

    ShapeLinkAdd(&menu, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_MAINBG, COLOR_MAINBG, COLOR_WHITE, COLOR_MAINBG, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&menu, RectangleCreate(POS(0, 0, SCREEN_W, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&menu, TextCenteredCreate(POS(0, 0, SCREEN_W, 50), "Back", COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);

    ShapeLinkAdd(&menu, TextCenteredCreate(POS(10, 60, SCREEN_W - 20, SCREEN_H - 70), cURLErrBuff, COLOR_WHITE, FONT_TEXT[FSize25]), TextBoxType);

    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

int ShowConnErrMenu(int res){
    char *message = CopyTextArgsUtil("Something went wrong when connecting to the themezer server! Error Code: %d", res);
    ShapeLinker_t *menu = CreateBaseMessagePopup("Connection Error!", message);
    free(message);

    bool showDetails = (cURLErrBuff[0] != '\0');

    ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, (showDetails) ? 390 : 780, 50), COLOR_MAINBG, COLOR_RED, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Alright", FONT_TEXT[FSize28], exitFunc), ButtonType);
    if (showDetails)
        ShapeLinkAdd(&menu, ButtonCreate(POS(640, 470, 390, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Show Details", FONT_TEXT[FSize28], ShowCurlError), ButtonType);

    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

void UpdateMainMenuUI(Context_t *ctx, RequestInfo_t *rI, ShapeLinker_t *items){
    ShapeLinker_t *all = ctx->all;
    ListGrid_t *gv = ShapeLinkFind(all, ListGridType)->item;
    TextCentered_t *pageText = FindTextCenteredByRect(all, POS(920, 0, 240, 60));

    UpdateMainMenuBackground(rI, all);

    if (gv->text)
        ShapeLinkDispose(&gv->text);
    gv->text = items;
    SETBIT(gv->options, LIST_DISABLED, !items);
    gv->highlight = 0;

    if (pageText){
        free(pageText->text.text);
        pageText->text.text = CopyTextArgsUtil("%d/%d (%d)", rI->page, rI->pageCount, rI->itemCount);
    }

    if (items){
        SetMainMenuEmptyMessage(all, " ");
        SetMainMenuNoContentState(all, false);
    }
    else {
        SetMainMenuEmptyMessage(all, " ");
        SetMainMenuNoContentState(all, true);
    }

    Button_t *leftButton = FindButtonByRect(all, POS(800, 0, 120, 60));
    Glyph_t *leftButtonIcon = FindGlyphByPosition(all, 804, 2);
    Button_t *rightButton = FindButtonByRect(all, POS(1160, 0, 120, 60));
    Glyph_t *rightButtonIcon = FindGlyphByPosition(all, 1256, 2);

    if (leftButton && leftButtonIcon && rI->page > 1) {
        SETBIT(leftButtonIcon->options, TEXT_GLYPH_NO_RENDER, 0);
    } else if (leftButton && leftButtonIcon) {
        SETBIT(leftButtonIcon->options, TEXT_GLYPH_NO_RENDER, 1);
    }
    if (rightButton && rightButtonIcon && rI->page < rI->pageCount) {
        SETBIT(rightButtonIcon->options, TEXT_GLYPH_NO_RENDER, 0);
    } else if (rightButton && rightButtonIcon) {
        SETBIT(rightButtonIcon->options, TEXT_GLYPH_NO_RENDER, 1);
    }
}

void ShowLoadingPageUI(Context_t *ctx, RequestInfo_t *rI){
    UpdateMainMenuUI(ctx, rI, NULL);
    SetMainMenuEmptyMessage(ctx->all, "Loading...");
    SetMainMenuNoContentState(ctx->all, false);
    RenderShapeLinkList(ctx->all);
}

int MakeRequestAsCtx(Context_t *ctx, RequestInfo_t *rI){
    ShapeLinker_t *items = NULL;
    int res = -1;

    CleanupTransferInfo(rI);

    printf("Making JSON request...\n");
    if (!(res = MakeJsonRequest(GenLink(rI), &rI->response))){
        printf("JSON request got! parsing...\n");
        if (!(res = GenThemeArray(rI))){
            printf("JSON data parsed!\n");
            items = GenListItemList(rI);
            AddThemeImagesToDownloadQueue(rI, true);

            UpdateMainMenuUI(ctx, rI, items);
        }
    }
    else {
        ShowConnErrMenu(res);
    }

    printf("Res: %d\n", res);
    return res;
}
