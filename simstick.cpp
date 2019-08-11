#include "stdafx.h"
#include "simstick.h"
#include <math.h>
#include <iostream>
#include <chrono>
#include <algorithm>

#define OPTFILENAME _T("opt.dat") //File name where settings are saved
#define MAGICNUMBER 145011977 //Just a random number to lower the probability of reading trash from the settings file

LPDIRECTINPUT8        g_pDI       = NULL;         
LPDIRECTINPUTDEVICE8  g_pFFDevice   = NULL;
                        
LPDIRECTINPUTEFFECT   g_pEffectSpring   = NULL;
LPDIRECTINPUTEFFECT   g_pEffectDamper   = NULL;
LPDIRECTINPUTEFFECT   g_pEffectFricti   = NULL;

LPDIRECTINPUTEFFECT   g_pEffectDamper2 = NULL;
LPDIRECTINPUTEFFECT   g_pEffectFricti2 = NULL;

DWORD                 g_dwNumForceFeedbackAxis = 0;
INT                   g_nXForce;
INT                   g_nYForce;
BYTE                  g_bBoton[MAXBUTTONS];
DIJOYSTATE2              g_js;
BYTE                  g_bSpring = 1;  //Wether there is spring force or not
int g_iNsticks = 0;
int g_iFF = -1;
BYTE old_btrimToggle_state = 0x00; // Keeps track of old trim toggle state
bool is_centered = true; // Keeps track of joystick centering status to prevent unnecessary recentering
time_t time_last = INT_MAX; // Used for debouncing trim toggle
sticks g_Sticks[MAXSTICKS];
stoptions g_Opt={
    0,      // joystick      
    0,      // trim button (hold)    
    0,      // trim button (toggle)
    0,      // trim button (center)
    4500,   // spring force 
    8000,   // dampering level (force trim on)    
    0,      // friction level (force trim on)    
    8000,   // dampering level (force trim off)
    0,      // friction level (force trim off)
    0,      // key for reinitializing dinput
    true,   // swap axes        
    1,      // no trim - only instant - only progressive - both 
};       

//Private functions
BOOL CALLBACK EnumFFDevicesCallback( const DIDEVICEINSTANCE* pInst, VOID* pContext );
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext );
HRESULT poll(LPDIRECTINPUTDEVICE8 Device,DIJOYSTATE2 *js);
HRESULT SetDeviceForcesXY();
HRESULT SetDeviceSpring();
HRESULT SetDeviceConditions();
void Botones(); //Poll buttons states
BOOL LoadOptionsFromFile();
BOOL SaveOptionsToFile();

