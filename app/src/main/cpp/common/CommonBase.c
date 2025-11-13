#include "CommonBase.h"

// Array of pubsub URLs
char *pubsuburls[MAX_APP_PROC] = {
    "ipc:///tmp/mainPubsub.ipc",
    "ipc:///tmp/halPubsub.ipc",
    "ipc:///tmp/mediaPubsub.ipc",
    "ipc:///tmp/kvsPubsub.ipc",
    "ipc:///tmp/lvglPubsub.ipc",
    //"tcp://127.0.0.1:60000", /* main app */
    //"tcp://127.0.0.1:60010", /* hal app */
    //"tcp://127.0.0.1:60020", /* media app */
    //"tcp://127.0.0.1:60030", /* kvs app */
    //"tcp://127.0.0.1:60040", /* lvgl app */
    // Add more URLs as needed
};

// Array of pubsub URLs
char *surveyurls[MAX_APP_PROC] = {
    "ipc:///tmp/mainSurvey.ipc",
    "ipc:///tmp/halSurvey.ipc",
    "ipc:///tmp/mediaSurvey.ipc",
    "ipc:///tmp/kvsSurvey.ipc",
    "ipc:///tmp/lvglSurvey.ipc",
     //"tcp://127.0.0.1:1235", /* main app */
    //"tcp://127.0.0.1:1236", /* hal app */
    //"tcp://127.0.0.1:1237", /* media app */
    //"tcp://127.0.0.1:1238", /* kvs app */
    //"tcp://127.0.0.1:1239", /* lvgl app */
    // Add more URLs as needed
};

// Array of pubsub URLs
char *proctopic[MAX_APP_PROC] = {
    "TOPIC_MAIN",
    "TOPIC_HAL",
    "TOPIC_MEDIA",
    "TOPIC_KVS",
    "TOPIC_LVGL",
    // Add more URLs as needed
};  
