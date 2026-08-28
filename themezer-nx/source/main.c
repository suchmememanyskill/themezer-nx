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
    InitCurlSession();
    InitHid();
    //nxlinkStdio();

    RequestInfo_t rI = {0};
    SetDefaultsRequestInfo(&rI);
    rI.target = 0;
    rI.contentType = RequestContentPacks;

    AllocateInstalls(INSTALL_QUEUE_COUNT);

    mkdir("/Themes/", 0777);
    mkdir("/Themes/ThemezerNX", 0777);
    mkdir("/Themes/ThemezerNX/Splashes", 0777);

    const char *themeInstallerLocation = GetThemeInstallerPath();
    if (!themeInstallerLocation){
        ShapeLinker_t *warnMenu = WarnMenu();
        MakeMenu(warnMenu, ButtonHandlerBExit, NULL);
        ShapeLinkDispose(&warnMenu);
    }
    else {
        SetInstallButtonState(1);
    }

    while (true){
        ResetBootMenuAction();
        ShapeLinker_t *bootMenu = CreateBootMenu();
        MakeMenu(bootMenu, ButtonHandlerBootMenu, NULL);
        ShapeLinkDispose(&bootMenu);

        if (!ConsumeBootMenuBrowseRequested())
            break;

        if (!RunMainMenu(&rI))
            break;
    }
    
    FreeRequestContent(&rI);

    if (themeInstallerLocation){
        if (CheckIfInstallsQueued()){
            char *installArgs = GetInstallArgs(themeInstallerLocation);
            if (R_SUCCEEDED(envSetNextLoad(themeInstallerLocation, installArgs))){
                printf("Env set!\n");
            }
            else {
                printf("Env ded!\n");
            }
            free(installArgs);
        }
    }

    DestroyTextures();
    romfsExit();
    FontExit();
    ExitSDL();
    CleanupCurlSession();
    curl_global_cleanup();
    socketExit();
    //consoleExit(NULL);
    return 0;
}