HRESULT poll(LPDIRECTINPUTDEVICE8 Device,DIJOYSTATE2 *js)
{
    HRESULT     hr;

    if (Device == NULL) {
        return S_OK;
    }

    // Poll the device to read the current state
    hr = Device->Poll(); 
    if (FAILED(hr)) {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        hr = Device->Acquire();
        while (hr == DIERR_INPUTLOST) {
            hr = Device->Acquire();
        }

        // If we encounter a fatal error, return failure.
        if ((hr == DIERR_INVALIDPARAM) || (hr == DIERR_NOTINITIALIZED)) {
            return E_FAIL;
        }

        // If another application has control of this device, return successfully.
        // We'll just have to wait our turn to use the joystick.
        if (hr == DIERR_OTHERAPPHASPRIO) {
            return S_OK;
        }
    }

    // Get the input's device state
    if (FAILED(hr = Device->GetDeviceState(sizeof(DIJOYSTATE2), js))) {
        return hr; // The device should have been acquired during the Poll()
    }
    Botones();
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: InitDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
HRESULT InitDirectInput(HWND hCon)
{
    DIPROPDWORD dipdw;
    HRESULT     hr;

    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
                                         IID_IDirectInput8, (VOID**)&g_pDI, NULL ) ) )
    {
        return hr;
    }
    
    // Look for a force feedback device we can use
    if( FAILED( hr = g_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, 
                                         EnumFFDevicesCallback, NULL, 
                                         DIEDFL_ATTACHEDONLY /*| DIEDFL_FORCEFEEDBACK*/ ) ) )
    {
        return hr;
    }

    if( -1==g_iNsticks )
    {
        MessageBox( NULL, _T("No Joystick found.")
                          _T("The sample will now exit."), 
                          _T("FFConst"), MB_ICONERROR | MB_OK );
        return S_OK;
    }
    if (g_pFFDevice==NULL)
    {
        MessageBox( NULL, _T("No FFB Joystick found.")
                          _T("The sample will now exit."), 
                          _T("FFConst"), MB_ICONERROR | MB_OK );
        return S_OK;
    }
    
    LoadOptionsFromFile();

    // Set the data format to "simple joystick" - a predefined data format. A
    // data format specifies which controls on a device we are interested in,
    // and how they should be reported.
    //
    // This tells DirectInput that we will be passing a DIJOYSTATE structure to
    // IDirectInputDevice8::GetDeviceState(). Even though we won't actually do
    // it in this sample. But setting the data format is important so that the
    // DIJOFS_* values work properly.
    for (int i=0;i<g_iNsticks;i++) {
        hr = g_Sticks[i].dev->SetDataFormat( &c_dfDIJoystick2 );
        hr = g_Sticks[i].dev->SetCooperativeLevel( hCon,DISCL_EXCLUSIVE | DISCL_BACKGROUND );
    }

    // Since we will be playing force feedback effects, we should disable the
    // auto-centering spring.
    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;
    dipdw.dwData            = FALSE;

    if( FAILED( hr = g_pFFDevice->SetProperty( DIPROP_AUTOCENTER, &dipdw.diph ) ) )
        return hr;

    // Enumerate and count the axes of the joystick 
    if ( FAILED( hr = g_pFFDevice->EnumObjects( EnumAxesCallback, 
                                              (VOID*)&g_dwNumForceFeedbackAxis, DIDFT_AXIS ) ) )
        return hr;

    // This simple sample only supports one or two axis joysticks
    if( g_dwNumForceFeedbackAxis > 2 )
        g_dwNumForceFeedbackAxis = 2;

    DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
    LONG            rglDirection[2] = { 0, 0 };
    int ax1,ax2;
    DICONDITION condition[2];

    ZeroMemory(&condition,sizeof(condition));
    if (g_Opt.swap) {
        ax1=0;
        ax2=1;
    } else {
        ax1=1;
        ax2=0;
    }

    DIEFFECT eff;
    ZeroMemory(&eff,sizeof(eff));
    eff.dwSize                    = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                = INFINITE;
    eff.dwSamplePeriod            = 0;
    eff.dwGain                    = DI_FFNOMINALMAX;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval    = 0;
    eff.cAxes                    = g_dwNumForceFeedbackAxis;
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope                = 0;
    eff.cbTypeSpecificParams    = sizeof(condition);
    eff.lpvTypeSpecificParams    = &condition;
    eff.dwStartDelay            = 0;

    //Create damper effect
    condition[ax1].lOffset=0;
    condition[ax1].dwNegativeSaturation=g_Opt.damper;
    condition[ax1].dwPositiveSaturation=g_Opt.damper;
    condition[ax1].lDeadBand=0;
    condition[ax1].lNegativeCoefficient=g_Opt.damper;
    condition[ax1].lPositiveCoefficient=g_Opt.damper;
    condition[ax2].lOffset=0;
    condition[ax2].dwNegativeSaturation=g_Opt.damper;
    condition[ax2].dwPositiveSaturation=g_Opt.damper;
    condition[ax2].lDeadBand=0;
    condition[ax2].lNegativeCoefficient=g_Opt.damper;
    condition[ax2].lPositiveCoefficient=g_Opt.damper;

    if( FAILED( hr = g_pFFDevice->CreateEffect( GUID_Damper , &eff,&g_pEffectDamper, NULL ) ) )
    {
        return hr;
    }
    if (g_pEffectDamper) g_pEffectDamper->Start(1,0);

    // Create damper effect 2
    condition[ax1].lOffset=0;
    condition[ax1].dwNegativeSaturation=g_Opt.damper2;
    condition[ax1].dwPositiveSaturation=g_Opt.damper2;
    condition[ax1].lDeadBand=0;
    condition[ax1].lNegativeCoefficient=g_Opt.damper2;
    condition[ax1].lPositiveCoefficient=g_Opt.damper2;
    condition[ax2].lOffset=0;
    condition[ax2].dwNegativeSaturation=g_Opt.damper2;
    condition[ax2].dwPositiveSaturation=g_Opt.damper2;
    condition[ax2].lDeadBand=0;
    condition[ax2].lNegativeCoefficient=g_Opt.damper2;
    condition[ax2].lPositiveCoefficient=g_Opt.damper2;

    if( FAILED( hr = g_pFFDevice->CreateEffect( GUID_Damper , &eff,&g_pEffectDamper2, NULL ) ) )
    {
        return hr;
    }

    //Create Friction effect
    condition[ax1].dwNegativeSaturation=g_Opt.friction;
    condition[ax1].dwPositiveSaturation=g_Opt.friction;
    condition[ax1].lNegativeCoefficient=g_Opt.friction;
    condition[ax1].lPositiveCoefficient=g_Opt.friction;
    condition[ax2].dwNegativeSaturation=g_Opt.friction;
    condition[ax2].dwPositiveSaturation=g_Opt.friction;
    condition[ax2].lNegativeCoefficient=g_Opt.friction;
    condition[ax2].lPositiveCoefficient=g_Opt.friction;
    //eff.dwGain = g_Opt.friction;
    if( FAILED( hr = g_pFFDevice->CreateEffect( GUID_Friction , &eff,&g_pEffectFricti, NULL ) ) )
    {
        return hr;
    }
    if (g_pEffectFricti) g_pEffectFricti->Start(1,0);
    
    //Create Friction effect 2
    condition[ax1].dwNegativeSaturation=g_Opt.friction2;
    condition[ax1].dwPositiveSaturation=g_Opt.friction2;
    condition[ax1].lNegativeCoefficient=g_Opt.friction2;
    condition[ax1].lPositiveCoefficient=g_Opt.friction2;
    condition[ax2].dwNegativeSaturation=g_Opt.friction2;
    condition[ax2].dwPositiveSaturation=g_Opt.friction2;
    condition[ax2].lNegativeCoefficient=g_Opt.friction2;
    condition[ax2].lPositiveCoefficient=g_Opt.friction2;
    //eff.dwGain = g_Opt.friction;
    if( FAILED( hr = g_pFFDevice->CreateEffect( GUID_Friction , &eff,&g_pEffectFricti2, NULL ) ) )
    {
        return hr;
    }
    
    //Create Spring effect
    condition[ax1].dwNegativeSaturation=g_Opt.spring;
    condition[ax1].dwPositiveSaturation=g_Opt.spring;
    condition[ax1].lNegativeCoefficient=g_Opt.spring;
    condition[ax1].lPositiveCoefficient=g_Opt.spring;
    condition[ax2].dwNegativeSaturation=g_Opt.spring;
    condition[ax2].dwPositiveSaturation=g_Opt.spring;
    condition[ax2].lNegativeCoefficient=g_Opt.spring;
    condition[ax2].lPositiveCoefficient=g_Opt.spring;
    //eff.dwGain = g_Opt.spring;
    if( FAILED( hr = g_pFFDevice->CreateEffect( GUID_Spring, &eff, &g_pEffectSpring, NULL ) ) )
    {
        return hr;
    }

    if( NULL == g_pEffectSpring )
        return E_FAIL;
    g_pEffectSpring->Start(1,0);
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick and counting
//       each force feedback enabled axis
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                VOID* pContext )
{
    DWORD* pdwNumForceFeedbackAxis = (DWORD*) pContext;

    if( (pdidoi->dwFlags & DIDOI_FFACTUATOR) != 0 )
        (*pdwNumForceFeedbackAxis)++;

    return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// Name: EnumFFDevicesCallback()
// Desc: Called once for each enumerated force feedback device. If we find
//       one, create a device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumFFDevicesCallback( const DIDEVICEINSTANCE* pInst, 
                                     VOID* pContext )
{
    LPDIRECTINPUTDEVICE8 pDevice;
    HRESULT              hr;

    
    // Obtain an interface to the enumerated force feedback device.
    hr = g_pDI->CreateDevice( pInst->guidInstance, &pDevice, NULL );

    // If it failed, then we can't use this device for some
    // bizarre reason.  (Maybe the user unplugged it while we
    // were in the middle of enumerating it.)  So continue enumerating
    if( FAILED(hr) ) 
        return DIENUM_CONTINUE;

    // We successfully created an IDirectInputDevice8.
    wcscpy_s(g_Sticks[g_iNsticks].name,pInst->tszInstanceName);
    g_Sticks[g_iNsticks].dev=pDevice;
    if (pInst->guidFFDriver.Data1!=NULL) {
        g_iFF=g_iNsticks;
        g_pFFDevice=pDevice;
    }
    g_iNsticks++;
    if (32==g_iNsticks)
        return DIENUM_STOP;
    return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
VOID FreeDirectInput()
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
    for (int i=0;i<g_iNsticks;i++)
        if( g_Sticks[i].dev) 
            g_Sticks[i].dev->Unacquire();
    
    // Release any DirectInput objects.
    SAFE_RELEASE( g_pEffectSpring );
    SAFE_RELEASE( g_pEffectDamper );
    SAFE_RELEASE( g_pEffectFricti );
    for (int i=0;i<g_iNsticks;i++)
        SAFE_RELEASE(g_Sticks[i].dev);
    //SAFE_RELEASE( g_pFFDevice );
    SAFE_RELEASE( g_pDI );

    g_iNsticks=0;
    g_iFF=-1;
    g_nXForce=0;
    g_nYForce=0;

    SaveOptionsToFile();
}

