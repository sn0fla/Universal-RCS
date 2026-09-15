#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <string>
#include <sstream>
#include <commdlg.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")

#define IDT_MOUSE_MOVE        1

#define IDC_LEFT_SLIDER      101
#define IDC_RIGHT_SLIDER     102
#define IDC_UP_SLIDER        103
#define IDC_DOWN_SLIDER      104
#define IDC_SENS_SLIDER      105

#define IDC_SETKEY_BTN       106
#define IDC_KEY_DISPLAY      107
#define IDC_STATUS           108
#define IDC_SAVE_BTN         109
#define IDC_LOAD_BTN         110
#define IDC_CREATE_BTN       111
#define IDC_ACTIVATION_MODE  112

#define IDC_DELAY_ENABLE     120
#define IDC_DELAY_SLIDER     121
#define IDC_DELAY_LABEL      122
#define IDC_DELAY2_ENABLE    123

#define IDC_LEFT2_SLIDER     131
#define IDC_RIGHT2_SLIDER    132
#define IDC_UP2_SLIDER       133
#define IDC_DOWN2_SLIDER     134

#define IDC_VAL_LEFT         140
#define IDC_VAL_RIGHT        141
#define IDC_VAL_UP           142
#define IDC_VAL_DOWN         143
#define IDC_SENS_LABEL       144
#define IDC_VAL_LEFT2        145
#define IDC_VAL_RIGHT2       146
#define IDC_VAL_UP2          147
#define IDC_VAL_DOWN2        148
#define IDC_VAL_DELAY        149

#define IDC_LEFT3_SLIDER     151
#define IDC_RIGHT3_SLIDER    152
#define IDC_UP3_SLIDER       153
#define IDC_DOWN3_SLIDER     154
#define IDC_VAL_LEFT3        155
#define IDC_VAL_RIGHT3       156
#define IDC_VAL_UP3          157
#define IDC_VAL_DOWN3        158
#define IDC_DELAY2_SLIDER    159
#define IDC_VAL_DELAY2       160

#define IDC_MASTER_TOGGLE_BTN 161
#define IDC_MASTER_KEY_DISPLAY 162

#define WM_APP_UPDATE_KEY    (WM_APP + 1)
#define WM_APP_UPDATE_STATUS (WM_APP + 2)

#define CAPTURE_NONE         0
#define CAPTURE_ACTIVATION   1
#define CAPTURE_MASTER       2

#define MODE_HOLD_M1         0
#define MODE_HOLD_M1M2       1
#define MODE_TOGGLE_KEY      2
#define MODE_HOLD_KEY        3

HINSTANCE hInst;
HWND hMainWnd = nullptr;

HWND hTrackLeft, hTrackRight, hTrackUp, hTrackDown, hTrackSens;
HWND hTrackLeft2, hTrackRight2, hTrackUp2, hTrackDown2;
HWND hTrackLeft3, hTrackRight3, hTrackUp3, hTrackDown3;
HWND hDelayCheck, hDelayCheck2, hTrackDelay, hTrackDelay2, hDelayLabel;
HWND hStatus, hKeyDisplay, hActivationCombo, hSetKeyBtn;
HWND hMasterToggleBtn, hMasterKeyDisplay;

HFONT hUIFont = nullptr;
HFONT hUIFontBold = nullptr;

bool  active = false;
int   capturingMode = CAPTURE_NONE;

int   leftVal = 0, rightVal = 0, upVal = 0, downVal = 0;
int   leftVal2 = 0, rightVal2 = 0, upVal2 = 0, downVal2 = 0;
int   leftVal3 = 0, rightVal3 = 0, upVal3 = 0, downVal3 = 0;
int   sensVal = 10;
UINT  toggleKey = VK_F2;
UINT  masterToggleKey = VK_F3;
int   activationMode = MODE_HOLD_M1;

bool  secondaryEnabled = false;
bool  tertiaryEnabled = false;
int   delay1Ms = 500;
int   delay2Ms = 1000;
bool  masterEnabled = true;
bool  m1Down = false, m2Down = false;
bool  keyHeld = false;
int   stage = 0;
DWORD activationTime = 0;
DWORD stageTime = 0;

float accumX = 0.0f, accumY = 0.0f;

HHOOK hhkKeyboard = nullptr;
HHOOK hhkMouse = nullptr;

const float STEP_SCALE = 0.1f;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelMouseProc(int, WPARAM, LPARAM);
void UpdateStatusDisplay();
void UpdateKeyDisplay();
void UpdateMasterKeyDisplay();
void UpdateAllValueLabels(HWND hwnd);
void UpdateSliderLabel(HWND hwnd, int staticId, int value, bool isDelay = false);
void UpdateDelayControlsEnabled();
void SaveConfig(HWND);
bool LoadConfig(HWND);
void ResetConfig();
void ApplyLoadedConfig();
void UpdateActiveState();
void ActivateNow();
void DeactivateNow();

static int  SliderVal(HWND h) { return (int)SendMessage(h, TBM_GETPOS, 0, 0); }
static void SetSlider(HWND h, int v) { SendMessage(h, TBM_SETPOS, TRUE, v); }

