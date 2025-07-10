//========= Copyright © 2024, DOTA 2 Beta Mod. All rights reserved. ============//
//
// Purpose: DOTA 2 client mode implementation
//
//=============================================================================//

#include "cbase.h"
#include "dota_clientmode.h"
#include "dota_camera.h"
#include "view.h"
#include "iviewrender.h"
#include "c_baseplayer.h"
#include "iclientmode.h"
#include "vgui/ISurface.h"
#include "vgui/IPanel.h"
#include "vgui_controls/Panel.h"
#include "engine/IEngineSound.h"
#include "cdll_client_int.h"
#include "cdll_util.h"
#include "mathlib/mathlib.h"
#include "tier0/dbg.h"
#include "IGameUIFuncs.h"
#include "ivmodemanager.h"
#include "baseviewport.h"
#include "hud.h"
#include "hud_basechat.h"
#include "hud_macros.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Declare HUD elements
DECLARE_HUDELEMENT(CBaseHudChat);

// ConVars
ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );

// Mode manager for DOTA
class CDotaModeManager : public IVModeManager
{
public:
    virtual void	Init() {}
    virtual void	SwitchMode( bool commander, bool force ) {}
    virtual void	LevelInit( const char *newmap ) {}
    virtual void	LevelShutdown( void ) {}
    virtual void	ActivateMouse( bool isactive ) {}
};

static CDotaModeManager g_DotaModeManager;
IVModeManager *modemanager = &g_DotaModeManager;

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
    DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
    virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
    {
        BaseClass::ApplySchemeSettings( pScheme );

        SetPaintBackgroundEnabled( false );
    }

    virtual void CreateDefaultPanels( void ) { /* don't create any panels yet*/ };
    virtual void UpdateAllPanels( void ) { /* don't update any panels yet*/ };
    virtual void Start( IGameUIFuncs *pGameUIFuncs, IGameEventManager2 *pGameEventManager ) { BaseClass::Start( pGameUIFuncs, pGameEventManager ); }
    virtual void SetStagingRenderTarget( void ) {}
};

// Global client mode instances
CDotaClientMode g_DotaClientMode[MAX_SPLITSCREEN_PLAYERS];
static CDotaClientMode g_DotaFullscreenClientMode;

IClientMode *GetClientMode()
{
    return &g_DotaClientMode[0];
}

IClientMode *GetClientModeNormal()
{
    Assert( engine->IsLocalPlayerResolvable() );
    return &g_DotaClientMode[ engine->GetActiveSplitScreenPlayerSlot() ];
}

IClientMode *GetFullscreenClientMode( void )
{
    return &g_DotaFullscreenClientMode;
}

//----------------------------------------------------------------------------- 
// CDotaClientMode Implementation
//----------------------------------------------------------------------------- 
CDotaClientMode::CDotaClientMode()
{
    m_bInitialized = false;
}

CDotaClientMode::~CDotaClientMode()
{
    Shutdown();
}

void CDotaClientMode::Init()
{
    if (m_bInitialized)
        return;
        
    BaseClass::Init();
    
    // Initialize DOTA camera system
    InitDotaCamera();
    
    m_bInitialized = true;
    
    // Warning("DOTA Client Mode: Initialized\n");
}

void CDotaClientMode::Shutdown()
{
    if (!m_bInitialized)
        return;
        
    // Shutdown DOTA camera system
    ShutdownDotaCamera();
    
    BaseClass::Shutdown();
    
    m_bInitialized = false;
}

void CDotaClientMode::InitViewport()
{
    // Initialize the viewport for DOTA
    m_pViewport = new CHudViewport();
    m_pViewport->Start( gameuifuncs, gameeventmanager );
}

void CDotaClientMode::Update()
{
    BaseClass::Update();
    
    // Update DOTA camera system
    UpdateDotaCamera(gpGlobals->frametime);
}

