#include "gfx.h"

SDL_Texture *menuIcon, *searchIcon, *setIcon, *arrowLIcon, *arrowRIcon, *LeImg, *XIcon, *loadingScreen, *logo; 

void InitTextures(){
    menuIcon = LoadImageSDL("romfs:/menu.png");
    searchIcon = LoadImageSDL("romfs:/search.png");
    setIcon = LoadImageSDL("romfs:/set.png");
    arrowLIcon = LoadImageSDL("romfs:/arrowL.png");
    arrowRIcon = LoadImageSDL("romfs:/arrowR.png");
    LeImg = LoadImageSDL("romfs:/lenny.png");
    XIcon = LoadImageSDL("romfs:/x.png");
    loadingScreen = LoadImageSDL("romfs:/loading.png");
    logo = LoadImageSDL("romfs:/logo.png");
}

void DestroyTextures(){
    SDL_DestroyTexture(menuIcon);
    SDL_DestroyTexture(searchIcon);
    SDL_DestroyTexture(setIcon);
    SDL_DestroyTexture(arrowLIcon);
    SDL_DestroyTexture(arrowRIcon);
    SDL_DestroyTexture(LeImg);
    SDL_DestroyTexture(XIcon);
    SDL_DestroyTexture(loadingScreen);
    SDL_DestroyTexture(logo);
}