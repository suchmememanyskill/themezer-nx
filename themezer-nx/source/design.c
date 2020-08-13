#include "design.h"
#include <JAGL.h>
#include "utils.h"
#include <unistd.h>
#include <switch.h>
#include <stdio.h>
#include <sys/stat.h>
#include "curl.h"
#include "libs/cJSON.h"

int exitFunc(Context_t *ctx){
    return -1;
}

int ButtonHandlerBExit(Context_t *ctx){
    if (ctx->kHeld & KEY_B)
        return -1;

    return 0;
}

/*

Don't forget to test lImg and RText in LV!

Add topbar with:
- Credits
- Quick launch payload

Add mainmenu with:
- Explore files
- Explore nand (?)
- Explore mods
- Explore payloads
- Dump GC (?)

*/
/*
ShapeLinker_t *CreateCreateMenu(){
    ShapeLinker_t *out = NULL;
    ShapeLinker_t *entries = NULL;

    ShapeLinkAdd(&entries, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Create folder", NULL), ListItemType);
    ShapeLinkAdd(&entries, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Create file", NULL), ListItemType);

    int len = ShapeLinkCount(entries) * 60 + 50;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H)), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,130), 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(200, 50, SCREEN_W - 400, len), COLOR_TOPBAR, 1), RectangleType);
    //ShapeLinkAdd(&out, RectangleCreate(POS(200,0,200,50), COLOR_BUTTONPOWER, 1), RectangleType);
    //ShapeLinkAdd(&out, TextCenteredCreate(POS(200,0,200,50), "Power", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(400, 50, 200, 50), COLOR_TOPBAR, COLOR_BUTTONCREATE, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleTopStrip, "Back", FONT_TEXT[FSize28], exitFunc), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(400,0,200,50), COLOR_BUTTONCREATE, COLOR_BUTTONCREATE, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleFlat, "Create", FONT_TEXT[FSize28], exitFunc), ButtonType);

    ShapeLinkAdd(&out, ListViewCreate(POS(200, 100, SCREEN_W - 400, len - 50), 60, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, LIST_CENTERLEFT, entries, exitFunc, NULL, FONT_TEXT[FSize33]), ListViewType);

    return out;
}
*/


/*
int CreateFolder(Context_t *ctx){
    char *curPath = ShapeLinkFind(ctx->all, DataType)->item;

    char *name = showKeyboard("Give a name for the new folder", 128);
    if (isStringNullOrEmpty(name)){
        if (name != NULL)
            free(name);
    }
    else {
        if (mkdir(fsutil_getnextloc(curPath, name), 0777)){
            // Assuming it already exists. Probably should have better error catching
            return -1;
        }
        else {
            return -1;
        }
    }

    return 0;
}

int CreateFile(Context_t *ctx){
    char *curPath = ShapeLinkFind(ctx->all, DataType)->item;

    char *name = showKeyboard("Give a name for the new file", 128);
    if (isStringNullOrEmpty(name)){
        if (name != NULL)
            free(name);
    }
    else {
        FILE *file = fopen(fsutil_getnextloc(curPath, name), "w");

        if (file == NULL){
            // Assuming it already exists. Probably should have better error catching
            return -1;
        }
        else {
            fclose(file);
            return -1;
        }
    }

    return 0;
}

TopMenuEntry_t CreateMenuEntries[] = {
    {"Create folder", CreateFolder},
    {"Create file", CreateFile}
};

int CreateMenu(Context_t *ctx){
    /*
    ShapeLinker_t *menu = CreateCreateMenu();
    
    Context_t newCtx;
    do {
        newCtx = MakeMenu(menu, ButtonHandlerBExit);

        
        if (newCtx.curOffset == 5){
            ListView_t *lv = newCtx.selected->item;
            char *name;
            char *curPath = ShapeLinkFind(ctx->all, DataType)->item;
            ShapeLinker_t *list = NULL;

            switch(lv->highlight){
                case 0:;
                    name = showKeyboard("Give a name for the new folder", 128);
                    if (isStringNullOrEmpty(name)){
                        if (name != NULL)
                            free(name);
                        
                        continue;
                    }

                    if (mkdir(fsutil_getnextloc(curPath, name), 0777)){
                        // Assuming it already exists
                        newCtx.curOffset = 0;
                        break;
                    }
                    
                    //
                    ShapeLinkAdd(&list, ListItemCreate(COLOR_WHITE, COLOR_WHITE, folderSIcon, name, "<DIR>"), ListItemType);
                    lv = ShapeLinkFind(ctx->all, ListViewType)->item; // Make function to regen file listing instead of this!
                    list->next = lv->text;
                    lv->text = list;
                    lv->highlight = 0;
                    newCtx.curOffset = 0;
                    //
                   newCtx.curOffset = 0;
                    RegenFileListing(ctx);
                    break;
                case 1:;
                    name = showKeyboard("Give a name for the new file", 128);
                    if (isStringNullOrEmpty(name)){
                        if (name != NULL)
                            free(name);
                        
                        continue;
                    }

                    FILE *file = fopen(fsutil_getnextloc(curPath, name), "w");
                    if (file == NULL){
                        // Assuming it already exists
                        newCtx.curOffset = 0;
                        break;
                    }
                    fclose(file);
                    
                    ShapeLinkAdd(&list, ListItemCreate(COLOR_WHITE, COLOR_WHITE,fileSIcon, name, "0 B"), ListItemType);
                    lv = ShapeLinkFind(ctx->all, ListViewType)->item; // Make function to regen file listing instead of this!
                    list->next = lv->text;
                    lv->text = list;
                    lv->highlight = 0;
                    newCtx.curOffset = 0;
                    break;
            }
        }
    } while (newCtx.curOffset == 5);
    */


    //ShapeLinkDispose(&menu);
