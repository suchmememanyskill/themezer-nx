#include "curl.h"
#include <switch.h>
#include <curl/curl.h>
#include "libs/cJSON.h"
#include "gfx/gfx.h"
#include <JAGL.h>
#include "utils.h"

const char *requestTargets[] = {
    "ResidentMenu",
    "Entrance",
    "Flaunch",
    "Set",
    "Psl",
    "MyPage",
    "Notification"
};

const char *requestSorts[] = {
    "UPDATED",
    "DOWNLOADS",
    "SAVES",
    "CREATED"
};

const char *requestOrders[] = {
    "DESC",
    "ASC"
};

char *GenLink(RequestInfo_t *rI){
    char *searchQuoted;
    if (rI->search[0] != '\0')
        searchQuoted = CopyTextArgsUtil("\"%s\"", rI->search);
    else 
        searchQuoted = CopyTextUtil("null");

    char *requestTarget;
    if (rI->target < 0 || rI->target >= 7)
        requestTarget = CopyTextUtil("null");
    else 
        requestTarget = CopyTextArgsUtil("\"%s\"",requestTargets[rI->target]);
    
    static char request[0x600];
    char variables[0x400];
    char *query;
    if (rI->target <= 7)
    {
        // query($target:Target,$page:PositiveInt,$limit:PositiveInt,$sort:ItemSort,$order:SortOrder,$query:String){themes(target:$target,page:$page,limit:$limit,sort:$sort,order:$order,query:$query){nodes{hexId,creator{username},name,description,updatedAt,downloadCount,saveCount,target,previewJpgLargeUrl,previewJpgSmallUrl}}}
        query = "query%28%24target%3ATarget%2C%24page%3APositiveInt%2C%24limit%3APositiveInt%2C%24sort%3AItemSort%2C%24order%3ASortOrder%2C%24query%3AString%29%7Bthemes%28target%3A%24target%2Cpage%3A%24page%2Climit%3A%24limit%2Csort%3A%24sort%2Corder%3A%24order%2Cquery%3A%24query%29%7Bnodes%7BhexId%2Ccreator%7Busername%7D%2Cname%2Cdescription%2CupdatedAt%2CdownloadCount%2CsaveCount%2Ctarget%2CpreviewJpgLargeUrl%2CpreviewJpgSmallUrl%7DpageInfo%7BitemCount%2Climit%2Cpage%2CpageCount%7D%7D%7D";
        snprintf(variables, 0x400,"{\"target\":%s,\"page\":%d,\"limit\":%d,\"sort\":\"%s\",\"order\":\"%s\",\"query\":%s}",\
            requestTarget, rI->page, rI->limit, requestSorts[rI->sort], requestOrders[rI->order], searchQuoted);
    }
    else if (rI->target == 8)
    {
        // query($page:PositiveInt,$limit:PositiveInt,$sort:ItemSort,$order:SortOrder,$query:String){packs(page:$page,limit:$limit,sort:$sort,order:$order,query:$query){nodes{hexId,creator{username},name,description,updatedAt,downloadCount,saveCount,themes{hexId,creator{username},name,description,updatedAt,downloadCount,saveCount,target,previewJpgLargeUrl,previewJpgSmallUrl}}pageInfo{itemCount,limit,page,pageCount}}}
        query = "query%28%24page%3APositiveInt%2C%24limit%3APositiveInt%2C%24sort%3AItemSort%2C%24order%3ASortOrder%2C%24query%3AString%29%7Bpacks%28page%3A%24page%2Climit%3A%24limit%2Csort%3A%24sort%2Corder%3A%24order%2Cquery%3A%24query%29%7Bnodes%7BhexId%2Ccreator%7Busername%7D%2Cname%2Cdescription%2CupdatedAt%2CdownloadCount%2CsaveCount%2Cthemes%7BhexId%2Ccreator%7Busername%7D%2Cname%2Cdescription%2CupdatedAt%2CdownloadCount%2CsaveCount%2Ctarget%2CpreviewJpgLargeUrl%2CpreviewJpgSmallUrl%7D%7DpageInfo%7BitemCount%2Climit%2Cpage%2CpageCount%7D%7D%7D";
        snprintf(variables, 0x400, "{\"page\":%d,\"limit\":%d,\"sort\":\"%s\",\"order\":\"%s\",\"query\":%s}",\
            rI->page, rI->limit, requestSorts[rI->sort], requestOrders[rI->order], searchQuoted);
    }

    CURL *curl = curl_easy_init();
    if(curl) {
        char *output = curl_easy_escape(curl, variables, 0);
        if(output) {
            printf("Encoded: %s\n", output);
            snprintf(request, 0x600, "https://api.themezer.net/graphql?query=%s&variables=%s", query, output);
            curl_free(output);
        }
        else 
        {
            snprintf(request, 0x600, "https://api.themezer.net/graphql?query=%s&variables=%s", query, variables);
        }
        curl_easy_cleanup(curl);
    }

    free(searchQuoted);
    free(requestTarget);
    
    printf("Request: %s\n\n", request);
    return request;
}

