#include "gfx.h"

#define BG_TILE_ALPHA 77
#define BG_THUMBHASH_ALPHA 128

static void SetTextureAlpha(SDL_Texture *texture, Uint8 alpha){
    if (!texture)
        return;

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(texture, alpha);
}

SDL_Texture *menuIcon, *searchIcon, *queueIcon, *arrowLIcon, *arrowRIcon, *LeImg, *XIcon, *logo, *icon, *banner, *bgTile, *themeBgThumbHash, *packBgThumbHash, *moodDown, *browseIcon, *quickIdIcon;
SDL_Texture *targetIcons[9];
SDL_Texture *sortIcons[SORT_OPTION_COUNT];
SDL_Texture *orderIcons[2];

void InitTextures(){
    // General Icons
    menuIcon = LoadImageSDL("romfs:/menu.png");
    searchIcon = LoadImageSDL("romfs:/search.png");
    queueIcon = LoadImageSDL("romfs:/queue.png");
    arrowLIcon = LoadImageSDL("romfs:/arrowL.png");
    arrowRIcon = LoadImageSDL("romfs:/arrowR.png");
    browseIcon = LoadImageSDL("romfs:/bootscreen/browse.png");
    quickIdIcon = LoadImageSDL("romfs:/bootscreen/quickId.png");
    LeImg = LoadImageSDL("romfs:/lenny.png");
    XIcon = LoadImageSDL("romfs:/x.png");
    logo = LoadImageSDL("romfs:/logo.png");
    icon = LoadImageSDL("romfs:/icon.png");
    banner = LoadImageSDL("romfs:/banner.jpg");
    bgTile = LoadImageSDL("romfs:/bgTile.png");
    moodDown = LoadImageSDL("romfs:/moodDown.png");
    themeBgThumbHash = CreateThumbHashTexture("GreAA4AYlEe/aUowqjD6lTRnblczZgY");
    packBgThumbHash = CreateThumbHashTexture("21aDA4AYhKa/OExQa+kF+DRnblczZgY");

    SetTextureAlpha(bgTile, BG_TILE_ALPHA);
    SetTextureAlpha(themeBgThumbHash, BG_THUMBHASH_ALPHA);
    SetTextureAlpha(packBgThumbHash, BG_THUMBHASH_ALPHA);

    // Target Icons
    targetIcons[0] = LoadImageSDL("romfs:/targetIcons/packs.png");
    targetIcons[1] = LoadImageSDL("romfs:/targetIcons/home.png");
    targetIcons[2] = LoadImageSDL("romfs:/targetIcons/lock.png");
    targetIcons[3] = LoadImageSDL("romfs:/targetIcons/apps.png");
    targetIcons[4] = LoadImageSDL("romfs:/targetIcons/set.png");
    targetIcons[5] = LoadImageSDL("romfs:/targetIcons/psl.png");
    targetIcons[6] = LoadImageSDL("romfs:/targetIcons/user.png");
    targetIcons[7] = LoadImageSDL("romfs:/targetIcons/news.png");
    targetIcons[8] = LoadImageSDL("romfs:/targetIcons/all.png");

    // Filter Icons
    sortIcons[0] = LoadImageSDL("romfs:/filterIcons/trending.png");
    sortIcons[1] = LoadImageSDL("romfs:/filterIcons/created.png");
    sortIcons[2] = LoadImageSDL("romfs:/filterIcons/updated.png");
    sortIcons[3] = LoadImageSDL("romfs:/filterIcons/downloads.png");
    sortIcons[4] = LoadImageSDL("romfs:/filterIcons/saves.png");

    // Order Icons
    orderIcons[0] = LoadImageSDL("romfs:/filterIcons/desc.png");
    orderIcons[1] = LoadImageSDL("romfs:/filterIcons/asc.png");
}

void DestroyTextures(){
    // General Icons
    SDL_DestroyTexture(menuIcon);
    SDL_DestroyTexture(searchIcon);
    SDL_DestroyTexture(queueIcon);
    SDL_DestroyTexture(arrowLIcon);
    SDL_DestroyTexture(arrowRIcon);
    SDL_DestroyTexture(quickIdIcon);
    SDL_DestroyTexture(LeImg);
    SDL_DestroyTexture(XIcon);
    SDL_DestroyTexture(logo);
    SDL_DestroyTexture(icon);
    SDL_DestroyTexture(banner);
    SDL_DestroyTexture(bgTile);
    SDL_DestroyTexture(moodDown);
    SDL_DestroyTexture(browseIcon);
    SDL_DestroyTexture(themeBgThumbHash);
    SDL_DestroyTexture(packBgThumbHash);

    // Target Icons
    for (int i = 0; i < 9; i++){
        SDL_DestroyTexture(targetIcons[i]);
    }

    // Filter Icons
    for (int i = 0; i < SORT_OPTION_COUNT; i++){
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
