// simFFB.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "simFFB.h"
#include "simstick.h"
#include "Keymap.h"

#include <strsafe.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <vector>
#include <algorithm>

#define MAX_LOADSTRING 100

#define LAUNCHKEY 220

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                    // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND g_hwnd;
const int WIN_HEIGHT = 473;
const int WIN_WIDTH = 325;
HMENU g_hm;
UINT modMenuInit[4] = { ID_INIT_MOD_CTRL, ID_INIT_MOD_ALT, ID_INIT_MOD_SHIFT, ID_INIT_MOD_WIN };
UINT modMenuTrim[4] = { ID_CYCLE_MOD_CTRL, ID_CYCLE_MOD_ALT, ID_CYCLE_MOD_SHIFT, ID_CYCLE_MOD_WIN };
int g_Init=1;  //flag for first run to call the initialization function
BOOL g_ptrim=false; //Progressive trimming flag
BOOL g_itrim=true;  //Instantaneous trimming flag
stoptions jopt;

BOOL isLabelUpdatedToggle = false; // keep track of label updates to 
BOOL isLabelUpdatedHold = false;   // prevent redraw every loop

Input::Keymap keymap;
short iKeyState;

// Controls
HWND hwndCBSticks;     // Combobox for Joystick List
HWND hwndCBSticksPOV;  // Combobox for Joystick List (POV PT)
HWND hwndCBTrimHold;   // Combobox for trim hold button
HWND hwndCBTrimToggle; // Combobox for trim toggle button
HWND hwndCBTrimCenter; // Combobox for trim center button

HWND hwndLBSpring; // Label for spring force
HWND hwndTBSpring,hwndTBDamper,hwndTBFriction; //Track bar for spring, damper and friction strenght
HWND hwndEBSpring,hwndEBDamper,hwndEBFriction; //read-only edit boxes to show strenght percentage

HWND hwndLBSpring2; // Label for spring 2 force
HWND hwndTBSpring2; // Track bar for spring 2 force
HWND hwndEBSpring2;  // Read-only edit box to show strength percentage

HWND hwndTBDamper2; // Track bar for damper 2 strength
HWND hwndEBDamper2; // Read-only edit box to show strength percentage

HWND hwndTBFriction2; // Track bar for friction 2 strength
HWND hwndEBFriction2; // Read-only edit box to show strength percentage

HWND hwndRDTrimNone; // Radio button for no trim
HWND hwndRDTrimProg; // Radio button for progressive trim
HWND hwndRDTrimInst; // Radio button for instant trim
HWND hwndRDTrimBoth; // Radio button for both trim