int GetIndexOfStrArr(const char **toSearch, int limit, const char *search){
    for (int i = 0; i < limit; i++){
        if (!strcmp(search, toSearch[i]))
            return i;
    }

    return 0;
}

#define CHUNK_SIZE 8192

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb; 
    get_request_t *req = userdata;

    while (req->buflen < req->len + realsize + 1)
    {
        req->buffer = realloc(req->buffer, req->buflen * 2);
        req->buflen *= 2;
    }
    memcpy(&req->buffer[req->len], ptr, realsize);
    req->len += realsize;
    req->buffer[req->len] = 0;

    return realsize;
}

char cURLErrBuff[CURL_ERROR_SIZE] = "";

CURL *CreateRequest(char *url, get_request_t *data){
    CURL *curl = NULL;

    curl = curl_easy_init();
    if (curl){
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "themezer-nx"

        data->buffer = malloc(CHUNK_SIZE);
        data->buflen = CHUNK_SIZE;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, cURLErrBuff);
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

        printf("Buffer: %s\n", req.buffer);
        free(req.buffer);
    }

    curl_easy_cleanup(curl);
    return res;
}

int MakeDownloadRequest(char *url, char *path){
    get_request_t req = {0};
    int res;
    CURL *curl = CreateRequest(url, &req);

    if (!(res = curl_easy_perform(curl))){
        FILE *fp = fopen(path, "wb");
        if (fp){
            fwrite(req.buffer, req.len, 1, fp);
            fclose(fp);
        }
        else {
            res = 1;
        }
    }

    curl_easy_cleanup(curl);
    return res;
}

int hasError(cJSON *root){
    cJSON *err = cJSON_GetObjectItemCaseSensitive(root, "errors");

    if (err){
        cJSON *errItem = cJSON_GetArrayItem(err, 0);
        if (errItem){
            cJSON *messageItem = cJSON_GetObjectItemCaseSensitive(errItem, "message");
            char *message = cJSON_GetStringValue(messageItem);
            
            if (message){
                ShapeLinker_t *menu = CreateBaseMessagePopup("Error during request", message);
                ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, 780, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Ok", FONT_TEXT[FSize28], exitFunc), ButtonType);
                MakeMenu(menu, ButtonHandlerBExit, NULL);
                ShapeLinkDispose(&menu);
            }
        }

        return 1;
    }

    return 0;
}

int DownloadThemeFromUrl(char *url, char *path){
    int res = 1;

    if (url){
        res = MakeDownloadRequest(url, path);
        free(url);
    }

    printf("Res: %d", res);
    return res;
}

#define MIN(x, y) ((x < y) ? x : y)

