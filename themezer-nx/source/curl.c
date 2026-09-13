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
    "Trending",
    "Created",
    "Updated",
    "Downloads",
    "Saves"
};

const char *requestOrders[] = {
    "Desc",
    "Asc"
};

static int GetPreviewUrls(cJSON *item, const char *fieldName, cJSON **original, cJSON **thumb);
static int ParseThemeList(ThemeInfo_t **storage, int size, cJSON *themesList);
static int ParseSplashList(SplashInfo_t **storage, int size, cJSON *splashesList);
static int ParseSplash(SplashInfo_t *splashInfo, cJSON *splash);
static int ParseRemoteInstall(RemoteInstallInfo_t *remoteInstall, cJSON *content, RemoteInstallKind_t kind);
int GetIndexOfStrArr(const char **toSearch, int limit, const char *search);
static int ShowApiError(cJSON *root);
static void SetThemePackInfo(ThemeInfo_t *themeInfo, const char *packId, const char *packCreator, const char *packName);
static char *CopyJsonStringLiteral(const char *text);
static CURLM *GetTransferer(void);
static size_t DownloadHeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata);
static int DownloadProgressCallback(void *clientp, curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow);
static int JsonRequestProgressCallback(void *clientp, curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow);
static int MakeJsonRequestInternal(char *url, cJSON **response, volatile bool *cancelRequested, bool pollController);
static void CleanupActiveTransferQueue(RequestInfo_t *except);
static void HandleCompletedTransfer(Transfer_t *transfer, CURLcode result, Context_t *ctx);
static bool AreTransfersFinished(RequestInfo_t *rI);

// JAGL owns the controller state, but the download is synchronous and therefore
// needs to poll that state while the normal menu loop is paused.
extern PadState pad;

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
    const char *query = "query($quickId:String!){switch{lookupByQuickId(quickId:$quickId){__typename ... on SwitchPack{hexId name creator{username} collageThumbHash collagePreview{hdUrl thumbUrl} themes{hexId creator{username} name description createdAt updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl}} ... on SwitchTheme{hexId creator{username} name description createdAt updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl pack{hexId name creator{username}}} ... on SwitchSplash{hexId name creator{username} description createdAt updatedAt downloadCount saveCount previewThumbHash preview{hdUrl thumbUrl} downloadUrl quickId} ... on SwitchRemoteInstallTheme{author createdAt downloadUrl name quickId target} ... on SwitchRemoteInstallSplash{author createdAt downloadUrl name quickId}}}}";
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
    cJSON *created_at = cJSON_GetObjectItemCaseSensitive(theme, "createdAt");
    cJSON *last_updated = cJSON_GetObjectItemCaseSensitive(theme, "updatedAt");
    cJSON *dl_count = cJSON_GetObjectItemCaseSensitive(theme, "downloadCount");
    cJSON *like_count = cJSON_GetObjectItemCaseSensitive(theme, "saveCount");
    cJSON *original = NULL;
    cJSON *thumb = NULL;
    cJSON *thumb_hash = cJSON_GetObjectItemCaseSensitive(theme, "screenshotThumbHash");
    cJSON *download = cJSON_GetObjectItemCaseSensitive(theme, "downloadUrl");
    cJSON *target = cJSON_GetObjectItemCaseSensitive(theme, "target");
    cJSON *pack = cJSON_GetObjectItemCaseSensitive(theme, "pack");

    if (!GetPreviewUrls(theme, "screenshotPreview", &original, &thumb) || !cJSON_IsString(thumb_hash) || !cJSON_IsNumber(dl_count) || !cJSON_IsNumber(like_count) || !cJSON_IsString(created_at) || !cJSON_IsString(last_updated) ||
        !(cJSON_IsString(description) || cJSON_IsNull(description)) || !cJSON_IsString(name) || !cJSON_IsString(display_name) || !cJSON_IsString(id) || !cJSON_IsString(download) || !cJSON_IsString(target)){
        return 1;
    }

    themeInfo->dlCount = dl_count->valueint;
    themeInfo->likeCount = like_count->valueint;
    themeInfo->createdAt = CopyTextUtil(created_at->valuestring);
    themeInfo->lastUpdated = CopyTextUtil(last_updated->valuestring);
    if (!cJSON_IsNull(description))
        themeInfo->description = CopyTextUtil(description->valuestring);

    themeInfo->name = SafeFilenameText(name->valuestring);
    themeInfo->creator = SafeFilenameText(display_name->valuestring);
    themeInfo->id = CopyTextUtil(id->valuestring);
    themeInfo->imgLink = CopyTextUtil(original->valuestring);
    themeInfo->thumbLink = CopyTextUtil(thumb->valuestring);
    themeInfo->downloadLink = CopyTextUtil(download->valuestring);
    themeInfo->target = GetIndexOfStrArr(requestTargets, THEME_TARGET_COUNT, target->valuestring);
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

