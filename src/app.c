#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "app.h"

static int onClosing(uiWindow *w, void *data) {
  uiQuit();
  return 1;
}

static int onShouldQuit(void *data) {
  uiWindow *mMainwin = uiWindow(data);

  uiControlDestroy(uiControl(mMainwin));
  return 1;
}

int main(int argc, char const *argv[]) {
  uiInitOptions options;
  const char *err;
  uiTab *tab;
  uiMenu *menu;
  uiMenuItem *item;

  memset(&options, 0, sizeof(uiInitOptions));
  err = uiInit(&options);
  if (err != NULL) {
    fprintf(stderr, "error initializing libui: %s", err);
    uiFreeInitError(err);
    return 1;
  }

  menu = uiNewMenu("Options");
  item = uiMenuAppendItem(menu, "Change TwitchDownloaderCLI binary path");
  // uiMenuItemOnClicked(item, saveClicked, NULL);
  item = uiMenuAppendQuitItem(menu);

  mainwin = uiNewWindow("TwitchDownloader-gui", 1250, 700, 1);
  uiWindowOnClosing(mainwin, onClosing, NULL);
  uiOnShouldQuit(onShouldQuit, mainwin);

  tab = uiNewTab();
  uiWindowSetChild(mainwin, uiControl(tab));
  uiWindowSetMargined(mainwin, 1);

  uiTabAppend(tab, "Vod Downloader", VodDownloaderDrawUi());
  uiTabSetMargined(tab, 0, 1);

  uiTabAppend(tab, "Clip Downloader", ClipDownloaderDrawUi());
  uiTabSetMargined(tab, 1, 1);

  uiTabAppend(tab, "Chat Downloader", ChatDownloaderDrawUi());
  uiTabSetMargined(tab, 2, 1);

  uiTabAppend(tab, "Chat Render", ChatRenderDrawUi());
  uiTabSetMargined(tab, 3, 1);

  uiControlShow(uiControl(mainwin));
  // uiMsgBox(mainwin, "", "");  TODO: check for TwitchDownloaderCLI binary
  uiMain();

  return 0;
}