//-----------------------------------------------------------------------------
// Name: SetDeviceForcesXY()
// Desc: Apply the X and Y forces to the effect we prepared.
//-----------------------------------------------------------------------------
HRESULT SetDeviceForcesXY() //This function is part of the original sample but not used in this application
{
    // Modifying an effect is basically the same as creating a new one, except
    // you need only specify the parameters you are modifying
    LONG rglDirection[2] = { 0, 0 };

    DICONSTANTFORCE cf;

    if( g_dwNumForceFeedbackAxis == 1 )
    {
        // If only one force feedback axis, then apply only one direction and 
        // keep the direction at zero
        cf.lMagnitude = g_nXForce;
        rglDirection[0] = 0;
    }
    else
    {
        // If two force feedback axis, then apply magnitude from both directions 
        rglDirection[0] = g_nXForce;
        rglDirection[1] = g_nYForce;
        cf.lMagnitude = (DWORD)sqrt( (double)g_nXForce * (double)g_nXForce +
                                     (double)g_nYForce * (double)g_nYForce );
    }

    DIEFFECT eff;
    ZeroMemory( &eff, sizeof(eff) );
    eff.dwSize                = sizeof(DIEFFECT);
    eff.dwFlags               = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.cAxes                 = g_dwNumForceFeedbackAxis;
    eff.rglDirection          = rglDirection;
    eff.lpEnvelope            = 0;
    eff.cbTypeSpecificParams  = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;
    eff.dwStartDelay            = 0;

    // Now set the new parameters and start the effect immediately.
    return g_pEffectSpring->SetParameters( &eff, DIEP_DIRECTION |
                                           DIEP_TYPESPECIFICPARAMS |
                                           DIEP_START );
}

