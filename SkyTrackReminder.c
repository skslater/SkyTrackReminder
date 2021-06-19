#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "XPLMDefs.h"
#include "XPLMDataAccess.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMMenus.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"

#include "SkyTrackReminder.h"

// Global variables
static bool gvar_batteryOn = false;
static bool gvar_enginesRunning = false;
static bool gvar_acknowledged = false;
static bool gvar_colFlip = true;
static int gvar_screenWidth = 1000;
static int gvar_screenHeight = 1000;
static char gvar_drawText[128];
static float gvar_drawTextLength;
static XPLMFlightLoopID gvar_flightloopId = NULL;
static XPLMDataRef gref_batteryOn = NULL;
static XPLMDataRef gref_numEngines = NULL;
static XPLMDataRef gref_engineState = NULL;
static XPLMDataRef gref_screenWidth = NULL;
static XPLMDataRef gref_screenHeight = NULL;

//
// Called at sim initialisation
//
PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    // Pass our details to the plugin system
    strncpy(outName, PLUGIN_TITLE, 255);
    strncpy(outSig, PLUGIN_SIGNATURE, 255);
    strncpy(outDesc, PLUGIN_DESCRIPTION, 255);

    // Get a reference to the plugins menu
    XPLMMenuID pluginMenu = XPLMFindPluginsMenu();

    // Create a new menu for our commands
    int myMenuIdx = XPLMAppendMenuItem(pluginMenu, MENU_TITLE, 0, 0);
    XPLMMenuID myRootMenu = XPLMCreateMenu(MENU_TITLE, pluginMenu, myMenuIdx, MenuCallback, NULL);
    XPLMAppendMenuItem(myRootMenu, MENU_TEXT_ACKNOWLEDGE, (void*) MENU_ID_ACKNOWLEDGE, 0);

    // Cache references to our required datarefs
    gref_batteryOn = XPLMFindDataRef("sim/cockpit/electrical/battery_on");
    gref_numEngines = XPLMFindDataRef("sim/aircraft/engine/acf_num_engines");
    gref_engineState = XPLMFindDataRef("sim/flightmodel/engine/ENGN_running");
    gref_screenWidth = XPLMFindDataRef("sim/graphics/view/window_width");
    gref_screenHeight = XPLMFindDataRef("sim/graphics/view/window_height");

    // Do this here once so we're not calculating it every frame
    strncpy(gvar_drawText, REMINDER_TEXT, sizeof(gvar_drawText));
    gvar_drawTextLength = XPLMMeasureString(xplmFont_Basic, gvar_drawText, strlen(REMINDER_TEXT));

    // Register for graphics drawing callbacks during a late 2D phase
    XPLMRegisterDrawCallback(DrawCallback, xplm_Phase_Window, 0, NULL);

    // Return success
    return 1;
}

//
// Called at sim shutdown
//
PLUGIN_API void XPluginStop()
{
}

//
// Called when enabled via plugin admin
//
PLUGIN_API int XPluginEnable()
{
    // Set up a flight loop callback to update our state so we don't
    // lag the sim by checking every frame
    XPLMCreateFlightLoop_t flightLoopParams;
    flightLoopParams.structSize = sizeof(flightLoopParams);
    flightLoopParams.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
    flightLoopParams.callbackFunc = FlightLoopCallback;
    flightLoopParams.refcon = NULL;

    gvar_flightloopId = XPLMCreateFlightLoop(&flightLoopParams);

    XPLMScheduleFlightLoop(gvar_flightloopId, UPDATE_TIME, 1);

    // Return success
    return 1;
}

//
// Called when disabled via plugin admin
//
PLUGIN_API void XPluginDisable()
{
    // Destroy the callback timer
    XPLMDestroyFlightLoop(gvar_flightloopId);
}

//
// Called when a plugin message is received from the sim
//
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void *inParam)
{
    switch (inMsg)
    {
        case XPLM_MSG_PLANE_LOADED:
            // User's plane has been reloaded, reset the reminder
            if (((size_t) inParam) == 0)
                gvar_acknowledged = false;
            break;
    }
}

//
// Menu item callback handler
//
static void MenuCallback(void *inMenuRef, void *inItemRef)
{
    switch ((size_t) inItemRef)
    {
        case MENU_ID_ACKNOWLEDGE:
            // Acknowledged via the menu, hide the reminder
            gvar_acknowledged = true;
            break;
        default:
            break;
    }
}

//
// Graphics redraw callback handler
//
static int DrawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon)
{
    float colReminder1[] = {0.5, 0.5, 0.5};
    float colReminder2[] = {1.0, 1.0, 1.0};

    int offsetX = (gvar_screenWidth - gvar_drawTextLength) / 2;
    int offsetY = gvar_screenWidth / 2;

    if (!gvar_acknowledged && gvar_batteryOn && !gvar_enginesRunning)
    {
        XPLMDrawTranslucentDarkBox(offsetX - 20, offsetY + 20, offsetX + gvar_drawTextLength + 20, offsetY - 10);
        XPLMDrawString(gvar_colFlip ? colReminder1 : colReminder2, offsetX, offsetY, gvar_drawText, NULL, xplmFont_Basic);
    }

    // Return success
    return 1;
}

//
// Flight loop periodic callback
//
static float FlightLoopCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon)
{
    // Flip the message display colour
    gvar_colFlip = !gvar_colFlip;

    // Update the aircraft state
    int engineState[8];

    gvar_batteryOn = XPLMGetDatai(gref_batteryOn);
    gvar_screenHeight = XPLMGetDatai(gref_screenHeight);
    gvar_screenWidth = XPLMGetDatai(gref_screenWidth);
    int numEngines = XPLMGetDatai(gref_numEngines);
    XPLMGetDatavi(gref_engineState, &engineState[0], 0, 8);

    int tmpEnginesRunning = 0;
    for (int n=0; n < numEngines; n++)
    {
        if (engineState[n])
            tmpEnginesRunning++;
    }

    gvar_enginesRunning = (tmpEnginesRunning > 0);

    // update frequency depends if we're actively reminding or not
    return gvar_acknowledged ? UPDATE_TIME * 6: UPDATE_TIME;
}

//
// Required entry point for Windows DLL
//
#ifdef IBM
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // Perform actions based on the reason for calling.
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        break;

    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:
        // Perform any necessary cleanup.
        break;
    }

    return TRUE; // Successful DLL_PROCESS_ATTACH.
}
#endif // IBM