HWND hwndCHSwap; //Swap axes chechbox
HWND hwndCBInitDInput; //Set key for init dinput
HWND hwndCBCycleTrim;  //Set key for cycle trim
std::vector<int> initDinputKeyCodes; //Key codes

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void InitAll(BOOL firstrun);
void SetSwapCheckbox();
void ToggleMenuModifier(UINT menuItem, int(&modArr)[4], int i);
void InitMenuModifier(MENUITEMINFO mii, UINT menuItem, int(&modArr)[4], int i);
void SwapForceActiveLabel(BOOL b);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

     // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_SIMFFB, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMFFB));

    g_hm=GetMenu(g_hwnd);
    // Main message loop:
    // This kind of message loop allows to constantly poll the joystick (or whatever
    // it's needed to do constantly)
    // instead of having the application stopped waiting for events
    GetJtOptions(&jopt);
    while(true) {
        if(::PeekMessage(&msg,0,0,0,PM_REMOVE)) { //Process events
             if (msg.message==WM_QUIT)
                break;
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        } else { //no events, do you stuff here
            if (g_Init) { //First run only
                InitAll(true);
                g_Init=0;
            } else {
                if (g_itrim)
                    JoystickStuffIT();
                if (g_ptrim)
                    JoystickStuffPT();

                if (jopt.trimmode == 1 || jopt.trimmode == 3) {
                    if (g_bBoton[jopt.btrimHold] == UP && g_bBoton[jopt.btrimToggle] == DOWN && !isLabelUpdatedToggle) {
                        SwapForceActiveLabel(isLabelUpdatedToggle);
                        isLabelUpdatedToggle = true;
                    }
                    else if (g_bBoton[jopt.btrimToggle] == UP && isLabelUpdatedToggle) {
                        SwapForceActiveLabel(isLabelUpdatedToggle);
                        isLabelUpdatedToggle = false;
                    }

                    if (g_bBoton[jopt.btrimToggle] == UP && g_bBoton[jopt.btrimHold] == DOWN && !isLabelUpdatedHold) {
                        SwapForceActiveLabel(isLabelUpdatedHold);
                        isLabelUpdatedHold = true;
                    }
                    else if (g_bBoton[jopt.btrimHold] == UP && isLabelUpdatedHold) {
                        SwapForceActiveLabel(isLabelUpdatedHold);
                        isLabelUpdatedHold = false;
                    }
                }

                iKeyState = GetAsyncKeyState(jopt.iKey);
                if ((1 << 16) & iKeyState)
                {
                    //if (GetKeyState(VK_CONTROL) & 0x8000) {
                        InitAll(false);
                    //}
                }
                
                iKeyState = GetAsyncKeyState(jopt.ctKey);
                if ((1 << 16) & iKeyState) {  
                    int trimMode = (jopt.trimmode + 1) % 4;              
                    switch (trimMode) {
                        case 0: 
                            SendMessage(hwndRDTrimNone, BM_CLICK, BST_CHECKED, 1); 
                            break; 
                        case 1: 
                            SendMessage(hwndRDTrimInst, BM_CLICK, BST_CHECKED, 1); 
                            break;
                        case 2: 
                            SendMessage(hwndRDTrimProg, BM_CLICK, BST_CHECKED, 1); 
                            break;
                        case 3: 
                            SendMessage(hwndRDTrimBoth, BM_CLICK, BST_CHECKED, 1); 
                            break;
                    }
                }
            }
            Sleep(50);
        }
    }
    StopEffects();
    FreeDirectInput();
    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SIMFFB));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW);
    wcex.lpszMenuName    = MAKEINTRESOURCE(IDC_SIMFFB);
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable
   
   // Load saved options so we can move the window if needed
   LoadOptionsFromFile();
   GetJtOptions(&jopt);
   hWnd = CreateWindow(szWindowClass, szTitle, WS_SYSMENU|WS_CAPTION|WS_MINIMIZEBOX|WS_DLGFRAME, jopt.windowX, jopt.windowY, WIN_HEIGHT, WIN_WIDTH, NULL, NULL, hInstance, NULL);
  

   if (!hWnd)
   {
      return FALSE;
   }
   g_hwnd=hWnd;
   ShowWindow(hWnd,SW_SHOWNOACTIVATE); //the shownoactivate flag allows to open the application in the background so it doesn't interefere the simulation if it's already running
   UpdateWindow(hWnd);

   /////////////CONTROLS
    CreateWindow(_T("static"),_T("Device for Buttons / Progressive Trim Hat"),WS_CHILD|WS_VISIBLE,5,3,290,20,hWnd,NULL,hInstance,NULL);
    CreateWindow(_T("static"),_T("Hold"),WS_CHILD|WS_VISIBLE,305,3,130,20,hWnd,NULL,hInstance,NULL);
    CreateWindow(_T("static"),_T("Toggle"),WS_CHILD|WS_VISIBLE,355,3,130,20,hWnd,NULL,hInstance,NULL);
    CreateWindow(_T("static"),_T("Center"),WS_CHILD|WS_VISIBLE,405,3,130,20,hWnd,NULL,hInstance,NULL);
    hwndCBSticks=CreateWindow(_T("combobox"),TEXT(""), CBS_DROPDOWNLIST |WS_CHILD | WS_VISIBLE,3,23,300,255,hWnd,NULL,hInstance,NULL);
    hwndCBTrimHold =CreateWindow(_T("combobox"),TEXT(""),CBS_DROPDOWNLIST|WS_VSCROLL | WS_CHILD | WS_VISIBLE,305,23,50,255,hWnd,NULL,hInstance,NULL);
    hwndCBTrimToggle = CreateWindow(_T("combobox"), TEXT(""), CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE, 355, 23, 50, 255, hWnd, NULL, hInstance, NULL);
    hwndCBTrimCenter = CreateWindow(_T("combobox"), TEXT(""), CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE, 405, 23, 50, 255, hWnd, NULL, hInstance, NULL);

    TCHAR *b=new TCHAR[11];
    for (int i=0;i<32;i++) {
        _stprintf_s(b, 11, _T("%i"),i+1);
        ComboBox_AddString(hwndCBTrimHold,b);
        ComboBox_AddString(hwndCBTrimToggle, b);
        ComboBox_AddString(hwndCBTrimCenter, b);
    }
    ComboBox_SetCurSel(hwndCBTrimHold,0);
    ComboBox_SetCurSel(hwndCBTrimToggle, 0);
    ComboBox_SetCurSel(hwndCBTrimCenter, 0);
    delete b;

    hwndCBSticksPOV=CreateWindow(_T("combobox"),TEXT(""), CBS_DROPDOWNLIST |WS_CHILD | WS_VISIBLE,3,53,300,255,hWnd,NULL,hInstance,NULL);

    CreateWindow(_T("static"),_T("SimFFB"),WS_CHILD|WS_VISIBLE|WS_BORDER|ES_CENTER,309,55,140,20,hWnd,NULL,hInstance,NULL);
    
    hwndLBSpring = CreateWindow(_T("static"),_T("Spring Force *\t%"),WS_CHILD|WS_VISIBLE,5,83,130,20,hWnd,NULL,hInstance,NULL);
    hwndEBSpring=CreateWindow(_T("Edit"),_T("55"),WS_CHILD|WS_VISIBLE|ES_LEFT|ES_NUMBER|ES_READONLY,135,83,30,20,hWnd,NULL,hInstance,NULL);
    hwndTBSpring=CreateWindow(TRACKBAR_CLASS,NULL,WS_CHILD|WS_VISIBLE|TBS_ENABLESELRANGE,168,83,290,20,hWnd,NULL,hInstance,NULL);
    SendMessage(hwndTBSpring, TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0, 100));
    SendMessage(hwndTBSpring, TBM_SETPAGESIZE,0, (LPARAM) 1);
    SendMessage(hwndTBSpring, TBM_SETSEL,(WPARAM) FALSE,(LPARAM) MAKELONG(0, 100)); 
    SendMessage(hwndTBSpring, TBM_SETPOS,(WPARAM) TRUE,(LPARAM) 55);

    CreateWindow(_T("static"),_T("Damper Force\t%"),WS_CHILD|WS_VISIBLE,5,103,130,20,hWnd,NULL,hInstance,NULL);
    hwndEBDamper=CreateWindow(_T("Edit"),_T("55"),WS_CHILD|WS_VISIBLE|ES_LEFT|ES_NUMBER|ES_READONLY,135,103,30,20,hWnd,NULL,hInstance,NULL);
    hwndTBDamper=CreateWindow(TRACKBAR_CLASS,NULL,WS_CHILD|WS_VISIBLE |TBS_ENABLESELRANGE,168,103,290,20,hWnd,NULL,hInstance,NULL);
    SendMessage(hwndTBDamper, TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0, 100));
    SendMessage(hwndTBDamper, TBM_SETPAGESIZE,0, (LPARAM) 1);
    SendMessage(hwndTBDamper, TBM_SETSEL,(WPARAM) FALSE,(LPARAM) MAKELONG(0, 100)); 
    SendMessage(hwndTBDamper, TBM_SETPOS,(WPARAM) TRUE,(LPARAM) 55);

    CreateWindow(_T("static"),_T("Friction Force\t%"),WS_CHILD|WS_VISIBLE,5,123,130,20,hWnd,NULL,hInstance,NULL);
    hwndEBFriction=CreateWindow(_T("Edit"),_T("55"),WS_CHILD|WS_VISIBLE|ES_LEFT|ES_NUMBER|ES_READONLY,135,123,30,20,hWnd,NULL,hInstance,NULL);
    hwndTBFriction=CreateWindow(TRACKBAR_CLASS,NULL,WS_CHILD|WS_VISIBLE |TBS_ENABLESELRANGE,168,123,290,20,hWnd,NULL,hInstance,NULL);
    SendMessage(hwndTBFriction, TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0, 100));
    SendMessage(hwndTBFriction, TBM_SETPAGESIZE,0, (LPARAM) 1);
    SendMessage(hwndTBFriction, TBM_SETSEL,(WPARAM) FALSE,(LPARAM) MAKELONG(0, 100)); 
    SendMessage(hwndTBFriction, TBM_SETPOS,(WPARAM) TRUE,(LPARAM) 55);
    
    hwndLBSpring2 = CreateWindow(_T("static"), _T("Spring Force 2\t%"), WS_CHILD | WS_VISIBLE, 5, 143, 130, 20, hWnd, NULL, hInstance, NULL);
    hwndEBSpring2 = CreateWindow(_T("Edit"), _T("55"), WS_CHILD | WS_VISIBLE | ES_LEFT | ES_NUMBER | ES_READONLY, 135, 143, 30, 20, hWnd, NULL, hInstance, NULL);
    hwndTBSpring2 = CreateWindow(TRACKBAR_CLASS, NULL, WS_CHILD | WS_VISIBLE | TBS_ENABLESELRANGE, 168, 143, 290, 20, hWnd, NULL, hInstance, NULL);
    SendMessage(hwndTBSpring2, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 70));
    SendMessage(hwndTBSpring2, TBM_SETPAGESIZE, 0, (LPARAM)1);
    SendMessage(hwndTBSpring2, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(0, 70));
    SendMessage(hwndTBSpring2, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)55);

    CreateWindow(_T("static"),_T("Damper Force 2\t%"),WS_CHILD|WS_VISIBLE,5,163,130,20,hWnd,NULL,hInstance,NULL);
    hwndEBDamper2=CreateWindow(_T("Edit"),_T("55"),WS_CHILD|WS_VISIBLE|ES_LEFT|ES_NUMBER|ES_READONLY,135,163,30,20,hWnd,NULL,hInstance,NULL);
    hwndTBDamper2=CreateWindow(TRACKBAR_CLASS,NULL,WS_CHILD|WS_VISIBLE |TBS_ENABLESELRANGE,168,163,290,20,hWnd,NULL,hInstance,NULL);
    SendMessage(hwndTBDamper2, TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0, 100));
    SendMessage(hwndTBDamper2, TBM_SETPAGESIZE,0, (LPARAM) 1);
    SendMessage(hwndTBDamper2, TBM_SETSEL,(WPARAM) FALSE,(LPARAM) MAKELONG(0, 100)); 
    SendMessage(hwndTBDamper2, TBM_SETPOS,(WPARAM) TRUE,(LPARAM) 55);

    CreateWindow(_T("static"),_T("Friction Force 2\t%"),WS_CHILD|WS_VISIBLE,5,183,130,20,hWnd,NULL,hInstance,NULL);
    hwndEBFriction2=CreateWindow(_T("Edit"),_T("55"),WS_CHILD|WS_VISIBLE|ES_LEFT|ES_NUMBER|ES_READONLY,135,183,30,20,hWnd,NULL,hInstance,NULL);
    hwndTBFriction2=CreateWindow(TRACKBAR_CLASS,NULL,WS_CHILD|WS_VISIBLE |TBS_ENABLESELRANGE,168,183,290,20,hWnd,NULL,hInstance,NULL);
    SendMessage(hwndTBFriction2, TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0, 100));
    SendMessage(hwndTBFriction2, TBM_SETPAGESIZE,0, (LPARAM) 1);
    SendMessage(hwndTBFriction2, TBM_SETSEL,(WPARAM) FALSE,(LPARAM) MAKELONG(0, 100)); 
    SendMessage(hwndTBFriction2, TBM_SETPOS,(WPARAM) TRUE,(LPARAM) 55);

    CreateWindow(_T("static"), _T("Trim\t"), WS_CHILD | WS_VISIBLE, 5, 210, 130, 20, hWnd, NULL, hInstance, NULL);
    hwndRDTrimNone = CreateWindow(L"Button", L"None", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 45, 204, 70, 30, hWnd, NULL, hInstance, NULL);
    hwndRDTrimInst = CreateWindow(L"Button", L"Instant", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 115, 204, 70, 30, hWnd, NULL, hInstance, NULL);
    hwndRDTrimProg = CreateWindow(L"Button", L"Progressive", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 195, 204, 100, 30, hWnd, NULL, hInstance, NULL);
    hwndRDTrimBoth = CreateWindow(L"Button", L"Both", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 310, 204, 70, 30, hWnd, NULL, hInstance, NULL);

    hwndCHSwap=CreateWindow(_T("button"),_T("Swap Axis"),WS_CHILD|WS_VISIBLE|BS_CHECKBOX|BS_LEFTTEXT,3,239,90,20,hWnd,NULL,hInstance,NULL);
    // Init dinput hotkey 
    CreateWindow(_T("static"), _T("Init Key"), WS_CHILD | WS_VISIBLE | ES_CENTER, 80, 239, 80, 20, hWnd, NULL, hInstance, NULL);
    hwndCBInitDInput = CreateWindow(_T("combobox"), _T(""), CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE | ES_CENTER, 150, 235, 110, 300, hWnd, NULL, hInstance, NULL);
    // Cycle trim hotkey
    CreateWindow(_T("static"), _T("Trim Mode"), WS_CHILD | WS_VISIBLE | ES_CENTER, 265, 239, 80, 20, hWnd, NULL, hInstance, NULL);
    hwndCBCycleTrim = CreateWindow(_T("combobox"), _T(""), CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE | ES_CENTER, 345, 235, 110, 300, hWnd, NULL, hInstance, NULL);
    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND    - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY    - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmEvent) {
        case CBN_CLOSEUP:
            unsigned int z;
            if ((HWND)lParam == hwndCBInitDInput)
            {
            z = ComboBox_GetCurSel(hwndCBInitDInput);

                if (!(z > initDinputKeyCodes.size()) && !(z < 0))
                {
                    jopt.iKey = initDinputKeyCodes.at(z);
                }
            }
            if ((HWND)lParam == hwndCBCycleTrim) {
                z = ComboBox_GetCurSel(hwndCBCycleTrim);

                if (!(z > initDinputKeyCodes.size()) && !(z < 0))
                {
                    jopt.ctKey = initDinputKeyCodes.at(z);
                }
            }
            break;
        case CBN_SELCHANGE:
            jopt.jtrim=ComboBox_GetCurSel(hwndCBSticks);
            jopt.jPOV=ComboBox_GetCurSel(hwndCBSticksPOV);
            jopt.btrimHold=ComboBox_GetCurSel(hwndCBTrimHold); 
            jopt.btrimToggle=ComboBox_GetCurSel(hwndCBTrimToggle); 
            jopt.btrimCenter=ComboBox_GetCurSel(hwndCBTrimCenter); 
            SetJtOptions(&jopt);
            break;
        case BN_CLICKED:
            if ((HWND)lParam==hwndCHSwap) {
                jopt.swap=!jopt.swap;
                SetSwapCheckbox();
            }

            if ((HWND)lParam == hwndRDTrimNone) {
                g_itrim = false;
                g_ptrim = false;
                jopt.trimmode = 0;
            }
            else if ((HWND)lParam == hwndRDTrimInst) {
                g_itrim = true;
                g_ptrim = false;
                jopt.trimmode = 1;
            }
            else if ((HWND)lParam == hwndRDTrimProg) {
                g_itrim = false;
                g_ptrim = true;
                jopt.trimmode = 2;
            }
            else if ((HWND)lParam == hwndRDTrimBoth) {
                g_itrim = true;
                g_ptrim = true;
                jopt.trimmode = 3;
            }

            SetJtOptions(&jopt);
            break;
        }
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case ID_OPTIONS_RE: // Reinitialize
            InitAll(false);
            break;
        case ID_INIT_MOD_CTRL:
            ToggleMenuModifier(ID_INIT_MOD_CTRL, jopt.iKeyMod, 0);
            break;
        case ID_INIT_MOD_ALT:
            ToggleMenuModifier(ID_INIT_MOD_ALT, jopt.iKeyMod, 1);
            break;
        case ID_INIT_MOD_SHIFT:
            ToggleMenuModifier(ID_INIT_MOD_SHIFT, jopt.iKeyMod, 2);
            break;
        case ID_INIT_MOD_WIN:
            ToggleMenuModifier(ID_INIT_MOD_WIN, jopt.iKeyMod, 3);
            break;
        case ID_CYCLE_MOD_CTRL:
            ToggleMenuModifier(ID_CYCLE_MOD_CTRL, jopt.ctKeyMod, 0);
            break;
        case ID_CYCLE_MOD_ALT:
            ToggleMenuModifier(ID_CYCLE_MOD_ALT, jopt.ctKeyMod, 1);
            break;
        case ID_CYCLE_MOD_SHIFT:
            ToggleMenuModifier(ID_CYCLE_MOD_SHIFT, jopt.ctKeyMod, 2);
            break;
        case ID_CYCLE_MOD_WIN:
            ToggleMenuModifier(ID_CYCLE_MOD_WIN, jopt.ctKeyMod, 3);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        EndPaint(hWnd, &ps);
        break;
    case WM_QUIT:
        // Save window position
        break;
    case WM_DESTROY:
        RECT hWndRect;
        GetWindowRect(hWnd, &hWndRect);
        jopt.windowX = hWndRect.left;
        jopt.windowY = hWndRect.top;
        SetJtOptions(&jopt);
        PostQuitMessage(0);
        break;
    case WM_HSCROLL:
        if ((HWND)lParam == hwndTBSpring) {
            jopt.spring = SendMessage(hwndTBSpring, TBM_GETPOS, 0, 0);
            TCHAR* tmp = new TCHAR[11];
            _stprintf_s(tmp, 11, _T("%i"), jopt.spring);
            Edit_SetText(hwndEBSpring, tmp);
            SetJtOptions(&jopt);
        }
        else if ((HWND)lParam == hwndTBSpring2) {
            jopt.spring2 = SendMessage(hwndTBSpring2, TBM_GETPOS, 0, 0);
            TCHAR* tmp = new TCHAR[11];
            _stprintf_s(tmp, 11, _T("%i"), jopt.spring2);
            Edit_SetText(hwndEBSpring2, tmp);
            SetJtOptions(&jopt);
        }
        else if ((HWND)lParam == hwndTBDamper2) {
            jopt.damper2=SendMessage(hwndTBDamper2, TBM_GETPOS, 0, 0);
            TCHAR *tmp=new TCHAR[11];
            _stprintf_s(tmp,11, _T("%i"),jopt.damper2);
            Edit_SetText(hwndEBDamper2,tmp);
            SetJtOptions(&jopt);
            //break;
        } else if ((HWND)lParam==hwndTBDamper) {
            jopt.damper=SendMessage(hwndTBDamper, TBM_GETPOS, 0, 0);
            TCHAR *tmp=new TCHAR[11];
            _stprintf_s(tmp, 11, _T("%i"),jopt.damper);
            Edit_SetText(hwndEBDamper,tmp);
            SetJtOptions(&jopt);
            //break;
        } else if ((HWND)lParam==hwndTBFriction) {
            jopt.friction=SendMessage(hwndTBFriction, TBM_GETPOS, 0, 0);
            TCHAR *tmp=new TCHAR[11];
            _stprintf_s(tmp, 11, _T("%i"),jopt.friction);
            Edit_SetText(hwndEBFriction,tmp);
            SetJtOptions(&jopt);
        } else if ((HWND)lParam==hwndTBFriction2) {
            jopt.friction2=SendMessage(hwndTBFriction2, TBM_GETPOS, 0, 0);
            TCHAR *tmp=new TCHAR[11];
            _stprintf_s(tmp, 11, _T("%i"),jopt.friction2);
            Edit_SetText(hwndEBFriction2,tmp);
            SetJtOptions(&jopt);
        }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void InitAll(BOOL firstrun)
{
    MENUITEMINFO mii;
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;

    if (!firstrun) {
        FreeDirectInput();
        Sleep(100);
    }
    InitDirectInput(g_hwnd);
    if (FAILED(Adquirir()))
        return;
    StartEffects();

    // Fetch JtOptions again since we polled for devices in InitDirectInput
    GetJtOptions(&jopt);

    switch (jopt.trimmode) {
        case 0:
            SendMessage(hwndRDTrimNone,BM_SETCHECK,(WPARAM)BST_CHECKED,1);
            g_itrim=false;
            g_ptrim=false;
            break;
        case 1:
            SendMessage(hwndRDTrimInst, BM_SETCHECK, (WPARAM)BST_CHECKED, 1);
            g_itrim=true;
            g_ptrim=false;
            break;
        case 2: 
            SendMessage(hwndRDTrimProg, BM_SETCHECK, (WPARAM)BST_CHECKED, 1);
            g_itrim=false;
            g_ptrim=true;
            break;
        case 3: 
            SendMessage(hwndRDTrimBoth, BM_SETCHECK, (WPARAM)BST_CHECKED, 1);
            g_itrim=true;
            g_ptrim=true;
            break;
    }

    ComboBox_ResetContent(hwndCBSticks);
    ComboBox_ResetContent(hwndCBSticksPOV);
    for (int k = 0; k < JoysticksNumber(); k++) {
        ComboBox_AddString(hwndCBSticks,JoystickName(k));
        ComboBox_AddString(hwndCBSticksPOV,JoystickName(k));
    }

    //Fill combobox with keys
    int savedKeyIndexInit = -1; // find index of key if saved in opt file
    int savedKeyIndexTrim = -1; // find index of key if saved in opt file
    if (firstrun)
    {
        ComboBox_ResetContent(hwndCBInitDInput);
        ComboBox_ResetContent(hwndCBCycleTrim);

        for (unsigned int i = 0; i < keymap.keys.size(); i++) {
            if (keymap.keys.at(i).second == jopt.iKey)
                savedKeyIndexInit = i;
            if (keymap.keys.at(i).second == jopt.ctKey)
                savedKeyIndexTrim = i;
            std::string* str = &keymap.keys.at(i).first;
            TCHAR* kName = new TCHAR[str->length() + 1];
            kName[str->length()] = 0;
            std::copy(str->begin(), str->end(), kName);
            ComboBox_AddString(hwndCBInitDInput, kName);
            ComboBox_AddString(hwndCBCycleTrim, kName);
            initDinputKeyCodes.push_back(keymap.keys.at(i).second);
        }
    }

    // Set combobox to saved key
    if (savedKeyIndexInit >= 0) ComboBox_SetCurSel(hwndCBInitDInput, savedKeyIndexInit);
    if (savedKeyIndexTrim >= 0) ComboBox_SetCurSel(hwndCBCycleTrim, savedKeyIndexTrim);
    

    int j,k,b,b2,b3;
    GetTrimmer(j,k,b,b2,b3);
    ComboBox_SetCurSel(hwndCBSticks,j);
    ComboBox_SetCurSel(hwndCBSticksPOV,k);
    ComboBox_SetCurSel(hwndCBTrimHold,b);
    ComboBox_SetCurSel(hwndCBTrimToggle, b2); 
    ComboBox_SetCurSel(hwndCBTrimCenter, b3); 

    TCHAR *tmp=new TCHAR[11];
    _stprintf_s(tmp,11,_T("%i"),jopt.spring);
    Edit_SetText(hwndEBSpring,tmp);
    _stprintf_s(tmp,11,_T("%i"),jopt.damper);
    Edit_SetText(hwndEBDamper,tmp);
    _stprintf_s(tmp,11,_T("%i"),jopt.friction);
    Edit_SetText(hwndEBFriction,tmp);

    _stprintf_s(tmp, 11,_T("%i"), jopt.spring2);
    Edit_SetText(hwndEBSpring2, tmp);
    _stprintf_s(tmp,11,_T("%i"),jopt.damper2);
    Edit_SetText(hwndEBDamper2,tmp);
    _stprintf_s(tmp,11,_T("%i"),jopt.friction2);
    Edit_SetText(hwndEBFriction2,tmp);

    SendMessage(hwndTBSpring, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) jopt.spring);
    SendMessage(hwndTBDamper, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) jopt.damper);
    SendMessage(hwndTBFriction, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) jopt.friction);
    SendMessage(hwndTBSpring2, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)jopt.spring2);
    SendMessage(hwndTBDamper2, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) jopt.damper2);
    SendMessage(hwndTBFriction2, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) jopt.friction2);

    if (jopt.swap)
        SetSwapCheckbox();

    for (int i = 0; i < sizeof(modMenuInit) / sizeof(modMenuInit[0]); i++) {
        InitMenuModifier(mii, modMenuInit[i], jopt.iKeyMod, i);
        InitMenuModifier(mii, modMenuTrim[i], jopt.ctKeyMod, i);
    }
}