static int ParseSplash(SplashInfo_t *splashInfo, cJSON *splash){
    cJSON *id = cJSON_GetObjectItemCaseSensitive(splash, "hexId");
    cJSON *quick_id = cJSON_GetObjectItemCaseSensitive(splash, "quickId");
    cJSON *creator = cJSON_GetObjectItemCaseSensitive(splash, "creator");
    cJSON *display_name = cJSON_GetObjectItemCaseSensitive(creator, "username");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(splash, "name");
    cJSON *description = cJSON_GetObjectItemCaseSensitive(splash, "description");
    cJSON *created_at = cJSON_GetObjectItemCaseSensitive(splash, "createdAt");
    cJSON *last_updated = cJSON_GetObjectItemCaseSensitive(splash, "updatedAt");
    cJSON *dl_count = cJSON_GetObjectItemCaseSensitive(splash, "downloadCount");
    cJSON *like_count = cJSON_GetObjectItemCaseSensitive(splash, "saveCount");
    cJSON *original = NULL;
    cJSON *thumb = NULL;
    cJSON *thumb_hash = cJSON_GetObjectItemCaseSensitive(splash, "previewThumbHash");
    cJSON *download = cJSON_GetObjectItemCaseSensitive(splash, "downloadUrl");

    if (!GetPreviewUrls(splash, "preview", &original, &thumb) || !cJSON_IsString(thumb_hash) || !cJSON_IsNumber(dl_count) || !cJSON_IsNumber(like_count) || !cJSON_IsString(created_at) || !cJSON_IsString(last_updated) ||
        !(cJSON_IsString(description) || cJSON_IsNull(description)) || !cJSON_IsString(name) || !cJSON_IsString(display_name) || !cJSON_IsString(id) || !cJSON_IsString(quick_id) || !cJSON_IsString(download)){
        return 1;
    }

    splashInfo->dlCount = dl_count->valueint;
    splashInfo->likeCount = like_count->valueint;
    splashInfo->createdAt = CopyTextUtil(created_at->valuestring);
    splashInfo->lastUpdated = CopyTextUtil(last_updated->valuestring);
    if (!cJSON_IsNull(description))
        splashInfo->description = CopyTextUtil(description->valuestring);

    splashInfo->name = SafeFilenameText(name->valuestring);
    splashInfo->creator = SafeFilenameText(display_name->valuestring);
    splashInfo->id = CopyTextUtil(id->valuestring);
    splashInfo->quickId = CopyTextUtil(quick_id->valuestring);
    splashInfo->imgLink = CopyTextUtil(original->valuestring);
    splashInfo->thumbLink = CopyTextUtil(thumb->valuestring);
    splashInfo->downloadLink = CopyTextUtil(download->valuestring);
    splashInfo->preview = CreateThumbHashTexture(thumb_hash->valuestring);

    return 0;
}

