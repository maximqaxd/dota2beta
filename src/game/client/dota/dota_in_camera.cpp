//========= Copyright © 2024, DOTA 2 Beta Mod. All rights reserved. ============//
//
// Purpose: DOTA 2 style RTS camera system for Source Engine SDK 2010
//
//=============================================================================//

#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "iclientmode.h"
#include "prediction.h"
#include "viewrender.h"
#include "c_baseentity.h"
#include "c_baseplayer.h"
#include "mathlib/mathlib.h"
#include "engine/ivdebugoverlay.h"
#include "tier0/vprof.h"
#include "in_buttons.h"
#include "inputsystem/iinputsystem.h"
#include "convar.h"
#include "kbutton.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// DOTA 2 camera constants
#define DOTA_CAMERA_DISTANCE 1134.0f
#define DOTA_CAMERA_PITCH_ANGLE 25.0f
#define DOTA_CAMERA_HEIGHT_OFFSET 512.0f
#define DOTA_CAMERA_MOVE_SPEED 800.0f
#define DOTA_CAMERA_EDGE_SCROLL_SPEED 400.0f
#define DOTA_CAMERA_SMOOTH_FACTOR 0.08f
#define DOTA_CAMERA_ZOOM_MIN 800.0f
#define DOTA_CAMERA_ZOOM_MAX 1600.0f
#define DOTA_CAMERA_ZOOM_SPEED 200.0f
#define DOTA_CAMERA_EDGE_BOUNDARY 50.0f

// ConVars for DOTA camera system
static ConVar dota_camera_distance( "dota_camera_distance", "1134", FCVAR_CHEAT, "DOTA 2 camera distance from target" );
static ConVar dota_camera_pitch( "dota_camera_pitch", "25", FCVAR_CHEAT, "DOTA 2 camera pitch angle" );
static ConVar dota_camera_move_speed( "dota_camera_move_speed", "800", FCVAR_CHEAT, "DOTA 2 camera movement speed" );
static ConVar dota_camera_edge_scroll( "dota_camera_edge_scroll", "1", FCVAR_CHEAT, "Enable edge scrolling for DOTA camera" );
static ConVar dota_camera_smooth( "dota_camera_smooth", "0.08", FCVAR_CHEAT, "DOTA 2 camera smoothing factor" );
static ConVar dota_camera_zoom_enabled( "dota_camera_zoom_enabled", "1", FCVAR_CHEAT, "Enable camera zoom" );
static ConVar dota_camera_lock_to_hero( "dota_camera_lock_to_hero", "0", FCVAR_CHEAT, "Lock camera to hero" );

// Input system externs
extern kbutton_t	in_forward;
extern kbutton_t	in_back;
extern kbutton_t	in_moveleft;
extern kbutton_t	in_moveright;
extern kbutton_t	in_attack;
extern kbutton_t	in_attack2;

//----------------------------------------------------------------------------- 
// DOTA 2 RTS Camera System
//----------------------------------------------------------------------------- 
class CDotaCamera
{
public:
    CDotaCamera();
    ~CDotaCamera();

    void Init();
    void Shutdown();
    void Update(float flFrameTime);
    void UpdateViewAngles();
    void UpdateViewPosition();
    
    // Camera control
    void SetTargetPosition(const Vector &vecTarget);
    void SetCameraDistance(float flDistance);
    void SetCameraPitch(float flPitch);
    void LockToHero(bool bLock);
    void EnableEdgeScrolling(bool bEnable);
    
    // Input handling
    void HandleInput(float flFrameTime);
    void HandleMouseInput(float flFrameTime);
    void HandleKeyboardInput(float flFrameTime);
    void HandleEdgeScrolling(float flFrameTime);
    
    // View setup
    void SetupView(CViewSetup &view);
    void GetViewOrigin(Vector &vecOrigin);
    void GetViewAngles(QAngle &angAngles);
    
