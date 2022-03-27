#include "ClipDownloader.h"

uiControl *ClipDownloaderDrawUi(void);
static void infoBtnClicked(uiButton *b, void *data);
static void downloadBtnClicked(uiButton *b, void *data);
static int setQualities(char *id, uiCombobox *cBox);
static void setInfo(char *id, ClipOptions *clipOptions);
static void *downloadTask(void *arg);
static void clearFreeUI(ClipOptions *clipOptions);
static char *getId(const char *link);
static void runOnUiThread(void *args);
static void handlerMouseEvent(uiAreaHandler *ah, uiArea *area, uiAreaMouseEvent *e);
static void handlerMouseCrossed(uiAreaHandler *ah, uiArea *area, int left);
static void handlerDragBroken(uiAreaHandler *ah, uiArea *area);
static int handlerKeyEvent(uiAreaHandler *ah, uiArea *area, uiAreaKeyEvent *e);
static void handlerDraw(uiAreaHandler *ah, uiArea *area, uiAreaDrawParams *p);
static void setThumbnail(char *link, ClipOptions *clipOptions);

uiControl *ClipDownloaderDrawUi() {
  uiBox *mainVerticalBox = uiNewVerticalBox();
  uiBoxSetPadded(mainVerticalBox, 1);

  uiBox *linkBox = uiNewHorizontalBox();
  uiBoxSetPadded(linkBox, 1);
  uiForm *linkForm = uiNewForm();
  uiFormSetPadded(linkForm, 1);
  uiEntry *linkEntry = uiNewEntry();
  uiButton *infoBtn = uiNewButton("Get info");
  uiFormAppend(linkForm, "Clip Link: ", uiControl(linkEntry), 0);
  uiBoxAppend(linkBox, uiControl(linkForm), 1);
  uiBoxAppend(linkBox, uiControl(infoBtn), 0);
  uiBoxAppend(mainVerticalBox, uiControl(linkBox), 0);

  uiBox *middleHorizontalBox = uiNewHorizontalBox();
  uiBoxSetPadded(middleHorizontalBox, 1);
  uiBoxAppend(mainVerticalBox, uiControl(middleHorizontalBox), 1);

  uiBox *leftVerticalBox = uiNewVerticalBox();
  uiBoxAppend(middleHorizontalBox, uiControl(leftVerticalBox), 1);

  uiBox *infoBox = uiNewVerticalBox();
  uiGrid *infoGrid = uiNewGrid();
  uiBoxAppend(infoBox, uiControl(infoGrid), 1);
  uiBoxAppend(leftVerticalBox, uiControl(infoBox), 1);

  uiForm *nameForm = uiNewForm();
  uiFormSetPadded(nameForm, 1);
  uiLabel *nameLabel = uiNewLabel("");
  uiFormAppend(nameForm, "Streamer: ", uiControl(nameLabel), 0);

  uiForm *titleForm = uiNewForm();
  uiFormSetPadded(titleForm, 1);
  uiLabel *titleLabel = uiNewLabel("");
  uiFormAppend(titleForm, "Title: ", uiControl(titleLabel), 0);

  uiForm *durationForm = uiNewForm();
  uiFormSetPadded(durationForm, 1);
  uiLabel *durationLabel = uiNewLabel("");
  uiFormAppend(durationForm, "Duration: ", uiControl(durationLabel), 0);

  uiForm *createdForm = uiNewForm();
  uiFormSetPadded(createdForm, 1);
  uiLabel *createdLabel = uiNewLabel("");
  uiFormAppend(createdForm, "Created at: ", uiControl(createdLabel), 0);

  uiGridAppend(infoGrid, uiControl(nameForm), 0, 0, 1, 1, 1, uiAlignStart, 1, uiAlignCenter);
  uiGridAppend(infoGrid, uiControl(titleForm), 0, 1, 1, 1, 1, uiAlignStart, 1, uiAlignCenter);
  uiGridAppend(infoGrid, uiControl(durationForm), 0, 2, 1, 1, 1, uiAlignStart, 1, uiAlignCenter);
  uiGridAppend(infoGrid, uiControl(createdForm), 0, 3, 1, 1, 1, uiAlignStart, 1, uiAlignCenter);

  struct handler *handler = malloc(sizeof(struct handler));
  *handler = (struct handler){.binaryData = NULL, .img = NULL};
  handler->ah.DragBroken = handlerDragBroken;
  handler->ah.Draw = handlerDraw;
  handler->ah.KeyEvent = handlerKeyEvent;
  handler->ah.MouseCrossed = handlerMouseCrossed;
  handler->ah.MouseEvent = handlerMouseEvent;
  uiBox *thumbnailBox = uiNewVerticalBox();
  uiArea *imageArea = uiNewArea((uiAreaHandler *)handler);
  uiBoxAppend(thumbnailBox, uiControl(imageArea), 1);
  uiBoxAppend(leftVerticalBox, uiControl(thumbnailBox), 1);

  uiBoxAppend(middleHorizontalBox, uiControl(uiNewVerticalSeparator()), 0);

  // BUG, will be solved in the next release: https://github.com/lay295/TwitchDownloader/issues/289
  uiForm *qualityForm = uiNewForm();
  uiFormSetPadded(qualityForm, 1);
  uiCombobox *qualities = uiNewCombobox();
  uiFormAppend(qualityForm, "Quality: ", uiControl(qualities), 0);
  uiBoxAppend(middleHorizontalBox, uiControl(qualityForm), 0);

  uiBoxAppend(middleHorizontalBox, uiControl(uiNewVerticalSeparator()), 0);

  uiForm *logForm = uiNewForm();
  uiFormSetPadded(logForm, 1);
  uiMultilineEntry *logsEntry = uiNewMultilineEntry();
  uiMultilineEntrySetReadOnly(logsEntry, 1);
  uiFormAppend(logForm, "Logs: ", uiControl(logsEntry), 1);
  uiBoxAppend(middleHorizontalBox, uiControl(logForm), 1);

  uiGrid *downloadGrid = uiNewGrid();
  uiGridSetPadded(downloadGrid, 1);
  uiButton *downloadBtn = uiNewButton("Download");
  uiControlDisable(uiControl(downloadBtn));
  uiGridAppend(downloadGrid, uiControl(downloadBtn), 0, 0, 1, 1, 1, uiAlignCenter, 1, uiAlignFill);
  uiBoxAppend(mainVerticalBox, uiControl(downloadGrid), 0);

  uiBox *pBarBox = uiNewHorizontalBox();
  uiBoxSetPadded(pBarBox, 1);
  uiProgressBar *pBar = uiNewProgressBar();
  uiLabel *status = uiNewLabel("Idle...");
  uiBoxAppend(pBarBox, uiControl(status), 0);
  uiBoxAppend(pBarBox, uiControl(pBar), 1);
  uiBoxAppend(mainVerticalBox, uiControl(pBarBox), 0);

  ClipOptions *clipOptions = malloc(sizeof(ClipOptions));
  *clipOptions = (ClipOptions){
      qualities, linkEntry, nameLabel, titleLabel, durationLabel, createdLabel, logsEntry, pBar, status, downloadBtn, infoBtn, NULL, NULL, imageArea, handler,
  };

  uiButtonOnClicked(infoBtn, infoBtnClicked, clipOptions);
  uiButtonOnClicked(downloadBtn, downloadBtnClicked, clipOptions);

  return uiControl(mainVerticalBox);
}