static int ParseRemoteInstall(RemoteInstallInfo_t *remoteInstall, cJSON *content, RemoteInstallKind_t kind){
    cJSON *author = cJSON_GetObjectItemCaseSensitive(content, "author");
    cJSON *created_at = cJSON_GetObjectItemCaseSensitive(content, "createdAt");
    cJSON *download = cJSON_GetObjectItemCaseSensitive(content, "downloadUrl");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(content, "name");
    cJSON *quick_id = cJSON_GetObjectItemCaseSensitive(content, "quickId");
    cJSON *target = cJSON_GetObjectItemCaseSensitive(content, "target");

    if (!cJSON_IsString(author) || !cJSON_IsString(created_at) || !cJSON_IsString(download) || !cJSON_IsString(name) || !cJSON_IsString(quick_id))
        return 1;

    if (kind == RemoteInstallKindTheme && !cJSON_IsString(target))
        return 1;

    remoteInstall->creator = SafeFilenameText(author->valuestring);
    remoteInstall->name = SafeFilenameText(name->valuestring);
    remoteInstall->quickId = CopyTextUtil(quick_id->valuestring);
    remoteInstall->lastUpdated = CopyTextUtil(created_at->valuestring);
    remoteInstall->downloadLink = CopyTextUtil(download->valuestring);
    remoteInstall->target = -1;
    remoteInstall->kind = kind;

    if (kind == RemoteInstallKindTheme)
        remoteInstall->target = GetIndexOfStrArr(requestTargets, THEME_TARGET_COUNT, target->valuestring);

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
    bool queryIsEncoded = true;
    if (rI->target == SPLASH_TARGET_INDEX)
    {
        query = "query($paginationArgs:PaginationInput,$sort:ItemSort,$order:SortOrder,$query:String,$includeNSFW:Boolean!){switch{splash{splashes(paginationArgs:$paginationArgs,sort:$sort,order:$order,query:$query,includeNSFW:$includeNSFW){nodes{hexId creator{username} name description createdAt updatedAt downloadCount saveCount previewThumbHash preview{hdUrl thumbUrl} downloadUrl quickId}pageInfo{itemCount limit page pageCount}}}}}";
        queryIsEncoded = false;
        snprintf(variables, 0x400, "{\"paginationArgs\":{\"page\":%d,\"limit\":%d},\"sort\":\"%s\",\"order\":\"%s\",\"query\":%s,\"includeNSFW\":false}",\
            rI->page, rI->limit, requestSorts[rI->sort], requestOrders[rI->order], searchQuoted);
    }
    else if (rI->target >= 1)
    {
        // query($target:Target,$paginationArgs:PaginationInput,$sort:ItemSort,$order:SortOrder,$query:String){switch{themes(target:$target,paginationArgs:$paginationArgs,sort:$sort,order:$order,query:$query){nodes{hexId creator{username} name description createdAt updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl pack{hexId name creator{username}}}pageInfo{itemCount limit page pageCount}}}}
        query = "query($target:Target,$paginationArgs:PaginationInput,$sort:ItemSort,$order:SortOrder,$query:String){switch{themes(target:$target,paginationArgs:$paginationArgs,sort:$sort,order:$order,query:$query){nodes{hexId creator{username} name description createdAt updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl pack{hexId name creator{username}}}pageInfo{itemCount limit page pageCount}}}}";
        queryIsEncoded = false;
        snprintf(variables, 0x400,"{\"target\":%s,\"paginationArgs\":{\"page\":%d,\"limit\":%d},\"sort\":\"%s\",\"order\":\"%s\",\"query\":%s}",\
            requestTarget, rI->page, rI->limit, requestSorts[rI->sort], requestOrders[rI->order], searchQuoted);
    }
    else if (rI->target == 0)
    {
        // query($paginationArgs:PaginationInput,$sort:ItemSort,$order:SortOrder,$query:String){switch{packs(paginationArgs:$paginationArgs,sort:$sort,order:$order,query:$query){nodes{hexId creator{username} name description updatedAt downloadCount saveCount collageThumbHash collagePreview{hdUrl thumbUrl} themes{hexId creator{username} name description createdAt updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl}}pageInfo{itemCount limit page pageCount}}}}
        query = "query($paginationArgs:PaginationInput,$sort:ItemSort,$order:SortOrder,$query:String){switch{packs(paginationArgs:$paginationArgs,sort:$sort,order:$order,query:$query){nodes{hexId creator{username} name description updatedAt downloadCount saveCount collageThumbHash collagePreview{hdUrl thumbUrl} themes{hexId creator{username} name description createdAt updatedAt downloadCount saveCount target screenshotThumbHash screenshotPreview{hdUrl thumbUrl} downloadUrl}}pageInfo{itemCount limit page pageCount}}}}";
        queryIsEncoded = false;
        snprintf(variables, 0x400, "{\"paginationArgs\":{\"page\":%d,\"limit\":%d},\"sort\":\"%s\",\"order\":\"%s\",\"query\":%s}",\
            rI->page, rI->limit, requestSorts[rI->sort], requestOrders[rI->order], searchQuoted);
    }

    CURL *curl = curl_easy_init();
    if(curl) {
        char *output = curl_easy_escape(curl, variables, 0);
        char *encodedQuery = queryIsEncoded ? NULL : curl_easy_escape(curl, query, 0);
        const char *requestQuery = encodedQuery ? encodedQuery : query;
        if(output) {
            printf("Encoded: %s\n", output);
            snprintf(request, sizeof(request), "https://api.themezer.net/graphql?query=%s&variables=%s", requestQuery, output);
            curl_free(output);
        }
        else 
        {
            snprintf(request, sizeof(request), "https://api.themezer.net/graphql?query=%s&variables=%s", requestQuery, variables);
        }
        if (encodedQuery)
            curl_free(encodedQuery);
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
#define CONNECT_TIMEOUT_SECONDS 10L
#define JSON_TIMEOUT_SECONDS 20L
#define LOW_SPEED_LIMIT_BYTES 1L
#define LOW_SPEED_TIMEOUT_SECONDS 30L

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb; 
    get_request_t *req = userdata;
    size_t required = req->len + realsize + 1;

    if (required > req->buflen){
        size_t newBuflen = req->buflen ? req->buflen : CHUNK_SIZE;
        while (newBuflen < required){
            if (newBuflen > ((size_t)-1) / 2)
                return 0;

            newBuflen *= 2;
        }

        unsigned char *newBuffer = realloc(req->buffer, newBuflen);
        if (!newBuffer)
            return 0;

        req->buffer = newBuffer;
        req->buflen = newBuflen;
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
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "themezer-nx/" APP_VERSION);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT_SECONDS);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT_BYTES);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIMEOUT_SECONDS);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        data->buffer = malloc(CHUNK_SIZE);
        if (!data->buffer){
            curl_easy_cleanup(curl);
            return NULL;
        }
        data->buflen = CHUNK_SIZE;

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, cURLErrBuff);
    }

    return curl;
}

