#include "stdafx.h"

#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>

//#define FORCESPRING DI_FFNOMINALMAX
//#define FORCEDAMPER DI_FFNOMINALMAX*0.7
//#define FORCEFRICTI DI_FFNOMINALMAX*0.7
#define TRIMSTEP    DI_FFNOMINALMAX*0.0125

#define MAXSTICKS 32
#define MAXBUTTONS 32

typedef struct _sticks {
    TCHAR name[MAX_PATH];
    LPDIRECTINPUTDEVICE8 dev;
} sticks;

typedef struct options {
    int jtrim;          // joystick  
    int jPOV;           // POV hat for progressive trimmer
    int btrimHold;      // trim button (hold)
    int btrimToggle;    // trim button (toggle)
    int btrimCenter;    // trim button (center)
    int spring;         // spring force 
    int damper;         // dampering level (force trim on)
    int friction;       // friction level (force trim on)
    int damper2;        // dampering level (force trim off)
    int friction2;      // friction level (force trim off)
    int iKey;           // key for reinitializing dinput
    byte swap;          // swap axes
    byte trimmode;      // no trim - only instant - only progressive - both
} stoptions;

//-----------------------------------------------------------------------------
// Function prototypes 
//-----------------------------------------------------------------------------
HRESULT InitDirectInput(HWND);
VOID    FreeDirectInput();

HRESULT Adquirir();
void StartEffects();
void StopEffects();
void JoystickStuffPT();
void JoystickStuffIT();
void NoSpring();
void InstantTrim();
void CenterTrim(); // Centers joystick & turns on spring
int JoysticksNumber();
LPCTSTR JoystickName(int);
void SetTrimmer(int,int,int,int,int);
void GetTrimmer(int&,int&,int&,int&,int&);
void SetJtOptions(stoptions *);
void GetJtOptions(stoptions *);


//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

#define ADDLIM(a,b,l) {a+=b; if (a>l) a=l;}  //addition with upper limit
#define SUBLIM(a,b,l) {a-=b; if (a<l) a=l;}  //substraction with bottom limit
#define UP    0
#define DOWN 1
#define RELEASED 2
#define CENTER_DAMP_COEFF 1.1
