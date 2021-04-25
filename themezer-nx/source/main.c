// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <sys/stat.h> 
#include <JAGL.h>
#include <curl/curl.h>
#include "libs/cJSON.h"
#include "utils.h"
#include "gfx/gfx.h"

#include "curl.h"

ShapeLinker_t *errorMenu(char *message, int errLoc){
    ShapeLinker_t *out = NULL;
    
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 50, SCREEN_W, SCREEN_H - 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_MAINBG, 0, ButtonStyleFlat, "Could not connect to the Themezer. Press A to exit", FONT_TEXT[FSize35], exitFunc), ButtonType);
    if (message)
        ShapeLinkAdd(&out, TextCenteredCreate(POS(0, SCREEN_H - 50, 1280, 50), message, COLOR_RED, FONT_TEXT[FSize30]), TextCenteredType);

    bool ShowErrMenu = (errLoc == 1 && cURLErrBuff[0] != '\0');
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, 50), COLOR_TOPBAR, COLOR_BTN4, COLOR_WHITE, COLOR_TOPBARCURSOR, (ShowErrMenu) ? 0 : BUTTON_DISABLED, ButtonStyleTopStrip, (ShowErrMenu) ? "Details" : "Details Unavailable", FONT_TEXT[FSize30], ShowCurlError), ButtonType);

    return out;
}

ShapeLinker_t *WarnMenu(){
    ShapeLinker_t *warnMenu = CreateBaseMessagePopup("Warning!", "The NXThemes Installer could not be found!\nMake sure it is in the following location:\n\nsd:/switch/NXThemesInstaller.nro");

    ShapeLinkAdd(&warnMenu, RectangleCreate(POS(250, 470, 780, 50), COLOR_CURSOR, 1), RectangleType);
    ShapeLinkAdd(&warnMenu, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), COLOR(0,0,0,0), 0, ButtonStyleFlat, NULL, NULL, exitFunc), ButtonType);
    ShapeLinkAdd(&warnMenu, TextCenteredCreate(POS(250, 470, 780, 50), "Alright", COLOR_WHITE, FONT_TEXT[FSize28]), TextCenteredType);

    return warnMenu;
}

int main(int argc, char* argv[])
{
    //consoleInit(NULL);
    InitSDL();
    FontInit();
    romfsInit();
    InitTextures();
    socketInitializeDefault();
    curl_global_init(CURL_GLOBAL_ALL);
    InitHid();
    //nxlinkStdio();

    RequestInfo_t rI = {0, 0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, {NULL, 0, NULL, true}, NULL};
    SetDefaultsRequestInfo(&rI);
    rI.maxDls = 3;
    rI.limit = 12;
    rI.target = 8;
    ShapeLinker_t *items = NULL;

    AllocateInstalls(7);
    int res;

    mkdir("/Themes/", 0777);
    mkdir("/Themes/ThemezerNX", 0777);

    const char *themeInstallerLocation = GetThemeInstallerPath();
    if (!themeInstallerLocation){
        ShapeLinker_t *warnMenu = WarnMenu();
        MakeMenu(warnMenu, ButtonHandlerBExit, NULL);
        ShapeLinkDispose(&warnMenu);
    }
    else {
        SetInstallButtonState(1);
    }

    char *errMessage = NULL;
    int errLoc = 0;

    if (!(res = MakeJsonRequest(GenLink(&rI), &rI.response))){
        if (!(res = GenThemeArray(&rI))){
            items = GenListItemList(&rI);
            AddThemeImagesToDownloadQueue(&rI, true);
        }
        else {
            printf(CopyTextArgsUtil("Theme array gen failed, %d", res));
            errMessage = CopyTextArgsUtil("Parsing Json data failed! Error Code: %d", res);
        }       
    }
    else {
        printf("Request failed");
        errMessage = CopyTextArgsUtil("Themezer request failed! Error Code: %d", res);
        errLoc = 1;
    }
        
    ShapeLinker_t *mainMenu = (items != NULL) ? CreateMainMenu(items, &rI) : errorMenu(errMessage, errLoc);
    MakeMenu(mainMenu, ButtonHandlerMainMenu, (items != NULL) ? HandleDownloadQueue : NULL);
    ShapeLinkDispose(&mainMenu);
    
    FreeThemes(&rI);

    NNFREE(errMessage);

    if (themeInstallerLocation){
        if (CheckIfInstallsQueued()){
            if (R_SUCCEEDED(envSetNextLoad(themeInstallerLocation, GetInstallArgs(themeInstallerLocation)))){
                printf("Env set!\n");
            }
            else {
                printf("Env ded!\n");
            }
        }
    }

    DestroyTextures();
    romfsExit();
    FontExit();
    ExitSDL();
    curl_global_cleanup();
    socketExit();
    //consoleExit(NULL);
    return 0;
}