static void infoBtnClicked(uiButton *b, void *data) {
  ClipOptions *clipOptions = (ClipOptions *)data;
  clearFreeUI(clipOptions);
  char *link = uiEntryText(clipOptions->linkEntry);
  char *id = getId(link);
  if (id == NULL) {
    uiMsgBoxError(mainwin, "Error", "Invalid Url");
    goto err;
  }
  int validID = setQualities(id, clipOptions->qualities);
  if (!validID) {
    uiMsgBoxError(mainwin, "Error", "Invalid Clip ID");
    free(id);
    goto err;
  }
  setInfo(id, clipOptions);
  clipOptions->id = id;
  uiControlEnable(uiControl(clipOptions->downloadBtn));
err:
  uiFreeText(link);
}

static void downloadBtnClicked(uiButton *b, void *data) {
  char *fileName = uiSaveFile(mainwin, NULL, "clip.mp4", "mp4 File (*.mp4)|*.mp4");
  if (fileName == NULL) {
    return;
  }

  ClipOptions *clipOptions = (ClipOptions *)data;
  // because the command is always the same for ClipDownload it's easier to make
  // a template with format string
  char cmd[commandTemplateLength + strlen(clipOptions->id) + strlen(fileName) + 1];
  sprintf(cmd, "TwitchDownloaderCLI -m ClipDownload -u '%s' -q '%s' -o %s 2>&1", clipOptions->id, qualityArray[uiComboboxSelected(clipOptions->qualities)], fileName);
  clipOptions->cmd = strdup(cmd);

  uiFreeText(fileName);

  pthread_t thread;
  pthread_create(&thread, NULL, downloadTask, data);
  pthread_detach(thread);
}