void ApplyFontToChildren(HWND parent, HFONT hFont) {
    EnumChildWindows(parent, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, MAKELPARAM(TRUE, 0));
        return TRUE;
        }, (LPARAM)hFont);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    hInst = hInstance;
    timeBeginPeriod(1);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    hUIFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hUIFontBold = CreateFontW(-15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"RecoilControlClass";
    if (!RegisterClassEx(&wc)) {
        MessageBox(nullptr, L"Window Registration Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    hMainWnd = CreateWindowEx(0, L"RecoilControlClass", L"Recoil Control",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 980,
        nullptr, nullptr, hInstance, nullptr);
    if (!hMainWnd) {
        MessageBox(nullptr, L"Window Creation Failed!", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    hhkKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    if (!hhkKeyboard)
        MessageBox(hMainWnd, L"Failed to install keyboard hook!", L"Error", MB_ICONERROR);

    hhkMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);
    if (!hhkMouse)
        MessageBox(hMainWnd, L"Failed to install mouse hook!", L"Error", MB_ICONERROR);

    SetTimer(hMainWnd, IDT_MOUSE_MOVE, 1, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    timeEndPeriod(1);
    if (hhkKeyboard) UnhookWindowsHookEx(hhkKeyboard);
    if (hhkMouse)    UnhookWindowsHookEx(hhkMouse);

    if (hUIFont) DeleteObject(hUIFont);
    if (hUIFontBold) DeleteObject(hUIFontBold);

    return (int)msg.wParam;
}

static HWND MakeSliderRow(HWND parent, HFONT hFont,
    const wchar_t* labelText,
    int col1, int col2, int sw, int row,
    int sliderId, int valLabelId,
    int lo, int hi, int initVal)
{
    HWND hLabel = CreateWindowEx(0, L"STATIC", labelText, WS_CHILD | WS_VISIBLE,
        col1, row + 5, col2 - col1 - 10, 18, parent, nullptr, hInst, nullptr);
    SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND h = CreateWindowEx(0, TRACKBAR_CLASS, L"",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
        col2, row, sw, 28, parent, (HMENU)(INT_PTR)sliderId, hInst, nullptr);
    SendMessage(h, TBM_SETRANGE, TRUE, MAKELONG(lo, hi));
    SendMessage(h, TBM_SETPOS, TRUE, initVal);
    SendMessage(h, WM_SETFONT, (WPARAM)hFont, TRUE);

    wchar_t buf[16]; swprintf_s(buf, L"%d", initVal);
    HWND hVal = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", buf,
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        col2 + sw + 15, row + 4, 50, 22,
        parent, (HMENU)(INT_PTR)valLabelId, hInst, nullptr);
    SendMessage(hVal, WM_SETFONT, (WPARAM)hFont, TRUE);

    return h;
}

void UpdateSliderLabel(HWND hwnd, int staticId, int value, bool)
{
    wchar_t buf[16];
    swprintf_s(buf, L"%d", value);
    SetDlgItemText(hwnd, staticId, buf);
}

void UpdateAllValueLabels(HWND hwnd)
{
    UpdateSliderLabel(hwnd, IDC_VAL_LEFT, leftVal, false);
    UpdateSliderLabel(hwnd, IDC_VAL_RIGHT, rightVal, false);
    UpdateSliderLabel(hwnd, IDC_VAL_UP, upVal, false);
    UpdateSliderLabel(hwnd, IDC_VAL_DOWN, downVal, false);
    {
        wchar_t buf[16]; swprintf_s(buf, L"%.2f", sensVal / 10.0f);
        SetDlgItemText(hwnd, IDC_SENS_LABEL, buf);
    }
    UpdateSliderLabel(hwnd, IDC_VAL_LEFT2, leftVal2, false);
    UpdateSliderLabel(hwnd, IDC_VAL_RIGHT2, rightVal2, false);
    UpdateSliderLabel(hwnd, IDC_VAL_UP2, upVal2, false);
    UpdateSliderLabel(hwnd, IDC_VAL_DOWN2, downVal2, false);
    UpdateSliderLabel(hwnd, IDC_VAL_DELAY, delay1Ms, true);
    UpdateSliderLabel(hwnd, IDC_VAL_LEFT3, leftVal3, false);
    UpdateSliderLabel(hwnd, IDC_VAL_RIGHT3, rightVal3, false);
    UpdateSliderLabel(hwnd, IDC_VAL_UP3, upVal3, false);
    UpdateSliderLabel(hwnd, IDC_VAL_DOWN3, downVal3, false);
    UpdateSliderLabel(hwnd, IDC_VAL_DELAY2, delay2Ms, true);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        LoadConfig(hwnd);

        int marginX = 20;
        int marginY = 20;
        int labelW = 90;
        int sliderX = 120;
        int sliderW = 320;
        int rowH = 34;

        HWND hGroup1 = CreateWindowEx(0, L"BUTTON", L" Primary Movement ",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            marginX, marginY, 560, 200, hwnd, nullptr, hInst, nullptr);
        SendMessage(hGroup1, WM_SETFONT, (WPARAM)hUIFontBold, TRUE);

        int currentY = marginY + 25;
        int col1 = marginX + 15;

        hTrackLeft = MakeSliderRow(hwnd, hUIFont, L"Left:", col1, sliderX, sliderW, currentY, IDC_LEFT_SLIDER, IDC_VAL_LEFT, 0, 100, leftVal);  currentY += rowH;
        hTrackRight = MakeSliderRow(hwnd, hUIFont, L"Right:", col1, sliderX, sliderW, currentY, IDC_RIGHT_SLIDER, IDC_VAL_RIGHT, 0, 100, rightVal); currentY += rowH;
        hTrackUp = MakeSliderRow(hwnd, hUIFont, L"Up:", col1, sliderX, sliderW, currentY, IDC_UP_SLIDER, IDC_VAL_UP, 0, 100, upVal);    currentY += rowH;
        hTrackDown = MakeSliderRow(hwnd, hUIFont, L"Down:", col1, sliderX, sliderW, currentY, IDC_DOWN_SLIDER, IDC_VAL_DOWN, 0, 100, downVal);  currentY += rowH;

        HWND hSensLabel = CreateWindowEx(0, L"STATIC", L"Sensitivity:", WS_CHILD | WS_VISIBLE,
            col1, currentY + 5, labelW, 18, hwnd, nullptr, hInst, nullptr);
        SendMessage(hSensLabel, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        hTrackSens = CreateWindowEx(0, TRACKBAR_CLASS, L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            sliderX, currentY, sliderW, 28, hwnd, (HMENU)IDC_SENS_SLIDER, hInst, nullptr);
        SendMessage(hTrackSens, TBM_SETRANGE, TRUE, MAKELONG(1, 1000));
        SendMessage(hTrackSens, TBM_SETPOS, TRUE, sensVal);
        SendMessage(hTrackSens, WM_SETFONT, (WPARAM)hUIFont, TRUE);
        {
            wchar_t buf[16]; swprintf_s(buf, L"%.2f", sensVal / 100.0f);
            HWND hSensVal = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", buf,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                sliderX + sliderW + 15, currentY + 4, 50, 22, hwnd, (HMENU)IDC_SENS_LABEL, hInst, nullptr);
            SendMessage(hSensVal, WM_SETFONT, (WPARAM)hUIFont, TRUE);
        }

        int actY = marginY + 210;
        HWND hGroup2 = CreateWindowEx(0, L"BUTTON", L" Activation ",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            marginX, actY, 560, 140, hwnd, nullptr, hInst, nullptr);
        SendMessage(hGroup2, WM_SETFONT, (WPARAM)hUIFontBold, TRUE);

        HWND hActLabel = CreateWindowEx(0, L"STATIC", L"Activation Mode:", WS_CHILD | WS_VISIBLE,
            marginX + 15, actY + 25, 110, 18, hwnd, nullptr, hInst, nullptr);
        SendMessage(hActLabel, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        hActivationCombo = CreateWindowEx(0, WC_COMBOBOX, L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
            marginX + 130, actY + 22, 200, 200, hwnd, (HMENU)IDC_ACTIVATION_MODE, hInst, nullptr);
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Hold Mouse 1");
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Hold Mouse 1+2");
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Toggle Key");
        SendMessage(hActivationCombo, CB_ADDSTRING, 0, (LPARAM)L"Hold Key");
        SendMessage(hActivationCombo, CB_SETCURSEL, activationMode, 0);
        SendMessage(hActivationCombo, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        hMasterToggleBtn = CreateWindowEx(0, L"BUTTON", L"Set Master Toggle Key",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            marginX + 15, actY + 65, 150, 28, hwnd, (HMENU)IDC_MASTER_TOGGLE_BTN, hInst, nullptr);
        SendMessage(hMasterToggleBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        hMasterKeyDisplay = CreateWindowEx(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
            marginX + 175, actY + 69, 200, 20, hwnd, (HMENU)IDC_MASTER_KEY_DISPLAY, hInst, nullptr);
        SendMessage(hMasterKeyDisplay, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        hSetKeyBtn = CreateWindowEx(0, L"BUTTON", L"Set Activation Key",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            marginX + 15, actY + 100, 150, 28, hwnd, (HMENU)IDC_SETKEY_BTN, hInst, nullptr);
        SendMessage(hSetKeyBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        hKeyDisplay = CreateWindowEx(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
            marginX + 175, actY + 104, 200, 20, hwnd, (HMENU)IDC_KEY_DISPLAY, hInst, nullptr);
        SendMessage(hKeyDisplay, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        int statY = actY + 145;
        hStatus = CreateWindowEx(0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE, marginX + 5, statY, 500, 18, hwnd, (HMENU)IDC_STATUS, hInst, nullptr);
        SendMessage(hStatus, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        int secY = statY + 30;
        HWND hGroup3 = CreateWindowEx(0, L"BUTTON", L" Secondary Movement (Post-Delay 1) ",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            marginX, secY, 560, 240, hwnd, nullptr, hInst, nullptr);
        SendMessage(hGroup3, WM_SETFONT, (WPARAM)hUIFontBold, TRUE);

        hDelayCheck = CreateWindowEx(0, L"BUTTON", L"Enable Secondary Stage",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            marginX + 15, secY + 25, 200, 20, hwnd, (HMENU)IDC_DELAY_ENABLE, hInst, nullptr);
        SendMessage(hDelayCheck, BM_SETCHECK, secondaryEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessage(hDelayCheck, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        int delayY = secY + 55;
        HWND hDelayLabel = CreateWindowEx(0, L"STATIC", L"Delay 1:", WS_CHILD | WS_VISIBLE,
            marginX + 15, delayY + 5, 80, 18, hwnd, nullptr, hInst, nullptr);
        SendMessage(hDelayLabel, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        hTrackDelay = CreateWindowEx(0, TRACKBAR_CLASS, L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            sliderX, delayY, sliderW, 28, hwnd, (HMENU)IDC_DELAY_SLIDER, hInst, nullptr);
        SendMessage(hTrackDelay, TBM_SETRANGE, TRUE, MAKELONG(100, 5000));
        SendMessage(hTrackDelay, TBM_SETPOS, TRUE, delay1Ms);
        SendMessage(hTrackDelay, WM_SETFONT, (WPARAM)hUIFont, TRUE);
        {
            wchar_t buf[16]; swprintf_s(buf, L"%d", delay1Ms);
            HWND hDelayVal = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", buf,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                sliderX + sliderW + 15, delayY + 4, 50, 22, hwnd, (HMENU)IDC_VAL_DELAY, hInst, nullptr);
            SendMessage(hDelayVal, WM_SETFONT, (WPARAM)hUIFont, TRUE);
        }
        HWND hMsLabel = CreateWindowEx(0, L"STATIC", L"ms", WS_CHILD | WS_VISIBLE,
            sliderX + sliderW + 70, delayY + 5, 24, 18, hwnd, nullptr, hInst, nullptr);
        SendMessage(hMsLabel, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        HWND hNote = CreateWindowEx(0, L"STATIC",
            L"After Delay 1, secondary values replace primary ones.",
            WS_CHILD | WS_VISIBLE | SS_WORDELLIPSIS,
            marginX + 15, delayY + 35, 520, 16, hwnd, nullptr, hInst, nullptr);
        SendMessage(hNote, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        int secRowY = delayY + 65;
        hTrackLeft2 = MakeSliderRow(hwnd, hUIFont, L"Left (2nd):", col1, sliderX, sliderW, secRowY, IDC_LEFT2_SLIDER, IDC_VAL_LEFT2, 0, 100, leftVal2);  secRowY += rowH;
        hTrackRight2 = MakeSliderRow(hwnd, hUIFont, L"Right (2nd):", col1, sliderX, sliderW, secRowY, IDC_RIGHT2_SLIDER, IDC_VAL_RIGHT2, 0, 100, rightVal2); secRowY += rowH;
        hTrackUp2 = MakeSliderRow(hwnd, hUIFont, L"Up (2nd):", col1, sliderX, sliderW, secRowY, IDC_UP2_SLIDER, IDC_VAL_UP2, 0, 100, upVal2);    secRowY += rowH;
        hTrackDown2 = MakeSliderRow(hwnd, hUIFont, L"Down (2nd):", col1, sliderX, sliderW, secRowY, IDC_DOWN2_SLIDER, IDC_VAL_DOWN2, 0, 100, downVal2);

        int terY = secY + 245;
        HWND hGroup4 = CreateWindowEx(0, L"BUTTON", L" Tertiary Movement (Post-Delay 2) ",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            marginX, terY, 560, 240, hwnd, nullptr, hInst, nullptr);
        SendMessage(hGroup4, WM_SETFONT, (WPARAM)hUIFontBold, TRUE);

        hDelayCheck2 = CreateWindowEx(0, L"BUTTON", L"Enable Tertiary Stage",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            marginX + 15, terY + 25, 200, 20, hwnd, (HMENU)IDC_DELAY2_ENABLE, hInst, nullptr);
        SendMessage(hDelayCheck2, BM_SETCHECK, tertiaryEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessage(hDelayCheck2, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        int delay2Y = terY + 55;
        HWND hDelay2Label = CreateWindowEx(0, L"STATIC", L"Delay 2:", WS_CHILD | WS_VISIBLE,
            marginX + 15, delay2Y + 5, 80, 18, hwnd, nullptr, hInst, nullptr);
        SendMessage(hDelay2Label, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        hTrackDelay2 = CreateWindowEx(0, TRACKBAR_CLASS, L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
            sliderX, delay2Y, sliderW, 28, hwnd, (HMENU)IDC_DELAY2_SLIDER, hInst, nullptr);
        SendMessage(hTrackDelay2, TBM_SETRANGE, TRUE, MAKELONG(100, 5000));
        SendMessage(hTrackDelay2, TBM_SETPOS, TRUE, delay2Ms);
        SendMessage(hTrackDelay2, WM_SETFONT, (WPARAM)hUIFont, TRUE);
        {
            wchar_t buf[16]; swprintf_s(buf, L"%d", delay2Ms);
            HWND hDelay2Val = CreateWindowEx(WS_EX_CLIENTEDGE, L"STATIC", buf,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                sliderX + sliderW + 15, delay2Y + 4, 50, 22, hwnd, (HMENU)IDC_VAL_DELAY2, hInst, nullptr);
            SendMessage(hDelay2Val, WM_SETFONT, (WPARAM)hUIFont, TRUE);
        }
        HWND hMs2Label = CreateWindowEx(0, L"STATIC", L"ms", WS_CHILD | WS_VISIBLE,
            sliderX + sliderW + 70, delay2Y + 5, 24, 18, hwnd, nullptr, hInst, nullptr);
        SendMessage(hMs2Label, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        HWND hNote2 = CreateWindowEx(0, L"STATIC",
            L"After Delay 2, tertiary values replace secondary ones.",
            WS_CHILD | WS_VISIBLE | SS_WORDELLIPSIS,
            marginX + 15, delay2Y + 35, 520, 16, hwnd, nullptr, hInst, nullptr);
        SendMessage(hNote2, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        int terRowY = delay2Y + 65;
        hTrackLeft3 = MakeSliderRow(hwnd, hUIFont, L"Left (3rd):", col1, sliderX, sliderW, terRowY, IDC_LEFT3_SLIDER, IDC_VAL_LEFT3, 0, 100, leftVal3);  terRowY += rowH;
        hTrackRight3 = MakeSliderRow(hwnd, hUIFont, L"Right (3rd):", col1, sliderX, sliderW, terRowY, IDC_RIGHT3_SLIDER, IDC_VAL_RIGHT3, 0, 100, rightVal3); terRowY += rowH;
        hTrackUp3 = MakeSliderRow(hwnd, hUIFont, L"Up (3rd):", col1, sliderX, sliderW, terRowY, IDC_UP3_SLIDER, IDC_VAL_UP3, 0, 100, upVal3);    terRowY += rowH;
        hTrackDown3 = MakeSliderRow(hwnd, hUIFont, L"Down (3rd):", col1, sliderX, sliderW, terRowY, IDC_DOWN3_SLIDER, IDC_VAL_DOWN3, 0, 100, downVal3);

        int btnY = terY + 255;
        HWND hSaveBtn = CreateWindowEx(0, L"BUTTON", L"Save Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            marginX, btnY, 110, 30, hwnd, (HMENU)IDC_SAVE_BTN, hInst, nullptr);
        SendMessage(hSaveBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        HWND hLoadBtn = CreateWindowEx(0, L"BUTTON", L"Load Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            marginX + 120, btnY, 110, 30, hwnd, (HMENU)IDC_LOAD_BTN, hInst, nullptr);
        SendMessage(hLoadBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        HWND hNewBtn = CreateWindowEx(0, L"BUTTON", L"New Config", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            marginX + 240, btnY, 110, 30, hwnd, (HMENU)IDC_CREATE_BTN, hInst, nullptr);
        SendMessage(hNewBtn, WM_SETFONT, (WPARAM)hUIFont, TRUE);

        ApplyFontToChildren(hwnd, hUIFont);

        UpdateStatusDisplay();
        UpdateKeyDisplay();
        UpdateMasterKeyDisplay();
        UpdateAllValueLabels(hwnd);
        UpdateDelayControlsEnabled();
        break;
    }

    case WM_HSCROLL:
    {
        HWND hTrack = (HWND)lParam;
        int  pos = SliderVal(hTrack);
        int  id = GetDlgCtrlID(hTrack);

        switch (id) {
        case IDC_LEFT_SLIDER:   leftVal = pos; UpdateSliderLabel(hwnd, IDC_VAL_LEFT, pos); break;
        case IDC_RIGHT_SLIDER:  rightVal = pos; UpdateSliderLabel(hwnd, IDC_VAL_RIGHT, pos); break;
        case IDC_UP_SLIDER:     upVal = pos; UpdateSliderLabel(hwnd, IDC_VAL_UP, pos); break;
        case IDC_DOWN_SLIDER:   downVal = pos; UpdateSliderLabel(hwnd, IDC_VAL_DOWN, pos); break;
        case IDC_LEFT2_SLIDER:  leftVal2 = pos; UpdateSliderLabel(hwnd, IDC_VAL_LEFT2, pos); break;
        case IDC_RIGHT2_SLIDER: rightVal2 = pos; UpdateSliderLabel(hwnd, IDC_VAL_RIGHT2, pos); break;
        case IDC_UP2_SLIDER:    upVal2 = pos; UpdateSliderLabel(hwnd, IDC_VAL_UP2, pos); break;
        case IDC_DOWN2_SLIDER:  downVal2 = pos; UpdateSliderLabel(hwnd, IDC_VAL_DOWN2, pos); break;
        case IDC_LEFT3_SLIDER:  leftVal3 = pos; UpdateSliderLabel(hwnd, IDC_VAL_LEFT3, pos); break;
        case IDC_RIGHT3_SLIDER: rightVal3 = pos; UpdateSliderLabel(hwnd, IDC_VAL_RIGHT3, pos); break;
        case IDC_UP3_SLIDER:    upVal3 = pos; UpdateSliderLabel(hwnd, IDC_VAL_UP3, pos); break;
        case IDC_DOWN3_SLIDER:  downVal3 = pos; UpdateSliderLabel(hwnd, IDC_VAL_DOWN3, pos); break;
        case IDC_SENS_SLIDER:
            sensVal = pos;
            { wchar_t buf[16]; swprintf_s(buf, L"%.2f", sensVal / 100.0f); SetDlgItemText(hwnd, IDC_SENS_LABEL, buf); }
            break;
        case IDC_DELAY_SLIDER:
            delay1Ms = pos;
            UpdateSliderLabel(hwnd, IDC_VAL_DELAY, pos, true);
            break;
        case IDC_DELAY2_SLIDER:
            delay2Ms = pos;
            UpdateSliderLabel(hwnd, IDC_VAL_DELAY2, pos, true);
            break;
        }
        break;
    }

    case WM_COMMAND:
    {
        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == IDC_SETKEY_BTN && capturingMode == CAPTURE_NONE) {
            capturingMode = CAPTURE_ACTIVATION;
            SetWindowText(hSetKeyBtn, L"Press a key...");
            EnableWindow(hSetKeyBtn, FALSE);
        }
        else if (id == IDC_MASTER_TOGGLE_BTN && capturingMode == CAPTURE_NONE) {
            capturingMode = CAPTURE_MASTER;
            SetWindowText(hMasterToggleBtn, L"Press a key...");
            EnableWindow(hMasterToggleBtn, FALSE);
        }
        else if (id == IDC_DELAY_ENABLE) {
            secondaryEnabled = (SendMessage(hDelayCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
            UpdateDelayControlsEnabled();
        }
        else if (id == IDC_DELAY2_ENABLE) {
            tertiaryEnabled = (SendMessage(hDelayCheck2, BM_GETCHECK, 0, 0) == BST_CHECKED);
            UpdateDelayControlsEnabled();
        }
        else if (id == IDC_ACTIVATION_MODE && code == CBN_SELCHANGE) {
            activationMode = (int)SendMessage(hActivationCombo, CB_GETCURSEL, 0, 0);
            if (activationMode < 0 || activationMode > 3) activationMode = MODE_HOLD_M1;
            DeactivateNow();
            keyHeld = false;
            UpdateStatusDisplay();
        }
        else if (id == IDC_SAVE_BTN) {
            SaveConfig(hwnd);
            MessageBox(hwnd, L"Config saved.", L"Info", MB_OK);
        }
        else if (id == IDC_LOAD_BTN) {
            if (LoadConfig(hwnd)) {
                ApplyLoadedConfig();
                MessageBox(hwnd, L"Config loaded.", L"Info", MB_OK);
            }
        }
        else if (id == IDC_CREATE_BTN) {
            ResetConfig();
            ApplyLoadedConfig();
            MessageBox(hwnd, L"New config created with defaults.", L"Info", MB_OK);
        }
        break;
    }

    case WM_TIMER:
    {
        if (wParam != IDT_MOUSE_MOVE || !active) break;

        int lv, rv, uv, dv;

        if (secondaryEnabled || tertiaryEnabled) {
            DWORD now = timeGetTime();
            DWORD elapsed = now - activationTime;
            if (secondaryEnabled && stage < 1 && elapsed >= (DWORD)delay1Ms) {
                stage = 1;
                stageTime = now;
                UpdateStatusDisplay();
            }
            if (tertiaryEnabled && stage < 2) {
                DWORD tertiaryTrigger = (DWORD)delay1Ms + (DWORD)delay2Ms;
                if (!secondaryEnabled) tertiaryTrigger = (DWORD)delay1Ms;
                if (elapsed >= tertiaryTrigger) {
                    stage = 2;
                    UpdateStatusDisplay();
                }
            }
        }

        if (stage == 0) {
            lv = leftVal;  rv = rightVal;  uv = upVal;  dv = downVal;
        }
        else if (stage == 1) {
            lv = leftVal2; rv = rightVal2; uv = upVal2; dv = downVal2;
        }
        else {
            lv = leftVal3; rv = rightVal3; uv = upVal3; dv = downVal3;
        }

        int rawDX = rv - lv;
        int rawDY = dv - uv;

        if (rawDX != 0 || rawDY != 0) {
            float invSens = 1.0f / (sensVal / 100.0f);
            accumX += rawDX * invSens * STEP_SCALE;
            accumY += rawDY * invSens * STEP_SCALE;

            int ix = (int)accumX;
            int iy = (int)accumY;
            accumX -= ix;
            accumY -= iy;

            if (ix != 0 || iy != 0) {
                INPUT input = {};
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_MOVE;
                input.mi.dx = ix;
                input.mi.dy = iy;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
        break;
    }

    case WM_APP_UPDATE_KEY:
        if (capturingMode == CAPTURE_ACTIVATION) {
            capturingMode = CAPTURE_NONE;
            EnableWindow(hSetKeyBtn, TRUE);
            SetWindowText(hSetKeyBtn, L"Set Activation Key");
            UpdateKeyDisplay();
        }
        else if (capturingMode == CAPTURE_MASTER) {
            capturingMode = CAPTURE_NONE;
            EnableWindow(hMasterToggleBtn, TRUE);
            SetWindowText(hMasterToggleBtn, L"Set Master Toggle Key");
            UpdateMasterKeyDisplay();
        }
        break;

    case WM_APP_UPDATE_STATUS:
        UpdateStatusDisplay();
        break;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_MOUSE_MOVE);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ActivateNow()
{
    if (active) return;
    active = true;
    accumX = accumY = 0.0f;
    stage = 0;
    activationTime = timeGetTime();
    stageTime = activationTime;
    PostMessage(hMainWnd, WM_APP_UPDATE_STATUS, 0, 0);
}

void DeactivateNow()
{
    if (!active) return;
    active = false;
    accumX = accumY = 0.0f;
    stage = 0;
    PostMessage(hMainWnd, WM_APP_UPDATE_STATUS, 0, 0);
}

void UpdateActiveState()
{
    if (!masterEnabled) {
        DeactivateNow();
        return;
    }
    if (activationMode == MODE_TOGGLE_KEY) return;
    bool shouldBeActive = false;
    if (activationMode == MODE_HOLD_M1)   shouldBeActive = m1Down;
    else if (activationMode == MODE_HOLD_M1M2) shouldBeActive = m1Down && m2Down;
    else if (activationMode == MODE_HOLD_KEY)  shouldBeActive = keyHeld;
    if (shouldBeActive) ActivateNow();
    else                DeactivateNow();
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool keyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        if (capturingMode != CAPTURE_NONE && keyDown) {
            if (capturingMode == CAPTURE_ACTIVATION) {
                toggleKey = p->vkCode;
            }
            else if (capturingMode == CAPTURE_MASTER) {
                masterToggleKey = p->vkCode;
            }
            PostMessage(hMainWnd, WM_APP_UPDATE_KEY, 0, 0);
            return 1;
        }

        static bool masterKeyPhysDown = false;
        if (p->vkCode == masterToggleKey) {
            if (keyDown && !masterKeyPhysDown) {
                masterKeyPhysDown = true;
                masterEnabled = !masterEnabled;
                UpdateActiveState();
                UpdateStatusDisplay();
                return 1;
            }
            if (keyUp) {
                masterKeyPhysDown = false;
                return 1;
            }
            return 1;
        }

        static bool actKeyPhysDown = false;
        if (p->vkCode == toggleKey) {
            if (activationMode == MODE_TOGGLE_KEY) {
                if (keyDown && !actKeyPhysDown) {
                    actKeyPhysDown = true;
                    if (!masterEnabled) return 1;
                    if (active) DeactivateNow();
                    else        ActivateNow();
                    return 1;
                }
                if (keyUp) { actKeyPhysDown = false; return 1; }
                return 1;
            }
            else if (activationMode == MODE_HOLD_KEY) {
                if (keyDown && !actKeyPhysDown) {
                    actKeyPhysDown = true;
                    keyHeld = true;
                    UpdateActiveState();
                    return 1;
                }
                if (keyUp) {
                    actKeyPhysDown = false;
                    keyHeld = false;
                    UpdateActiveState();
                    return 1;
                }
                return 1;
            }
        }
    }
    return CallNextHookEx(hhkKeyboard, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION) {
        switch (wParam) {
        case WM_LBUTTONDOWN: m1Down = true;  break;
        case WM_LBUTTONUP:   m1Down = false; break;
        case WM_RBUTTONDOWN: m2Down = true;  break;
        case WM_RBUTTONUP:   m2Down = false; break;
        }
        if (activationMode == MODE_HOLD_M1 || activationMode == MODE_HOLD_M1M2) {
            UpdateActiveState();
        }
    }
    return CallNextHookEx(hhkMouse, nCode, wParam, lParam);
}

void UpdateStatusDisplay()
{
    if (!hStatus) return;
    std::wstring text = L"Status: ";
    if (!masterEnabled) {
        text += L"Disabled (Master Toggle Off)";
    }
    else if (active) {
        if (stage == 0) text += L"Active - Primary";
        else if (stage == 1) text += L"Active - Secondary";
        else text += L"Active - Tertiary";
    }
    else {
        text += L"Inactive";
    }
    if (activationMode == MODE_HOLD_M1)        text += L"  [Hold M1]";
    else if (activationMode == MODE_HOLD_M1M2) text += L"  [Hold M1+M2]";
    else if (activationMode == MODE_TOGGLE_KEY) text += L"  [Toggle Key]";
    else if (activationMode == MODE_HOLD_KEY)   text += L"  [Hold Key]";
    SetWindowText(hStatus, text.c_str());
}

void UpdateKeyDisplay()
{
    if (!hKeyDisplay) return;
    wchar_t keyName[64] = {};
    UINT scanCode = MapVirtualKey(toggleKey, MAPVK_VK_TO_VSC);
    LONG lParamKey = scanCode << 16;
    switch (toggleKey) {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
    case VK_PRIOR:  case VK_NEXT:   case VK_LEFT: case VK_RIGHT:
    case VK_UP:     case VK_DOWN:   case VK_NUMLOCK: case VK_CANCEL:
    case VK_DIVIDE: case VK_RSHIFT: case VK_RCONTROL: case VK_RMENU:
        lParamKey |= (1 << 24);
        break;
    }
    GetKeyNameTextW(lParamKey, keyName, 64);
    SetWindowText(hKeyDisplay, (std::wstring(L"Key: ") + keyName).c_str());
}

void UpdateMasterKeyDisplay()
{
    if (!hMasterKeyDisplay) return;
    wchar_t keyName[64] = {};
    UINT scanCode = MapVirtualKey(masterToggleKey, MAPVK_VK_TO_VSC);
    LONG lParamKey = scanCode << 16;
    switch (masterToggleKey) {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END:
    case VK_PRIOR:  case VK_NEXT:   case VK_LEFT: case VK_RIGHT:
    case VK_UP:     case VK_DOWN:   case VK_NUMLOCK: case VK_CANCEL:
    case VK_DIVIDE: case VK_RSHIFT: case VK_RCONTROL: case VK_RMENU:
        lParamKey |= (1 << 24);
        break;
    }
    GetKeyNameTextW(lParamKey, keyName, 64);
    SetWindowText(hMasterKeyDisplay, (std::wstring(L"Key: ") + keyName).c_str());
}

void UpdateDelayControlsEnabled()
{
    BOOL secEn = secondaryEnabled ? TRUE : FALSE;
    BOOL terEn = tertiaryEnabled ? TRUE : FALSE;

    if (hTrackDelay)  EnableWindow(hTrackDelay, secEn);
    if (hTrackLeft2)  EnableWindow(hTrackLeft2, secEn);
    if (hTrackRight2) EnableWindow(hTrackRight2, secEn);
    if (hTrackUp2)    EnableWindow(hTrackUp2, secEn);
    if (hTrackDown2)  EnableWindow(hTrackDown2, secEn);

    if (hTrackDelay2) EnableWindow(hTrackDelay2, terEn);
    if (hTrackLeft3)  EnableWindow(hTrackLeft3, terEn);
    if (hTrackRight3) EnableWindow(hTrackRight3, terEn);
    if (hTrackUp3)    EnableWindow(hTrackUp3, terEn);
    if (hTrackDown3)  EnableWindow(hTrackDown3, terEn);

    HWND secIds[] = {
        GetDlgItem(hMainWnd, IDC_VAL_DELAY),
        GetDlgItem(hMainWnd, IDC_VAL_LEFT2),
        GetDlgItem(hMainWnd, IDC_VAL_RIGHT2),
        GetDlgItem(hMainWnd, IDC_VAL_UP2),
        GetDlgItem(hMainWnd, IDC_VAL_DOWN2),
    };
    for (HWND h : secIds) if (h) EnableWindow(h, secEn);

    HWND terIds[] = {
        GetDlgItem(hMainWnd, IDC_VAL_DELAY2),
        GetDlgItem(hMainWnd, IDC_VAL_LEFT3),
        GetDlgItem(hMainWnd, IDC_VAL_RIGHT3),
        GetDlgItem(hMainWnd, IDC_VAL_UP3),
        GetDlgItem(hMainWnd, IDC_VAL_DOWN3),
    };
    for (HWND h : terIds) if (h) EnableWindow(h, terEn);
}

void ApplyLoadedConfig()
{
    SetSlider(hTrackLeft, leftVal);
    SetSlider(hTrackRight, rightVal);
    SetSlider(hTrackUp, upVal);
    SetSlider(hTrackDown, downVal);
    SetSlider(hTrackSens, sensVal);
    SetSlider(hTrackLeft2, leftVal2);
    SetSlider(hTrackRight2, rightVal2);
    SetSlider(hTrackUp2, upVal2);
    SetSlider(hTrackDown2, downVal2);
    SetSlider(hTrackLeft3, leftVal3);
    SetSlider(hTrackRight3, rightVal3);
    SetSlider(hTrackUp3, upVal3);
    SetSlider(hTrackDown3, downVal3);
    SetSlider(hTrackDelay, delay1Ms);
    SetSlider(hTrackDelay2, delay2Ms);
    SendMessage(hActivationCombo, CB_SETCURSEL, activationMode, 0);
    SendMessage(hDelayCheck, BM_SETCHECK, secondaryEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(hDelayCheck2, BM_SETCHECK, tertiaryEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
    UpdateKeyDisplay();
    UpdateMasterKeyDisplay();
    UpdateAllValueLabels(hMainWnd);
    UpdateDelayControlsEnabled();
    UpdateStatusDisplay();
}

std::wstring BrowseConfigFile(HWND hwnd, bool saveDialog)
{
    wchar_t fileName[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"INI Files (*.ini)\0*.ini\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"ini";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (saveDialog) {
        wcscpy_s(fileName, L"config.ini");
        ofn.Flags |= OFN_OVERWRITEPROMPT;
        return GetSaveFileNameW(&ofn) ? fileName : L"";
    }
    else {
        ofn.Flags |= OFN_FILEMUSTEXIST;
        return GetOpenFileNameW(&ofn) ? fileName : L"";
    }
}

void SaveConfig(HWND hwnd)
{
    std::wstring ini = BrowseConfigFile(hwnd, true);
    if (ini.empty()) return;
    auto W = [](int v) { return std::to_wstring(v); };
    const wchar_t* f = ini.c_str();

    WritePrivateProfileStringW(L"Recoil", L"Left", W(leftVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Right", W(rightVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Up", W(upVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Down", W(downVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Sensitivity", W(sensVal).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"ToggleKey", W(toggleKey).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"MasterToggleKey", W(masterToggleKey).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"ActivationMode", W(activationMode).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"SecondaryEnabled", W(secondaryEnabled ? 1 : 0).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"TertiaryEnabled", W(tertiaryEnabled ? 1 : 0).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Delay1Ms", W(delay1Ms).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Delay2Ms", W(delay2Ms).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Left2", W(leftVal2).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Right2", W(rightVal2).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Up2", W(upVal2).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Down2", W(downVal2).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Left3", W(leftVal3).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Right3", W(rightVal3).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Up3", W(upVal3).c_str(), f);
    WritePrivateProfileStringW(L"Recoil", L"Down3", W(downVal3).c_str(), f);
}

bool LoadConfig(HWND hwnd)
{
    std::wstring ini = BrowseConfigFile(hwnd, false);
    if (ini.empty()) return false;
    const wchar_t* f = ini.c_str();

    leftVal = GetPrivateProfileIntW(L"Recoil", L"Left", 0, f);
    rightVal = GetPrivateProfileIntW(L"Recoil", L"Right", 0, f);
    upVal = GetPrivateProfileIntW(L"Recoil", L"Up", 0, f);
    downVal = GetPrivateProfileIntW(L"Recoil", L"Down", 0, f);
    sensVal = GetPrivateProfileIntW(L"Recoil", L"Sensitivity", 100, f);
    toggleKey = GetPrivateProfileIntW(L"Recoil", L"ToggleKey", VK_F2, f);
    masterToggleKey = GetPrivateProfileIntW(L"Recoil", L"MasterToggleKey", VK_F3, f);
    activationMode = GetPrivateProfileIntW(L"Recoil", L"ActivationMode", MODE_HOLD_M1, f);
    secondaryEnabled = GetPrivateProfileIntW(L"Recoil", L"SecondaryEnabled", 0, f) != 0;
    tertiaryEnabled = GetPrivateProfileIntW(L"Recoil", L"TertiaryEnabled", 0, f) != 0;
    delay1Ms = GetPrivateProfileIntW(L"Recoil", L"Delay1Ms", 500, f);
    delay2Ms = GetPrivateProfileIntW(L"Recoil", L"Delay2Ms", 1000, f);
    leftVal2 = GetPrivateProfileIntW(L"Recoil", L"Left2", 0, f);
    rightVal2 = GetPrivateProfileIntW(L"Recoil", L"Right2", 0, f);
    upVal2 = GetPrivateProfileIntW(L"Recoil", L"Up2", 0, f);
    downVal2 = GetPrivateProfileIntW(L"Recoil", L"Down2", 0, f);
    leftVal3 = GetPrivateProfileIntW(L"Recoil", L"Left3", 0, f);
    rightVal3 = GetPrivateProfileIntW(L"Recoil", L"Right3", 0, f);
    upVal3 = GetPrivateProfileIntW(L"Recoil", L"Up3", 0, f);
    downVal3 = GetPrivateProfileIntW(L"Recoil", L"Down3", 0, f);

    if (activationMode < 0 || activationMode > 3) activationMode = MODE_HOLD_M1;
    if (sensVal < 1)    sensVal = 1;
    if (sensVal > 1000) sensVal = 1000;
    if (delay1Ms < 100)  delay1Ms = 100;
    if (delay1Ms > 5000) delay1Ms = 5000;
    if (delay2Ms < 100)  delay2Ms = 100;
    if (delay2Ms > 5000) delay2Ms = 5000;

    return true;
}

void ResetConfig()
{
    leftVal = rightVal = upVal = downVal = 0;
    leftVal2 = rightVal2 = upVal2 = downVal2 = 0;
    leftVal3 = rightVal3 = upVal3 = downVal3 = 0;
    sensVal = 100;
    toggleKey = VK_F2;
    masterToggleKey = VK_F3;
    activationMode = MODE_HOLD_M1;
    secondaryEnabled = false;
    tertiaryEnabled = false;
    delay1Ms = 500;
    delay2Ms = 1000;
    masterEnabled = true;
}