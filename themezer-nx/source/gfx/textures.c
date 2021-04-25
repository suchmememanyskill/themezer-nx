#include "gfx.h"

SDL_Texture *menuIcon, *searchIcon, *setIcon, *arrowLIcon, *arrowRIcon, *LeImg, *XIcon, *loadingScreen, *logo;
SDL_Texture *targetIcons[9];
SDL_Texture *sortIcons[4];
SDL_Texture *orderIcons[2];

void InitTextures(){
    // General Icons
    menuIcon = LoadImageSDL("romfs:/menu.png");
    searchIcon = LoadImageSDL("romfs:/search.png");
    setIcon = LoadImageSDL("romfs:/set.png");
    arrowLIcon = LoadImageSDL("romfs:/arrowL.png");
    arrowRIcon = LoadImageSDL("romfs:/arrowR.png");
    LeImg = LoadImageSDL("romfs:/lenny.png");
    XIcon = LoadImageSDL("romfs:/x.png");
    loadingScreen = LoadImageSDL("romfs:/loading.png");
    logo = LoadImageSDL("romfs:/logo.png");

    // Target Icons
    targetIcons[0] = LoadImageSDL("romfs:/targetIcons/home.png");
    targetIcons[1] = LoadImageSDL("romfs:/targetIcons/lock.png");
    targetIcons[2] = LoadImageSDL("romfs:/targetIcons/apps.png");
    targetIcons[3] = LoadImageSDL("romfs:/targetIcons/settings.png");
    targetIcons[4] = LoadImageSDL("romfs:/targetIcons/psl.png");
    targetIcons[5] = LoadImageSDL("romfs:/targetIcons/user.png");
    targetIcons[6] = LoadImageSDL("romfs:/targetIcons/news.png");
    targetIcons[7] = LoadImageSDL("romfs:/targetIcons/all.png");
    targetIcons[8] = LoadImageSDL("romfs:/targetIcons/packs.png");

    // Filter Icons
    sortIcons[0] = LoadImageSDL("romfs:/filterIcons/updated.png");
    sortIcons[1] = LoadImageSDL("romfs:/filterIcons/downloads.png");
    sortIcons[2] = LoadImageSDL("romfs:/filterIcons/likes.png");
    sortIcons[3] = LoadImageSDL("romfs:/filterIcons/created.png");

    // Order Icons
    orderIcons[0] = LoadImageSDL("romfs:/filterIcons/desc.png");
    orderIcons[1] = LoadImageSDL("romfs:/filterIcons/asc.png");
}

void DestroyTextures(){
    // General Icons
    SDL_DestroyTexture(menuIcon);
    SDL_DestroyTexture(searchIcon);
    SDL_DestroyTexture(setIcon);
    SDL_DestroyTexture(arrowLIcon);
    SDL_DestroyTexture(arrowRIcon);
    SDL_DestroyTexture(LeImg);
    SDL_DestroyTexture(XIcon);
    SDL_DestroyTexture(loadingScreen);
    SDL_DestroyTexture(logo);

    // Target Icons
    for (int i = 0; i < 9; i++){
        SDL_DestroyTexture(targetIcons[i]);
    }

    // Filter Icons
    for (int i = 0; i < 4; i++){
        SDL_DestroyTexture(sortIcons[i]);
    }


    // order Icons
    for (int i = 0; i < 2; i++){
        SDL_DestroyTexture(orderIcons[i]);
    }
}

void SetActiveColorTexture(SDL_Texture *texture){
    SetTextureColorSDL(texture, COLOR_FILTERACTIVE);
}

void SetInactiveColorTexture(SDL_Texture *texture){
    SetTextureColorSDL(texture, COLOR_WHITE);
}