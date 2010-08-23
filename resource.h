/*++

Copyright (c) 2010 Evan Green

Module Name:

    Resource.h

Abstract:

    This header contains resource definitions for Starry Night GUI elements.

Author:

    Evan Green 22-Aug-2010

--*/

//
// ---------------------------------------------------------------- Definitions
//

#define ID_ICON 100

//
// Main dialog.
//

#define IDD_MAIN_WINDOW 10000
#define IDC_OK 101
#define IDC_CANCEL 102
#define IDE_BUILDING_HEIGHT 103
#define IDE_BUILDING_COUNT 104
#define IDC_FLASHER 105

//
// Define UI element sizes.
//

#define UI_BORDER 1
#define UI_BUTTON_WIDTH 40
#define UI_SMALL_BOX_WIDTH 20
#define UI_LINE_HEIGHT 12
#define UI_INITIAL_WIDTH 175
#define UI_INITIAL_HEIGHT OK_CANCEL_OFFSET_Y + UI_LINE_HEIGHT + UI_BORDER
#define UI_DESCRIPTION_WIDTH (UI_INITIAL_WIDTH / 2) - UI_BORDER

//
// Individual UI element definitions.
//

#define BUILDING_HEIGHT_OFFSET_Y UI_BORDER
#define BUILDING_COUNT_OFFSET_Y \
    BUILDING_HEIGHT_OFFSET_Y + UI_LINE_HEIGHT + (2 * UI_BORDER)

#define FLASHER_OFFSET_Y \
    BUILDING_COUNT_OFFSET_Y + UI_LINE_HEIGHT + (2 * UI_BORDER)

#define OK_OFFSET_X \
    (UI_INITIAL_WIDTH / 2) - UI_BUTTON_WIDTH - (4 * UI_BORDER)

#define OK_CANCEL_OFFSET_Y \
    FLASHER_OFFSET_Y + UI_LINE_HEIGHT + (2 * UI_BORDER)

#define CANCEL_OFFSET_X \
    (UI_INITIAL_WIDTH / 2) + (4 * UI_BORDER)

#define EDIT_BOX_OFFSET_X UI_DESCRIPTION_WIDTH + (2 * UI_BORDER)

