// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <sys/stat.h> 
#include <JAGL.h>
#include "design.h"
#include <curl/curl.h>
#include "libs/cJSON.h"
#include "utils.h"

#include "curl.h"

ShapeLinker_t *errorMenu(){
    ShapeLinker_t *out = NULL;
    
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_BLACK, COLOR_BLACK, COLOR_WHITE, COLOR_BLACK, 0, ButtonStyleFlat, "Could not connect to the themezer server. Press A to exit", FONT_TEXT[FSize35], exitFunc), ButtonType);

    return out;
}

ShapeLinker_t *WarnMenu(){
    ShapeLinker_t *warnMenu = CreateBaseMessagePopup("Warning!", "The theme installer could not be found!\nMake sure the theme installer is in the following location:\n\nsd:/switch/NXThemeInstaller.nro");

    ShapeLinkAdd(&warnMenu, RectangleCreate(POS(250, 470, 780, 50), COLOR_CENTERLISTSELECTION, 1), RectangleType);
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
    InitDesign();
    socketInitializeDefault();
    curl_global_init(CURL_GLOBAL_ALL);
    //nxlinkStdio();

    RequestInfo_t rI = {0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, {NULL, 0, NULL, true}};
    SetDefaultsRequestInfo(&rI);
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

    if (!MakeJsonRequest(GenLink(&rI), &rI.response)){
        if (!(res = GenThemeArray(&rI))){
            items = GenListItemList(&rI);
            AddThemeImagesToDownloadQueue(&rI);
        }
        else
            printf(CopyTextArgsUtil("Theme array gen failed, %d", res));
    }
    else 
        printf("Request failed");
    

    ShapeLinker_t *mainMenu = (items != NULL) ? CreateMainMenu(items, &rI) : errorMenu();
    MakeMenu(mainMenu, NULL, (items != NULL) ? HandleDownloadQueue : NULL);
    ShapeLinkDispose(&mainMenu);
    
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

    ExitDesign();
    romfsExit();
    FontExit();
    ExitSDL();
    curl_global_cleanup();
    socketExit();
    //consoleExit(NULL);
    return 0;
}