static size_t DownloadHeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata){
    size_t len = size * nitems;
    DownloadProgressContext_t *progress = userdata;

    if (progress && len >= 5 && memcmp(buffer, "HTTP/", 5) == 0){
        char *statusSeparator = memchr(buffer, ' ', len);
        if (statusSeparator && (size_t)(buffer + len - statusSeparator) >= 4 &&
            statusSeparator[1] >= '0' && statusSeparator[1] <= '9' &&
            statusSeparator[2] >= '0' && statusSeparator[2] <= '9' &&
            statusSeparator[3] >= '0' && statusSeparator[3] <= '9'){
            int responseCode = (statusSeparator[1] - '0') * 100 +
                (statusSeparator[2] - '0') * 10 + statusSeparator[3] - '0';
            progress->finalResponseReady = responseCode >= 200 && responseCode < 300;
        }
    }

    return len;
}

static int DownloadProgressCallback(void *clientp, curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow){
    (void)uploadTotal;
    (void)uploadNow;

    DownloadProgressContext_t *progress = clientp;
    if (!progress)
        return 0;

    padUpdate(&pad);
    if (progress->cancelled || (padGetButtons(&pad) & HidNpadButton_B)){
        progress->cancelled = true;
        return 1;
    }

    if (!progress->finalResponseReady)
        return 0;

    bool shouldRender = false;
    if (progress->bar && downloadTotal > 0){
        u8 percentage = (downloadNow >= downloadTotal) ? 100 : (u8)((downloadNow * 100) / downloadTotal);
        shouldRender = progress->bar->percentage != percentage;
        progress->bar->percentage = percentage;
    }

    if (shouldRender && progress->menu)
        RenderShapeLinkList(progress->menu);

    return 0;
}

typedef struct {
    volatile bool *cancelRequested;
    bool pollController;
} JsonRequestProgress_t;

static int JsonRequestProgressCallback(void *clientp, curl_off_t downloadTotal, curl_off_t downloadNow, curl_off_t uploadTotal, curl_off_t uploadNow){
    (void)downloadTotal;
    (void)downloadNow;
    (void)uploadTotal;
    (void)uploadNow;

    JsonRequestProgress_t *progress = clientp;
    if (!progress)
        return 0;

    if (progress->cancelRequested && *progress->cancelRequested)
        return 1;

    if (progress->pollController){
        padUpdate(&pad);
        if (padGetButtons(&pad) & HidNpadButton_B){
            if (progress->cancelRequested)
                *progress->cancelRequested = true;
            return 1;
        }
    }

    return 0;
}