/*
    ShapeLinker_t path = *ShapeLinkFind(ctx->all, DataType);
    path.next = NULL;
    Context_t newCtx = createTopMenu("Create", COLOR_BUTTONCREATE, 400, CreateMenuEntries, 2, &path);

    if (newCtx.curOffset == 5 && newCtx.origin == OriginFunction){
        RegenFileListing(ctx);
    } 

    return 0;
}
*/
/*
int PowerList(Context_t *ctx){
    ListView_t *lv = ctx->selected->item;
    ListItem_t *item = ShapeLinkOffset(lv->text, lv->highlight)->item;
    
    switch (lv->highlight){
        case 0:
            if (access("/atmosphere/reboot_payload.bin", F_OK) != -1)
                return RebootToPayload("/atmosphere/reboot_payload.bin");

        case 1: // Rollover is intentional here
            if (access("/bootloader/update.bin", F_OK) != -1)
                return RebootToPayload("/bootloader/update.bin");

        case 2:
            if (!strcmp("Reboot to RCM", item->leftText)){
                if (R_FAILED(splInitialize())) 
                    return -1;

                splSetConfig((SplConfigItem) 65001, 1);

                return -1;
            }
        case 3:
            if (R_FAILED(spsmInitialize()))
                return -1;

            spsmShutdown(0);
            return -1;
    }

    return -1;
}
*/
/*
ShapeLinker_t *CreatePowerMenu() {
    ShapeLinker_t *out = NULL;

    ShapeLinker_t *entries = NULL;

    if (access("/atmosphere/reboot_payload.bin", F_OK) != -1)
        ShapeLinkAdd(&entries, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Reboot to atmosphere/reboot_payload.bin", NULL), ListItemType);

    if (access("/bootloader/update.bin", F_OK) != -1)
        ShapeLinkAdd(&entries, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Reboot to bootloader/update.bin", NULL), ListItemType);

    ShapeLinkAdd(&entries, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Reboot to RCM", NULL), ListItemType);
    ShapeLinkAdd(&entries, ListItemCreate(COLOR_WHITE, COLOR_WHITE, NULL, "Power Off", NULL), ListItemType);

    int len = ShapeLinkCount(entries) * 60 + 50;

    SDL_Texture *screenshot = ScreenshotToTexture();
    ShapeLinkAdd(&out, ImageCreate(screenshot, POS(0, 0, SCREEN_W, SCREEN_H)), ImageType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,130), 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(200, 50, SCREEN_W - 400, len), COLOR_TOPBAR, 1), RectangleType);
    //ShapeLinkAdd(&out, RectangleCreate(POS(200,0,200,50), COLOR_BUTTONPOWER, 1), RectangleType);
    //ShapeLinkAdd(&out, TextCenteredCreate(POS(200,0,200,50), "Power", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);
    ShapeLinkAdd(&out, ButtonCreate(POS(200, 50, 200, 50), COLOR_TOPBAR, COLOR_BUTTONPOWER, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleTopStrip, "Back", FONT_TEXT[FSize28], exitFunc), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(200,0,200,50), COLOR_BUTTONPOWER, COLOR_BUTTONPOWER, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleFlat, "Power", FONT_TEXT[FSize28], exitFunc), ButtonType);

    ShapeLinkAdd(&out, ListViewCreate(POS(200, 100, SCREEN_W - 400, len - 50), 60, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, LIST_CENTERLEFT, entries, PowerList, NULL, FONT_TEXT[FSize33]), ListViewType);

    //
    payloadExists = (access("/atmosphere/reboot_payload.bin", F_OK) == -1) ? BUTTON_DISABLED : 0;
    ShapeLinkAdd(&out, ButtonCreate(POS(240, 150, 800, 50), COLOR_DARKGREY, COLOR_AQUA, COLOR_WHITE, COLOR_BLUEGREY, payloadExists, ButtonStyleBorder, "Reboot to atmosphere/reboot_payload.bin", FONT_TEXT[FSize28], RebootToAtmosphere), ButtonType);

    payloadExists = (access("/bootloader/update.bin", F_OK) == -1) ? BUTTON_DISABLED : 0;
    ShapeLinkAdd(&out, ButtonCreate(POS(240, 150 + 80, 800, 50), COLOR_DARKGREY, COLOR_AQUA, COLOR_WHITE, COLOR_BLUEGREY, payloadExists, ButtonStyleBorder, "Reboot to bootloader/update.bin", FONT_TEXT[FSize28], RebootToHekate), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(240, 150 + 160, 800, 50), COLOR_DARKGREY, COLOR_ORANGE, COLOR_WHITE, COLOR_DARKORANGE, 0, ButtonStyleBorder, "Reboot to RCM", FONT_TEXT[FSize28], rebootRCM), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(240, 150 + 240, 800, 50), COLOR_DARKGREY, COLOR_RED, COLOR_WHITE, COLOR_DARKRED, 0, ButtonStyleBorder, "Power Off", FONT_TEXT[FSize28], PowerOff), ButtonType);
    //

    return out;
}
*/
/*
int RebootToAtmosphere(Context_t *ctx){
    return RebootToPayload("/atmosphere/reboot_payload.bin");
}

int RebootToHekate(Context_t *ctx){
    return RebootToPayload("/bootloader/update.bin");
}

int RebootToRCM(Context_t *ctx){
    if (R_FAILED(splInitialize())) 
        return -1;

    splSetConfig((SplConfigItem) 65001, 1);

    return -1;
}

int PowerOff(Context_t *ctx){
    if (R_FAILED(spsmInitialize()))
        return -1;

    spsmShutdown(0);
    return -1;
}

TopMenuEntry_t PowerMenuEntries[] = {
    {"Reboot to atmosphere/reboot_payload.bin", RebootToAtmosphere},
    {"Reboot to bootloader/update.bin", RebootToHekate},
    {"Reboot to RCM", RebootToRCM},
    {"Power Off", PowerOff}
};

int PowerMenu(Context_t *ctx){
    int i = 0;
    TopMenuEntry_t entries[4] = {0};
    if (access("/atmosphere/reboot_payload.bin", F_OK) != -1)
        entries[i++] = PowerMenuEntries[0];
    
    if (access("/bootloader/update.bin", F_OK) != -1)
        entries[i++] = PowerMenuEntries[1];

    entries[i++] = PowerMenuEntries[2];
    entries[i++] = PowerMenuEntries[3];

    createTopMenu("Power", COLOR_BUTTONPOWER, 200, entries, i, NULL);

    //ShapeLinker_t *menu = CreatePowerMenu();

    //MakeMenu(menu, ButtonHandlerBExit);

    //ShapeLinkDispose(&menu);

    return 0;
}

ShapeLinker_t *CreateFileExplorerMenu(ShapeLinker_t *dirList, char *curPath) { // Add functions later
    ShapeLinker_t *out = NULL;

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 100, SCREEN_W, SCREEN_H - 100), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, 100), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(1179, 0, 100, 50), "JAM", COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);

    TextCentered_t *path = TextCenteredCreate(POS(5, 50, SCREEN_W, 50), curPath, COLOR_WHITE, FONT_TEXT[FSize28]);
    ShapeLinkAdd(&out, path, TextCenteredType);
    ShapeLinkAdd(&out, path->text.text, DataType);

    ShapeLinker_t *listitems = ConvertFolderListToListItems(dirList);

    if (listitems != NULL)
        ShapeLinkAdd(&out, ListViewCreate(POS(0, 100, SCREEN_W, SCREEN_H - 100), 40, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, 0, listitems, FolderExplorer, NULL, FONT_TEXT[FSize28]), ListViewType);
    else
        ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 100, SCREEN_W, SCREEN_H - 100), "This directory is empty", COLOR_WHITE, FONT_TEXT[FSize33]), TextCenteredType);

    ShapeLinkAdd(&out, ButtonCreate(POS(0,0,200,50), COLOR_TOPBARBUTTONS, COLOR_BUTTONBACK, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleBottomStrip, "Back", FONT_TEXT[FSize28], exitFunc), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(200,0,200,50), COLOR_TOPBARBUTTONS, COLOR_BUTTONPOWER, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleBottomStrip, "Power", FONT_TEXT[FSize28], PowerMenu), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(400,0,200,50), COLOR_TOPBARBUTTONS, COLOR_BUTTONCREATE, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleBottomStrip, "Create", FONT_TEXT[FSize28], CreateMenu), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(600,0,200,50), COLOR_TOPBARBUTTONS, COLOR_BUTTONCURDIR, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleBottomStrip, "Current Folder", FONT_TEXT[FSize28], NULL), ButtonType);

    return out;
}

int FileExplorer(Context_t *ctx){
    return 0; //Stubbed
}

int FolderExplorer(Context_t *ctx){
    ListView_t *lv;
    ListItem_t *item = NULL;
    if (ctx->curOffset > 0){
        lv = ShapeLinkFind(ctx->all, ListViewType)->item;
        item = ShapeLinkOffset(lv->text, lv->highlight)->item;

        if (strcmp(item->rightText, "<DIR>"))
            return FileExplorer(ctx);
    }

    char *curPath = ShapeLinkFind(ctx->all, DataType)->item;
    char *newPath = curPath;

    if (ctx->curOffset > 0){
        newPath = CopyTextUtil(fsutil_getnextloc(curPath, item->leftText));
    }

    ShapeLinker_t *dirList = ListFolderSorted(newPath);
    ShapeLinker_t *menu = CreateFileExplorerMenu(dirList, newPath);

    if (ctx->curOffset > 0){
        free(newPath);
    }

    Context_t newCtx = MakeMenu(menu, ButtonHandlerBExit);

    FreeFileInfoList(&dirList);
    ShapeLinkDispose(&newCtx.all);
    return 0;
}

int FileExplorerStarter(char *startPath){
    Context_t ctx = {-1, 0, NULL};
    ShapeLinkAdd(&ctx.all, startPath, DataType);

    FolderExplorer(&ctx);
    return 0;
}
*/
/*
ShapeLinker_t *CreateThemeViewMenu() { 
    ShapeLinker_t *out = NULL;

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 50, SCREEN_W, SCREEN_H - 50), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, 50), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(1179, 0, 100, 50), "JAM", COLOR_WHITE, FONT_TEXT[FSize30]), TextCenteredType);

    cJSON *req = NULL;
    ShapeLinker_t *listitems = NULL;
    int temp;
    if ((temp = GetThemesList("http://api.themezer.ga/", "{\"query\": \"query{themesList{details{name} creator{discord_user{username}}}}\"}", &req))){
        //Log(CopyTextArgsUtil("Fuck, the request failed\nErrcode %d\n", temp));
    }
    if (req != NULL){
        listitems = GenListItemsFromJson(req);
        //if (listitems == NULL)
            //Log("Conversion failed!");
        cJSON_Delete(req);
    }
    else {
        //Log("Json failed to parse\n");
    }
    

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 50, SCREEN_W, SCREEN_H - 50), 60, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, 0, listitems, NULL, NULL, FONT_TEXT[FSize33]), ListViewType);

    ShapeLinkAdd(&out, ButtonCreate(POS(0,0,200,50), COLOR_TOPBARBUTTONS, COLOR_BUTTONBACK, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleBottomStrip, "Back", FONT_TEXT[FSize28], exitFunc), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(200,0,200,50), COLOR_TOPBARBUTTONS, COLOR_BUTTONPOWER, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleBottomStrip, "Power", FONT_TEXT[FSize28], PowerMenu), ButtonType);
        
    return out;
}

int ThemeViewMenu(){
    ShapeLinker_t *menu = CreateThemeViewMenu();
    MakeMenu(menu, ButtonHandlerBExit);
    ShapeLinkDispose(&menu);
    return 0;
}

int MainMenu(Context_t *ctx){
    ListView_t *lv = ShapeLinkFind(ctx->all, ListViewType)->item;
    
    switch (lv->highlight){
        case 0:
            //FileExplorerStarter("/");
            ThemeViewMenu();
            break;
    }

    return 0;
}
*/

