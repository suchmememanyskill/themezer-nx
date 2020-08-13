// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <JAGL.h>
#include "design.h"
#include <curl/curl.h>
#include "libs/cJSON.h"

#include "curl.h"

ShapeLinker_t *errorMenu(){
    ShapeLinker_t *out = NULL;
    
    ShapeLinkAdd(&out, ButtonCreate(POS(0, 0, SCREEN_W, SCREEN_H), COLOR_BLACK, COLOR_BLACK, COLOR_WHITE, COLOR_BLACK, 0, ButtonStyleFlat, "Init failed! Press + or A to exit", FONT_TEXT[FSize35], exitFunc), ButtonType);

    return out;
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

    RequestInfo_t rI = {0, 20, 1, 0, 0, "", 0, 0, 0, NULL, NULL, {NULL, NULL, true}};
    SetDefaultsRequestInfo(&rI);
    ShapeLinker_t *items = NULL;

    int res;

    if (!MakeJsonRequest(GenLink(&rI), &rI.response)){
        if (!(res = GenThemeArray(&rI))){
            /*
            if (!FillThemeArrayWithImg(&rI)){
                items = GenListItemList(&rI);
            }
            else {
                Log("Something is fucked with the filler");
            }
            */
            items = GenListItemList(&rI);
            AddThemeImagesToDownloadQueue(&rI);
        }
        else
            Log(CopyTextArgsUtil("Theme array gen failed, %d", res));
    }
    else 
        Log("Request failed");
    

    

    ShapeLinker_t *mainMenu = (items != NULL) ? CreateMainMenu(items, &rI) : errorMenu();
    MakeMenu(mainMenu, NULL);
    ShapeLinkDispose(&mainMenu);
    
    ExitDesign();
    romfsExit();
    FontExit();
    ExitSDL();
    curl_global_cleanup();
    socketExit();
    //consoleExit(NULL);
    return 0;
}