void FreeThemes(RequestInfo_t *rI){
    if (!rI->themes)
        return;

    for (int i = 0; i < rI->curPageItemCount; i++){
        NNFREE(rI->themes[i].id);
        NNFREE(rI->themes[i].creator);
        NNFREE(rI->themes[i].name);
        NNFREE(rI->themes[i].description);
        NNFREE(rI->themes[i].lastUpdated);
        NNFREE(rI->themes[i].imgLink);
        NNFREE(rI->themes[i].thumbLink);
        if (rI->themes[i].preview && rI->packs == NULL)
            SDL_DestroyTexture(rI->themes[i].preview);
        
        if (rI->packs != NULL){
            free(rI->packs[i].creator);
            free(rI->packs[i].name);
            for (int j = 0; j < rI->packs[i].themeCount; j++){
                free(rI->packs[i].themes[j].id);
                free(rI->packs[i].themes[j].creator);
                free(rI->packs[i].themes[j].name);
                free(rI->packs[i].themes[j].description);
                free(rI->packs[i].themes[j].lastUpdated);
                free(rI->packs[i].themes[j].imgLink);
                free(rI->packs[i].themes[j].thumbLink);
                if  (rI->packs[i].themes[j].preview)
                    SDL_DestroyTexture(rI->packs[i].themes[j].preview);
            }
            free(rI->packs[i].themes);
        }
    }

    NNFREE(rI->themes);
    NNFREE(rI->packs);
}

int ParseThemeList(ThemeInfo_t **storage, int size, cJSON *themesList){
    *storage = calloc(sizeof(ThemeInfo_t), size);
    ThemeInfo_t *themes = *storage;

    cJSON *theme = NULL;
    int i = 0;
    
    cJSON_ArrayForEach(theme, themesList){
        cJSON *id = cJSON_GetObjectItemCaseSensitive(theme, "hexId");
        cJSON *creator = cJSON_GetObjectItemCaseSensitive(theme, "creator");
        cJSON *display_name = cJSON_GetObjectItemCaseSensitive(creator, "username");
        cJSON *name = cJSON_GetObjectItemCaseSensitive(theme, "name");
        cJSON *description = cJSON_GetObjectItemCaseSensitive(theme, "description");
        cJSON *last_updated = cJSON_GetObjectItemCaseSensitive(theme, "updatedAt");
        cJSON *dl_count = cJSON_GetObjectItemCaseSensitive(theme, "downloadCount");
        cJSON *like_count = cJSON_GetObjectItemCaseSensitive(theme, "saveCount");
        cJSON *original = cJSON_GetObjectItemCaseSensitive(theme, "previewJpgLargeUrl");
        cJSON *thumb = cJSON_GetObjectItemCaseSensitive(theme, "previewJpgSmallUrl");
        cJSON *target = cJSON_GetObjectItemCaseSensitive(theme, "target");

        if (cJSON_IsNumber(dl_count) && cJSON_IsNumber(like_count) && cJSON_IsString(last_updated) && (cJSON_IsString(description) || cJSON_IsNull(description)) &&\
        cJSON_IsString(name) && cJSON_IsString(display_name) && cJSON_IsString(id) && cJSON_IsString(original) && cJSON_IsString(thumb) && cJSON_IsString(target)){
                    
            themes[i].dlCount = dl_count->valueint;
            themes[i].likeCount = like_count->valueint;
                    
            themes[i].lastUpdated = CopyTextUtil(last_updated->valuestring);
            if (!cJSON_IsNull(description))
                themes[i].description = CopyTextUtil(description->valuestring);
                    
            themes[i].name = SanitizeString(name->valuestring);
            themes[i].creator = SanitizeString(display_name->valuestring);
            themes[i].id = CopyTextUtil(id->valuestring);
            themes[i].imgLink = CopyTextUtil(original->valuestring);
            themes[i].thumbLink = CopyTextUtil(thumb->valuestring);
            themes[i].target = GetIndexOfStrArr(requestTargets, 7, target->valuestring);
        }
        else {
            return 1;
        }

        i++;
    }

    return 0;
}

