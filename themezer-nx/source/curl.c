#include "curl.h"
#include <switch.h>
#include <curl/curl.h>
#include "libs/cJSON.h"
#include "design.h"
#include <JAGL.h>

typedef struct {
    unsigned char *buffer;
    size_t len;
    size_t buflen;
} get_request_t;

const char *requestTargets[] = {
    "ResidentMenu"
};

const char *requestSorts[] = {
    "updated",
    "downloads",
    "likes"
};

const char *requestOrders[] = {
    "desc", 
    "asc"
};

char *GenLink(RequestInfo_t *rI){
    char *searchQuoted;
    if (rI->search[0] != '\0'){
        searchQuoted = malloc(strlen(rI->search) + 3);
        sprintf(searchQuoted, "\"%s\"", rI->search);
    }
    else {
        searchQuoted = CopyTextUtil("null");
    }
    
    static char request[0x400];
    sprintf(request, "https://api.themezer.ga/?query=query{themeList(target:\"%s\",page:%d,limit:%d,sort:\"%s\",order:\"%s\",query:%s){id,creator{display_name},details{name,description},categories,last_updated,dl_count,like_count}}", requestTargets[rI->target], rI->page, rI->limit, requestSorts[rI->sort], requestOrders[rI->order], searchQuoted);
    
    free(searchQuoted);

    return request;
}

char *GenImgLink(char *id){
    static char request[0x50];
    sprintf(request, "https://api.themezer.ga/cdn/themes/%s/screenshot.jpg", id);
    return request;
}

#define CHUNK_SIZE 2048

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb; 
    get_request_t *req = userdata;

    while (req->buflen < req->len + realsize + 1)
    {
        req->buffer = realloc(req->buffer, req->buflen + CHUNK_SIZE);
        req->buflen += CHUNK_SIZE;
    }
    memcpy(&req->buffer[req->len], ptr, realsize);
    req->len += realsize;
    req->buffer[req->len] = 0;

    return realsize;
}

CURL *CreateRequest(char *url, get_request_t *data){
    CURL *curl = NULL;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        data->buffer = malloc(CHUNK_SIZE);
        data->buflen = CHUNK_SIZE;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    }

    return curl;
}

int MakeJsonRequest(char *url, cJSON **response){
    get_request_t req = {0};

    int res;
    CURL *curl = CreateRequest(url, &req);


    if (!(res = curl_easy_perform(curl))){
        if (response != NULL){
            *response = cJSON_Parse(req.buffer);
        }

        //Log(req.buffer);
        free(req.buffer);
    }

    curl_easy_cleanup(curl);
    return res;
}

int MakeImageRequest(char *url, SDL_Texture **img){
    get_request_t req = {0};
    int res;
    CURL *curl = CreateRequest(url, &req);

    if (!(res = curl_easy_perform(curl))){
        if (img){
            *img = LoadImageMemSDL(req.buffer, req.len);
        }
    }

    curl_easy_cleanup(curl);
    return res;
}

void FreeThemes(RequestInfo_t *rI){
    if (!rI->themes)
        return;

    for (int i = 0; i < rI->itemCount; i++){
        free(rI->themes[i].id);
        free(rI->themes[i].creator);
        free(rI->themes[i].name);
        free(rI->themes[i].description);
        free(rI->themes[i].lastUpdated);
        SDL_DestroyTexture(rI->themes[i].preview);
    }

    free(rI->themes);
    rI->themes = NULL;
}

#define MIN(x, y) ((x < y) ? x : y)