static void *downloadTask(void *args) {
  ClipOptions *clipOptions = (ClipOptions *)args;
  char buf[200];
  FILE *fp;

  if ((fp = popen(clipOptions->cmd, "r")) == NULL) {
    printf("Error opening pipe!\n");
    return NULL;
  }

  uiData *data = malloc(sizeof(uiData));
  *data = (uiData){.clipOptions = clipOptions, .flag = DOWNLOADING};
  uiQueueMain(runOnUiThread, data);

  while (mygets(buf, 200, fp) != NULL) {
    uiData *logData = malloc(sizeof(uiData));
    *data = (uiData){.buf = strdup(buf), .clipOptions = clipOptions, .flag = LOGGING};
    uiQueueMain(runOnUiThread, logData);
  }

  data = malloc(sizeof(uiData));
  *data = (uiData){.clipOptions = clipOptions, .flag = FINISH, .i = pclose(fp)};
  uiQueueMain(runOnUiThread, data);

  free(clipOptions->cmd);
  clipOptions->cmd = NULL;

  return NULL;
}

static int setQualities(char *id, uiCombobox *cBox) {
  int validID = 1;
  cJSON *root = NULL;
  cJSON *qualities = NULL;
  string *qualityRes = getClipQualities(id);
  uiComboboxClear(cBox);
  root = cJSON_Parse((char *)qualityRes->memory);
  qualities = json(json(json(cJSON_GetArrayItem(root, 0), "data"), "clip"), "videoQualities");
  if (qualities == NULL) {
    validID = 0;
    goto err;
  }
  cJSON *quality = NULL;
  int i = 0;
  cJSON_ArrayForEach(quality, qualities) {
    int fr = json(quality, "frameRate")->valueint;
    char *res = json(quality, "quality")->valuestring;
    char qualityString[8];
    sprintf(qualityString, "%sp%d", res, fr);
    sprintf(qualityArray[i], "%sp%d", res, fr);
    uiComboboxAppend(cBox, qualityString);
    i++;
  }
  uiComboboxSetSelected(cBox, 0);

err:
  free(qualityRes->memory);
  free(qualityRes);
  cJSON_Delete(root);
  return validID;
}

static void setInfo(char *id, ClipOptions *clipOptions) {
  string *infoRes = getClipInfo(id);
  cJSON *root = cJSON_Parse((char *)infoRes->memory);
  char duration[11];
  uiLabelSetText(clipOptions->nameLabel, json(json(json(json(root, "data"), "clip"), "broadcaster"), "displayName")->valuestring);
  uiLabelSetText(clipOptions->titleLabel, json(json(json(root, "data"), "clip"), "title")->valuestring);
  sprintf(duration, "%d %s", (json(json(json(root, "data"), "clip"), "durationSeconds")->valueint), "Seconds");
  uiLabelSetText(clipOptions->durationLabel, duration);
  char *createdLocalTime = getLocalTime(json(json(json(root, "data"), "clip"), "createdAt")->valuestring);
  uiLabelSetText(clipOptions->createdLabel, createdLocalTime);
  cJSON *thumbnail = json(json(json(root, "data"), "clip"), "thumbnailURL");
  if (thumbnail)
    setThumbnail(thumbnail->valuestring, clipOptions);

  free(infoRes->memory);
  free(infoRes);
  free(createdLocalTime);
  cJSON_Delete(root);
}

