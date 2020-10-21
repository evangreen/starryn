/*++

Copyright (c) 2010 Evan Green

Module Name:

    starryn.c

Abstract:

    This module implements the Starry Night screensaver, based on the original
    Starry Night screensaver in the After Dark series.

Author:

    Evan Green 22-Aug-2010

Environment:

    Windows

--*/

//
// ------------------------------------------------------------------- Includes
//

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "resource.h"

//
// ---------------------------------------------------------------- Definitions
//

#define APPLICATION_NAME "Starry Night"

#define DLLEXPORT __declspec(dllexport)

#define SETTLING_TIME 500000
#define MOUSE_TOLERANCE 5

#define BUILDING_STYLE_COUNT 6
#define TILE_HEIGHT 8
#define TILE_WIDTH 8

//
// Configuration key names.
//

#define CONFIGURATION_FILE "Starryn.ini"
#define KEY_BUILDING_HEIGHT "BuildingHeight"
#define KEY_BUILDING_COUNT "BuildingCount"
#define KEY_FLASHER "Flasher"
#define KEY_TIMER_RATE "TimerRate"
#define KEY_STARS_PER_UPDATE "StarsPerUpdate"
#define KEY_BUILDINGS_PER_UPDATE "BuildingsPerUpdate"
#define KEY_RAIN_PER_UPDATE "RainPerUpdate"
#define KEY_FLASHER_PERIOD "FlasherPeriod"
#define KEY_SHOOTING_STAR_PERIOD "ShootingStarPeriod"
#define KEY_SHOOTING_STAR_DURATION "ShootingStarDuration"
#define KEY_SHOOTING_STAR_WIDTH "ShootingStarWidth"
#define KEY_MAX_SHOOTING_SPEED_X "MaxShootingSpeedX"
#define KEY_MAX_SHOOTING_SPEED_Y "MaxShootingSpeedY"
#define KEY_MIN_SHOOTING_SPEED_Y "MinShootingSpeedY"

//
// ------------------------------------------------------ Data Type Definitions
//

typedef struct _BUILDING {
    ULONG Style;
    ULONG Height;
    ULONG Width;
    ULONG BeginX;
    ULONG ZCoordinate;
} BUILDING, *PBUILDING;

typedef struct _STARRY_SCREEN {
    HWND Window;
    ULONG Width;
    ULONG Height;
    BOOLEAN Clear;
    BOOLEAN FlasherOn;
    BOOLEAN ShootingStarActive;
    PBUILDING Buildings;
    ULONG FlasherX;
    ULONG FlasherY;
    ULONG FlasherTime;
    ULONG ShootingStarTime;
    ULONG ShootingStarDuration;
    ULONG ShootingStarStartX;
    ULONG ShootingStarStartY;
    float ShootingStarVelocityX;
    float ShootingStarVelocityY;
} STARRY_SCREEN, *PSTARRY_SCREEN;

//
// -------------------------------------------------------- Function Prototypes
//