void SetSwapCheckbox()
{
    if (jopt.swap)
        SendMessage(hwndCHSwap,BM_SETCHECK,(WPARAM)BST_CHECKED,NULL);
    else
        SendMessage(hwndCHSwap,BM_SETCHECK,(WPARAM)BST_UNCHECKED,NULL);
}

void ToggleMenuModifier(UINT menuItem, int (&modArr)[4], int i) {
    MENUITEMINFO mii;
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;

    if (modArr[i] == 0) {
        mii.fState = MFS_CHECKED;
    }
    else {
        mii.fState = MFS_UNCHECKED;
    }
    modArr[i] = (modArr[i] == 0) ? 1 : 0;

    SetMenuItemInfo(g_hm, menuItem, FALSE, &mii);
    SetJtOptions(&jopt);
}

void InitMenuModifier(MENUITEMINFO mii, UINT menuItem, int(&modArr)[4], int i) {
    mii.fState = modArr[i] == 0 ? MFS_UNCHECKED : MFS_CHECKED;
    SetMenuItemInfo(g_hm, menuItem, FALSE, &mii);
    SetJtOptions(&jopt);
}

void SwapForceActiveLabel(BOOL b) {
    if (b) {
        SetWindowText(hwndLBSpring, L"Spring Force *\t%");
        SetWindowText(hwndLBSpring2, L"Spring Force 2\t%");
    }
    else {
        SetWindowText(hwndLBSpring, L"Spring Force\t%");
        SetWindowText(hwndLBSpring2, L"Spring Force 2 *\t%");
    }
}
