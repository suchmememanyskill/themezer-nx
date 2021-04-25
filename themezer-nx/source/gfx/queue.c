#include "gfx.h"

int HandleQueueList(Context_t *ctx){
    ListView_t *lv = ShapeLinkFind(ctx->all, ListViewType)->item;
    ListItem_t *selected = ShapeLinkOffset(lv->text, lv->highlight)->item;
    int installSlotOffset = GetInstallSlotOffset(selected->leftText);

    if (installSlotOffset < 0)
        return 0;

    if (!CheckIfInstallSlotIsFree(installSlotOffset)){
        char *message = CopyTextArgsUtil("Are you sure you want to remove the %s's queued install?", targetOptions[installSlotOffset]);
        ShapeLinker_t *menu = CreateBaseMessagePopup("Remove Queued item?", message);
        free(message);

        ShapeLinkAdd(&menu, ButtonCreate(POS(640, 470, 390, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "No", FONT_TEXT[FSize28], exitFunc), ButtonType);
        ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, 390, 50), COLOR_MAINBG, COLOR_RED, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Yes", FONT_TEXT[FSize28], exitFunc), ButtonType);

        Context_t menuCtx = MakeMenu(menu, ButtonHandlerBExit, NULL);
        ShapeLinkDispose(&menu);

        if (menuCtx.curOffset == 7 && menuCtx.origin == OriginFunction){
            SetInstallSlot(installSlotOffset, NULL);
            ShapeLinkDel(&lv->text, lv->highlight);
            lv->highlight = 0;

            if (lv->text == NULL){
                return -1;
            }
        }
    }

    return 0;
}

ShapeLinker_t *CreateSideQueueMenu(){
    ShapeLinker_t *out = CreateSideBaseMenu("Queued Installs");

    ShapeLinker_t *text = NULL;
    int hasAtLeastOne = 0;

    for (int i = 0; i < 7; i++){
        if (!CheckIfInstallSlotIsFree(i)){
            hasAtLeastOne = 1;
            char *t = CopyTextUtil(targetOptions[i]);
            ShapeLinkAdd(&text, ListItemCreate(COLOR_FILTERACTIVE, COLOR_WHITE, NULL, t, NULL), ListItemType);
            free(t);
        }
    }

    if (hasAtLeastOne)
        ShapeLinkAdd(&out, ListViewCreate(POS(0, 50, 400, SCREEN_H - 100), 75, COLOR_MAINBG, COLOR_CURSOR, COLOR_CURSORPRESS, COLOR_SCROLLBAR, COLOR_SCROLLBARTHUMB, LIST_CENTERLEFT, text, HandleQueueList, NULL, FONT_TEXT[FSize28]), ListViewType);

    return out;
}

int ShowSideQueueMenu(Context_t *ctx){
    ShapeLinker_t *menu = CreateSideQueueMenu();
    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}