HRESULT SetDeviceConditions()
{
    LONG rglDirection[2] = {0, 0};
    DICONDITION condition[2];
    DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
    int ax1,ax2;
    DIEFFECT eff;

    if (g_Opt.swap) {
        ax1=0;
        ax2=1;
    } else {
        ax1=1;
        ax2=0;
    }
    if (g_bSpring) SetDeviceSpring();
    

    // Friction
    condition[ax1].lOffset=0;
    condition[ax1].dwNegativeSaturation=g_Opt.friction;
    condition[ax1].dwPositiveSaturation=g_Opt.friction;    
    condition[ax1].lDeadBand=0;
    condition[ax1].lNegativeCoefficient=g_Opt.friction;
    condition[ax1].lPositiveCoefficient=g_Opt.friction;
    condition[ax2].lOffset=0;
    condition[ax2].dwNegativeSaturation=g_Opt.friction;
    condition[ax2].dwPositiveSaturation=g_Opt.friction;
    condition[ax2].lDeadBand=0;
    condition[ax2].lNegativeCoefficient=g_Opt.friction;
    condition[ax2].lPositiveCoefficient=g_Opt.friction;
    ZeroMemory(&eff,sizeof(eff));
    eff.dwSize                    = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                = INFINITE;
    eff.dwSamplePeriod            = 0;
    eff.dwGain                    = DI_FFNOMINALMAX;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval    = 0;
    eff.cAxes                    = g_dwNumForceFeedbackAxis;
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope                = 0;
    eff.cbTypeSpecificParams    = sizeof(condition);
    eff.lpvTypeSpecificParams    = condition;
    eff.dwStartDelay            = 0;

    if (g_pEffectFricti)
        g_pEffectFricti->SetParameters(&eff,DIEP_TYPESPECIFICPARAMS);
    
    // Friction 2
    condition[ax1].lOffset=0;
    condition[ax1].dwNegativeSaturation=g_Opt.friction2;
    condition[ax1].dwPositiveSaturation=g_Opt.friction2;    
    condition[ax1].lDeadBand=0;
    condition[ax1].lNegativeCoefficient=g_Opt.friction2;
    condition[ax1].lPositiveCoefficient=g_Opt.friction2;
    condition[ax2].lOffset=0;
    condition[ax2].dwNegativeSaturation=g_Opt.friction2;
    condition[ax2].dwPositiveSaturation=g_Opt.friction2;
    condition[ax2].lDeadBand=0;
    condition[ax2].lNegativeCoefficient=g_Opt.friction2;
    condition[ax2].lPositiveCoefficient=g_Opt.friction2;
    ZeroMemory(&eff,sizeof(eff));
    eff.dwSize                    = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                = INFINITE;
    eff.dwSamplePeriod            = 0;
    eff.dwGain                    = DI_FFNOMINALMAX;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval    = 0;
    eff.cAxes                    = g_dwNumForceFeedbackAxis;
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope                = 0;
    eff.cbTypeSpecificParams    = sizeof(condition);
    eff.lpvTypeSpecificParams    = condition;
    eff.dwStartDelay            = 0;

    if (g_pEffectFricti2)
        g_pEffectFricti2->SetParameters(&eff,DIEP_TYPESPECIFICPARAMS);

    // Damper 1
    condition[ax1].lOffset=0;
    condition[ax1].dwNegativeSaturation=g_Opt.damper;
    condition[ax1].dwPositiveSaturation=g_Opt.damper;
    condition[ax1].lDeadBand=0;
    condition[ax1].lNegativeCoefficient=g_Opt.damper;
    condition[ax1].lPositiveCoefficient=g_Opt.damper;
    condition[ax2].lOffset=0;
    condition[ax2].dwNegativeSaturation=g_Opt.damper;
    condition[ax2].dwPositiveSaturation=g_Opt.damper;
    condition[ax2].lDeadBand=0;
    condition[ax2].lNegativeCoefficient=g_Opt.damper;
    condition[ax2].lPositiveCoefficient=g_Opt.damper;
    ZeroMemory(&eff,sizeof(eff));
    eff.dwSize                    = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                = INFINITE;
    eff.dwSamplePeriod            = 0;
    eff.dwGain                    = DI_FFNOMINALMAX;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval    = 0;
    eff.cAxes                    = g_dwNumForceFeedbackAxis;
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope                = 0;
    eff.cbTypeSpecificParams    = sizeof(condition);
    eff.lpvTypeSpecificParams    = condition;
    eff.dwStartDelay            = 0;

    if (g_pEffectDamper)
        g_pEffectDamper->SetParameters(&eff,DIEP_TYPESPECIFICPARAMS);
    
    // Damper 2
    condition[ax1].lOffset=0;
    condition[ax1].dwNegativeSaturation=g_Opt.damper2;
    condition[ax1].dwPositiveSaturation=g_Opt.damper2;
    condition[ax1].lDeadBand=0;
    condition[ax1].lNegativeCoefficient=g_Opt.damper2;
    condition[ax1].lPositiveCoefficient=g_Opt.damper2;
    condition[ax2].lOffset=0;
    condition[ax2].dwNegativeSaturation=g_Opt.damper2;
    condition[ax2].dwPositiveSaturation=g_Opt.damper2;
    condition[ax2].lDeadBand=0;
    condition[ax2].lNegativeCoefficient=g_Opt.damper2;
    condition[ax2].lPositiveCoefficient=g_Opt.damper2;
    ZeroMemory(&eff,sizeof(eff));
    eff.dwSize                    = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                = INFINITE;
    eff.dwSamplePeriod            = 0;
    eff.dwGain                    = DI_FFNOMINALMAX;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval    = 0;
    eff.cAxes                    = g_dwNumForceFeedbackAxis;
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope                = 0;
    eff.cbTypeSpecificParams    = sizeof(condition);
    eff.lpvTypeSpecificParams    = condition;
    eff.dwStartDelay            = 0;

    if (g_pEffectDamper2) 
        g_pEffectDamper2->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
    
    return 0;
}