// https://www.cairographics.org/manual/cairo-Image-Surfaces.html#cairo-format-t
// stb gets the image in RGBA, but cairo needs it in ARGB, AND in native-endian, so little-endian, so it will be BGRA
// so I have to swap the B and the R, the image is 480x272 so should be fast.
static void setThumbnail(char *link, ClipOptions *clipOptions) {
  int x, y, n;
  string *response = requestImage(link);
  clipOptions->handler->binaryData = stbi_load_from_memory(response->memory, response->used, &x, &y, &n, 4);
  clipOptions->handler->width = x;
  clipOptions->handler->height = y;
  for (int i = 0; i < x * y; i++) {
    unsigned char red = clipOptions->handler->binaryData[i * 4];
    clipOptions->handler->binaryData[i * 4] = clipOptions->handler->binaryData[i * 4 + 2];
    clipOptions->handler->binaryData[i * 4 + 2] = red;
  }
  clipOptions->handler->img = uiNewImageBuffer(NULL, x, y, 1);
  uiImageBufferUpdate(clipOptions->handler->img, (void *)clipOptions->handler->binaryData);
  uiAreaQueueRedrawAll(clipOptions->imageArea);
  free(response->memory);
  free(response);
}

// TODO: support recieving ID as input
static char *getId(const char *link) {
  regex_t regex;
  regcomp(&regex, "twitch[.]tv/[^[:space:]]+/clip/", REG_EXTENDED);
  if (regexec(&regex, link, 0, NULL, 0) && (strstr(link, "clips.twitch.tv/") == NULL)) {
    regfree(&regex);
    return NULL;
  }

  CURLU *h;
  CURLUcode uc;
  char *path;
  char *id;

  h = curl_url();
  uc = curl_url_set(h, CURLUPART_URL, link, 0);
  uc = curl_url_get(h, CURLUPART_PATH, &path, 0);
  if (!uc) {
    id = strdup(strrchr(path, '/') + 1);
    curl_free(path);
  }
  curl_url_cleanup(h);
  regfree(&regex);
  return id;
}

static void runOnUiThread(void *args) {
  uiData *data = (uiData *)args;
  ClipOptions *clipOptions = data->clipOptions;
  switch (data->flag) {
  case DOWNLOADING:
    uiControlDisable(uiControl(clipOptions->downloadBtn));
    uiControlDisable(uiControl(clipOptions->infoBtn));
    uiLabelSetText(clipOptions->status, "Downloading...");
    uiProgressBarSetValue(clipOptions->pBar, -1);
    break;
  case LOGGING:
    uiMultilineEntryAppend(clipOptions->logsEntry, data->buf);
    free(data->buf);
    break;
  case FINISH:
    if (data->i) {
      uiLabelSetText(clipOptions->status, "Error...");
      uiProgressBarSetValue(clipOptions->pBar, 0);
    } else {
      uiLabelSetText(clipOptions->status, "Done!");
      uiProgressBarSetValue(clipOptions->pBar, 100);
    }
    uiControlEnable(uiControl(clipOptions->downloadBtn));
    uiControlEnable(uiControl(clipOptions->infoBtn));
    break;
  default:
    break;
  }
  free(data);
}

static void clearFreeUI(ClipOptions *clipOptions) {
  free(clipOptions->id);
  clipOptions->id = NULL;
  stbi_image_free(clipOptions->handler->binaryData);
  clipOptions->handler->binaryData = NULL;
  if (clipOptions->handler->img) {
    uiFreeImageBuffer(clipOptions->handler->img);
    clipOptions->handler->img = NULL;
  }
  uiComboboxClear(clipOptions->qualities);
  uiLabelSetText(clipOptions->nameLabel, "");
  uiLabelSetText(clipOptions->titleLabel, "");
  uiLabelSetText(clipOptions->durationLabel, "");
  uiLabelSetText(clipOptions->createdLabel, "");
  uiLabelSetText(clipOptions->status, "Idle...");
  uiProgressBarSetValue(clipOptions->pBar, 0);
  uiControlDisable(uiControl(clipOptions->downloadBtn));
}

static void handlerMouseEvent(uiAreaHandler *ah, uiArea *area, uiAreaMouseEvent *e) {}
static void handlerMouseCrossed(uiAreaHandler *ah, uiArea *area, int left) {}
static void handlerDragBroken(uiAreaHandler *ah, uiArea *area) {}
static int handlerKeyEvent(uiAreaHandler *ah, uiArea *area, uiAreaKeyEvent *e) { return 0; }
static void handlerDraw(uiAreaHandler *ah, uiArea *area, uiAreaDrawParams *p) {
  struct handler *handler = (struct handler *)ah;
  if (!(handler->img))
    return;
  uiRect src = {0, 0, handler->width, handler->height};
  uiRect dest = {0, 0, 320, 180};
  uiImageBufferDraw(p->Context, handler->img, &src, &dest, 1);
}