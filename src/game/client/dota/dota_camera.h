//========= Copyright © 2024, DOTA 2 Beta Mod. All rights reserved. ============//
//
// Purpose: DOTA 2 style RTS camera system header
//
//=============================================================================//

#ifndef DOTA_CAMERA_H
#define DOTA_CAMERA_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "mathlib/vmatrix.h"

class CViewSetup;

//----------------------------------------------------------------------------- 
// DOTA Camera Class Forward Declaration
//----------------------------------------------------------------------------- 
class CDotaCamera
{
public:
    void SetTargetPosition(const Vector &vecTarget);
    void SetCameraDistance(float flDistance);
    void LockToHero(bool bLock);
    bool IsActive() const;
    Vector GetCameraPosition() const;
    Vector GetTargetPosition() const;
};

//----------------------------------------------------------------------------- 
// External Camera Access Functions
//----------------------------------------------------------------------------- 
CDotaCamera* GetDotaCamera();
void InitDotaCamera();
void ShutdownDotaCamera();
void UpdateDotaCamera(float flFrameTime);
void SetupDotaCameraView(CViewSetup &view);

//----------------------------------------------------------------------------- 
// Console Commands (declared here for external access)
//----------------------------------------------------------------------------- 
// Console commands are defined in the .cpp file

#endif // DOTA_CAMERA_H 