void CDotaClientMode::OverrideView(CViewSetup *pSetup)
{
    // Let the DOTA camera system override the view
    SetupDotaCameraView(*pSetup);
}

int CDotaClientMode::KeyInput(int down, ButtonCode_t keynum, const char *pszCurrentBinding)
{
    // Handle DOTA-specific key input
    if (down)
    {
        switch (keynum)
        {
            case KEY_SPACE:
                // Space bar - center camera on hero
                {
                    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
                    if (pPlayer && GetDotaCamera())
                    {
                        GetDotaCamera()->SetTargetPosition(pPlayer->GetAbsOrigin());
                        GetDotaCamera()->LockToHero(true);
                    }
                }
                return 1;
                
            case KEY_Y:
                // Y key - toggle camera lock
                engine->ClientCmd("dota_camera_lock_hero");
                return 1;
                
            case KEY_F1:
                // F1 - camera info
                engine->ClientCmd("dota_camera_info");
                return 1;
        }
    }
    
    return BaseClass::KeyInput(down, keynum, pszCurrentBinding);
}

int CDotaClientMode::HudElementKeyInput(int down, ButtonCode_t keynum, const char *pszCurrentBinding)
{
    // Handle HUD element key input
    return BaseClass::HudElementKeyInput(down, keynum, pszCurrentBinding);
}

void CDotaClientMode::DoPostScreenSpaceEffects(const CViewSetup *pSetup)
{
    // Simple DOTA post-screen effects
    // Skip the base class call since we're implementing our own
}

void CDotaClientMode::PostRender()
{
    // Post-render processing for DOTA
    BaseClass::PostRender();
}

bool CDotaClientMode::CreateMove(float flInputSampleTime, CUserCmd *cmd)
{
    // Create movement commands for DOTA
    return BaseClass::CreateMove(flInputSampleTime, cmd);
}

void CDotaClientMode::LevelInit(const char *newmap)
{
    BaseClass::LevelInit(newmap);
    
    // Initialize DOTA camera for the new map
    InitDotaCamera();
}

void CDotaClientMode::LevelShutdown()
{
    // Shutdown DOTA camera
    ShutdownDotaCamera();
    
    BaseClass::LevelShutdown();
}

bool CDotaClientMode::ShouldDrawDetailObjects()
{
    // DOTA should draw detail objects
    return true;
}

bool CDotaClientMode::ShouldDrawEntity(C_BaseEntity *pEnt)
{
    // Custom entity culling for DOTA
    return BaseClass::ShouldDrawEntity(pEnt);
}

bool CDotaClientMode::ShouldDrawParticles()
{
    // DOTA should draw particles
    return true;
}

bool CDotaClientMode::ShouldDrawFog()
{
    // DOTA should draw fog
    return true;
}

void CDotaClientMode::OnColorCorrectionWeightsReset()
{
    // Handle color correction reset for DOTA
    BaseClass::OnColorCorrectionWeightsReset();
}

float CDotaClientMode::GetColorCorrectionScale() const
{
    // Return color correction scale for DOTA
    return BaseClass::GetColorCorrectionScale();
}

void CDotaClientMode::FireGameEvent(IGameEvent *event)
{
    // Handle DOTA-specific game events
    BaseClass::FireGameEvent(event);
}

void CDotaClientMode::PostRenderVGui()
{
    // Post-render VGUI for DOTA
    BaseClass::PostRenderVGui();
}

//----------------------------------------------------------------------------- 
// Console Commands
//----------------------------------------------------------------------------- 
CON_COMMAND(dota_clientmode_info, "Display DOTA client mode information")
{
    ConMsg("DOTA Client Mode Status:\n");
    ConMsg("Initialized: %s\n", g_DotaClientMode[0].IsInitialized() ? "Yes" : "No");
    ConMsg("Camera Active: %s\n", GetDotaCamera() && GetDotaCamera()->IsActive() ? "Yes" : "No");
} 