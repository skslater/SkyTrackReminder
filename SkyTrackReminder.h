#ifndef _SkyTrackReminder_H_
#define _SkyTrackReminder_H_

#include "XPLMDisplay.h"

#define PLUGIN_TITLE            "SkyTrack Reminder"
#define PLUGIN_SIGNATURE        "com.skslater.skytrackreminder"
#define PLUGIN_DESCRIPTION      "VA Flight Tracker Reminder for Fly UK"

#define REMINDER_TEXT           "** START SKYTRACK **"

#define MENU_TITLE              "SkyTrack Reminder"

#define MENU_TEXT_ACKNOWLEDGE   "Acknowledge Reminder"
#define MENU_ID_ACKNOWLEDGE     1000

#define TEXT_OFFSET_X           100
#define TEXT_OFFSET_Y           100

#define UPDATE_TIME             0.5f

static void MenuCallback(void *inMenuRef, void *inItemRef);
static int DrawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon);
static float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon);

#endif // _SkyTrackReminder_H_