HRESULT SetDeviceSpring()
{
    LONG rglDirection[2] = {0, 0};
    DICONDITION condition[2];
    DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
    int ax1,ax2;

    if (g_Opt.swap) {
        ax1=0;
        ax2=1;
    } else {
        ax1=1;
        ax2=0;
    }
    ZeroMemory(condition,sizeof(condition));
    condition[ax1].lOffset=g_nYForce;
    condition[ax1].dwNegativeSaturation=g_Opt.spring;
    condition[ax1].dwPositiveSaturation=g_Opt.spring;
    condition[ax1].lDeadBand=0;
    condition[ax1].lNegativeCoefficient=g_Opt.spring;
    condition[ax1].lPositiveCoefficient=g_Opt.spring;

    condition[ax2].lOffset=(g_Opt.swap?-g_nXForce:g_nXForce);
    condition[ax2].dwNegativeSaturation=g_Opt.spring;
    condition[ax2].dwPositiveSaturation=g_Opt.spring;
    condition[ax2].lDeadBand=0;
    condition[ax2].lNegativeCoefficient=g_Opt.spring;
    condition[ax2].lPositiveCoefficient=g_Opt.spring;

    DIEFFECT eff;
    ZeroMemory(&eff,sizeof(eff));
    eff.dwSize                    = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                = INFINITE;
    eff.dwSamplePeriod            = 0;
    eff.dwGain                    = DI_FFNOMINALMAX;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval    = 0;
    eff.cAxes                    = g_dwNumForceFeedbackAxis;
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope                = 0;
    eff.cbTypeSpecificParams    = sizeof(condition);
    eff.lpvTypeSpecificParams    = condition;
    eff.dwStartDelay            = 0;

    if (g_pEffectSpring)
        return g_pEffectSpring->SetParameters( &eff, DIEP_TYPESPECIFICPARAMS | DIEP_START );
    return -1;

}

