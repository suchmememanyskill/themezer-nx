#include "gfx.h"

int HandleQueueList(Context_t *ctx){
    ListView_t *lv = ShapeLinkFind(ctx->all, ListViewType)->item;

    if (!CheckIfInstallSlotIsFree(lv->highlight)){
        char *message = CopyTextArgsUtil("Are you sure you want to remove the %s's queued install?", targetOptions[lv->highlight]);
        ShapeLinker_t *menu = CreateBaseMessagePopup("Remove Queued item?", message);
        free(message);

        ShapeLinkAdd(&menu, ButtonCreate(POS(640, 470, 390, 50), COLOR_MAINBG, COLOR_CURSORPRESS, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "No", FONT_TEXT[FSize28], exitFunc), ButtonType);
        ShapeLinkAdd(&menu, ButtonCreate(POS(250, 470, 390, 50), COLOR_MAINBG, COLOR_RED, COLOR_WHITE, COLOR_CURSOR, 0, ButtonStyleBottomStrip, "Yes", FONT_TEXT[FSize28], exitFunc), ButtonType);

        Context_t menuCtx = MakeMenu(menu, ButtonHandlerBExit, NULL);
        ShapeLinkDispose(&menu);

        if (menuCtx.curOffset == 7 && menuCtx.origin == OriginFunction){
            SetInstallSlot(lv->highlight, NULL);
            ShapeLinkDispose(&lv->text);

            ShapeLinker_t *text = NULL;
            for (int i = 0; i < 7; i++){
                char *t = CopyTextArgsUtil("%s: %s", targetOptions[i], (CheckIfInstallSlotIsFree(i)) ? "Not Queued" : "Queued");
                ShapeLinkAdd(&text, ListItemCreate((CheckIfInstallSlotIsFree(i)) ? COLOR_WHITE : COLOR_GREEN, COLOR_WHITE, NULL, t, NULL), ListItemType);
                free(t);
            }

            lv->text = text;
        }
    }

    return 0;
}

ShapeLinker_t *CreateSideQueueMenu(){
    ShapeLinker_t *out = CreateSideBaseMenu("Currently queued installs:");

    ShapeLinker_t *text = NULL;
    for (int i = 0; i < 7; i++){
        char *t = CopyTextArgsUtil("%s: %s", targetOptions[i], (CheckIfInstallSlotIsFree(i)) ? "Not Queued" : "Queued");
        ShapeLinkAdd(&text, ListItemCreate((CheckIfInstallSlotIsFree(i)) ? COLOR_WHITE : COLOR_GREEN, COLOR_WHITE, NULL, t, NULL), ListItemType);
        free(t);
    }

    ShapeLinkAdd(&out, ListViewCreate(POS(0, 50, 400, SCREEN_H - 100), 75, COLOR_MAINBG, COLOR_CURSOR, COLOR_CURSORPRESS, COLOR_SCROLLBAR, COLOR_SCROLLBARTHUMB, LIST_CENTERLEFT, text, HandleQueueList, NULL, FONT_TEXT[FSize28]), ListViewType);

    return out;
}

int ShowSideQueueMenu(Context_t *ctx){
    ShapeLinker_t *menu = CreateSideQueueMenu();
    MakeMenu(menu, ButtonHandlerBExit, NULL);
    ShapeLinkDispose(&menu);

    return 0;
}