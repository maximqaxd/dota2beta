//========= Copyright © 2024, DOTA 2 Beta Mod. All rights reserved. ============//
//
// Purpose: DOTA 2 client mode header
//
//=============================================================================//

#ifndef DOTA_CLIENTMODE_H
#define DOTA_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "igameevents.h"
#include "const.h"

class CViewSetup;
class C_BaseEntity;
class CUserCmd;
class KeyValues;

//----------------------------------------------------------------------------- 
// DOTA Client Mode Class
//----------------------------------------------------------------------------- 
class CDotaClientMode : public ClientModeShared
{
public:
    DECLARE_CLASS(CDotaClientMode, ClientModeShared);
    
    CDotaClientMode();
    virtual ~CDotaClientMode();
    
    // IClientMode overrides
    virtual void Init();
    virtual void Shutdown();
    virtual void InitViewport();
    virtual void Update();
    virtual void OverrideView(CViewSetup *pSetup);
    virtual int KeyInput(int down, ButtonCode_t keynum, const char *pszCurrentBinding);
    virtual int HudElementKeyInput(int down, ButtonCode_t keynum, const char *pszCurrentBinding);
    virtual void DoPostScreenSpaceEffects(const CViewSetup *pSetup);
    virtual void PostRender();
    virtual bool CreateMove(float flInputSampleTime, CUserCmd *cmd);
    virtual void LevelInit(const char *newmap);
    virtual void LevelShutdown();
    virtual bool ShouldDrawDetailObjects();
    virtual bool ShouldDrawEntity(C_BaseEntity *pEnt);
    virtual bool ShouldDrawParticles();
    virtual bool ShouldDrawFog();
    virtual void OnColorCorrectionWeightsReset();
    virtual float GetColorCorrectionScale() const;
    virtual void FireGameEvent(IGameEvent *event);
    virtual void PostRenderVGui();
    
    // DOTA-specific methods
    bool IsInitialized() const { return m_bInitialized; }
    
private:
    bool m_bInitialized;
};

extern CDotaClientMode g_DotaClientMode[MAX_SPLITSCREEN_PLAYERS];

#endif // DOTA_CLIENTMODE_H 