void Botones()
{
    using namespace std::chrono;

    // Handle force trim toggle
    int i = g_Opt.btrimToggle;
    BYTE new_btrimToggle_state = g_js.rgbButtons[i];
    // debounce
    auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (new_btrimToggle_state == 0x80 && old_btrimToggle_state == 0x00 && ms - time_last > 100) {
        g_bBoton[i] = (g_bBoton[i] == DOWN) ? UP : DOWN;
        time_last = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
    old_btrimToggle_state = new_btrimToggle_state;

    // handle other buttons
     for (int i = 0; i < MAXBUTTONS; i++) {
        if (i != g_Opt.btrimToggle) {
            if (g_js.rgbButtons[i] == 0x80) {
                g_bBoton[i] = DOWN;
            }
            else {
                if (g_bBoton[i] == DOWN) {
                    g_bBoton[i] = RELEASED;
                }
                else {
                    g_bBoton[i] = UP;
                }
            }
        }
    }
}

HRESULT Adquirir()
{
    HRESULT hr;
    for (int i=0;i<g_iNsticks;i++)
        hr=g_Sticks[i].dev->Acquire();
    return 0;
}

void StartEffects()
{
    Sleep(100);
    if( g_pEffectSpring ) 
        g_pEffectSpring->Start( 1, 0 );
    if( g_pEffectDamper ) 
        g_pEffectDamper->Start( 1, 0 );
    if( g_pEffectFricti ) 
        g_pEffectFricti->Start( 1, 0 );
}

void StopEffects()
{
    Sleep(100);
    if( g_pEffectSpring ) 
        g_pEffectSpring->Stop();
    if( g_pEffectDamper ) 
        g_pEffectDamper->Stop();
    if( g_pEffectFricti ) 
        g_pEffectFricti->Stop();
    if( g_pEffectDamper2 ) 
        g_pEffectDamper2->Stop();
    if( g_pEffectFricti2 ) 
        g_pEffectFricti2->Stop();
}

void JoystickStuffIT()  //IT -> instantaneous trimming
{
    if (FAILED(poll(g_Sticks[g_Opt.jtrim].dev,&g_js)))
        return;

    // Handle force trim toggle
    if (g_bBoton[g_Opt.btrimToggle] == DOWN && (g_bSpring)) { // Disable spring force if force trim toggle button was pressed
        NoSpring();
        is_centered = false;
    } else if (g_bBoton[g_Opt.btrimToggle] == UP && !is_centered) {  // Update spring center if trim toggle button pressed & joystick un-centered
        InstantTrim();
        is_centered = true;
    }

    // Handle center trim
    if (g_bBoton[g_Opt.btrimCenter] == DOWN && (g_bSpring)) // Trim center
        CenterTrim();

    // Handle hold trim
    if ((g_bBoton[g_Opt.btrimHold] == DOWN) && (g_bSpring)) //Disable spring force if trim button is down
        NoSpring();
    else if (g_bBoton[g_Opt.btrimHold]==RELEASED) //Update spring center if trim button is released
        InstantTrim();
}

void JoystickStuffPT()  //PT -> progressive trimming
{
    if (0==g_bSpring)
        return; //Do nothing if the trimming button is down
    if (FAILED(poll(g_pFFDevice,&g_js)))
        return;
    switch (g_js.rgdwPOV[0]) { //To do: add diagonals for simultaneous directions
        case 0:                                            //Direction away from the user
            SUBLIM(g_nYForce,TRIMSTEP,-DI_FFNOMINALMAX); 
            break;
        case 18000:                                        //Direction TO THE USER
            ADDLIM(g_nYForce,TRIMSTEP,DI_FFNOMINALMAX); 
            break;
        case 9000:                                        //Direction right
            ADDLIM(g_nXForce,TRIMSTEP,DI_FFNOMINALMAX); 
            break;
        case 27000:                                        //Direction left
            SUBLIM(g_nXForce,TRIMSTEP,-DI_FFNOMINALMAX);
            break;
    }
    SetDeviceSpring();
}

//nospring changes spring tension to a very low value so it feels as not tension is being applied.
//Initialy it was just a function that called effect->stop, but it was troublesome, specially with the
//logitech g940
void NoSpring()
{
    LONG rglDirection[2] = {0, 0};
    DICONDITION condition[2];
    DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
    int ax1,ax2;

    if (g_Opt.swap) {
        ax1=0;
        ax2=1;
    } else {
        ax1=1;
        ax2=0;
    }
    ZeroMemory(condition,sizeof(condition));
    condition[ax1].lOffset=g_nYForce;
    condition[ax1].dwNegativeSaturation=100;
    condition[ax1].dwPositiveSaturation=100;
    condition[ax1].lDeadBand=0;
    condition[ax1].lNegativeCoefficient=100;
    condition[ax1].lPositiveCoefficient=100;

    condition[ax2].lOffset=(g_Opt.swap?-g_nXForce:g_nXForce);
    condition[ax2].dwNegativeSaturation=100;
    condition[ax2].dwPositiveSaturation=100;
    condition[ax2].lDeadBand=0;
    condition[ax2].lNegativeCoefficient=100;
    condition[ax2].lPositiveCoefficient=100;

    DIEFFECT eff;
    ZeroMemory(&eff,sizeof(eff));
    eff.dwSize                    = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                = INFINITE;
    eff.dwSamplePeriod            = 0;
    eff.dwGain                    = DI_FFNOMINALMAX;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval    = 0;
    eff.cAxes                    = g_dwNumForceFeedbackAxis;
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope                = 0;
    eff.cbTypeSpecificParams    = sizeof(condition);
    eff.lpvTypeSpecificParams    = condition;
    eff.dwStartDelay            = 0;

    if (g_pEffectSpring)
        g_pEffectSpring->SetParameters( &eff, DIEP_TYPESPECIFICPARAMS | DIEP_START );
    
    if (g_pEffectDamper) g_pEffectDamper->Stop();
    if (g_pEffectDamper2) g_pEffectDamper2->Start(1, 0);
    if (g_pEffectFricti) g_pEffectFricti->Stop();
    if (g_pEffectFricti2) g_pEffectFricti2->Start(1, 0);
    g_bSpring=0; //flag the spring as not working
}

void InstantTrim()
{
    if (FAILED(poll(g_pFFDevice,&g_js)))
        return;

    g_nXForce = ((double)g_js.lX/65535)*20000-10000;  //Range has to be mapped from -65535,65535 to -10000,10000
    g_nYForce = ((double)g_js.lY/65535)*20000-10000;
    if (g_pEffectDamper2) g_pEffectDamper2->Stop();
    if (g_pEffectDamper) g_pEffectDamper->Start(1, 0);
    if (g_pEffectFricti2) g_pEffectFricti2->Stop();
    if (g_pEffectFricti) g_pEffectFricti->Start(1, 0);
    SetDeviceSpring();
    g_bSpring=1;
}

// Center joystick and turn on spring
void CenterTrim() {
    if (FAILED(poll(g_pFFDevice, &g_js)))
        return;
    while (g_nXForce != 0 && g_nYForce != 0) {
        if (g_nXForce != 0) g_nXForce = g_nXForce / CENTER_DAMP_COEFF;
        if (g_nYForce != 0) g_nYForce = g_nYForce / CENTER_DAMP_COEFF;
        SetDeviceSpring();
    }
    SetDeviceSpring();
    g_bSpring = 1;
}

int JoysticksNumber()
{
    return g_iNsticks;
}

LPCTSTR JoystickName(int i)
{
    if (i>=g_iNsticks)
        return NULL;
    return g_Sticks[i].name;
}

void SetTrimmer(int j,int b,int b2,int b3)
{
    if (j<g_iNsticks)
        g_Opt.jtrim=j;
    if (b < MAXBUTTONS) {
        g_Opt.btrimHold = b;
        g_Opt.btrimToggle = b2;
        g_Opt.btrimCenter = b3; 
    }
}

void GetTrimmer(int&j,int&b,int&b2,int&b3)
{
    j=g_Opt.jtrim;
    b=g_Opt.btrimHold;
    b2 = g_Opt.btrimToggle; 
    b3 = g_Opt.btrimCenter; 
}

void SetJtOptions(stoptions *so)
{
    SetTrimmer(so->jtrim,so->btrimHold,so->btrimToggle,so->btrimCenter); 
    g_Opt.damper=so->damper*100;
    g_Opt.damper2 = so->damper2 * 100;
    g_Opt.friction=so->friction*100;
    g_Opt.friction2=so->friction2*100;
    g_Opt.spring=so->spring*100;
    g_Opt.iKey = so->iKey;
    g_Opt.swap=so->swap;
    g_Opt.trimmode=so->trimmode;
    SetDeviceConditions();
}

void GetJtOptions(stoptions *so)
{
    GetTrimmer(so->jtrim,so->btrimHold,so->btrimToggle,so->btrimCenter); 
    so->spring=g_Opt.spring/100;
    so->damper=g_Opt.damper/100;
    so->damper2 = g_Opt.damper2 / 100;
    so->friction=g_Opt.friction/100;
    so->friction2=g_Opt.friction2/100;
    so->iKey = g_Opt.iKey;
    so->swap=g_Opt.swap;
    so->trimmode=g_Opt.trimmode;
}

BOOL LoadOptionsFromFile()
{
    HANDLE fhnd;
    stoptions tmpopt;
    DWORD r;
    unsigned long mn;

    fhnd=CreateFile(OPTFILENAME,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (INVALID_HANDLE_VALUE==fhnd) {
            return false;
    }
    ReadFile(fhnd,&mn,sizeof(mn),&r,NULL);
    if (mn!=MAGICNUMBER) {
        CloseHandle(fhnd);
        return false;
    }
    ReadFile(fhnd,&tmpopt,sizeof(stoptions),&r,NULL);
    SetJtOptions(&tmpopt);
    CloseHandle(fhnd);
    return true;
}

BOOL SaveOptionsToFile()
{    
    HANDLE fhnd;
    DWORD w;
    unsigned long mn=MAGICNUMBER;
    stoptions so;

    fhnd=CreateFile(OPTFILENAME,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (INVALID_HANDLE_VALUE==fhnd) {
            return false;
    }
    WriteFile(fhnd,&mn,sizeof(mn),&w,NULL);
    GetJtOptions(&so);
    WriteFile(fhnd,&so,sizeof(stoptions),&w,NULL);
    CloseHandle(fhnd);
    return true;
}