static int MakeJsonRequestInternal(char *url, cJSON **response, volatile bool *cancelRequested, bool pollController){
    get_request_t req = {0};

    int res;
    CURL *curl = CreateRequest(url, &req);
    if (!curl)
        return CURLE_FAILED_INIT;

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, JSON_TIMEOUT_SECONDS);

    JsonRequestProgress_t progress = {cancelRequested, pollController};
    if (cancelRequested || pollController){
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, JsonRequestProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }

    if (response)
        *response = NULL;

    if (!(res = curl_easy_perform(curl))){
        if (response != NULL){
            *response = cJSON_ParseWithLengthOpts((const char *)req.buffer, req.len, NULL, 0);
        }

        printf("Buffer: %s\n", req.buffer);
    }

    free(req.buffer);
    curl_easy_cleanup(curl);
    return res;
}

int MakeJsonRequest(char *url, cJSON **response){
    return MakeJsonRequestInternal(url, response, NULL, false);
}

int MakeJsonRequestCancelable(char *url, cJSON **response, volatile bool *cancelRequested){
    return MakeJsonRequestInternal(url, response, cancelRequested, false);
}

int MakeDownloadRequest(char *url, char *path, DownloadProgressContext_t *progress){
    get_request_t req = {0};
    int res;

    CleanupActiveTransferQueue(NULL);

    CURL *curl = CreateRequest(url, &req);
    if (!curl)
        return CURLE_FAILED_INIT;

    if (progress){
        progress->cancelled = false;
        progress->finalResponseReady = false;
        if (progress->bar)
            progress->bar->percentage = 0;
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, DownloadProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progress);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, DownloadHeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, progress);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }

    if (!(res = curl_easy_perform(curl))){
        long responseCode = 0;
        char *contentType = NULL;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);

        // Successful theme/splash responses are binary. Only try to parse a
        // response as JSON when the server says it is JSON or returned an
        // HTTP error; parsing a .nxtheme as cJSON can stall or read past its
        // binary contents.
        bool responseIsJson = contentType && strstr(contentType, "json");
        if (responseCode >= 400 || responseIsJson){
            cJSON *json = cJSON_ParseWithLengthOpts((const char *)req.buffer, req.len, NULL, 0);
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
                res = 1;
            }

            cJSON_Delete(json);
        }
        else {
            FILE *fp = fopen(path, "wb");
            if (fp){
                size_t written = fwrite(req.buffer, 1, req.len, fp);
                if (fclose(fp) != 0 || written != req.len)
                    res = 1;
            }
            else {
                res = 1;
            }
        }
    }

    free(req.buffer);
    curl_easy_cleanup(curl);
    return res;
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

int DownloadThemeFromUrl(char *url, char *path, DownloadProgressContext_t *progress){
    int res = 1;

    if (url){
        if (EnsureDirectoryForFile(path))
            res = 1;
        else
            res = MakeDownloadRequest(url, path, progress);
        free(url);
    }

    printf("Res: %d", res);
    return res;
}

#define MIN(x, y) ((x < y) ? x : y)