    // Game event handling
    void OnPlayerSpawn();
    void OnPlayerDeath();
    
    // Utility
    bool IsActive() const { return m_bActive; }
    void SetActive(bool bActive) { m_bActive = bActive; }
    Vector GetCameraPosition() const { return m_vecCameraPosition; }
    Vector GetTargetPosition() const { return m_vecTargetPosition; }

private:
    void UpdateCameraPosition(float flFrameTime);
    void UpdateCameraZoom(float flFrameTime);
    void ClampCameraToWorldBounds();
    void SmoothCameraMovement(float flFrameTime);
    Vector CalculateIdealCameraPosition();
    void TraceForValidCameraPosition(Vector &vecDesiredPos);
    
    // Camera state
    bool m_bActive;
    bool m_bInitialized;
    Vector m_vecCameraPosition;
    Vector m_vecTargetPosition;
    Vector m_vecVelocity;
    QAngle m_angViewAngles;
    
    // Camera settings
    float m_flCameraDistance;
    float m_flCameraPitch;
    float m_flCurrentZoom;
    float m_flTargetZoom;
    bool m_bLockToHero;
    bool m_bEdgeScrollEnabled;
    
    // Input state
    Vector m_vecInputDirection;
    bool m_bMouseDragging;
    int m_nLastMouseX;
    int m_nLastMouseY;
    
    // Smooth movement
    Vector m_vecDesiredPosition;
    float m_flSmoothFactor;
    
    // World bounds
    Vector m_vecWorldMin;
    Vector m_vecWorldMax;
};

// Global camera instance
static CDotaCamera g_DotaCamera;

//----------------------------------------------------------------------------- 
// CDotaCamera Implementation
//----------------------------------------------------------------------------- 
CDotaCamera::CDotaCamera()
{
    m_bActive = false;
    m_bInitialized = false;
    m_vecCameraPosition = vec3_origin;
    m_vecTargetPosition = vec3_origin;
    m_vecVelocity = vec3_origin;
    m_angViewAngles = vec3_angle;
    
    m_flCameraDistance = DOTA_CAMERA_DISTANCE;
    m_flCameraPitch = DOTA_CAMERA_PITCH_ANGLE;
    m_flCurrentZoom = DOTA_CAMERA_DISTANCE;
    m_flTargetZoom = DOTA_CAMERA_DISTANCE;
    m_bLockToHero = false;
    m_bEdgeScrollEnabled = true;
    
    m_vecInputDirection = vec3_origin;
    m_bMouseDragging = false;
    m_nLastMouseX = 0;
    m_nLastMouseY = 0;
    
    m_vecDesiredPosition = vec3_origin;
    m_flSmoothFactor = DOTA_CAMERA_SMOOTH_FACTOR;
    
    m_vecWorldMin = Vector(-8192, -8192, -8192);
    m_vecWorldMax = Vector(8192, 8192, 8192);
}

CDotaCamera::~CDotaCamera()
{
    Shutdown();
}

void CDotaCamera::Init()
{
    if (m_bInitialized)
        return;
        
    // Initialize camera position
    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer)
    {
        m_vecTargetPosition = pPlayer->GetAbsOrigin();
        m_vecDesiredPosition = m_vecTargetPosition;
        m_vecCameraPosition = CalculateIdealCameraPosition();
    }
    
    m_bInitialized = true;
    m_bActive = true;
    
    DevMsg("DOTA Camera: Initialized\n");
}

void CDotaCamera::Shutdown()
{
    if (!m_bInitialized)
        return;
        
    m_bInitialized = false;
    m_bActive = false;
}

