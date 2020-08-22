#pragma once
#include <switch.h>
#include <JAGL.h>
#include <curl/curl.h>
#include "libs/cJSON.h"

extern const char *targetOptions[], *sortOptions[], *orderOptions[];

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
    int dlCount;
    int likeCount;
    int target;
    SDL_Texture *preview;
} ThemeInfo_t;

typedef struct {
    CURL *transfer;
    get_request_t data;
    int index;
} Transfer_t;

typedef struct {
    Transfer_t *transfers;
    int queueOffset;
    CURLM *transferer;
    bool finished;
} TransferInfo_t;

typedef struct {
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
} RequestInfo_t;