int ParsePackList(PackInfo_t **storage, int size, cJSON *packList){
    *storage = calloc(sizeof(PackInfo_t), size);
    PackInfo_t *packs = *storage;

    cJSON *pack = NULL;
    int i = 0;

    cJSON_ArrayForEach(pack, packList){
        cJSON *creator = cJSON_GetObjectItemCaseSensitive(pack, "creator");
        cJSON *display_name = cJSON_GetObjectItemCaseSensitive(creator, "username");
        cJSON *name = cJSON_GetObjectItemCaseSensitive(pack, "name");
        cJSON *themes = cJSON_GetObjectItemCaseSensitive(pack, "themes");

        if (cJSON_IsString(name) && cJSON_IsString(display_name) && cJSON_IsArray(themes)){
            
            packs[i].creator = SanitizeString(display_name->valuestring);
            packs[i].name = SanitizeString(name->valuestring);
            int arraySize = cJSON_GetArraySize(themes);
            printf("Index: %d, size: %d\n", i, arraySize);
            packs[i].themeCount = arraySize;
            if (ParseThemeList(&packs[i].themes, arraySize, themes))
                return 2;
        }
        else {
            return 1;
        }

        i++;
    }

    return 0;
}

void FillThemesWithPacks(RequestInfo_t *rI){
    rI->themes = calloc(sizeof(ThemeInfo_t), rI->curPageItemCount);
    for (int i = 0; i < rI->curPageItemCount; i++){
        rI->themes[i].name = CopyTextUtil(rI->packs[i].name);
        rI->themes[i].creator = CopyTextUtil(rI->packs[i].creator);
        rI->themes[i].thumbLink = CopyTextUtil(rI->packs[i].themes[0].thumbLink);
        rI->themes[i].imgLink = CopyTextUtil(rI->packs[i].themes[0].imgLink);
    }
}

int GenThemeArray(RequestInfo_t *rI){
    if (rI->response == NULL)
        return -1;

    int res = -1;

    if (hasError(rI->response))
        return -4;

    cJSON *data = cJSON_GetObjectItemCaseSensitive(rI->response, "data");
    if (data){
        cJSON *queryData;
        if (rI->target != 8){
            queryData = cJSON_GetObjectItemCaseSensitive(data, "themes");
        } else {
            queryData = cJSON_GetObjectItemCaseSensitive(data, "packs");
        }
        cJSON *pagination = cJSON_GetObjectItemCaseSensitive(queryData, "pageInfo");
        cJSON *page_count = cJSON_GetObjectItemCaseSensitive(pagination, "pageCount");
        cJSON *item_count = cJSON_GetObjectItemCaseSensitive(pagination, "itemCount");

        if (cJSON_IsNumber(page_count) && cJSON_IsNumber(item_count)){
            rI->pageCount = page_count->valueint;
            rI->itemCount = item_count->valueint;
        }
        else
            return -1;


        FreeThemes(rI);
        rI->curPageItemCount = MIN(rI->limit, rI->itemCount - rI->limit * (rI->page - 1));

        if (rI->itemCount <= 0)
            return 0;

        cJSON *nodes = cJSON_GetObjectItemCaseSensitive(queryData, "nodes");
        if (rI->target != 8){
            if (nodes){
                if (ParseThemeList(&rI->themes, rI->curPageItemCount, nodes))
                    return -3;

                res = 0;
                cJSON_Delete(rI->response);
            }
        }
        else {
            if (nodes){
                if (ParsePackList(&rI->packs, rI->curPageItemCount, nodes)){
                    printf("Pack parser failed!");
                    return -3;
                }
                    

                FillThemesWithPacks(rI);

                res = 0;
                cJSON_Delete(rI->response);
            }
        }
    }

    return res;
}



ShapeLinker_t *GenListItemList(RequestInfo_t *rI){
    ShapeLinker_t *link = NULL;

    printf("Gen: ArraySize: %d", rI->curPageItemCount);

    for (int i = 0; i < rI->curPageItemCount; i++){
        ShapeLinkAdd(&link, ListItemCreate(COLOR(255,255,255,255), COLOR(170, 170, 170, 255), (rI->themes[i].preview) ? rI->themes[i].preview : loadingScreen, rI->themes[i].name, rI->themes[i].creator), ListItemType);
    }

    return link;
}

