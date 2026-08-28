#pragma once
#include <switch.h>
#include <JAGL.h>
#include <curl/curl.h>
#include "libs/cJSON.h"

extern const char *targetOptions[], *sortOptions[], *orderOptions[];

#define SORT_OPTION_COUNT 5
#define TRENDING_SORT_INDEX 0
#define THEME_TARGET_COUNT 7
#define TARGET_OPTION_COUNT 10
#define SPLASH_TARGET_INDEX 9
#define SPLASH_INSTALL_SLOT THEME_TARGET_COUNT
#define INSTALL_QUEUE_COUNT (THEME_TARGET_COUNT + 1)

typedef enum {
    RequestContentPacks = 0,
    RequestContentThemes,
    RequestContentSplashes,
} RequestContentType_t;

typedef struct {
    int sort;
    int order;
    char *search;
} FilterOptions_t;

typedef struct {
    unsigned char *buffer;
    size_t len;
    size_t buflen;
} get_request_t;

typedef struct {
    char *id;
    char *creator;
    char *name;
    char *description;
    char *lastUpdated;
    char *imgLink;
    char *thumbLink;
    char *downloadLink;
    char *packId;
    char *packCreator;
    char *packName;
    int dlCount;
    int likeCount;
    int target;
    SDL_Texture *preview;
} ThemeInfo_t;

typedef struct {
    char *id;
    char *quickId;
    char *creator;
    char *name;
    char *description;
    char *createdAt;
    char *lastUpdated;
    char *imgLink;
    char *thumbLink;
    char *downloadLink;
    int dlCount;
    int likeCount;
    SDL_Texture *preview;
} SplashInfo_t;

typedef enum {
    RemoteInstallKindTheme = 0,
    RemoteInstallKindSplash,
} RemoteInstallKind_t;

typedef struct {
    char *creator;
    char *name;
    char *quickId;
    char *lastUpdated;
    char *downloadLink;
    int target;
    RemoteInstallKind_t kind;
} RemoteInstallInfo_t;

typedef struct { // We are not going to display like half of these
    char *id;
    char *creator;
    char *name;
    //char *description;
    //char *lastUpdated;
    //int dlCount;
    //int likeCount;
    char *imgLink;
    char *thumbLink;
    SDL_Texture *preview;
    int themeCount;
    int isDlDone;
    ThemeInfo_t *themes;
} PackInfo_t;

typedef struct {
    CURL *transfer;
    get_request_t data;
    struct RequestInfo *owner;
    int index;
} Transfer_t;

typedef struct {
    Transfer_t *transfers;
    int queueOffset;
    bool finished;
} TransferInfo_t;

typedef struct RequestInfo {
    int maxDls;
    int target;
    int limit;
    int page;
    int sort;
    int order;
    char *search;
    int pageCount;
    int itemCount;
    int curPageItemCount;
    cJSON *response;
    ThemeInfo_t *themes;
    TransferInfo_t tInfo;
    PackInfo_t *packs;
    SplashInfo_t *splashes;
    RequestContentType_t contentType;
    RemoteInstallInfo_t *remoteInstall;
} RequestInfo_t;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