void CDotaCamera::Update(float flFrameTime)
{
    if (!m_bActive || !m_bInitialized)
        return;
        
    VPROF_BUDGET("CDotaCamera::Update", VPROF_BUDGETGROUP_GAME);
    
    // Update camera parameters from ConVars
    m_flCameraDistance = dota_camera_distance.GetFloat();
    m_flCameraPitch = dota_camera_pitch.GetFloat();
    m_flSmoothFactor = dota_camera_smooth.GetFloat();
    m_bEdgeScrollEnabled = dota_camera_edge_scroll.GetBool();
    
    // Handle input
    HandleInput(flFrameTime);
    
    // Update camera position
    UpdateCameraPosition(flFrameTime);
    
    // Update camera zoom
    UpdateCameraZoom(flFrameTime);
    
    // Smooth camera movement
    SmoothCameraMovement(flFrameTime);
    
    // Update view angles
    UpdateViewAngles();
}

void CDotaCamera::HandleInput(float flFrameTime)
{
    if (!::input)
        return;
        
    // Handle keyboard input
    HandleKeyboardInput(flFrameTime);
    
    // Handle mouse input
    HandleMouseInput(flFrameTime);
    
    // Handle edge scrolling
    if (m_bEdgeScrollEnabled)
    {
        HandleEdgeScrolling(flFrameTime);
    }
}

void CDotaCamera::HandleKeyboardInput(float flFrameTime)
{
    m_vecInputDirection = vec3_origin;
    
    // Use Source SDK 2010 kbutton system
    if (in_forward.GetPerUser().state & 1)
        m_vecInputDirection.y += 1.0f;
    if (in_back.GetPerUser().state & 1)
        m_vecInputDirection.y -= 1.0f;
    if (in_moveleft.GetPerUser().state & 1)
        m_vecInputDirection.x -= 1.0f;
    if (in_moveright.GetPerUser().state & 1)
        m_vecInputDirection.x += 1.0f;
        
    // Normalize input direction
    if (m_vecInputDirection.LengthSqr() > 0.0f)
    {
        m_vecInputDirection.NormalizeInPlace();
        
        // Apply movement speed
        float flMoveSpeed = dota_camera_move_speed.GetFloat();
        Vector vecMovement = m_vecInputDirection * flMoveSpeed * flFrameTime;
        
        // Move target position
        m_vecDesiredPosition += vecMovement;
    }
}

void CDotaCamera::HandleMouseInput(float flFrameTime)
{
    // Mouse wheel zoom - simplified for SDK 2010
    if (dota_camera_zoom_enabled.GetBool())
    {
        // Mouse wheel will be handled through client mode
        // This is a placeholder for now
    }
    
    // Middle mouse button drag - simplified for SDK 2010
    if (in_attack2.GetPerUser().state & 1)
    {
        if (!m_bMouseDragging)
        {
            m_bMouseDragging = true;
            // Get mouse position would need to be implemented differently in SDK 2010
        }
        else
        {
            // Mouse drag logic would go here
            // For now, just placeholder
        }
    }
    else
    {
        m_bMouseDragging = false;
    }
}

void CDotaCamera::HandleEdgeScrolling(float flFrameTime)
{
    // Edge scrolling would need to be implemented differently in SDK 2010
    // For now, just placeholder
}

void CDotaCamera::UpdateCameraPosition(float flFrameTime)
{
    // Lock to hero if enabled
    if (dota_camera_lock_to_hero.GetBool())
    {
        C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
        if (pPlayer)
        {
            m_vecDesiredPosition = pPlayer->GetAbsOrigin();
        }
    }
    
    // Clamp to world bounds
    ClampCameraToWorldBounds();
    
    // Calculate ideal camera position
    Vector vecIdealPos = CalculateIdealCameraPosition();
    
    // Trace for valid position
    TraceForValidCameraPosition(vecIdealPos);
    
    m_vecCameraPosition = vecIdealPos;
}

