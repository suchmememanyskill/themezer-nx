#include "design.h"
#include <JAGL.h>
#include "utils.h"
#include <unistd.h>
#include <switch.h>
#include <stdio.h>
#include <sys/stat.h>
#include "curl.h"
#include "libs/cJSON.h"
#include <sys/stat.h> 

int exitFunc(Context_t *ctx){
    return -1;
}

int ButtonHandlerBExit(Context_t *ctx){
    if (ctx->kHeld & KEY_B)
        return -1;

    return 0;
}

const char *targetOptions[] = {
    "Home Menu",
    "Lock Screen",
    "All Apps",
    "Player Select",
    "Settings",
    "User Page",
    "News"
};

typedef struct {
    int sort;
    int order;
    char *search;
} FilterOptions_t;

const char *sortOptions[] = {
    "Last updated",
    "Downloads",
    "Likes",
    "Creation date"
};

const char *orderOptions[] = {
    "Descending",
    "Ascending"
};

int InstallButtonState = 0;

void SetInstallButtonState(int state){
    InstallButtonState = state;
}

SDL_Texture *menuIcon, *searchIcon, *setIcon, *arrowLIcon, *arrowRIcon, *LeImg, *XIcon, *loadingScreen; 

int lennify(Context_t *ctx){
    static int lenny = false;
    if (!lenny){
        ShapeLinkAdd(&ctx->all, ImageCreate(LeImg, POS(525, 0, 170, 60), 0), ImageType);
        lenny = true;
    }
    return 0;
}

ShapeLinker_t *CreateSideBaseMenu(char *menuName){ // Count: 7
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 50, 400, SCREEN_H - 50), COLOR_CENTERLISTBG, 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, 350, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 0, 400, 50), menuName, COLOR_WHITE, FONT_TEXT[FSize23]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(350, 0, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
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
    ShapeLinkAdd(&out, RectangleCreate(POS(250, 250, SCREEN_W - 500, 220), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(260, 260, SCREEN_W - 520, 200), message, COLOR_WHITE, FONT_TEXT[FSize28]), TextBoxType);

    return out;
}

int EnlargePreviewImage(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    ThemeInfo_t *target = ShapeLinkFind(all, DataType)->item;

    ShapeLinker_t *menu = NULL;
    ShapeLinkAdd(&menu, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&menu, ImageCreate(target->preview, POS(0, 0, SCREEN_W, SCREEN_H), 0), ImageType);

    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

int DownloadThemeButton(Context_t *ctx){
    ShapeLinker_t *targetLink = ShapeLinkFind(ctx->all, DataType);
    ThemeInfo_t *target = targetLink->item;
    RequestInfo_t *rI = targetLink->next->item;

    ShapeLinker_t *render = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&render, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&render, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,200), 1), RectangleType);
    TextCentered_t *text = TextCenteredCreate(POS(0, 0, SCREEN_W, SCREEN_H), "Downloading Theme...", COLOR_WHITE, FONT_TEXT[FSize45]);
    ShapeLinkAdd(&render, text, TextCenteredType);

    RenderShapeLinkList(render);

    char *path = GetThemePath(target->creator, target->name, targetOptions[rI->target]);
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
    ShapeLinker_t *out = CreateBaseMessagePopup("Install Queued!", "Install Queued. Exit the app to apply the theme.\nPress A to return");

    ShapeLinkAdd(&out, RectangleCreate(POS(250, 470, 780, 50), COLOR_CENTERLISTSELECTION, 1), RectangleType);
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(250, 470, 780, 50), "Got it!", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);

    int res = DownloadThemeButton(ctx);
    if (!res){
        ShapeLinker_t *targetLink = ShapeLinkFind(ctx->all, DataType);
        ThemeInfo_t *target = targetLink->item;
        RequestInfo_t *rI = targetLink->next->item;
        char *path = GetThemePath(target->creator, target->name, targetOptions[rI->target]);

        SetInstallSlot(rI->target, path);

        MakeMenu(out, ButtonHandlerBExit, NULL);
    }

    ShapeLinkDispose(&out);

    return 0;
}