SDL_Texture *menuIcon, *searchIcon, *setIcon, *sortIcon, *arrowLIcon, *arrowRIcon, *LeImg; 

int lennify(Context_t *ctx){
    static int lenny = false;
    if (!lenny){
        ShapeLinkAdd(&ctx->all, ImageCreate(LeImg, POS(555, 0, 170, 60), 0), ImageType);
        lenny = true;
    }
    return 0;
}

ShapeLinker_t *CreateMainMenu(ShapeLinker_t *listItems) { // Add functions later
    ShapeLinker_t *out = NULL;

    //ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN1, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, "yes", FONT_TEXT[FSize30], NULL), ButtonType);
    //ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN1, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);

    ShapeLinkAdd(&out, RectangleCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), COLOR_CENTERLISTBG, 1), RectangleType);
    ShapeLinkAdd(&out, RectangleCreate(POS(0, 0, SCREEN_W, 60), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&out, TextCenteredCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), "X.X There seem to be no themes here!", COLOR_WHITE, FONT_TEXT[FSize45]), TextCenteredType);

    
    //ShapeLinker_t *listitems = NULL;

    // MenuButton
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN1, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(menuIcon, POS(30, 0, 60, 60), 0), ImageType);

    // SearchButton
    ShapeLinkAdd(&out, ButtonCreate(POS(120, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN2, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(searchIcon, POS(150, 0, 60, 60), 0), ImageType);

    // SettingsButton
    ShapeLinkAdd(&out, ButtonCreate(POS(240, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN3, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(setIcon, POS(270, 0, 60, 60), 0), ImageType);

    // SortButton
    ShapeLinkAdd(&out, ButtonCreate(POS(360, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN4, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(sortIcon, POS(390, 0, 60, 60), 0), ImageType);

    // LeftArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(800, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN5, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowLIcon, POS(830, 0, 60, 60), 0), ImageType);

    // Easter egg
    ShapeLinkAdd(&out, ButtonCreate(POS(480, 0, 320, 60), COLOR_TOPBARBUTTONS, COLOR_TOPBARBUTTONS, COLOR_WHITE, COLOR_TOPBARBUTTONS, BUTTON_NOJOYSEL, ButtonStyleFlat, NULL, NULL, lennify), ButtonType);

    // Text inbetween arrows
    ShapeLinkAdd(&out, TextCenteredCreate(POS(920, 0, 240, 60), "<Not implemented>", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);

    // RightArrow
    ShapeLinkAdd(&out, ButtonCreate(POS(1160, 0, 120, 60), COLOR_TOPBARBUTTONS, COLOR_BTN5, COLOR_WHITE, COLOR_HIGHLIGHT, 0, ButtonStyleBottomStrip, NULL, NULL, NULL), ButtonType);
    ShapeLinkAdd(&out, ImageCreate(arrowRIcon, POS(1190, 0, 60, 60), 0), ImageType);

    ShapeLinkAdd(&out, ListGridCreate(POS(0, 60, SCREEN_W, SCREEN_H - 60), 4, 260, COLOR_CENTERLISTBG, COLOR_CENTERLISTSELECTION, COLOR_CENTERLISTPRESS, (listItems) ? 0 : LIST_DISABLED, listItems, NULL, NULL, FONT_TEXT[FSize28]), ListGridType);
    // 4, 260

    /*
    ShapeLinkAdd(&out, ButtonCreate(POS(0,0,200,50), COLOR_TOPBARBUTTONS, COLOR_BUTTONBACK, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleBottomStrip, "Exit", FONT_TEXT[FSize28], exitFunc), ButtonType);
    ShapeLinkAdd(&out, ButtonCreate(POS(200,0,200,50), COLOR_TOPBARBUTTONS, COLOR_BUTTONPOWER, COLOR_WHITE, COLOR_TOPBARSELECTION, 0, ButtonStyleBottomStrip, "Power", FONT_TEXT[FSize28], PowerMenu), ButtonType);
    ShapeLinkAdd(&out, GlyphCreate(9, 9, BUTTON_PLUS, COLOR_WHITE, FONT_BTN[FSize30]), GlyphType);
    */
        
    return out;
}

void InitDesign(){
    menuIcon = LoadImageSDL("romfs:/menu.png");
    searchIcon = LoadImageSDL("romfs:/search.png");
    setIcon = LoadImageSDL("romfs:/set.png");
    arrowLIcon = LoadImageSDL("romfs:/arrowL.png");
    arrowRIcon = LoadImageSDL("romfs:/arrowR.png");
    sortIcon = LoadImageSDL("romfs:/sort.png");
    LeImg = LoadImageSDL("romfs:/lenny.png");
}

void ExitDesign(){
    SDL_DestroyTexture(menuIcon);
    SDL_DestroyTexture(searchIcon);
    SDL_DestroyTexture(setIcon);
    SDL_DestroyTexture(arrowLIcon);
    SDL_DestroyTexture(arrowRIcon);
    SDL_DestroyTexture(sortIcon);
    SDL_DestroyTexture(LeImg);
}