int AddThemeImagesToDownloadQueue(RequestInfo_t *rI, bool thumb){
    if (!rI->curPageItemCount)
        return 0;
        
    rI->tInfo.transfers = calloc(sizeof(Transfer_t), rI->curPageItemCount);
    rI->tInfo.transferer = curl_multi_init();
    rI->tInfo.finished = false;
    curl_multi_setopt(rI->tInfo.transferer, CURLMOPT_MAXCONNECTS, (long)rI->maxDls);
    curl_multi_setopt(rI->tInfo.transferer, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long)rI->maxDls);
    curl_multi_setopt(rI->tInfo.transferer, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    for (int i = 0; i < rI->curPageItemCount; i++){
            rI->tInfo.transfers[i].transfer = CreateRequest((thumb) ? rI->themes[i].thumbLink : rI->themes[i].imgLink, &rI->tInfo.transfers[i].data);
            rI->tInfo.transfers[i].index = i;
            curl_easy_setopt(rI->tInfo.transfers[i].transfer, CURLOPT_PRIVATE, &rI->tInfo.transfers[i].index); 
            curl_multi_add_handle(rI->tInfo.transferer, rI->tInfo.transfers[i].transfer);
    }

    return 0;
}

int CleanupTransferInfo(RequestInfo_t *rI){
    if (rI->tInfo.finished)
        return 0;

    for (int i = 0; i < rI->tInfo.queueOffset; i++){
        curl_multi_remove_handle(rI->tInfo.transferer, rI->tInfo.transfers[i].transfer);
        curl_easy_cleanup(rI->tInfo.transfers[i].transfer);
        free(rI->tInfo.transfers[i].data.buffer);
    }

    curl_multi_cleanup(rI->tInfo.transferer);
    free(rI->tInfo.transfers);
    rI->tInfo.finished = true;
    return 0;
}

int HandleDownloadQueue(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;
    ShapeLinker_t *gvLink = ShapeLinkFind(all, ListGridType);
    ListGrid_t *gv = NULL;
    Image_t *img;

    if (gvLink != NULL)
        gv = gvLink->item;
    else {
        img = ShapeLinkFind(ShapeLinkFind(all, ImageType)->next, ImageType)->item;
    }


    if (rI->tInfo.finished)
        return 0;

    int running_handles = 1;
    curl_multi_perform(rI->tInfo.transferer, &running_handles);

    int msgs_left = -1;
    struct CURLMsg *msg;
    while ((msg = curl_multi_info_read(rI->tInfo.transferer, &msgs_left))){
        if (msg->msg == CURLMSG_DONE){
            CURL *e = msg->easy_handle;
            int *index;
            curl_easy_getinfo(e, CURLINFO_PRIVATE, &index);

            if (msg->data.result != CURLE_OK){
                printf("Something went fucky with the downloader, index %d, %d\n", *index, msg->data.result);
            }
            else {
                printf("Download of index %d finished!\n", *index);
                get_request_t *req = &rI->tInfo.transfers[*index].data;
                rI->themes[*index].preview = LoadImageMemSDL(req->buffer, req->len);
                if (gvLink != NULL){
                    ListItem_t *li = ShapeLinkOffset(gv->text, *index)->item;
                    li->leftImg = rI->themes[*index].preview;
                }
                else {
                    img->texture = rI->themes[*index].preview;
                }
            } 
        }
    }

    if (!running_handles){
        printf("Downloading done!\n");
        CleanupTransferInfo(rI);
    }

    return 0;
}

void SetDefaultsRequestInfo(RequestInfo_t *rI){
    rI->target = 7;
    rI->limit = 20;
    rI->page = 1;
    rI->sort = 0;
    rI->order = 0;
    rI->search = CopyTextUtil("");
    rI->maxDls = 6;
}