LRESULT
DLLEXPORT
WINAPI
ScreenSaverProc (
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
DLLEXPORT
WINAPI
ScreenSaverConfigureDialog (
    HWND Dialog,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOLEAN
SnInitializeGlobalState (
    VOID
    );

BOOLEAN
SnInitializeScreen (
    PSTARRY_SCREEN Screen,
    HWND Window
    );

VOID
SnDestroyGlobalState (
    VOID
    );

VOID
SnDestroyScreen (
    PSTARRY_SCREEN Screen
    );

VOID
CALLBACK
SnTimerEvent (
    UINT TimerId,
    UINT Message,
    DWORD_PTR User,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2
    );

BOOLEAN
SnpUpdate (
    PSTARRY_SCREEN Screen,
    ULONG Microseconds
    );

VOID
SnpDrawStars (
    PSTARRY_SCREEN Screen,
    HDC Dc
    );

VOID
SnpDrawBuildings (
    PSTARRY_SCREEN Screen,
    HDC Dc
    );

ULONG
SnpGetTopBuilding (
    PSTARRY_SCREEN Screen,
    ULONG ScreenX,
    ULONG ScreenY
    );

VOID
SnpDrawRain (
    PSTARRY_SCREEN Screen,
    HDC Dc
    );

VOID
SnpDrawFlasher (
    PSTARRY_SCREEN Screen,
    ULONG TimeElapsed,
    HDC Dc
    );

VOID
SnpDrawShootingStar (
    PSTARRY_SCREEN Screen,
    ULONG TimeElapsed,
    HDC Dc
    );

BOOLEAN
SnpTakeInParameters (
    HWND Window
    );

BOOLEAN
SnpSaveParameters (
    );

BOOLEAN
SnpLoadParameters (
    );

//
// -------------------------------------------------------------------- Globals
//

BOOLEAN ScreenSaverWindowed = FALSE;

//
// Starry Night Configuration.
//

ULONG SnTimerRateMs = 50;
COLORREF SnBackground = RGB(0, 0, 0);
COLORREF SnBuildingColor = RGB(248, 241, 3);
ULONG SnStarsPerUpdate = 12;
ULONG SnBuildingPixelsPerUpdate = 15;
ULONG SnBuildingCount = 100;
ULONG SnBuildingHeightPercent = 35;
ULONG SnMinBuildingWidth = 5;
ULONG SnMaxBuildingWidth = 18;
ULONG SnMinRainWidth = 2;
ULONG SnMaxRainWidth = 16;
ULONG SnRainDropsPerUpdate = 15;
BOOLEAN SnFlasherEnabled = TRUE;
ULONG SnFlasherPeriodMs = 1700;
ULONG SnMaxShootingStarPeriodMs = 25000;
ULONG SnMaxShootingStarDurationMs = 1000;
float SnMaxShootingStarSpeedX = 3.0;
float SnMinShootingStarSpeedY = 0.1;
float SnMaxShootingStarSpeedY = 1.0;
ULONG SnMaxShootingStarWidth = 4;

//
// Starry Night global state.
//

POINT SnMousePosition;
MMRESULT SnTimer = 0;
ULONG SnTotalTimeUs = 0;

//
// Starry Night building styles. Buildings are made up of tiled 8x8 blocks.
//

UCHAR SnBuildingTiles[BUILDING_STYLE_COUNT][TILE_HEIGHT][TILE_WIDTH] = {
    {
        {0, 0, 0, 0, 1, 0, 0, 1},
        {0, 0, 0, 0, 1, 0, 0, 1},
        {0, 0, 0, 0, 1, 0, 0, 1},
        {0, 0, 0, 0, 1, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        {1, 1, 0, 0, 1, 1, 0, 0},
        {1, 1, 0, 0, 1, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        {1, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        {0, 1, 0, 1, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        {1, 0, 0, 0, 1, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
    },
    {
        {0, 1, 1, 0, 1, 1, 0, 0},
        {0, 1, 1, 0, 1, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
    },
};

//
// Global state, currently only for one screen.
//

STARRY_SCREEN SnScreen;

//
// ------------------------------------------------------------------ Functions
//

INT
WINAPI
WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInst,
    LPSTR lpszCmdParam,
    INT nCmdShow
    )

/*++

Routine Description:

    This routine is the main entry point for a Win32 application.

Arguments:

    hInstance - Supplies a handle to the current instance of the application.

    hPrevInstance - Supplies a handle to the previous instance of the
        application.

    lpCmdLine - Supplies a pointer to a null-terminated string specifying the
        command line for the application, excluding the program name.

    nCmdShow - Specifies how the window is to be shown.

Return Value:

    Returns TRUE on success, FALSE on failure.

--*/

{

    WNDCLASS Class;
    BOOLEAN Configure;
    MSG Message;
    HWND Parent;
    RECT ParentRect;
    ULONG Properties;
    BOOLEAN Result;
    INT Return;
    HWND Window;
    ULONG WindowHeight;
    ULONG WindowWidth;

    Parent = NULL;
    WindowWidth = 1024;
    WindowHeight = 768;

    //
    // Parse any parameters. C runs the configure dialog.
    //

    Configure = FALSE;
    if ((strstr(lpszCmdParam, "/c") != NULL) ||
        (strstr(lpszCmdParam, "/C") != NULL)) {

        Configure = TRUE;
    }

    //
    // W runs the application in a window.
    //

    if ((strstr(lpszCmdParam, "/w") != NULL) ||
        (strstr(lpszCmdParam, "/W") != NULL)) {

        ScreenSaverWindowed = TRUE;
    }

    //
    // P or I also runs the application in a window with a parent.
    //

    if ((strstr(lpszCmdParam, "/p") != NULL) ||
        (strstr(lpszCmdParam, "/P") != NULL) ||
        (strstr(lpszCmdParam, "/i") != NULL) ||
        (strstr(lpszCmdParam, "/I") != NULL)) {


        Parent = (HWND)(ULONG_PTR)atoi(lpszCmdParam + 3);
        GetWindowRect(Parent, &ParentRect);
        if (IsWindow(Parent) == FALSE) {
            Return = 0;
            goto WinMainEnd;
        }

        WindowWidth = ParentRect.right - ParentRect.left;
        WindowHeight = ParentRect.bottom - ParentRect.top;
        ScreenSaverWindowed = TRUE;
    }

    //
    // Register the window class.
    //

    Class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    Class.lpfnWndProc = ScreenSaverProc;
    Class.cbClsExtra = 0;
    Class.cbWndExtra = 0;
    Class.hInstance = hInstance;
    Class.hIcon = NULL;
    Class.hCursor = NULL;
    Class.hbrBackground = NULL;
    Class.lpszMenuName = NULL;
    Class.lpszClassName = APPLICATION_NAME;
    Result = RegisterClass(&Class);
    if (Result == FALSE) {
        return 0;
    }

    SnpLoadParameters();

    //
    // For configurations, run the dialog box and quit.
    //

    if (Configure != FALSE) {
        DialogBox(GetModuleHandle(NULL),
                  MAKEINTRESOURCE(IDD_MAIN_WINDOW),
                  NULL,
                  ScreenSaverConfigureDialog);

        SnpSaveParameters();
        goto WinMainEnd;
    }

    //
    // Create the window.
    //

    if (ScreenSaverWindowed != FALSE) {
        if (Parent != NULL) {
            Properties = WS_VISIBLE | WS_CHILD;

        } else {
            Properties = WS_VISIBLE | WS_POPUP;
        }

        Window = CreateWindowEx(WS_EX_TOPMOST,
                                APPLICATION_NAME,
                                APPLICATION_NAME,
                                Properties,
                                0,
                                0,
                                WindowWidth,
                                WindowHeight,
                                Parent,
                                NULL,
                                hInstance,
                                NULL);

    } else {
        Window = CreateWindowEx(WS_EX_TOPMOST,
                                APPLICATION_NAME,
                                APPLICATION_NAME,
                                WS_VISIBLE | WS_POPUP,
                                GetSystemMetrics(SM_XVIRTUALSCREEN),
                                GetSystemMetrics(SM_YVIRTUALSCREEN),
                                GetSystemMetrics(SM_CXVIRTUALSCREEN),
                                GetSystemMetrics(SM_CYVIRTUALSCREEN),
                                NULL,
                                NULL,
                                hInstance,
                                NULL);
    }

    if (Window == NULL) {
        Return = 0;
        goto WinMainEnd;
    }

    if (ScreenSaverWindowed == FALSE) {
        ShowCursor(0);
    }

    SetFocus(Window);
    UpdateWindow(Window);

    //
    // Pump messages to the window.
    //

    while (GetMessage(&Message, NULL, 0, 0) > 0) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

WinMainEnd:
    ShowCursor(1);
    UnregisterClass(APPLICATION_NAME, hInstance);
    return Return;
}

LRESULT
DLLEXPORT
WINAPI
ScreenSaverProc (
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    This routine is the main message pump for the screen saver window. It
    receives messages pertaining to the window and handles interesting ones.

Arguments:

    hWnd - Supplies the handle for the overall window.

    Message - Supplies the message being sent to the window.

    WParam - Supplies the "width" parameter, basically the first parameter of
        the message.

    LParam - Supplies the "length" parameter, basically the second parameter of
        the message.

Return Value:

    Returns FALSE if the message was handled, or TRUE if the message was not
    handled and the default handler should be invoked.

--*/

{

    HDC Dc;
    POINT Difference;
    POINT MousePosition;
    BOOL Result;
    RECT ScreenRect;

    switch (Message) {
    case WM_CREATE:
        Result = SnInitializeGlobalState();
        if (Result == FALSE) {
            PostQuitMessage(0);
        }

        Result = SnInitializeScreen(&SnScreen, hWnd);
        if (Result == FALSE) {
            PostQuitMessage(0);
        }

        GetCursorPos(&SnMousePosition);
        break;

    case WM_ERASEBKGND:
        Dc = GetDC(hWnd);
        GetClientRect(hWnd, &ScreenRect);
        FillRect(Dc, &ScreenRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
        ReleaseDC(hWnd, Dc);
        break;

    case WM_DESTROY:
        SnDestroyScreen(&SnScreen);
        SnDestroyGlobalState();
        PostQuitMessage(0);
        return 0;

    case WM_SETCURSOR:
        if (ScreenSaverWindowed == FALSE) {
            SetCursor(NULL);
        }

        break;

    case WM_CLOSE:
        if (ScreenSaverWindowed == FALSE) {
            ShowCursor(1);
        }

        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_KEYDOWN:
    case WM_KEYUP:
        if (SnTotalTimeUs > SETTLING_TIME) {
            SendMessage(hWnd, WM_CLOSE, 0, 0);
        }

        break;

    case WM_MOUSEMOVE:

        //
        // Ignore mouse movements if the screen saver is in the preview window.
        //

        if (ScreenSaverWindowed != FALSE) {
            break;
        }

        //
        // Random little mouse movements or spurious messages need to be
        // tolerated. If the mouse has moved more than a few pixels in any
        // direction, the user really is controlling it, and the screensaver
        // needs to close.
        //

        GetCursorPos(&MousePosition);
        Difference.x = SnMousePosition.x - MousePosition.x;
        Difference.y = SnMousePosition.y - MousePosition.y;
        if (Difference.x < 0) {
            Difference.x = -Difference.x;
        }

        if (Difference.y < 0) {
            Difference.y = -Difference.y;
        }

        if (((Difference.x > MOUSE_TOLERANCE) ||
             (Difference.y > MOUSE_TOLERANCE)) &&
            (SnTotalTimeUs > SETTLING_TIME)) {

            SendMessage(hWnd, WM_CLOSE, 0, 0);
        }

        break;

    case WM_SYSCOMMAND:
        if((wParam == SC_SCREENSAVE) || (wParam == SC_CLOSE)) {
            return FALSE;
        }

        break;

    default:
        break;
    }

    return DefWindowProc(hWnd, Message, wParam, lParam);
}

INT_PTR
DLLEXPORT
WINAPI
ScreenSaverConfigureDialog (
    HWND Dialog,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    This routine is the main message pump for the screen saver configuration
    window. It processes interesting messages coming from the user interacting
    with the window.

Arguments:

    Dialog - Supplies the handle for the overall window.

    Message - Supplies the message being sent to the window.

    wParam - Supplies the "width" parameter, basically the first parameter of
        the message.

    wParam - Supplies the "length" parameter, basically the second parameter of
        the message.

Return Value:

    Returns FALSE if the message was handled, or TRUE if the message was not
    handled and the default handler should be invoked.

--*/

{

    HWND CheckBox;
    HWND EditBox;
    BOOLEAN Result;
    PSTR String;

    switch(Message) {
    case WM_INITDIALOG:
        String = malloc(1024);
        if (String == NULL) {
            return TRUE;
        }

        //
        // Load the edits with the current values (or defaults).
        //

        EditBox = GetDlgItem(Dialog, IDE_BUILDING_HEIGHT);
        sprintf(String, "%d", (UINT)SnBuildingHeightPercent);
        Edit_SetText(EditBox, String);
        EditBox = GetDlgItem(Dialog, IDE_BUILDING_COUNT);
        sprintf(String, "%d", (UINT)SnBuildingCount);
        Edit_SetText(EditBox, String);
        EditBox = GetDlgItem(Dialog, IDE_STARS_COUNT);
        sprintf(String, "%d", (UINT)SnStarsPerUpdate);
        Edit_SetText(EditBox, String);
        EditBox = GetDlgItem(Dialog, IDE_LIGHTS_COUNT);
        sprintf(String, "%d", (UINT)SnBuildingPixelsPerUpdate);
        Edit_SetText(EditBox, String);
        free(String);
        CheckBox = GetDlgItem(Dialog, IDC_FLASHER);
        if (SnFlasherEnabled != FALSE) {
            Button_SetCheck(CheckBox, BST_CHECKED);
        }

        return TRUE;

    case WM_HSCROLL:
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDC_OK:

        //
        // Save all the valid entries. Bail on invalid ones.
        //

        Result = SnpTakeInParameters(Dialog);
        if (Result == FALSE) {
            break;
        }

        //
        // Fall through on success.
        //

        case IDCANCEL:
        case IDC_CANCEL:
            EndDialog(Dialog, LOWORD(wParam) == IDOK);
            return TRUE;
        }

        break;
    }

    return FALSE;
}

//
// --------------------------------------------------------- Internal Functions
//

BOOLEAN
SnInitializeGlobalState (
    VOID
    )

/*++

Routine Description:

    This routine initializes application-wide state for the Starry Night
    screensaver.

Arguments:

    None.

Return Value:

    TRUE on success.

    FALSE on failure.

--*/

{

    BOOL Result;

    srand(time(NULL));

    //
    // Sanity check.
    //

    if (SnMinBuildingWidth == 0) {
        SnMinBuildingWidth = 1;
    }

    if (SnMaxBuildingWidth < SnMinBuildingWidth) {
        SnMaxBuildingWidth = SnMinBuildingWidth + 1;
    }

    if (SnFlasherPeriodMs == 0) {
        SnFlasherPeriodMs = 1;
    }

    //
    // Kick off the timer.
    //

    SnTimer = timeSetEvent(SnTimerRateMs,
                           SnTimerRateMs,
                           SnTimerEvent,
                           SnTimerRateMs * 1000,
                           TIME_PERIODIC | TIME_CALLBACK_FUNCTION);

    if (SnTimer == 0) {
        Result = FALSE;
        goto InitializeGlobalStateEnd;
    }

    Result = TRUE;

InitializeGlobalStateEnd:
    return Result;
}

BOOLEAN
SnInitializeScreen (
    PSTARRY_SCREEN Screen,
    HWND Window
    )

/*++

Routine Description:

    This routine initializes the Starry Night screen saver.

Arguments:

    Screen - Supplies a pointer to the screen to initialize.

    Window - Supplies a pointer to the window being initialized.

Return Value:

    TRUE on success.

    FALSE on failure.

--*/

{

    PBUILDING Buildings;
    ULONG BuildingIndex;
    ULONG FlasherBuilding;
    ULONG Index2;
    ULONG MaxActualHeight;
    ULONG MaxHeight;
    ULONG MinX;
    ULONG MinXIndex;
    float RandomHeight;
    BOOL Result;
    BUILDING Swap;
    RECT WindowSize;

    Buildings = NULL;
    memset(Screen, 0, sizeof(*Screen));
    Screen->Clear = TRUE;
    Screen->ShootingStarVelocityX = 0.0;
    Screen->ShootingStarVelocityY = 0.0;

    //
    // Save the window.
    //

    Screen->Window = Window;

    //
    // Determine the size of the screen.
    //

    Result = GetWindowRect(Window, &WindowSize);
    if (Result == FALSE) {
        goto InitializeScreenEnd;
    }

    Screen->Width = WindowSize.right - WindowSize.left;
    Screen->Height = WindowSize.bottom - WindowSize.top;

    //
    // Determine the maximum height of a building.
    //

    MaxHeight =
              ((Screen->Height * SnBuildingHeightPercent) / 100) / TILE_HEIGHT;

    //
    // Allocate and initialize the buildings.
    //

    Buildings = malloc(sizeof(BUILDING) * SnBuildingCount);
    if (Buildings == NULL) {
        Result = FALSE;
        goto InitializeScreenEnd;
    }

    Screen->Buildings = Buildings;
    MaxActualHeight = 0;
    for (BuildingIndex = 0;
         BuildingIndex < SnBuildingCount;
         BuildingIndex += 1) {

        Buildings[BuildingIndex].Style = rand() % BUILDING_STYLE_COUNT;

        //
        // Squaring the random height makes for a more interesting distribution
        // of buildings.
        //

        RandomHeight = (float)rand() / (float)RAND_MAX;
        Buildings[BuildingIndex].Height =
                               RandomHeight * RandomHeight * (float)MaxHeight;

        Buildings[BuildingIndex].Height += 1;
        Buildings[BuildingIndex].Width =
                          SnMinBuildingWidth +
                          (rand() % (SnMaxBuildingWidth - SnMinBuildingWidth));

        Buildings[BuildingIndex].BeginX = rand() % Screen->Width;
        Buildings[BuildingIndex].ZCoordinate = BuildingIndex + 1;

        //
        // The tallest building on the landscape gets the flasher.
        //

        if (Buildings[BuildingIndex].Height > MaxActualHeight) {
            MaxActualHeight = Buildings[BuildingIndex].Height;
            FlasherBuilding = BuildingIndex;
        }
    }

    //
    // Determine the flasher coordinates. The flasher goes at the center of the
    // top of the tallest building.
    //

    Screen->FlasherOn = FALSE;
    Screen->FlasherTime = rand() % SnFlasherPeriodMs;
    Screen->FlasherX = Buildings[FlasherBuilding].BeginX +
                       (Buildings[FlasherBuilding].Width * TILE_WIDTH / 2);

    Screen->FlasherY = Screen->Height -
                       (Buildings[FlasherBuilding].Height * TILE_HEIGHT);

    //
    // Sort the buildings by X coordinate.
    //

    for (BuildingIndex = 0;
         BuildingIndex < SnBuildingCount - 1;
         BuildingIndex += 1) {

        //
        // Find the building with the lowest X coordinate.
        //

        MinX = Screen->Width;
        MinXIndex = -1;
        for (Index2 = BuildingIndex; Index2 < SnBuildingCount; Index2 += 1) {
            if (Buildings[Index2].BeginX < MinX) {
                MinX = Buildings[Index2].BeginX;
                MinXIndex = Index2;
            }
        }

        //
        // Swap it into position.
        //

        if (BuildingIndex != MinXIndex) {
            Swap = Buildings[BuildingIndex];
            Buildings[BuildingIndex] = Buildings[MinXIndex];
            Buildings[MinXIndex] = Swap;
        }
    }

    Result = TRUE;

InitializeScreenEnd:
    if (Result == FALSE) {
        if (Buildings != NULL) {
            free(Buildings);
            Screen->Buildings = NULL;
        }
    }

    return Result;
}

VOID
SnDestroyGlobalState (
    VOID
    )

/*++

Routine Description:

    This routine tears down the Starry Night runtime.

Arguments:

    None.

Return Value:

    None.

--*/

{

    if (SnTimer != 0) {
        timeKillEvent(SnTimer);
        SnTimer = 0;
    }

    return;
}

VOID
SnDestroyScreen (
    PSTARRY_SCREEN Screen
    )

/*++

Routine Description:

    This routine destroys a single screen.

Arguments:

    Screen - Supplies a pointer to the screen to tear down.

Return Value:

    None.

--*/

{

    if (Screen->Buildings != NULL) {
        free(Screen->Buildings);
        Screen->Buildings = NULL;
    }

    return;
}

VOID
CALLBACK
SnTimerEvent (
    UINT TimerId,
    UINT Message,
    DWORD_PTR User,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2
    )

/*++

Routine Description:

    This routine is the callback from the timer.

Arguments:

    TimerId - Supplies the timer ID that fired. Unused.

    Message - Supplies the timer message. Unused.

    User - Supplies the user paramater. In this case this is the period of the
        timer.

    Parameter1 - Supplies an unused parameter.

    Parameter2 - Supplies an unused parameter.

Return Value:

    None.

--*/

{

    BOOLEAN Result;

    Result = SnpUpdate(&SnScreen, (ULONG)User);
    if (Result == FALSE) {
        PostQuitMessage(0);
    }

    return;
}

BOOLEAN
SnpUpdate (
    PSTARRY_SCREEN Screen,
    ULONG Microseconds
    )

/*++

Routine Description:

    This routine updates the Starry Night runtime.

Arguments:

    Screen - Supplies a pointer to the context for this screen.

    Microseconds - Supplies the number of microseconds that have gone by since
        the last update.

Return Value:

    TRUE on success.

    FALSE if a serious failure occurred.

--*/

{

    HBRUSH BackgroundBrush;
    HDC Dc;
    HBRUSH OriginalBrush;
    BOOLEAN Result;

    if (Microseconds == 0) {
        return FALSE;
    }

    Dc = GetDC(Screen->Window);

    //
    // Update main time.
    //

    SnTotalTimeUs += Microseconds;

    //
    // If the window has not been set up, clear everything now.
    //

    if (Screen->Clear != FALSE) {

        //
        // Wipe the screen.
        //

        BackgroundBrush = CreateSolidBrush(SnBackground);
        OriginalBrush = SelectObject(Dc, BackgroundBrush);
        ExtFloodFill(Dc, 0, 0, GetPixel(Dc, 0, 0), FLOODFILLSURFACE);
        SelectObject(Dc, OriginalBrush);
        DeleteObject(OriginalBrush);
        Screen->Clear = FALSE;
    }

    Result = TRUE;
    SnpDrawStars(Screen, Dc);
    SnpDrawBuildings(Screen, Dc);
    SnpDrawRain(Screen, Dc);
    SnpDrawShootingStar(Screen, Microseconds / 1000, Dc);
    SnpDrawFlasher(Screen, Microseconds / 1000, Dc);
    ReleaseDC(Screen->Window, Dc);
    return Result;
}

VOID
SnpDrawStars (
    PSTARRY_SCREEN Screen,
    HDC Dc
    )

/*++

Routine Description:

    This routine draws stars to the sky.

Arguments:

    Screen - Supplies the context for this screen.

    Dc - Supplies the device context to draw stars to.

Return Value:

    None.

--*/

{

    float RandomY;
    ULONG StarIndex;
    ULONG StarX;
    ULONG StarY;

    //
    // Randomly sprinkle a certain number of stars on the screen.
    //

    StarIndex = 0;
    while (StarIndex < SnStarsPerUpdate) {
        StarX = rand() % Screen->Width;

        //
        // Squaring the Y coordinate puts more stars at the top and gives it
        // a more realistic (and less static-ish) view.
        //

        RandomY = (float)rand() / (float)RAND_MAX;
        StarY = (ULONG)(RandomY * RandomY * (float)Screen->Height);
        if (SnpGetTopBuilding(Screen, StarX, StarY) != -1) {
           continue;
        }

        SetPixel(Dc,
                 StarX,
                 StarY,
                 RGB(rand() % 180, rand() % 180, rand() % 256));

        StarIndex += 1;
    }

    return;
}

VOID
SnpDrawBuildings (
    PSTARRY_SCREEN Screen,
    HDC Dc
    )

/*++

Routine Description:

    This routine draws little lights into buildings, each one a hard little
    worker.

Arguments:

    Screen - Supplies the context for this screen.

    Dc - Supplies the device context to draw stars to.

Return Value:

    None.

--*/

{

    ULONG Building;
    ULONG BuildingHeightRange;
    ULONG BuildingHeightOffset;
    ULONG PixelsOn;
    ULONG PotentialX;
    ULONG PotentialY;
    ULONG Style;
    ULONG TileX;
    ULONG TileY;

    BuildingHeightRange = Screen->Height - Screen->FlasherY;
    BuildingHeightOffset = Screen->FlasherY;
    PixelsOn = 0;
    while (PixelsOn < SnBuildingPixelsPerUpdate) {
        PotentialX = rand() % Screen->Width;
        PotentialY = BuildingHeightOffset + (rand() % BuildingHeightRange);
        Building = SnpGetTopBuilding(Screen, PotentialX, PotentialY);
        if (Building == -1) {
            continue;
        }

        TileX = (PotentialX - Screen->Buildings[Building].BeginX) % TILE_WIDTH;
        TileY = PotentialY % TILE_HEIGHT;
        Style = Screen->Buildings[Building].Style;
        if (SnBuildingTiles[Style][TileY][TileX] == 0) {
            continue;
        }

        SetPixel(Dc, PotentialX, PotentialY, SnBuildingColor);
        PixelsOn += 1;
    }

    return;
}

ULONG
SnpGetTopBuilding (
    PSTARRY_SCREEN Screen,
    ULONG ScreenX,
    ULONG ScreenY
    )

/*++

Routine Description:

    This routine determines which building the given pixel is in.

Arguments:

    Screen - Supplies the context for this screen.

    ScreenX - Supplies the X coordinate, in screen space.

    ScreenY - Supplies the Y coordinate, in screen space.

Return Value:

    Returns the building index at the given screen location, or -1 if the
    coordinate is filled with sky.

--*/

{

    PBUILDING Building;
    ULONG BuildingIndex;
    ULONG BuildingRight;
    ULONG BuildingTop;
    ULONG FrontBuilding;
    ULONG MaxZ;

    FrontBuilding = -1;
    MaxZ = 0;
    for (BuildingIndex = 0;
         BuildingIndex < SnBuildingCount;
         BuildingIndex += 1) {

        Building = &(Screen->Buildings[BuildingIndex]);

        //
        // The buildings are sorted by X coordinate. If this building starts
        // to the right of the pixel in question, none of the rest intersect.
        //

        if (Building->BeginX > ScreenX) {
            break;
        }

        //
        // Check to see if the pixel is inside this building.
        //

        BuildingTop = Screen->Height -
                      (Building->Height * TILE_HEIGHT);

        BuildingRight = Building->BeginX +
                        (Building->Width * TILE_WIDTH);

        if ((ScreenX >= Building->BeginX) &&
            (ScreenX < BuildingRight) &&
            (ScreenY > BuildingTop)) {

            //
            // If this is the front-most building, mark it as the new winner.
            //

            if (Building->ZCoordinate > MaxZ) {
                FrontBuilding = BuildingIndex;
                MaxZ = Building->ZCoordinate;
            }
        }
    }

    return FrontBuilding;
}

VOID
SnpDrawRain (
    PSTARRY_SCREEN Screen,
    HDC Dc
    )

/*++

Routine Description:

    This routine draws black rain onto the sky, giving the illusion that stars
    and lights are going back off.

Arguments:

    Screen - Supplies the context for this screen.

    Dc - Supplies the context to draw the black rain on.

Return Value:

    None.

--*/

{

    LOGBRUSH BrushStyle;
    ULONG DropIndex;
    ULONG LineWidth;
    HPEN OriginalPen;
    HPEN Pen;
    DWORD PenStyle;
    ULONG RainX;
    ULONG RainY;

    //
    // Create the pen and select it.
    //

    PenStyle = PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND;
    BrushStyle.lbStyle = BS_SOLID;
    BrushStyle.lbColor = RGB(0, 0, 0);
    BrushStyle.lbHatch = 0;
    for (DropIndex = 0; DropIndex < SnRainDropsPerUpdate; DropIndex += 1) {
        LineWidth = SnMinRainWidth +
                    (rand() % (SnMaxRainWidth - SnMinRainWidth));

        Pen = ExtCreatePen(PenStyle, LineWidth, &BrushStyle, 0, NULL);
        if (Pen == NULL) {
            continue;
        }

        OriginalPen = SelectObject(Dc, Pen);
        RainX = rand() % Screen->Width;
        RainY = rand() % Screen->Height;
        MoveToEx(Dc, RainX, RainY, NULL);
        LineTo(Dc, RainX + 1, RainY + 1);

        //
        // Return to the original pen and destroy this one.
        //

        SelectObject(Dc, OriginalPen);
        DeleteObject(Pen);
    }

    return;
}

VOID
SnpDrawFlasher (
    PSTARRY_SCREEN Screen,
    ULONG TimeElapsed,
    HDC Dc
    )

/*++

Routine Description:

    This routine draws the flasher, if enabled.

Arguments:

    Screen - Supplies the context for this screen.

    TimeElapsed - Supplies the time elapsed since the last update, in
        milliseconds.

    Dc - Supplies the context to draw the flasher on.

Return Value:

    None.

--*/

{

    BOOLEAN BlackOutFlasher;
    LOGBRUSH BrushStyle;
    ULONG LineWidth;
    HPEN OriginalPen;
    HPEN Pen;
    DWORD PenStyle;

    BlackOutFlasher = FALSE;

    if (SnFlasherEnabled == FALSE) {
        Screen->FlasherOn = FALSE;
        return;
    }

    Screen->FlasherTime += TimeElapsed;
    if (Screen->FlasherTime >= SnFlasherPeriodMs) {
        Screen->FlasherTime -= SnFlasherPeriodMs;
        if (Screen->FlasherOn == FALSE) {
            Screen->FlasherOn = TRUE;

        } else {
            Screen->FlasherOn = FALSE;
            BlackOutFlasher = TRUE;
        }
    }

    //
    // Create the pen and select it.
    //

    if ((Screen->FlasherOn != FALSE) || (BlackOutFlasher != FALSE)) {
        PenStyle = PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND;
        BrushStyle.lbStyle = BS_SOLID;
        if (Screen->FlasherOn != FALSE) {
            BrushStyle.lbColor = RGB(190, 0, 0);

        } else {
            BrushStyle.lbColor = RGB(0, 0, 0);
        }

        BrushStyle.lbHatch = 0;
        LineWidth = 7;
        Pen = ExtCreatePen(PenStyle, LineWidth, &BrushStyle, 0, NULL);
        if (Pen == NULL) {
            return;
        }

        OriginalPen = SelectObject(Dc, Pen);
        MoveToEx(Dc, Screen->FlasherX, Screen->FlasherY, NULL);
        LineTo(Dc, Screen->FlasherX, Screen->FlasherY);

        //
        // Return to the original pen and destroy this one.
        //

        SelectObject(Dc, OriginalPen);
        DeleteObject(Pen);
    }

    return;
}

VOID
SnpDrawShootingStar (
    PSTARRY_SCREEN Screen,
    ULONG TimeElapsed,
    HDC Dc
    )

/*++

Routine Description:

    This routine updates any shooting stars on the screen, for those watching
    very closely.

Arguments:

    Screen - Supplies the context for this screen.

    TimeElapsed - Supplies the time elapsed since the last update, in
        milliseconds.

    Dc - Supplies the context to draw the flasher on.

    Screen - Supplies the context for this screen.

Return Value:

    None.

--*/

{

    HPEN BackgroundPen;
    LOGBRUSH BrushStyle;
    ULONG CurrentX;
    ULONG CurrentY;
    ULONG LineWidth;
    ULONG MaxStarY;
    ULONG NewX;
    ULONG NewY;
    HPEN OriginalPen;
    HPEN Pen;
    DWORD PenStyle;
    float RandomY;

    BackgroundPen = NULL;
    Pen = NULL;
    MaxStarY = Screen->Height - (Screen->Height *
                                 SnBuildingHeightPercent / 100 / TILE_HEIGHT);

    //
    // If there is no shooting star now, count time until the decided period
    // has ended.
    //

    if (Screen->ShootingStarActive == FALSE) {

        //
        // If this causes the shooting star time to fire, set up the shooting
        // star.
        //

        if (Screen->ShootingStarTime <= TimeElapsed) {
            Screen->ShootingStarTime = 0;
            Screen->ShootingStarActive = TRUE;

            //
            // The shooting star should start somewhere between the top of the
            // buildings and the top of the screen.
            //

            Screen->ShootingStarStartX = rand() % Screen->Width;
            RandomY = (float)rand() / (float)RAND_MAX;
            Screen->ShootingStarStartY =
                                  (ULONG)(RandomY * RandomY * (float)MaxStarY);

            Screen->ShootingStarDuration =
                (rand() % SnMaxShootingStarDurationMs) + 1;

            Screen->ShootingStarVelocityX =
                (((float)rand() / (float)RAND_MAX) *
                 (2.0 * SnMaxShootingStarSpeedX)) -
                SnMaxShootingStarSpeedX;

            Screen->ShootingStarVelocityY =
                (((float)rand() / (float)RAND_MAX) *
                 (SnMaxShootingStarSpeedY - SnMinShootingStarSpeedY)) +
                SnMinShootingStarSpeedY;

        //
        // No shooting star now, keep counting down.
        //

        } else {
            Screen->ShootingStarTime -= TimeElapsed;
            return;
        }
    }

    //
    // Create the shooting star pen and the background pen that erases the
    // tail.
    //

    PenStyle = PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND;
    BrushStyle.lbStyle = BS_SOLID;
    BrushStyle.lbColor = RGB(100, 0, 0);
    BrushStyle.lbHatch = 0;
    LineWidth = (ULONG)(Screen->ShootingStarTime * SnMaxShootingStarWidth /
                        Screen->ShootingStarDuration);

    Pen = ExtCreatePen(PenStyle, LineWidth, &BrushStyle, 0, NULL);
    if (Pen == NULL) {
        goto DrawShootingStarEnd;
    }

    BrushStyle.lbColor = RGB(0, 0, 0);
    LineWidth = SnMaxShootingStarWidth + 1;
    BackgroundPen = ExtCreatePen(PenStyle, LineWidth, &BrushStyle, 0, NULL);
    if (BackgroundPen == NULL) {
        goto DrawShootingStarEnd;
    }

    //
    // Draw the shooting star line from the current location to the next
    // location.
    //

    CurrentX = Screen->ShootingStarStartX +
               ((float)Screen->ShootingStarTime *
                Screen->ShootingStarVelocityX);

    CurrentY = Screen->ShootingStarStartY +
               ((float)Screen->ShootingStarTime *
                Screen->ShootingStarVelocityY);

    OriginalPen = SelectObject(Dc, Pen);
    if (Screen->ShootingStarTime < Screen->ShootingStarDuration) {
        NewX = CurrentX + ((float)TimeElapsed * Screen->ShootingStarVelocityX);
        NewY = CurrentY + ((float)TimeElapsed * Screen->ShootingStarVelocityY);

        //
        // If the shooting star is about to fall behind a building, cut it off
        // now. Otherwise, draw it.
        //

        if (SnpGetTopBuilding(Screen, NewX, NewY) != -1) {
            Screen->ShootingStarTime = Screen->ShootingStarDuration;

        } else {
            MoveToEx(Dc, CurrentX, CurrentY, NULL);
            LineTo(Dc, NewX, NewY);
        }
    }

    //
    // Draw background from the start to the current value.
    //

    SelectObject(Dc, BackgroundPen);
    MoveToEx(Dc,
             Screen->ShootingStarStartX,
             Screen->ShootingStarStartY,
             NULL);

    LineTo(Dc, CurrentX, CurrentY);
    SelectObject(Dc, OriginalPen);

    //
    // Update the counters. If there is more time on the shooting star, just
    // update time.
    //

    if (Screen->ShootingStarTime < Screen->ShootingStarDuration) {
        Screen->ShootingStarTime += TimeElapsed;

    //
    // The shooting star is sadly over. Reset the counters and patiently wait
    // for the next one.
    //

    } else {
        Screen->ShootingStarActive = FALSE;
        Screen->ShootingStarTime = rand() % SnMaxShootingStarPeriodMs;
    }

DrawShootingStarEnd:
    if (Pen != NULL) {
        DeleteObject(Pen);
    }

    if (BackgroundPen != NULL) {
        DeleteObject(BackgroundPen);
    }

    return;
}

BOOLEAN
SnpTakeInParameters (
    HWND Window
    )

/*++

Routine Description:

    This routine retrieves the values from the configuration edit boxes and
    sets the globals to the new values.

Arguments:

    Window - Supplies a pointer to the dialog window.

Return Value:

    TRUE if all parameters were successfully read.

    FALSE if one or more of the parameters was incorrect.

--*/

{

    PSTR AfterScan;
    ULONG BuildingCount;
    ULONG BuildingHeight;
    ULONG BuildingPixelCount;
    HWND CheckBox;
    HWND EditBox;
    BOOLEAN FlasherEnabled;
    BOOLEAN Result;
    ULONG StarCount;
    PSTR String;

    String = malloc(50);
    if (String == NULL) {
        Result = FALSE;
        goto TakeInParametersEnd;
    }

    //
    // Get the building height percent.
    //

    EditBox = GetDlgItem(Window, IDE_BUILDING_HEIGHT);
    Edit_GetText(EditBox, String, 50);
    BuildingHeight = strtoul(String, NULL, 10);
    if ((BuildingHeight < 5) || (BuildingHeight > 95)) {
        MessageBox(NULL,
                   "Please enter a building height as the percentage of the "
                   "total screen height. Valid values are 5 through 95.",
                   "Error",
                   MB_OK);

        Result = FALSE;
        goto TakeInParametersEnd;
    }

    //
    // Get the building count.
    //

    EditBox = GetDlgItem(Window, IDE_BUILDING_COUNT);
    Edit_GetText(EditBox, String, 50);
    BuildingCount = strtoul(String, NULL, 10);
    if (BuildingCount > 1000) {
        MessageBox(NULL,
                   "Please enter the number of buildings. Valid values are 0 "
                   "to 1000.",
                   "Error",
                   MB_OK);

        Result = FALSE;
        goto TakeInParametersEnd;
    }

    //
    // Get the star count.
    //

    EditBox = GetDlgItem(Window, IDE_STARS_COUNT);
    Edit_GetText(EditBox, String, 50);
    StarCount = strtoul(String, &AfterScan, 10);
    if (AfterScan == String) {
        MessageBox(NULL,
                   "Please enter a number of stars per update.",
                   "Error",
                   MB_OK);

        Result = FALSE;
        goto TakeInParametersEnd;
    }

    //
    // Get the building pixel count.
    //

    EditBox = GetDlgItem(Window, IDE_LIGHTS_COUNT);
    Edit_GetText(EditBox, String, 50);
    BuildingPixelCount = strtoul(String, &AfterScan, 10);
    if (AfterScan == String) {
        MessageBox(NULL,
                   "Please enter a number of building pixels per update.",
                   "Error",
                   MB_OK);

        Result = FALSE;
        goto TakeInParametersEnd;
    }

    FlasherEnabled = TRUE;
    CheckBox = GetDlgItem(Window, IDC_FLASHER);
    if (SendMessage(CheckBox, BM_GETSTATE, 0, 0) == BST_UNCHECKED) {
        FlasherEnabled = FALSE;
    }

    //
    // All globals are deemed valid. Save them.
    //

    SnBuildingHeightPercent = BuildingHeight;
    SnBuildingCount = BuildingCount;
    SnStarsPerUpdate = StarCount;
    SnBuildingPixelsPerUpdate = BuildingPixelCount;
    SnFlasherEnabled = FlasherEnabled;
    Result = TRUE;

TakeInParametersEnd:
    if (String != NULL) {
        free(String);
    }

    return Result;
}

BOOLEAN
SnpSaveParameters (
    )

/*++

Routine Description:

    This routine writes the current configuration out to disk.

Arguments:

    None.

Return Value:

    TRUE on success.

    FALSE on failure.

--*/

{

    BOOLEAN EndResult;
    BOOLEAN Result;
    PSTR String;

    EndResult = TRUE;
    String = malloc(50);
    if (String == NULL) {
        EndResult = FALSE;
        goto SaveParametersEnd;
    }

    sprintf(String, "%d", (UINT)SnBuildingHeightPercent);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_BUILDING_HEIGHT,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }


    sprintf(String, "%d", (UINT)SnBuildingCount);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_BUILDING_COUNT,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%d", (UINT)SnFlasherEnabled);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_FLASHER,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%d", (UINT)SnTimerRateMs);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_TIMER_RATE,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }


    sprintf(String, "%d", (UINT)SnStarsPerUpdate);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_STARS_PER_UPDATE,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%d", (UINT)SnBuildingPixelsPerUpdate);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_BUILDINGS_PER_UPDATE,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%d", (UINT)SnRainDropsPerUpdate);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_RAIN_PER_UPDATE,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%d", (UINT)SnFlasherPeriodMs);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_FLASHER_PERIOD,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%d", (UINT)SnMaxShootingStarPeriodMs);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_SHOOTING_STAR_PERIOD,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%d", (UINT)SnMaxShootingStarDurationMs);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_SHOOTING_STAR_DURATION,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%d", (UINT)SnMaxShootingStarWidth);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_SHOOTING_STAR_WIDTH,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%.2f", SnMaxShootingStarSpeedX);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_MAX_SHOOTING_SPEED_X,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%.2f", SnMaxShootingStarSpeedY);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_MAX_SHOOTING_SPEED_Y,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

    sprintf(String, "%.2f", SnMinShootingStarSpeedY);
    Result = WritePrivateProfileString(APPLICATION_NAME,
                                       KEY_MIN_SHOOTING_SPEED_Y,
                                       String,
                                       CONFIGURATION_FILE);

    if (Result == FALSE) {
        EndResult = FALSE;
    }

SaveParametersEnd:
    if (String != NULL) {
        free(String);
    }

    return EndResult;
}

BOOLEAN
SnpLoadParameters (
    )

/*++

Routine Description:

    This routine reads the saved configuration in from disk.

Arguments:

    None.

Return Value:

    TRUE on success.

    FALSE on failure.

--*/

{

    BOOLEAN Result;
    PSTR String;

    SnBuildingHeightPercent = GetPrivateProfileInt(APPLICATION_NAME,
                                                   KEY_BUILDING_HEIGHT,
                                                   SnBuildingHeightPercent,
                                                   CONFIGURATION_FILE);

    SnBuildingCount = GetPrivateProfileInt(APPLICATION_NAME,
                                           KEY_BUILDING_COUNT,
                                           SnBuildingCount,
                                           CONFIGURATION_FILE);

    SnFlasherEnabled = GetPrivateProfileInt(APPLICATION_NAME,
                                            KEY_FLASHER,
                                            SnFlasherEnabled,
                                            CONFIGURATION_FILE);

    SnTimerRateMs = GetPrivateProfileInt(APPLICATION_NAME,
                                         KEY_TIMER_RATE,
                                         SnTimerRateMs,
                                         CONFIGURATION_FILE);

    SnStarsPerUpdate = GetPrivateProfileInt(APPLICATION_NAME,
                                            KEY_STARS_PER_UPDATE,
                                            SnStarsPerUpdate,
                                            CONFIGURATION_FILE);

    SnBuildingPixelsPerUpdate = GetPrivateProfileInt(APPLICATION_NAME,
                                                     KEY_BUILDINGS_PER_UPDATE,
                                                     SnBuildingPixelsPerUpdate,
                                                     CONFIGURATION_FILE);

    SnRainDropsPerUpdate = GetPrivateProfileInt(APPLICATION_NAME,
                                                KEY_RAIN_PER_UPDATE,
                                                SnRainDropsPerUpdate,
                                                CONFIGURATION_FILE);

    SnFlasherPeriodMs = GetPrivateProfileInt(APPLICATION_NAME,
                                             KEY_FLASHER_PERIOD,
                                             SnFlasherPeriodMs,
                                             CONFIGURATION_FILE);

    SnMaxShootingStarPeriodMs = GetPrivateProfileInt(APPLICATION_NAME,
                                                     KEY_SHOOTING_STAR_PERIOD,
                                                     SnMaxShootingStarPeriodMs,
                                                     CONFIGURATION_FILE);

    SnMaxShootingStarDurationMs = GetPrivateProfileInt(
                                                    APPLICATION_NAME,
                                                    KEY_SHOOTING_STAR_DURATION,
                                                    SnMaxShootingStarDurationMs,
                                                    CONFIGURATION_FILE);

    SnMaxShootingStarWidth = GetPrivateProfileInt(APPLICATION_NAME,
                                                  KEY_SHOOTING_STAR_WIDTH,
                                                  SnMaxShootingStarWidth,
                                                  CONFIGURATION_FILE);

    String = malloc(50);
    if (String == NULL) {
        Result = FALSE;
        goto LoadParametersEnd;
    }

    GetPrivateProfileString(APPLICATION_NAME,
                            KEY_MAX_SHOOTING_SPEED_X,
                            "3.0",
                            String,
                            50,
                            CONFIGURATION_FILE);

    SnMaxShootingStarSpeedX = strtof(String, NULL);
    if ((SnMaxShootingStarSpeedX < 0) || (SnMaxShootingStarSpeedX > 1000.0)) {
        SnMaxShootingStarSpeedX = 3.0;
    }

    GetPrivateProfileString(APPLICATION_NAME,
                            KEY_MAX_SHOOTING_SPEED_Y,
                            "1.0",
                            String,
                            50,
                            CONFIGURATION_FILE);

    SnMaxShootingStarSpeedY = strtof(String, NULL);
    if ((SnMaxShootingStarSpeedY < 0) || (SnMaxShootingStarSpeedY > 100.0)) {
        SnMaxShootingStarSpeedY = 1.0;
    }

    GetPrivateProfileString(APPLICATION_NAME,
                            KEY_MIN_SHOOTING_SPEED_Y,
                            "0.1",
                            String,
                            50,
                            CONFIGURATION_FILE);

    SnMinShootingStarSpeedY = strtof(String, NULL);
    if ((SnMinShootingStarSpeedY < 0) || (SnMinShootingStarSpeedY > 99.9)) {
        SnMinShootingStarSpeedY = 0.1;
    }

    free(String);
    Result = TRUE;

LoadParametersEnd:
    return Result;
}

