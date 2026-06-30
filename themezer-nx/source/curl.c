#include "curl.h"
#include <switch.h>
#include <curl/curl.h>
#include <mbedtls/base64.h>
#include <string.h>
#include "libs/cJSON.h"
#include "gfx/gfx.h"
#include <JAGL.h>
#include "thumbhash.h"
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
    "RISING",
    "TRENDING",
    "CREATED",
    "UPDATED",
    "DOWNLOADS",
    "SAVES"
};

const char *requestOrders[] = {
    "DESC",
    "ASC"
};

static int GetPreviewUrls(cJSON *item, const char *fieldName, cJSON **original, cJSON **thumb);
static int ParseThemeList(ThemeInfo_t **storage, int size, cJSON *themesList);
int GetIndexOfStrArr(const char **toSearch, int limit, const char *search);
static int ShowApiError(cJSON *root);
static void SetThemePackInfo(ThemeInfo_t *themeInfo, const char *packId, const char *packCreator, const char *packName);
static char *CopyJsonStringLiteral(const char *text);
static CURLM *GetTransferer(void);
static CURLcode PerformRequest(CURL *curl);
static void CleanupActiveTransferQueue(RequestInfo_t *except);
static void HandleCompletedTransfer(Transfer_t *transfer, CURLcode result, Context_t *ctx);
static bool AreTransfersFinished(RequestInfo_t *rI);

static CURLM *sTransferer = NULL;
static RequestInfo_t *sActiveTransferQueue = NULL;