ShapeLinker_t *CreateSelectMenu(ThemeInfo_t *target, RequestInfo_t *rI){
    ShapeLinker_t *out = NULL;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H), IMAGE_CLEANUPTEX), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,170), 1), RectangleType);

    ShapeLinkAdd(&out, RectangleCreate(POS(50, 100, SCREEN_W - 100, SCREEN_H - 150), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(50, 50, SCREEN_W - 150, 50), COLOR_TOPBAR, 1), RectangleType);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(55, 50, 0 /* 0 width left alligns it */, 50), target->name, COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);

    ShapeLinkAdd(&out, ButtonCreate(POS(SCREEN_W - 100, 50, 50, 50), COLOR_TOPBAR, COLOR_RED, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(XIcon, POS(SCREEN_W - 100, 50, 50, 50), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(50, 100, 860, 488), COLOR_CENTERLISTBG, COLOR_WHITE, COLOR_WHITE, COLOR_CENTERLISTSELECTION, 0, ButtonStyleFlat, NULL, NULL, EnlargePreviewImage), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(target->preview, POS(55, 105, 850, 478), 0), ImageType);

    ShapeLinkAdd(&out, ButtonCreate(POS(915, 110, SCREEN_W - 980, 60), COLOR_INSTBTN, COLOR_GREEN, COLOR_WHITE, COLOR_INSTBTNSEL, (InstallButtonState) ? 0 : BUTTON_DISABLED, ButtonStyleFlat, "Install Theme", FONT_TEXT[FSize30], InstallThemeButton), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(915, 180, SCREEN_W - 980, 60), COLOR_DLBTN, COLOR_GREEN, COLOR_WHITE, COLOR_DLBTNSEL, 0, ButtonStyleFlat, "Download Theme", FONT_TEXT[FSize30], DownloadThemeButton), ButtonType);

    char *info = CopyTextArgsUtil("Creator: %s\n\nLast Updated: %s\n\nDownload Count: %d\nLike Count: %d\nID: t%s", target->creator, target->lastUpdated, target->dlCount, target->likeCount, target->id);
    char *desc = CopyTextArgsUtil("Description: %s", target->description);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 250, SCREEN_W - 990, 420), info, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(60, 593, SCREEN_W - 120, 80), desc, COLOR_WHITE, FONT_TEXT[FSize23]), TextBoxType);

    free(info);
    free(desc);
    //ShapeLinkAdd()

    ShapeLinkAdd(&out, target, DataType);
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

    ShapeLinker_t *menu = CreateSelectMenu(target, rI);
    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

int MakeRequestAsCtx(Context_t *ctx, RequestInfo_t *rI){
    ShapeLinker_t *items = NULL;
    int res = -1;

    CleanupTransferInfo(rI);

    printf("Making JSON request...\n");
    if (!MakeJsonRequest(GenLink(rI), &rI->response)){
        printf("JSON request got! parsing...\n");
        if (!(res = GenThemeArray(rI))){
            printf("JSON data parsed!\n");
            items = GenListItemList(rI);
            AddThemeImagesToDownloadQueue(rI);

            ShapeLinker_t *all = ctx->all;
            ListGrid_t *gv = ShapeLinkFind(all, ListGridType)->item;
            TextCentered_t *pageText = ShapeLinkFind(all, TextCenteredType)->item;
            if (gv->text)
                ShapeLinkDispose(&gv->text);
            
            gv->text = items;
            SETBIT(gv->options, LIST_DISABLED, !items);
            gv->highlight = 0;
            free(pageText->text.text);
            pageText->text.text = CopyTextArgsUtil("Page %d/%d", rI->page, rI->pageCount);
            
        }
    }

    printf("Res: %d\n", res);
    return res;
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

    char *out = showKeyboard("Input search terms. Max 100 characters", 100);

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

        MakeRequestAsCtx(ctx, rI);
    }
    else if (options.search != NULL)
        free(options.search);

    return 0;
}



ShapeLinker_t *CreateSideTargetMenu(){
    ShapeLinker_t *out = CreateSideBaseMenu("Select a target:");

    ShapeLinker_t *list = NULL;
    for (int i = 0; i < 7; i++)
        ShapeLinkAdd(&list, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, targetOptions[i], NULL), ListItemType);

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 50, 400, SCREEN_H - 100), 75, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, LIST_CENTERLEFT, list, exitFunc, NULL, FONT_TEXT[FSize30]), ListViewType);

    ShapeLinkAdd(&out, ButtonCreate(POS(0, SCREEN_H - 50, 400, 50), COLOR_CENTERLISTBG, COLOR_RED, COLOR_WHITE, COLOR_CENTERLISTSELECTION, 0, ButtonStyleBottomStrip, "Exit Themezer-NX", FONT_TEXT[FSize25], exitFunc), ButtonType);

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

int HandleQueueList(Context_t *ctx){
    ListView_t *lv = ShapeLinkFind(ctx->all, ListViewType)->item;

    if (!CheckIfInstallSlotIsFree(lv->highlight)){
        char *message = CopyTextArgsUtil("Are you sure you want to remove the %s's queued install?", targetOptions[lv->highlight]);
        ShapeLinker_t *menu = CreateBaseMessagePopup("Remove Queued item?", message);
        free(message);

        ShapeLinkAdd(&menu, ButtonCreate(POS(640, 470, 390, 50), COLOR_CENTERLISTBG, COLOR_CENTERLISTPRESS, COLOR_WHITE, COLOR_CENTERLISTSELECTION, 0, ButtonStyleBottomStrip, "No", FONT_TEXT[FSize28], exitFunc), ButtonType);
        ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, 390, 50), COLOR_CENTERLISTBG, COLOR_RED, COLOR_WHITE, COLOR_CENTERLISTSELECTION, 0, ButtonStyleBottomStrip, "Yes", FONT_TEXT[FSize28], exitFunc), ButtonType);

        Context_t menuCtx = MakeMenu(menu, ButtonHandlerBExit, NULL);
        ShapeLinkDispose(&menu);

        if (menuCtx.curOffset == 7 && menuCtx.origin == OriginFunction){
            SetInstallSlot(lv->highlight, NULL);
            ShapeLinkDispose(&lv->text);

            ShapeLinker_t *text = NULL;
            for (int i = 0; i < 7; i++){
                char *t = CopyTextArgsUtil("%s: %s", targetOptions[i], (CheckIfInstallSlotIsFree(i)) ? "Not Queued" : "Queued");
                ShapeLinkAdd(&text, ListItemCreate((CheckIfInstallSlotIsFree(i)) ? COLOR_WHITE : COLOR_GREEN, COLOR_WHITE, NULL, t, NULL), ListItemType);
                free(t);
            }

            lv->text = text;
        }
    }

    return 0;
}