void CDotaCamera::UpdateCameraZoom(float flFrameTime)
{
    // Smooth zoom transition
    if (fabs(m_flCurrentZoom - m_flTargetZoom) > 1.0f)
    {
        float flZoomSpeed = 1000.0f * flFrameTime;
        if (m_flCurrentZoom < m_flTargetZoom)
            m_flCurrentZoom = MIN(m_flCurrentZoom + flZoomSpeed, m_flTargetZoom);
        else
            m_flCurrentZoom = MAX(m_flCurrentZoom - flZoomSpeed, m_flTargetZoom);
    }
    else
    {
        m_flCurrentZoom = m_flTargetZoom;
    }
}

void CDotaCamera::SmoothCameraMovement(float flFrameTime)
{
    // Smooth target position transition
    Vector vecDelta = m_vecDesiredPosition - m_vecTargetPosition;
    float flDistance = vecDelta.Length();
    
    if (flDistance > 1.0f)
    {
        float flLerpSpeed = flDistance * m_flSmoothFactor * flFrameTime * 60.0f; // 60 FPS normalization
        flLerpSpeed = clamp(flLerpSpeed, 0.0f, 1.0f);
        
        m_vecTargetPosition = Lerp(flLerpSpeed, m_vecTargetPosition, m_vecDesiredPosition);
    }
    else
    {
        m_vecTargetPosition = m_vecDesiredPosition;
    }
}

Vector CDotaCamera::CalculateIdealCameraPosition()
{
    // DOTA 2 camera: positioned at 25 degrees looking down at the battlefield
    // Camera is positioned behind and above the target at a specific distance
    
    // Convert pitch angle to radians for calculations
    float pitchRadians = DEG2RAD(m_flCameraPitch);
    
    // Calculate the horizontal and vertical distances based on the camera distance and angle
    float horizontalDistance = m_flCurrentZoom * cos(pitchRadians);
    float verticalHeight = m_flCurrentZoom * sin(pitchRadians);
    
    // Position camera behind the target (negative Y direction) and above it
    Vector vecCameraPos = m_vecTargetPosition;
    vecCameraPos.y -= horizontalDistance;  // Move back along Y-axis
    vecCameraPos.z += verticalHeight;      // Move up along Z-axis
    
    return vecCameraPos;
}

void CDotaCamera::TraceForValidCameraPosition(Vector &vecDesiredPos)
{
    // Simple trace to prevent camera from going through walls
    trace_t tr;
    Vector vecStart = m_vecTargetPosition;
    Vector vecEnd = vecDesiredPos;
    
    UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr);
    
    if (tr.fraction < 1.0f)
    {
        // Pull camera back slightly from the collision point
        vecDesiredPos = tr.endpos + tr.plane.normal * 32.0f;
    }
}

void CDotaCamera::ClampCameraToWorldBounds()
{
    // Clamp desired position to world bounds
    m_vecDesiredPosition.x = clamp(m_vecDesiredPosition.x, m_vecWorldMin.x, m_vecWorldMax.x);
    m_vecDesiredPosition.y = clamp(m_vecDesiredPosition.y, m_vecWorldMin.y, m_vecWorldMax.y);
    m_vecDesiredPosition.z = clamp(m_vecDesiredPosition.z, m_vecWorldMin.z, m_vecWorldMax.z);
}

void CDotaCamera::UpdateViewAngles()
{
    // For DOTA camera, maintain consistent view angles that look toward the battlefield
    // The camera should always point toward the target area at the correct angle
    
    // Calculate the direction from camera to target
    Vector vecToTarget = m_vecTargetPosition - m_vecCameraPosition;
    vecToTarget.NormalizeInPlace();
    
    // Convert direction vector to angles, but maintain DOTA's specific perspective
    VectorAngles(vecToTarget, m_angViewAngles);
    
    // Ensure the pitch stays close to DOTA's characteristic angle
    // This prevents the camera from going completely top-down
    m_angViewAngles.x = clamp(m_angViewAngles.x, 15.0f, 35.0f);
    
    // Keep yaw pointing generally forward (can vary slightly based on camera position)
    // Roll should always be zero (no camera tilt)
    m_angViewAngles.z = 0.0f;
}