void FreeRequestContent(RequestInfo_t *rI){
    if (!rI)
        return;

    bool themePreviewsOwned = rI->packs == NULL;
    if (rI->themes){
        for (int i = 0; i < rI->curPageItemCount; i++){
            NNFREE(rI->themes[i].id);
            NNFREE(rI->themes[i].creator);
            NNFREE(rI->themes[i].name);
            NNFREE(rI->themes[i].description);
            NNFREE(rI->themes[i].createdAt);
            NNFREE(rI->themes[i].lastUpdated);
            NNFREE(rI->themes[i].imgLink);
            NNFREE(rI->themes[i].thumbLink);
            NNFREE(rI->themes[i].downloadLink);
            NNFREE(rI->themes[i].packId);
            NNFREE(rI->themes[i].packCreator);
            NNFREE(rI->themes[i].packName);
            if (themePreviewsOwned && rI->themes[i].preview)
                SDL_DestroyTexture(rI->themes[i].preview);
        }
        NNFREE(rI->themes);
    }

    if (rI->packs){
        for (int i = 0; i < rI->curPageItemCount; i++){
            NNFREE(rI->packs[i].id);
            NNFREE(rI->packs[i].creator);
            NNFREE(rI->packs[i].name);
            if (rI->packs[i].preview)
                SDL_DestroyTexture(rI->packs[i].preview);

            for (int j = 0; j < rI->packs[i].themeCount; j++){
                NNFREE(rI->packs[i].themes[j].id);
                NNFREE(rI->packs[i].themes[j].creator);
                NNFREE(rI->packs[i].themes[j].name);
                NNFREE(rI->packs[i].themes[j].description);
                NNFREE(rI->packs[i].themes[j].createdAt);
                NNFREE(rI->packs[i].themes[j].lastUpdated);
                NNFREE(rI->packs[i].themes[j].imgLink);
                NNFREE(rI->packs[i].themes[j].thumbLink);
                NNFREE(rI->packs[i].themes[j].downloadLink);
                NNFREE(rI->packs[i].themes[j].packId);
                NNFREE(rI->packs[i].themes[j].packCreator);
                NNFREE(rI->packs[i].themes[j].packName);
                if (rI->packs[i].themes[j].preview)
                    SDL_DestroyTexture(rI->packs[i].themes[j].preview);
            }
            NNFREE(rI->packs[i].themes);
        }
        NNFREE(rI->packs);
    }

    if (rI->splashes){
        for (int i = 0; i < rI->curPageItemCount; i++){
            NNFREE(rI->splashes[i].id);
            NNFREE(rI->splashes[i].quickId);
            NNFREE(rI->splashes[i].creator);
            NNFREE(rI->splashes[i].name);
            NNFREE(rI->splashes[i].description);
            NNFREE(rI->splashes[i].createdAt);
            NNFREE(rI->splashes[i].lastUpdated);
            NNFREE(rI->splashes[i].imgLink);
            NNFREE(rI->splashes[i].thumbLink);
            NNFREE(rI->splashes[i].downloadLink);
            if (rI->splashes[i].preview)
                SDL_DestroyTexture(rI->splashes[i].preview);
        }
        NNFREE(rI->splashes);
    }

    if (rI->remoteInstall){
        NNFREE(rI->remoteInstall->creator);
        NNFREE(rI->remoteInstall->name);
        NNFREE(rI->remoteInstall->quickId);
        NNFREE(rI->remoteInstall->lastUpdated);
        NNFREE(rI->remoteInstall->downloadLink);
        NNFREE(rI->remoteInstall);
    }
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

static int ParseSplashList(SplashInfo_t **storage, int size, cJSON *splashesList){
    *storage = calloc(sizeof(SplashInfo_t), size);
    SplashInfo_t *splashes = *storage;

    cJSON *splash = NULL;
    int i = 0;
    cJSON_ArrayForEach(splash, splashesList){
        if (ParseSplash(&splashes[i], splash))
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
            cJSON *queryData = NULL;
            if (rI->contentType == RequestContentSplashes){
                cJSON *splashNamespace = cJSON_GetObjectItemCaseSensitive(switchObj, "splash");
                queryData = cJSON_GetObjectItemCaseSensitive(splashNamespace, "splashes");
            }
            else if (rI->contentType == RequestContentPacks){
                queryData = cJSON_GetObjectItemCaseSensitive(switchObj, "packs");
            }

            if (rI->contentType == RequestContentThemes)
                queryData = cJSON_GetObjectItemCaseSensitive(switchObj, "themes");

            if (!queryData)
                return -1;

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
                


            FreeRequestContent(rI);
            rI->curPageItemCount = MIN(rI->limit, rI->itemCount - rI->limit * (rI->page - 1));

            if (rI->itemCount <= 0){
                cJSON_Delete(rI->response);
                rI->response = NULL;
                return 0;
            }

            cJSON *nodes = cJSON_GetObjectItemCaseSensitive(queryData, "nodes");
            if (rI->contentType == RequestContentThemes){
                if (nodes){
                    if (ParseThemeList(&rI->themes, rI->curPageItemCount, nodes))
                        return -3;

                    res = 0;
                    cJSON_Delete(rI->response);
                }
            }
            else if (rI->contentType == RequestContentPacks) {
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
            else if (rI->contentType == RequestContentSplashes) {
                if (nodes){
                    if (ParseSplashList(&rI->splashes, rI->curPageItemCount, nodes))
                        return -3;

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

    volatile bool cancelRequested = false;
    int res = MakeJsonRequestInternal(GenLookupByQuickIdLink(quickId), &rI->response, &cancelRequested, true);
    if (res){
        if (res != CURLE_ABORTED_BY_CALLBACK)
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
        rI->contentType = RequestContentThemes;
        rI->themes = calloc(sizeof(ThemeInfo_t), 1);
        if (!rI->themes)
            res = -3;
        else if (ParseTheme(&rI->themes[0], lookupData))
            res = -3;
        else
            *lookupType = QuickIdLookupTheme;
    }
    else if (!strcmp(typename->valuestring, "SwitchPack")){
        rI->contentType = RequestContentPacks;
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
    else if (!strcmp(typename->valuestring, "SwitchRemoteInstallTheme") || !strcmp(typename->valuestring, "SwitchRemoteInstallSplash")){
        RemoteInstallKind_t kind = !strcmp(typename->valuestring, "SwitchRemoteInstallSplash") ? RemoteInstallKindSplash : RemoteInstallKindTheme;
        rI->contentType = (kind == RemoteInstallKindSplash) ? RequestContentSplashes : RequestContentThemes;
        rI->remoteInstall = calloc(sizeof(RemoteInstallInfo_t), 1);
        if (!rI->remoteInstall)
            res = -3;
        else if (ParseRemoteInstall(rI->remoteInstall, lookupData, kind))
            res = -3;
        else
            *lookupType = QuickIdLookupRemoteInstall;
    }
    else if (!strcmp(typename->valuestring, "SwitchSplash")){
        rI->contentType = RequestContentSplashes;
        rI->splashes = calloc(sizeof(SplashInfo_t), 1);
        if (!rI->splashes)
            res = -3;
        else if (ParseSplash(&rI->splashes[0], lookupData))
            res = -3;
        else
            *lookupType = QuickIdLookupSplash;
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
        if (rI->contentType == RequestContentSplashes){
            ShapeLinkAdd(&link, ListItemCreate(COLOR_WHITE, COLOR_VERYLIGHTGREY_RGBA, rI->splashes[i].preview, rI->splashes[i].name, rI->splashes[i].creator), ListItemType);
        }
        else {
            ShapeLinkAdd(&link, ListItemCreate(COLOR_WHITE, COLOR_VERYLIGHTGREY_RGBA, rI->themes[i].preview, rI->themes[i].name, rI->themes[i].creator), ListItemType);
        }
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
            const char *imageUrl;
            if (rI->contentType == RequestContentSplashes)
                imageUrl = (thumb) ? rI->splashes[i].thumbLink : rI->splashes[i].imgLink;
            else
                imageUrl = (thumb) ? rI->themes[i].thumbLink : rI->themes[i].imgLink;

            rI->tInfo.transfers[i].transfer = CreateRequest((char *)imageUrl, &rI->tInfo.transfers[i].data);
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
        SDL_Texture *oldPreview;
        SDL_Texture *newPreview = LoadImageMemSDL(req->buffer, req->len);
        if (owner->contentType == RequestContentSplashes){
            oldPreview = owner->splashes[index].preview;
            owner->splashes[index].preview = newPreview;
        }
        else {
            oldPreview = owner->themes[index].preview;
            owner->themes[index].preview = newPreview;
            if (owner->packs != NULL)
                owner->packs[index].preview = newPreview;
        }

        if (ctx){
            ShapeLinker_t *all = ctx->all;
            ShapeLinker_t *dataLink = ShapeLinkFind(all, DataType);
            RequestInfo_t *visibleOwner = dataLink ? dataLink->item : NULL;

            if (visibleOwner == owner){
                ShapeLinker_t *gvLink = ShapeLinkFind(all, ListGridType);
                if (gvLink != NULL){
                    ListGrid_t *gv = gvLink->item;
                    ListItem_t *li = ShapeLinkOffset(gv->text, index)->item;
                    li->leftImg = newPreview;
                }
                else {
                    ShapeLinker_t *imageLink = ShapeLinkFind(all, ImageType);
                    if (imageLink && imageLink->next){
                        Image_t *img = ShapeLinkFind(imageLink->next, ImageType)->item;
                        img->texture = newPreview;
                    }
                }
            }
        }

        if (oldPreview && oldPreview != newPreview)
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
    NNFREE(rI->search);
    rI->target = 8;
    rI->contentType = RequestContentThemes;
    rI->limit = 12;
    rI->page = 1;
    rI->sort = TRENDING_SORT_INDEX;
    rI->order = 0;
    rI->search = CopyTextUtil("");
    rI->maxDls = 12;
}