ShapeLinker_t *CreateSideQueueMenu(){
    ShapeLinker_t *out = CreateSideBaseMenu("Currently queued installs:");

    ShapeLinker_t *text = NULL;
    for (int i = 0; i < 7; i++){
        char *t = CopyTextArgsUtil("%s: %s", targetOptions[i], (CheckIfInstallSlotIsFree(i)) ? "Not Queued" : "Queued");
        ShapeLinkAdd(&text, ListItemCreate((CheckIfInstallSlotIsFree(i)) ? COLOR_WHITE : COLOR_GREEN, COLOR_WHITE, NULL, t, NULL), ListItemType);
        free(t);
    }

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 50, 400, SCREEN_H - 100), 75, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, 0, text, HandleQueueList, NULL, FONT_TEXT[FSize28]), ListViewType);

    return out;
}

int ShowSideQueueMenu(Context_t *ctx){
    ShapeLinker_t *menu = CreateSideQueueMenu();
    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}

ShapeLinker_t *CreateMainMenu(ShapeLinker_t *listItems, RequestInfo_t *rI) { 
    ShapeLinker_t *out = NULL;

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, 60), COLOR_TOPBAR, 1), RectangleType);

    // Text inbetween arrows
    char *temp = CopyTextArgsUtil("Page %d/%d", rI->page, rI->pageCount);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 0, 240, 60), temp, COLOR_WHITE, FONT_TEXT[FSize25]), TextCenteredType);
    free(temp);

    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), "X.X There seem to be no themes here!", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);


    // MenuButton
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN1, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideTargetMenu), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(menuIcon, POS(30, 0, 60, 60), 0), ImageType);

    // SearchButton
    ShapeLinkAdd(&out, ButtonCreate(POS(120, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN2, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideFilterMenu), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(searchIcon, POS(150, 0, 60, 60), 0), ImageType);

    if (InstallButtonState){
        // SettingsButton
        ShapeLinkAdd(&out, ButtonCreate(POS(240, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN3, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideQueueMenu), ButtonType);
        ShapeLinkAdd(&out, ImageCreate(setIcon, POS(270, 0, 60, 60), 0), ImageType);
    }

    // SortButton
    //ShapeLinkAdd(&out, ButtonCreate(POS(360, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN4, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, ShowSideFilterMenu), ButtonType);
    //ShapeLinkAdd(&out, ImageCreate(sortIcon, POS(390, 0, 60, 60), 0), ImageType);

    // LeftArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(800, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN5, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, PrevPageButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowLIcon, POS(830, 0, 60, 60), 0), ImageType);

    // Easter egg
    ShapeLinkAdd(&out, ButtonCreate(POS(480, 0, 320, 60), COLOR_TOPBARBUTTONS, COLOR_TOPBARBUTTONS, COLOR_WHITE, COLOR_TOPBARBUTTONS, BUTTON_NOJOYSEL, ButtonStyleFlat, NULL, NULL, lennify), ButtonType);

    // RightArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(1160, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN5, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NextPageButton), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowRIcon, POS(1190, 0, 60, 60), 0), ImageType);

    ShapeLinkAdd(&out, ListGridCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), 4, 260, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, (listItems) ? 0 : LIST_DISABLED, listItems, ThemeSelect, NULL, FONT_TEXT[FSize23]), ListGridType);
    // 4, 260
        
    ShapeLinkAdd(&out, rI, DataType);

    return out;
}

void InitDesign(){
    menuIcon = LoadImageSDL("romfs:/menu.png");
    searchIcon = LoadImageSDL("romfs:/search.png");
    setIcon = LoadImageSDL("romfs:/set.png");
    arrowLIcon = LoadImageSDL("romfs:/arrowL.png");
    arrowRIcon = LoadImageSDL("romfs:/arrowR.png");
    LeImg = LoadImageSDL("romfs:/lenny.png");
    XIcon = LoadImageSDL("romfs:/x.png");
    loadingScreen = LoadImageSDL("romfs:/loading.png");
}

void ExitDesign(){
    SDL_DestroyTexture(menuIcon);
    SDL_DestroyTexture(searchIcon);
    SDL_DestroyTexture(setIcon);
    SDL_DestroyTexture(arrowLIcon);
    SDL_DestroyTexture(arrowRIcon);
    SDL_DestroyTexture(LeImg);
    SDL_DestroyTexture(XIcon);
    SDL_DestroyTexture(loadingScreen);
}