int InitCurlSession(void){
    if (sTransferer)
        return 0;

    sTransferer = curl_multi_init();
    if (!sTransferer)
        return 1;

    curl_multi_setopt(sTransferer, CURLMOPT_MAXCONNECTS, 12L);
    curl_multi_setopt(sTransferer, CURLMOPT_MAX_TOTAL_CONNECTIONS, 12L);
    curl_multi_setopt(sTransferer, CURLMOPT_MAX_HOST_CONNECTIONS, 12L);
    curl_multi_setopt(sTransferer, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
    return 0;
}

void CleanupCurlSession(void){
    CleanupActiveTransferQueue(NULL);

    if (!sTransferer)
        return;

    curl_multi_cleanup(sTransferer);
    sTransferer = NULL;
}

static CURLM *GetTransferer(void){
    if (!sTransferer)
        InitCurlSession();

    return sTransferer;
}

static void CleanupActiveTransferQueue(RequestInfo_t *except){
    if (sActiveTransferQueue && sActiveTransferQueue != except)
        CleanupTransferInfo(sActiveTransferQueue);
}

static char *GenLookupByQuickIdLink(const char *quickId){
    static char request[0x1200];
    request[0] = '\0';
    const char *query = "query($quickId:String!){switch{lookupByQuickId(quickId:$quickId){__typename ... on SwitchPack{hexId name creator{username} collageThumbHash collagePreview{hdUrl thumbUrl} themes{hexId creator{username} name description updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl}} ... on SwitchTheme{hexId creator{username} name description updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl pack{hexId name creator{username}}} ... on SwitchRemoteInstallTheme{author createdAt downloadUrl name quickId target}}}}";
    char *variables = NULL;

    cJSON *variablesJson = cJSON_CreateObject();
    if (variablesJson != NULL){
        cJSON_AddStringToObject(variablesJson, "quickId", quickId);
        variables = cJSON_PrintUnformatted(variablesJson);
        cJSON_Delete(variablesJson);
    }

    CURL *curl = curl_easy_init();
    if (curl){
        char *encodedQuery = curl_easy_escape(curl, query, 0);
        char *encodedVariables = curl_easy_escape(curl, variables ? variables : "{}", 0);

        snprintf(request, sizeof(request), "https://api.themezer.net/graphql?query=%s&variables=%s", encodedQuery ? encodedQuery : query, encodedVariables ? encodedVariables : (variables ? variables : "{}"));

        if (encodedQuery)
            curl_free(encodedQuery);
        if (encodedVariables)
            curl_free(encodedVariables);
        curl_easy_cleanup(curl);
    }
    else {
        snprintf(request, sizeof(request), "https://api.themezer.net/graphql?query=%s&variables=%s", query, variables ? variables : "{}");
    }

    free(variables);

    printf("Request: %s\n\n", request);
    return request;
}

static int ParseTheme(ThemeInfo_t *themeInfo, cJSON *theme){
    cJSON *id = cJSON_GetObjectItemCaseSensitive(theme, "hexId");
    cJSON *creator = cJSON_GetObjectItemCaseSensitive(theme, "creator");
    cJSON *display_name = cJSON_GetObjectItemCaseSensitive(creator, "username");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(theme, "name");
    cJSON *description = cJSON_GetObjectItemCaseSensitive(theme, "description");
    cJSON *last_updated = cJSON_GetObjectItemCaseSensitive(theme, "updatedAt");
    cJSON *dl_count = cJSON_GetObjectItemCaseSensitive(theme, "downloadCount");
    cJSON *like_count = cJSON_GetObjectItemCaseSensitive(theme, "saveCount");
    cJSON *original = NULL;
    cJSON *thumb = NULL;
    cJSON *thumb_hash = cJSON_GetObjectItemCaseSensitive(theme, "screenshotThumbHash");
    cJSON *download = cJSON_GetObjectItemCaseSensitive(theme, "downloadUrl");
    cJSON *target = cJSON_GetObjectItemCaseSensitive(theme, "target");
    cJSON *pack = cJSON_GetObjectItemCaseSensitive(theme, "pack");

    if (!GetPreviewUrls(theme, "screenshotPreview", &original, &thumb) || !cJSON_IsString(thumb_hash) || !cJSON_IsNumber(dl_count) || !cJSON_IsNumber(like_count) || !cJSON_IsString(last_updated) ||
        !(cJSON_IsString(description) || cJSON_IsNull(description)) || !cJSON_IsString(name) || !cJSON_IsString(display_name) || !cJSON_IsString(id) || !cJSON_IsString(download) || !cJSON_IsString(target)){
        return 1;
    }

    themeInfo->dlCount = dl_count->valueint;
    themeInfo->likeCount = like_count->valueint;
    themeInfo->lastUpdated = CopyTextUtil(last_updated->valuestring);
    if (!cJSON_IsNull(description))
        themeInfo->description = CopyTextUtil(description->valuestring);

    themeInfo->name = SafeFilenameText(name->valuestring);
    themeInfo->creator = SafeFilenameText(display_name->valuestring);
    themeInfo->id = CopyTextUtil(id->valuestring);
    themeInfo->imgLink = CopyTextUtil(original->valuestring);
    themeInfo->thumbLink = CopyTextUtil(thumb->valuestring);
    themeInfo->downloadLink = CopyTextUtil(download->valuestring);
    themeInfo->target = GetIndexOfStrArr(requestTargets, 7, target->valuestring);
    themeInfo->preview = CreateThumbHashTexture(thumb_hash->valuestring);

    if (cJSON_IsObject(pack)){
        cJSON *packId = cJSON_GetObjectItemCaseSensitive(pack, "hexId");
        cJSON *packCreator = cJSON_GetObjectItemCaseSensitive(pack, "creator");
        cJSON *packCreatorName = cJSON_GetObjectItemCaseSensitive(packCreator, "username");
        cJSON *packName = cJSON_GetObjectItemCaseSensitive(pack, "name");

        if (cJSON_IsString(packId) && cJSON_IsString(packCreatorName) && cJSON_IsString(packName))
            SetThemePackInfo(themeInfo, packId->valuestring, packCreatorName->valuestring, packName->valuestring);
    }

    return 0;
}

static int ParseRemoteTheme(ThemeInfo_t *themeInfo, cJSON *theme){
    cJSON *author = cJSON_GetObjectItemCaseSensitive(theme, "author");
    cJSON *created_at = cJSON_GetObjectItemCaseSensitive(theme, "createdAt");
    cJSON *download = cJSON_GetObjectItemCaseSensitive(theme, "downloadUrl");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(theme, "name");
    cJSON *quick_id = cJSON_GetObjectItemCaseSensitive(theme, "quickId");
    cJSON *target = cJSON_GetObjectItemCaseSensitive(theme, "target");

    if (!cJSON_IsString(author) || !cJSON_IsString(created_at) || !cJSON_IsString(download) || !cJSON_IsString(name) || !cJSON_IsString(quick_id) || !cJSON_IsString(target))
        return 1;

    themeInfo->creator = SafeFilenameText(author->valuestring);
    themeInfo->name = SafeFilenameText(name->valuestring);
    themeInfo->id = CopyTextUtil(quick_id->valuestring);
    themeInfo->lastUpdated = CopyTextUtil(created_at->valuestring);
    themeInfo->downloadLink = CopyTextUtil(download->valuestring);
    themeInfo->target = GetIndexOfStrArr(requestTargets, 7, target->valuestring);

    return 0;
}

static void SetThemePackInfo(ThemeInfo_t *themeInfo, const char *packId, const char *packCreator, const char *packName){
    NNFREE(themeInfo->packId);
    NNFREE(themeInfo->packCreator);
    NNFREE(themeInfo->packName);

    themeInfo->packId = CopyTextUtil(packId);
    themeInfo->packCreator = SafeFilenameText(packCreator);
    themeInfo->packName = SafeFilenameText(packName);
}

static int ParsePack(PackInfo_t *packInfo, cJSON *pack){
    cJSON *id = cJSON_GetObjectItemCaseSensitive(pack, "hexId");
    cJSON *creator = cJSON_GetObjectItemCaseSensitive(pack, "creator");
    cJSON *display_name = cJSON_GetObjectItemCaseSensitive(creator, "username");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(pack, "name");
    cJSON *original = NULL;
    cJSON *thumb = NULL;
    cJSON *thumb_hash = cJSON_GetObjectItemCaseSensitive(pack, "collageThumbHash");
    cJSON *themes = cJSON_GetObjectItemCaseSensitive(pack, "themes");

    if (!GetPreviewUrls(pack, "collagePreview", &original, &thumb) || !cJSON_IsString(thumb_hash) || !cJSON_IsString(id) || !cJSON_IsString(name) || !cJSON_IsString(display_name) || !cJSON_IsArray(themes))
        return 1;

    packInfo->id = CopyTextUtil(id->valuestring);
    packInfo->creator = SafeFilenameText(display_name->valuestring);
    packInfo->name = SafeFilenameText(name->valuestring);
    packInfo->imgLink = CopyTextUtil(original->valuestring);
    packInfo->thumbLink = CopyTextUtil(thumb->valuestring);
    packInfo->preview = CreateThumbHashTexture(thumb_hash->valuestring);
    packInfo->themeCount = cJSON_GetArraySize(themes);

    if (ParseThemeList(&packInfo->themes, packInfo->themeCount, themes))
        return 2;

    for (int i = 0; i < packInfo->themeCount; i++)
        SetThemePackInfo(&packInfo->themes[i], packInfo->id, packInfo->creator, packInfo->name);

    return 0;
}

char *GenLink(RequestInfo_t *rI){
    char *searchQuoted;
    if (rI->search[0] != '\0')
        searchQuoted = CopyJsonStringLiteral(rI->search);
    else 
        searchQuoted = CopyTextUtil("null");

    char *requestTarget;
    if (rI->target == 0 || rI->target >= 8)
        requestTarget = CopyTextUtil("null");
    else 
        requestTarget = CopyTextArgsUtil("\"%s\"",requestTargets[rI->target - 1]);
    
    static char request[0x1000];
    char variables[0x400];
    char *query = NULL;
    if (rI->target >= 1)
    {
        // query($target:Target,$paginationArgs:PaginationInput,$sort:ItemSort,$order:SortOrder,$query:String){switch{themes(target:$target,paginationArgs:$paginationArgs,sort:$sort,order:$order,query:$query){nodes{hexId creator{username} name description updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl}pageInfo{itemCount limit page pageCount}}}}
        query = "query%28%24target%3ATarget%2C%24paginationArgs%3APaginationInput%2C%24sort%3AItemSort%2C%24order%3ASortOrder%2C%24query%3AString%29%7Bswitch%7Bthemes%28target%3A%24target%2CpaginationArgs%3A%24paginationArgs%2Csort%3A%24sort%2Corder%3A%24order%2Cquery%3A%24query%29%7Bnodes%7BhexId%20creator%7Busername%7D%20name%20description%20updatedAt%20downloadCount%20saveCount%20target%20screenshotThumbHash%20screenshotPreview%7BhdUrl%20thumbUrl%7D%20downloadUrl%20pack%7BhexId%20name%20creator%7Busername%7D%7D%7DpageInfo%7BitemCount%20limit%20page%20pageCount%7D%7D%7D%7D";
        snprintf(variables, 0x400,"{\"target\":%s,\"paginationArgs\":{\"page\":%d,\"limit\":%d},\"sort\":\"%s\",\"order\":\"%s\",\"query\":%s}",\
            requestTarget, rI->page, rI->limit, requestSorts[rI->sort], requestOrders[rI->order], searchQuoted);
    }
    else if (rI->target == 0)
    {
        // query($paginationArgs:PaginationInput,$sort:ItemSort,$order:SortOrder,$query:String){switch{packs(paginationArgs:$paginationArgs,sort:$sort,order:$order,query:$query){nodes{hexId creator{username} name description updatedAt downloadCount saveCount collageThumbHash collagePreview{hdUrl thumbUrl} themes{hexId creator{username} name description updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl}}pageInfo{itemCount limit page pageCount}}}}
        query = "query%28%24paginationArgs%3APaginationInput%2C%24sort%3AItemSort%2C%24order%3ASortOrder%2C%24query%3AString%29%7Bswitch%7Bpacks%28paginationArgs%3A%24paginationArgs%2Csort%3A%24sort%2Corder%3A%24order%2Cquery%3A%24query%29%7Bnodes%7BhexId%20creator%7Busername%7D%20name%20description%20updatedAt%20downloadCount%20saveCount%20collageThumbHash%20collagePreview%7BhdUrl%20thumbUrl%7D%20themes%7BhexId%20creator%7Busername%7D%20name%20description%20updatedAt%20downloadCount%20saveCount%20target%20screenshotThumbHash%20screenshotPreview%7BhdUrl%20thumbUrl%7D%20downloadUrl%7D%7DpageInfo%7BitemCount%20limit%20page%20pageCount%7D%7D%7D%7D";
        snprintf(variables, 0x400, "{\"paginationArgs\":{\"page\":%d,\"limit\":%d},\"sort\":\"%s\",\"order\":\"%s\",\"query\":%s}",\
            rI->page, rI->limit, requestSorts[rI->sort], requestOrders[rI->order], searchQuoted);
    }

    CURL *curl = curl_easy_init();
    if(curl) {
        char *output = curl_easy_escape(curl, variables, 0);
        if(output) {
            printf("Encoded: %s\n", output);
            snprintf(request, sizeof(request), "https://api.themezer.net/graphql?query=%s&variables=%s", query, output);
            curl_free(output);
        }
        else 
        {
            snprintf(request, sizeof(request), "https://api.themezer.net/graphql?query=%s&variables=%s", query, variables);
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

static char *CopyJsonStringLiteral(const char *text){
    int len = 2;
    const unsigned char *c = (const unsigned char *)text;
    while (*c){
        if (*c == '"' || *c == '\\' || *c < 32)
            len += 2;
        else
            len++;
        c++;
    }

    char *out = calloc(1, len + 1);
    char *temp = out;
    *temp++ = '"';

    c = (const unsigned char *)text;
    while (*c){
        if (*c == '"' || *c == '\\'){
            *temp++ = '\\';
            *temp++ = *c;
        }
        else if (*c == '\n'){
            *temp++ = '\\';
            *temp++ = 'n';
        }
        else if (*c == '\r'){
            *temp++ = '\\';
            *temp++ = 'r';
        }
        else if (*c == '\t'){
            *temp++ = '\\';
            *temp++ = 't';
        }
        else if (*c < 32){
            *temp++ = ' ';
        }
        else {
            *temp++ = *c;
        }
        c++;
    }

    *temp++ = '"';
    return out;
}

static int GetPreviewUrls(cJSON *item, const char *fieldName, cJSON **original, cJSON **thumb){
    cJSON *preview = cJSON_GetObjectItemCaseSensitive(item, fieldName);
    if (!cJSON_IsObject(preview))
        return 0;

    *original = cJSON_GetObjectItemCaseSensitive(preview, "hdUrl");
    *thumb = cJSON_GetObjectItemCaseSensitive(preview, "thumbUrl");

    return cJSON_IsString(*original) && cJSON_IsString(*thumb);
}

SDL_Texture *CreateThumbHashTexture(const char *encodedThumbHash){
    if (!encodedThumbHash || !encodedThumbHash[0])
        return NULL;

    size_t decodedSize = 0;
    size_t encodedLen = strlen(encodedThumbHash);
    size_t decodedCapacity = encodedLen * 3 / 4 + 4;
    unsigned char *decoded = malloc(decodedCapacity);
    uint8_t *rgba = NULL;
    int width = 0;
    int height = 0;
    SDL_Texture *texture = NULL;

    if (!decoded)
        return NULL;

    if (mbedtls_base64_decode(decoded, decodedCapacity, &decodedSize, (const unsigned char *)encodedThumbHash, encodedLen) == 0){
        if (ThumbHashToRGBA(decoded, decodedSize, 64, &rgba, &width, &height))
            texture = LoadImageRGBASDL(rgba, width, height);
    }

    free(decoded);
    free(rgba);

    return texture;
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
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "themezer-nx/" APP_VERSION);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

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
    if (!curl)
        return CURLE_FAILED_INIT;

    if (!(res = PerformRequest(curl))){
        if (response != NULL){
            *response = cJSON_Parse((const char *)req.buffer);
        }

        printf("Buffer: %s\n", req.buffer);
    }

    free(req.buffer);
    curl_easy_cleanup(curl);
    return res;
}

int MakeDownloadRequest(char *url, char *path){
    get_request_t req = {0};
    int res;
    CURL *curl = CreateRequest(url, &req);
    if (!curl)
        return CURLE_FAILED_INIT;

    if (!(res = PerformRequest(curl))){
        long responseCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

        cJSON *json = cJSON_Parse((const char *)req.buffer);
        int apiError = ShowApiError(json);

        if (apiError){
            res = apiError;
        }
        else if (responseCode >= 400){
            char *message = CopyTextArgsUtil("The themezer server returned HTTP status %ld.", responseCode);
            ShapeLinker_t *menu = CreateBaseMessagePopup("Download Error", message);
            free(message);
            ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, 780, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Ok", FONT_TEXT[FSize28], exitFunc), ButtonType);
            MakeMenu(menu, ButtonHandlerBExit, NULL);
            ShapeLinkDispose(&menu);
            res = (int)responseCode;
        }
        else {
            FILE *fp = fopen(path, "wb");
            if (fp){
                fwrite(req.buffer, req.len, 1, fp);
                fclose(fp);
            }
            else {
                res = 1;
            }
        }

        cJSON_Delete(json);
    }

    free(req.buffer);
    curl_easy_cleanup(curl);
    return res;
}

static CURLcode PerformRequest(CURL *curl){
    CleanupActiveTransferQueue(NULL);

    CURLM *transferer = GetTransferer();
    if (!transferer)
        return CURLE_FAILED_INIT;

    CURLMcode multiRes = curl_multi_add_handle(transferer, curl);
    if (multiRes != CURLM_OK)
        return CURLE_FAILED_INIT;

    CURLcode result = CURLE_OK;
    bool finished = false;
    int runningHandles = 0;

    do {
        multiRes = curl_multi_perform(transferer, &runningHandles);

        int msgsLeft = -1;
        struct CURLMsg *msg;
        while ((msg = curl_multi_info_read(transferer, &msgsLeft))){
            if (msg->msg != CURLMSG_DONE)
                continue;

            if (msg->easy_handle == curl){
                result = msg->data.result;
                curl_multi_remove_handle(transferer, curl);
                finished = true;
            }
            else {
                char *privateData = NULL;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &privateData);
                if (privateData)
                    HandleCompletedTransfer((Transfer_t *)privateData, msg->data.result, NULL);
            }
        }

        if (finished || multiRes != CURLM_OK)
            break;

        int numfds = 0;
        multiRes = curl_multi_wait(transferer, NULL, 0, 1000, &numfds);
    } while (multiRes == CURLM_OK);

    if (!finished){
        curl_multi_remove_handle(transferer, curl);
        return (multiRes == CURLM_OK) ? CURLE_FAILED_INIT : CURLE_BAD_FUNCTION_ARGUMENT;
    }

    return result;
}

static void ShowRequestErrorPopup(char *title, char *message){
    ShapeLinker_t *menu = CreateBaseMessagePopup(title, message);
    ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, 780, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Ok", FONT_TEXT[FSize28], exitFunc), ButtonType);
    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);
}

static int ShowApiError(cJSON *root){
    if (!root)
        return 0;

    cJSON *statusCode = cJSON_GetObjectItemCaseSensitive(root, "statusCode");
    cJSON *messageItem = cJSON_GetObjectItemCaseSensitive(root, "message");

    if (cJSON_IsNumber(statusCode) && cJSON_IsString(messageItem)){
        ShowRequestErrorPopup("Error during request", messageItem->valuestring);
        return statusCode->valueint ? statusCode->valueint : 1;
    }

    cJSON *err = cJSON_GetObjectItemCaseSensitive(root, "errors");

    if (err){
        cJSON *errItem = cJSON_GetArrayItem(err, 0);
        if (errItem){
            messageItem = cJSON_GetObjectItemCaseSensitive(errItem, "message");
            char *message = cJSON_GetStringValue(messageItem);
            
            if (message)
                ShowRequestErrorPopup("Error during request", message);
        }

        return 1;
    }

    return 0;
}

int hasError(cJSON *root){
    return ShowApiError(root) ? 1 : 0;
}

int DownloadThemeFromUrl(char *url, char *path){
    int res = 1;

    if (url){
        if (EnsureDirectoryForFile(path))
            res = 1;
        else
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
        NNFREE(rI->themes[i].downloadLink);
        NNFREE(rI->themes[i].packId);
        NNFREE(rI->themes[i].packCreator);
        NNFREE(rI->themes[i].packName);
        if (rI->themes[i].preview && rI->packs == NULL)
            SDL_DestroyTexture(rI->themes[i].preview);
        
        if (rI->packs != NULL){
            free(rI->packs[i].id);
            free(rI->packs[i].creator);
            free(rI->packs[i].name);
            if (rI->packs[i].preview)
                SDL_DestroyTexture(rI->packs[i].preview);
            for (int j = 0; j < rI->packs[i].themeCount; j++){
                free(rI->packs[i].themes[j].id);
                free(rI->packs[i].themes[j].creator);
                free(rI->packs[i].themes[j].name);
                free(rI->packs[i].themes[j].description);
                free(rI->packs[i].themes[j].lastUpdated);
                free(rI->packs[i].themes[j].imgLink);
                free(rI->packs[i].themes[j].thumbLink);
                free(rI->packs[i].themes[j].downloadLink);
                free(rI->packs[i].themes[j].packId);
                free(rI->packs[i].themes[j].packCreator);
                free(rI->packs[i].themes[j].packName);
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
        if (ParseTheme(&themes[i], theme))
            return 1;

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
        if (ParsePack(&packs[i], pack))
            return 1;

        i++;
    }

    return 0;
}

void FillThemesWithPacks(RequestInfo_t *rI){
    rI->themes = calloc(sizeof(ThemeInfo_t), rI->curPageItemCount);
    for (int i = 0; i < rI->curPageItemCount; i++){
        rI->themes[i].name = CopyTextUtil(rI->packs[i].name);
        rI->themes[i].creator = CopyTextUtil(rI->packs[i].creator);
        rI->themes[i].thumbLink = CopyTextUtil(rI->packs[i].thumbLink);
        rI->themes[i].imgLink = CopyTextUtil(rI->packs[i].imgLink);
        rI->themes[i].preview = rI->packs[i].preview;
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
        cJSON *switchObj = cJSON_GetObjectItemCaseSensitive(data, "switch");
        if (switchObj) {
            cJSON *queryData;
            if (rI->target != 0){
                queryData = cJSON_GetObjectItemCaseSensitive(switchObj, "themes");
            } else {
                queryData = cJSON_GetObjectItemCaseSensitive(switchObj, "packs");
            }
            cJSON *pagination = cJSON_GetObjectItemCaseSensitive(queryData, "pageInfo");
            cJSON *page_count = cJSON_GetObjectItemCaseSensitive(pagination, "pageCount");
            cJSON *item_count = cJSON_GetObjectItemCaseSensitive(pagination, "itemCount");

            if (cJSON_IsNumber(page_count) && cJSON_IsNumber(item_count)){
                rI->pageCount = page_count->valueint;
                rI->itemCount = item_count->valueint;
            }
            else 
            {
                return -1;
            }
                


            FreeThemes(rI);
            rI->curPageItemCount = MIN(rI->limit, rI->itemCount - rI->limit * (rI->page - 1));

            if (rI->itemCount <= 0)
                return 0;

            cJSON *nodes = cJSON_GetObjectItemCaseSensitive(queryData, "nodes");
            if (rI->target != 0){
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
    }

    return res;
}

int LookupByQuickId(const char *quickId, RequestInfo_t *rI, QuickIdLookupType_t *lookupType){
    if (!quickId || !quickId[0] || !rI || !lookupType)
        return -1;

    *lookupType = QuickIdLookupNone;

    int res = MakeJsonRequest(GenLookupByQuickIdLink(quickId), &rI->response);
    if (res){
        ShowConnErrMenu(res);
        return res;
    }

    if (hasError(rI->response)){
        cJSON_Delete(rI->response);
        rI->response = NULL;
        return -4;
    }

    cJSON *data = cJSON_GetObjectItemCaseSensitive(rI->response, "data");
    cJSON *switchObj = cJSON_GetObjectItemCaseSensitive(data, "switch");
    cJSON *lookupData = cJSON_GetObjectItemCaseSensitive(switchObj, "lookupByQuickId");

    if (!lookupData || cJSON_IsNull(lookupData)){
        cJSON_Delete(rI->response);
        rI->response = NULL;
        return 1;
    }

    cJSON *typename = cJSON_GetObjectItemCaseSensitive(lookupData, "__typename");
    if (!cJSON_IsString(typename)){
        cJSON_Delete(rI->response);
        rI->response = NULL;
        return -2;
    }

    rI->maxDls = 12;
    rI->curPageItemCount = 1;

    if (!strcmp(typename->valuestring, "SwitchTheme")){
        rI->themes = calloc(sizeof(ThemeInfo_t), 1);
        if (!rI->themes)
            res = -3;
        else if (ParseTheme(&rI->themes[0], lookupData))
            res = -3;
        else
            *lookupType = QuickIdLookupTheme;
    }
    else if (!strcmp(typename->valuestring, "SwitchPack")){
        rI->packs = calloc(sizeof(PackInfo_t), 1);
        if (!rI->packs)
            res = -3;
        else if (ParsePack(&rI->packs[0], lookupData))
            res = -3;
        else {
            FillThemesWithPacks(rI);
            *lookupType = QuickIdLookupPack;
        }
    }
    else if (!strcmp(typename->valuestring, "SwitchRemoteInstallTheme")){
        rI->themes = calloc(sizeof(ThemeInfo_t), 1);
        if (!rI->themes)
            res = -3;
        else if (ParseRemoteTheme(&rI->themes[0], lookupData))
            res = -3;
        else
            *lookupType = QuickIdLookupRemoteTheme;
    }
    else {
        res = -2;
    }

    cJSON_Delete(rI->response);
    rI->response = NULL;

    return res;
}



ShapeLinker_t *GenListItemList(RequestInfo_t *rI){
    ShapeLinker_t *link = NULL;

    printf("Gen: ArraySize: %d", rI->curPageItemCount);

    for (int i = 0; i < rI->curPageItemCount; i++){
        ShapeLinkAdd(&link, ListItemCreate(COLOR_WHITE, COLOR_VERYLIGHTGREY_RGBA, rI->themes[i].preview, rI->themes[i].name, rI->themes[i].creator), ListItemType);
    }

    return link;
}

int AddThemeImagesToDownloadQueue(RequestInfo_t *rI, bool thumb){
    if (!rI->curPageItemCount)
        return 0;

    CleanupActiveTransferQueue(rI);
    if (rI->tInfo.transfers)
        CleanupTransferInfo(rI);
        
    rI->tInfo.transfers = calloc(sizeof(Transfer_t), rI->curPageItemCount);
    CURLM *transferer = GetTransferer();
    if (!rI->tInfo.transfers || !transferer){
        free(rI->tInfo.transfers);
        rI->tInfo.transfers = NULL;
        rI->tInfo.queueOffset = 0;
        rI->tInfo.finished = true;
        return 1;
    }

    rI->tInfo.queueOffset = rI->curPageItemCount;
    rI->tInfo.finished = false;
    curl_multi_setopt(transferer, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long)rI->maxDls);
    curl_multi_setopt(transferer, CURLMOPT_MAX_HOST_CONNECTIONS, (long)rI->maxDls);
    curl_multi_setopt(transferer, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    for (int i = 0; i < rI->curPageItemCount; i++){
            rI->tInfo.transfers[i].transfer = CreateRequest((thumb) ? rI->themes[i].thumbLink : rI->themes[i].imgLink, &rI->tInfo.transfers[i].data);
            rI->tInfo.transfers[i].owner = rI;
            rI->tInfo.transfers[i].index = i;
            if (!rI->tInfo.transfers[i].transfer)
                continue;

            curl_easy_setopt(rI->tInfo.transfers[i].transfer, CURLOPT_PRIVATE, &rI->tInfo.transfers[i]);
            curl_multi_add_handle(transferer, rI->tInfo.transfers[i].transfer);
    }

    if (AreTransfersFinished(rI)){
        CleanupTransferInfo(rI);
        return 1;
    }

    sActiveTransferQueue = rI;
    return 0;
}

int CleanupTransferInfo(RequestInfo_t *rI){
    if (sActiveTransferQueue == rI)
        sActiveTransferQueue = NULL;

    if (rI->tInfo.finished && !rI->tInfo.transfers)
        return 0;

    if (!rI->tInfo.transfers){
        rI->tInfo.finished = true;
        return 0;
    }

    CURLM *transferer = GetTransferer();
    for (int i = 0; i < rI->tInfo.queueOffset; i++){
        if (transferer && rI->tInfo.transfers[i].transfer){
            curl_multi_remove_handle(transferer, rI->tInfo.transfers[i].transfer);
            curl_easy_cleanup(rI->tInfo.transfers[i].transfer);
        }

        free(rI->tInfo.transfers[i].data.buffer);
        rI->tInfo.transfers[i].data.buffer = NULL;
    }

    free(rI->tInfo.transfers);
    rI->tInfo.transfers = NULL;
    rI->tInfo.queueOffset = 0;
    rI->tInfo.finished = true;
    return 0;
}

static bool AreTransfersFinished(RequestInfo_t *rI){
    if (!rI || !rI->tInfo.transfers)
        return true;

    for (int i = 0; i < rI->tInfo.queueOffset; i++){
        if (rI->tInfo.transfers[i].transfer)
            return false;
    }

    return true;
}

static void HandleCompletedTransfer(Transfer_t *transfer, CURLcode result, Context_t *ctx){
    if (!transfer || !transfer->owner || !transfer->transfer)
        return;

    RequestInfo_t *owner = transfer->owner;
    int index = transfer->index;

    if (result != CURLE_OK){
        printf("Something went wrong with the downloader, index %d, %d\n", index, result);
    }
    else {
        printf("Download of index %d finished!\n", index);
        get_request_t *req = &transfer->data;
        SDL_Texture *oldPreview = owner->themes[index].preview;
        owner->themes[index].preview = LoadImageMemSDL(req->buffer, req->len);
        if (owner->packs != NULL)
            owner->packs[index].preview = owner->themes[index].preview;

        if (ctx){
            ShapeLinker_t *all = ctx->all;
            ShapeLinker_t *dataLink = ShapeLinkFind(all, DataType);
            RequestInfo_t *visibleOwner = dataLink ? dataLink->item : NULL;

            if (visibleOwner == owner){
                ShapeLinker_t *gvLink = ShapeLinkFind(all, ListGridType);
                if (gvLink != NULL){
                    ListGrid_t *gv = gvLink->item;
                    ListItem_t *li = ShapeLinkOffset(gv->text, index)->item;
                    li->leftImg = owner->themes[index].preview;
                }
                else {
                    ShapeLinker_t *imageLink = ShapeLinkFind(all, ImageType);
                    if (imageLink && imageLink->next){
                        Image_t *img = ShapeLinkFind(imageLink->next, ImageType)->item;
                        img->texture = owner->themes[index].preview;
                    }
                }
            }
        }

        if (oldPreview && oldPreview != owner->themes[index].preview)
            SDL_DestroyTexture(oldPreview);
    }

    CURLM *transferer = GetTransferer();
    if (transferer)
        curl_multi_remove_handle(transferer, transfer->transfer);
    curl_easy_cleanup(transfer->transfer);
    transfer->transfer = NULL;
    free(transfer->data.buffer);
    transfer->data.buffer = NULL;
    transfer->data.len = 0;
    transfer->data.buflen = 0;

    if (AreTransfersFinished(owner))
        owner->tInfo.finished = true;
}

int HandleDownloadQueue(Context_t *ctx){
    ShapeLinker_t *all = ctx->all;
    RequestInfo_t *rI = ShapeLinkFind(all, DataType)->item;

    if (rI->tInfo.finished){
        CleanupTransferInfo(rI);
        return 0;
    }

    CURLM *transferer = GetTransferer();
    if (!transferer)
        return 1;

    int running_handles = 0;
    int pump_iterations = 0;
    CURLMcode multi_res = CURLM_OK;

    do {
        multi_res = curl_multi_perform(transferer, &running_handles);

        int msgs_left = -1;
        struct CURLMsg *msg;
        while ((msg = curl_multi_info_read(transferer, &msgs_left))){
            if (msg->msg == CURLMSG_DONE){
                char *privateData = NULL;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &privateData);
                if (privateData)
                    HandleCompletedTransfer((Transfer_t *)privateData, msg->data.result, ctx);
            }
        }

        if (multi_res != CURLM_OK || rI->tInfo.finished)
            break;

        int numfds = 0;
        if (++pump_iterations >= 8)
            break;

        multi_res = curl_multi_wait(transferer, NULL, 0, 0, &numfds);
        if (multi_res != CURLM_OK || numfds == 0)
            break;
    } while (1);

    if (rI->tInfo.finished || AreTransfersFinished(rI)){
        printf("Downloading done!\n");
        CleanupTransferInfo(rI);
    }

    return 0;
}

void SetDefaultsRequestInfo(RequestInfo_t *rI){
    rI->target = 8;
    rI->limit = 12;
    rI->page = 1;
    rI->sort = 0;
    rI->order = 0;
    rI->search = CopyTextUtil("");
    rI->maxDls = 12;
}