int GenThemeArray(RequestInfo_t *rI){
    if (rI->response == NULL)
        return -1;

    int res = -1;

    cJSON *data = cJSON_GetObjectItemCaseSensitive(rI->response, "data");
    if (data){
        cJSON *pagination = cJSON_GetObjectItemCaseSensitive(data, "pagination");
        cJSON *page_count = cJSON_GetObjectItemCaseSensitive(pagination, "page_count");
        cJSON *item_count = cJSON_GetObjectItemCaseSensitive(pagination, "item_count");

        if (cJSON_IsNumber(page_count) && cJSON_IsNumber(item_count)){
            rI->pageCount = page_count->valueint;
            rI->itemCount = item_count->valueint;
        }
        else
            return -1;

        if (rI->itemCount <= 0)
            return -2;

        int size = MIN(rI->itemCount, rI->limit);

        FreeThemes(rI);
        rI->themes = calloc(sizeof(ThemeInfo_t), size);

        cJSON *themesList = cJSON_GetObjectItemCaseSensitive(data, "themeList");

        if (themesList){
            cJSON *theme = NULL;
            int i = 0;

            cJSON_ArrayForEach(theme, themesList){
                cJSON *id = cJSON_GetObjectItemCaseSensitive(theme, "id");
                cJSON *creator = cJSON_GetObjectItemCaseSensitive(theme, "creator");
                cJSON *display_name = cJSON_GetObjectItemCaseSensitive(creator, "display_name");
                cJSON *details = cJSON_GetObjectItemCaseSensitive(theme, "details");
                cJSON *name = cJSON_GetObjectItemCaseSensitive(details, "name");
                cJSON *description = cJSON_GetObjectItemCaseSensitive(details, "description");
                cJSON *last_updated = cJSON_GetObjectItemCaseSensitive(theme, "last_updated");
                cJSON *dl_count = cJSON_GetObjectItemCaseSensitive(theme, "dl_count");
                cJSON *like_count = cJSON_GetObjectItemCaseSensitive(theme, "like_count");

                if (cJSON_IsNumber(dl_count) && cJSON_IsNumber(like_count) && cJSON_IsString(last_updated) && (cJSON_IsString(description) || cJSON_IsNull(description)) &&\
                cJSON_IsString(name) && cJSON_IsString(display_name) && cJSON_IsString(id)){
                    
                    rI->themes[i].dlCount = dl_count->valueint;
                    rI->themes[i].likeCount = like_count->valueint;
                    
                    rI->themes[i].lastUpdated = CopyTextUtil(last_updated->valuestring);
                    if (cJSON_IsNull(description))
                        rI->themes[i].description = CopyTextUtil("no description");
                    else
                        rI->themes[i].description = CopyTextUtil(description->valuestring);
                    
                    rI->themes[i].name = CopyTextUtil(name->valuestring);
                    rI->themes[i].creator = CopyTextUtil(display_name->valuestring);
                    rI->themes[i].id = CopyTextUtil(id->valuestring);
                    
                    // Add preview later!
                }
                else {
                    return -3;
                }

                i++;
            }

            res = 0;
        }
    }

    return res;
}

ShapeLinker_t *GenListItemList(RequestInfo_t *rI){
    ShapeLinker_t *link = NULL;
    int size = MIN(rI->itemCount, rI->limit);

    for (int i = 0; i < size; i++){
        ShapeLinkAdd(&link, ListItemCreate(COLOR(255,255,255,255), COLOR(170, 170, 170, 255), rI->themes[i].preview, rI->themes[i].name, rI->themes[i].creator), ListItemType);
    }

    return link;
}


int FillThemeArrayWithImg(RequestInfo_t *rI){
    int size = MIN(rI->itemCount, rI->limit);
    int res = 0;

    ShapeLinker_t *link = NULL;
    ShapeLinkAdd(&link, RectangleCreate(POS(300, 150, SCREEN_W - 600, SCREEN_H - 300), COLOR_TOPBAR, 1), RectangleType);
    ShapeLinkAdd(&link, TextCenteredCreate(POS(300, 150, SCREEN_W - 600, SCREEN_H - 350), "Downloading theme images...", COLOR_WHITE, FONT_TEXT[FSize35]), TextCenteredType);
    ProgressBar_t *pb = ProgressBarCreate(POS(300, 520, SCREEN_W - 600, 50), COLOR_GREEN, COLOR_WHITE, ProgressBarStyleSize, 0);
    ShapeLinkAdd(&link, pb, ProgressBarType);

    for (int i = 0; i < size; i++){
        if (!rI->themes[i].preview){
            pb->percentage = i * 100 / size;  
            RenderShapeLinkList(link);
            if ((res = MakeImageRequest(GenImgLink(rI->themes[i].id), &rI->themes[i].preview))){
                return -1;
            }
                
        }
    }

    return 0;
}