void CDotaCamera::SetupView(CViewSetup &view)
{
    if (!IsActive())
        return;
        
    // Set camera position and angles
    view.origin = m_vecCameraPosition;
    view.angles = m_angViewAngles;
    
    // Set FOV
    view.fov = 75.0f;
    
    // Set near/far planes
    view.zNear = 4.0f;
    view.zFar = 28400.0f;
}

void CDotaCamera::GetViewOrigin(Vector &vecOrigin)
{
    vecOrigin = m_vecCameraPosition;
}

void CDotaCamera::GetViewAngles(QAngle &angAngles)
{
    angAngles = m_angViewAngles;
}

void CDotaCamera::SetTargetPosition(const Vector &vecTarget)
{
    m_vecDesiredPosition = vecTarget;
}

void CDotaCamera::SetCameraDistance(float flDistance)
{
    m_flTargetZoom = clamp(flDistance, DOTA_CAMERA_ZOOM_MIN, DOTA_CAMERA_ZOOM_MAX);
}

void CDotaCamera::SetCameraPitch(float flPitch)
{
    m_flCameraPitch = flPitch;
}

void CDotaCamera::LockToHero(bool bLock)
{
    m_bLockToHero = bLock;
}

void CDotaCamera::EnableEdgeScrolling(bool bEnable)
{
    m_bEdgeScrollEnabled = bEnable;
}

void CDotaCamera::OnPlayerSpawn()
{
    // Handle player spawn
    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer)
    {
        SetTargetPosition(pPlayer->GetAbsOrigin());
    }
}

void CDotaCamera::OnPlayerDeath()
{
    // Handle player death
    // Keep camera at death location for a moment
}

//----------------------------------------------------------------------------- 
// Global Camera Access Functions
//----------------------------------------------------------------------------- 
CDotaCamera* GetDotaCamera()
{
    return &g_DotaCamera;
}

void InitDotaCamera()
{
    g_DotaCamera.Init();
}

void ShutdownDotaCamera()
{
    g_DotaCamera.Shutdown();
}

void UpdateDotaCamera(float flFrameTime)
{
    g_DotaCamera.Update(flFrameTime);
}

void SetupDotaCameraView(CViewSetup &view)
{
    g_DotaCamera.SetupView(view);
}

//----------------------------------------------------------------------------- 
// Console Commands
//----------------------------------------------------------------------------- 
CON_COMMAND(dota_camera_reset, "Reset DOTA camera to default position")
{
    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    if (pPlayer)
    {
        g_DotaCamera.SetTargetPosition(pPlayer->GetAbsOrigin());
        g_DotaCamera.SetCameraDistance(DOTA_CAMERA_DISTANCE);
    }
}

CON_COMMAND(dota_camera_lock_hero, "Toggle camera lock to hero")
{
    bool bCurrentLock = dota_camera_lock_to_hero.GetBool();
    dota_camera_lock_to_hero.SetValue(!bCurrentLock);
    
    if (!bCurrentLock)
        Msg("Camera locked to hero\n");
    else
        Msg("Camera unlocked from hero\n");
}

CON_COMMAND(dota_camera_info, "Display camera information")
{
    Vector vecCameraPos = g_DotaCamera.GetCameraPosition();
    Vector vecTargetPos = g_DotaCamera.GetTargetPosition();
    
    Msg("Camera Position: %.2f %.2f %.2f\n", vecCameraPos.x, vecCameraPos.y, vecCameraPos.z);
    Msg("Target Position: %.2f %.2f %.2f\n", vecTargetPos.x, vecTargetPos.y, vecTargetPos.z);
    Msg("Camera Distance: %.2f\n", dota_camera_distance.GetFloat());
    Msg("Camera Active: %s\n", g_DotaCamera.IsActive() ? "Yes" : "No");
} 