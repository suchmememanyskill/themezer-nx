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

int main(int argc, char* argv[])
{
    //consoleInit(NULL);
    InitSDL();
    FontInit();
    romfsInit();
    InitDesign();
    socketInitializeDefault();
    curl_global_init(CURL_GLOBAL_ALL);

    /*
    while(appletMainLoop()){
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        if (kDown & KEY_PLUS)
            break;
        consoleUpdate(NULL);
    }
    */

    //cJSON *res = NULL;
    //GetThemesList("https://api.themezer.ga/", "query{themeList(page:1,limit:5,target:\"ResidentMenu\"){id,creator{display_name},details{name,description}}}", &res);

    RequestInfo_t rI = {0, 20, 1, 0, 0, "", 0, 0, NULL, NULL};
    ShapeLinker_t *items = NULL;

    int res;

    if (!MakeJsonRequest(GenLink(&rI), &rI.response)){
        if (!(res = GenThemeArray(&rI))){
            Log("ayy");
            if (!FillThemeArrayWithImg(&rI)){
                items = GenListItemList(&rI);
            }
            else {
                Log("Something is fucked with the filler");
            }
            
        }
        else
            Log(CopyTextArgsUtil("Theme array gen failed, %d", res));
    }
    else 
        Log("Request failed");
    

    //MakeRequest("https://api.themezer.ga/?query=query{themeList(limit:5){id}}", NULL);

    ShapeLinker_t *mainMenu = CreateMainMenu(items);
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
