//========= Copyright © 2024, DOTA 2 Beta Mod. All rights reserved. ============//
//
// Purpose: DOTA 2 input system implementation
//
//=============================================================================//

#include "cbase.h"
#include "input.h"
#include "dota_camera.h"
#include "cdll_client_int.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Controls.h"
#include "mathlib/mathlib.h"
#include "convar.h"
#include "c_baseplayer.h"
#include "engine/IEngineSound.h"
#include "tier0/icommandline.h"
#include "ienginevgui.h"
#include "iclientmode.h"
#include "usercmd.h"
#include "kbutton.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ConVars for DOTA input
static ConVar dota_camera_edge_pan("dota_camera_edge_pan", "1", FCVAR_ARCHIVE, "Enable edge panning with mouse cursor");
static ConVar dota_camera_edge_threshold("dota_camera_edge_threshold", "20", FCVAR_ARCHIVE, "Pixel threshold for edge panning");
static ConVar dota_camera_edge_speed("dota_camera_edge_speed", "1200", FCVAR_ARCHIVE, "Speed of edge panning");
static ConVar dota_camera_drag_speed("dota_camera_drag_speed", "1.0", FCVAR_ARCHIVE, "Speed of middle mouse drag");
static ConVar dota_camera_acceleration("dota_camera_acceleration", "2.0", FCVAR_ARCHIVE, "Camera acceleration multiplier");
static ConVar dota_camera_smooth_time("dota_camera_smooth_time", "0.1", FCVAR_ARCHIVE, "Camera smoothing time");

//-----------------------------------------------------------------------------
// Purpose: DOTA-specific input handler
//-----------------------------------------------------------------------------
class CDotaInput : public CInput
{
public:
    typedef CInput BaseClass;
    
    CDotaInput();
    virtual ~CDotaInput();

    // Override input processing
    virtual void CreateMove(int sequence_number, float input_sample_frametime, bool active);
    virtual void ExtraMouseSample(float frametime, bool active);
    virtual int KeyEvent(int down, ButtonCode_t keynum, const char *pszCurrentBinding);
    virtual void AccumulateMouse(int nSlot);
    virtual void LevelInit(void);
    
    // Override mouse methods to prevent FPS-style mouse capture
    virtual void ActivateMouse(void);
    virtual void DeactivateMouse(void);
    virtual void GetFullscreenMousePos(int *mx, int *my, int *unclampedx = NULL, int *unclampedy = NULL);
    virtual void SetFullscreenMousePos(int mx, int my);
    virtual void ResetMouse(void);
    
    // Override view/mouse methods to prevent FPS-style angle changes
    virtual void ApplyMouse(int nSlot, QAngle& viewangles, CUserCmd *cmd, float mouse_x, float mouse_y);
    virtual void MouseMove(int nSlot, CUserCmd *cmd);

    // DOTA-specific input processing
    void ProcessDotaInput(float frametime);
    void HandleCameraInput(float frametime);
    void HandleEdgePanning(float frametime);
    void HandleMouseDrag(float frametime);
    void HandleKeyboardCameraMovement(float frametime);
    void HandleMouseWheelZoom(float frametime);

private:
    // Mouse tracking
    bool m_bMiddleMousePressed;
    bool m_bRightMousePressed;
    bool m_bLeftMousePressed;
    int m_nLastMouseX;
    int m_nLastMouseY;
    Vector2D m_vMouseDelta;
    Vector2D m_vCameraVelocity;
    float m_flLastInputTime;
    
    // Edge panning
    bool m_bEdgePanningActive;
    Vector2D m_vEdgePanDirection;
    
    // Camera smoothing
    Vector2D m_vTargetCameraPosition;
    Vector2D m_vCurrentCameraPosition;
    float m_flCameraSmoothTime;
    
    // Input state
    bool m_bCameraLocked;
    bool m_bInCameraMode;
    
    // Helper functions
    void UpdateMousePosition();
    void ResetCameraInput();
    bool IsMouseInEdgeZone(int &edgeX, int &edgeY);
    void SmoothCameraMovement(float frametime);
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CDotaInput::CDotaInput() : CInput()
{
    m_bMiddleMousePressed = false;
    m_bRightMousePressed = false;
    m_bLeftMousePressed = false;
    m_nLastMouseX = 0;
    m_nLastMouseY = 0;
    m_vMouseDelta.Init();
    m_vCameraVelocity.Init();
    m_flLastInputTime = 0.0f;
    
    m_bEdgePanningActive = false;
    m_vEdgePanDirection.Init();
    
    m_vTargetCameraPosition.Init();
    m_vCurrentCameraPosition.Init();
    m_flCameraSmoothTime = 0.0f;
    
    m_bCameraLocked = false;
    m_bInCameraMode = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDotaInput::~CDotaInput()
{
}

//-----------------------------------------------------------------------------
// Purpose: Level initialization
//-----------------------------------------------------------------------------
void CDotaInput::LevelInit(void)
{
    BaseClass::LevelInit();
    ResetCameraInput();
}

//-----------------------------------------------------------------------------
// Purpose: Main input processing
//-----------------------------------------------------------------------------
void CDotaInput::CreateMove(int sequence_number, float input_sample_frametime, bool active)
{
    BaseClass::CreateMove(sequence_number, input_sample_frametime, active);
    
    if (active)
    {
        ProcessDotaInput(input_sample_frametime);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Extra mouse sample processing
//-----------------------------------------------------------------------------
void CDotaInput::ExtraMouseSample(float frametime, bool active)
{
    BaseClass::ExtraMouseSample(frametime, active);
    
    if (active)
    {
        ProcessDotaInput(frametime);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Key event handling
//-----------------------------------------------------------------------------
int CDotaInput::KeyEvent(int down, ButtonCode_t keynum, const char *pszCurrentBinding)
{
    // Handle DOTA-specific key events
    if (down)
    {
        switch (keynum)
        {
            case MOUSE_MIDDLE:
                m_bMiddleMousePressed = true;
                UpdateMousePosition();
                return 1;
                
            case MOUSE_RIGHT:
                m_bRightMousePressed = true;
                UpdateMousePosition();
                break;
                
            case MOUSE_LEFT:
                m_bLeftMousePressed = true;
                UpdateMousePosition();
                break;
                
            case MOUSE_WHEEL_UP:
                HandleMouseWheelZoom(0.1f);
                return 1;
                
            case MOUSE_WHEEL_DOWN:
                HandleMouseWheelZoom(-0.1f);
                return 1;
        }
    }
    else
    {
        switch (keynum)
        {
            case MOUSE_MIDDLE:
                m_bMiddleMousePressed = false;
                return 1;
                
            case MOUSE_RIGHT:
                m_bRightMousePressed = false;
                break;
                
            case MOUSE_LEFT:
                m_bLeftMousePressed = false;
                break;
        }
    }
    
    return BaseClass::KeyEvent(down, keynum, pszCurrentBinding);
}

//-----------------------------------------------------------------------------
// Purpose: Mouse accumulation
//-----------------------------------------------------------------------------
void CDotaInput::AccumulateMouse(int nSlot)
{
    BaseClass::AccumulateMouse(nSlot);
    UpdateMousePosition();
}

//-----------------------------------------------------------------------------
// Purpose: Process DOTA-specific input
//-----------------------------------------------------------------------------
void CDotaInput::ProcessDotaInput(float frametime)
{
    if (!GetDotaCamera())
        return;
    
    m_flLastInputTime = frametime;
    
    // Handle different types of camera input
    HandleCameraInput(frametime);
    HandleEdgePanning(frametime);
    HandleMouseDrag(frametime);
    HandleKeyboardCameraMovement(frametime);
    
    // Apply smooth camera movement
    SmoothCameraMovement(frametime);
}

//-----------------------------------------------------------------------------
// Purpose: Handle camera input
//-----------------------------------------------------------------------------
void CDotaInput::HandleCameraInput(float frametime)
{
    // For DOTA, we want the cursor to always be visible and free-moving
    // Only disable camera input when game UI is visible (menus, etc.)
    m_bInCameraMode = !enginevgui->IsGameUIVisible();
    
    // For RTS gameplay, we want the mouse cursor to be visible and free-moving
    // Don't lock or capture the mouse like in FPS games
    
    if (!m_bInCameraMode)
    {
        ResetCameraInput();
        return;
    }
    
    // Update camera velocity based on input
    m_vCameraVelocity *= (1.0f - frametime * 5.0f); // Damping
}

//-----------------------------------------------------------------------------
// Purpose: Handle edge panning
//-----------------------------------------------------------------------------
void CDotaInput::HandleEdgePanning(float frametime)
{
    if (!dota_camera_edge_pan.GetBool() || !m_bInCameraMode)
    {
        m_bEdgePanningActive = false;
        return;
    }
    
    int edgeX, edgeY;
    bool inEdgeZone = IsMouseInEdgeZone(edgeX, edgeY);
    
    if (inEdgeZone)
    {
        m_bEdgePanningActive = true;
        m_vEdgePanDirection.x = edgeX;
        m_vEdgePanDirection.y = edgeY;
        
        // Calculate edge pan velocity
        Vector2D panVelocity = m_vEdgePanDirection * dota_camera_edge_speed.GetFloat() * frametime;
        
        // Apply to camera
        if (GetDotaCamera())
        {
            Vector currentPos = GetDotaCamera()->GetCameraPosition();
            Vector newPos = currentPos + Vector(panVelocity.x, panVelocity.y, 0);
            GetDotaCamera()->SetTargetPosition(newPos);
        }
    }
    else
    {
        m_bEdgePanningActive = false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Handle middle mouse drag
//-----------------------------------------------------------------------------
void CDotaInput::HandleMouseDrag(float frametime)
{
    if (!m_bMiddleMousePressed || !m_bInCameraMode)
        return;
    
    // Calculate mouse delta
    int currentX, currentY;
    GetMousePos(currentX, currentY);
    
    Vector2D dragDelta;
    dragDelta.x = (currentX - m_nLastMouseX) * dota_camera_drag_speed.GetFloat();
    dragDelta.y = (currentY - m_nLastMouseY) * dota_camera_drag_speed.GetFloat();
    
    // Apply drag movement to camera
    if (GetDotaCamera() && (dragDelta.x != 0 || dragDelta.y != 0))
    {
        Vector currentPos = GetDotaCamera()->GetCameraPosition();
        Vector newPos = currentPos + Vector(-dragDelta.x, dragDelta.y, 0); // Inverted for natural drag feel
        GetDotaCamera()->SetTargetPosition(newPos);
    }
    
    m_nLastMouseX = currentX;
    m_nLastMouseY = currentY;
}

//-----------------------------------------------------------------------------
// Purpose: Handle WASD keyboard camera movement
//-----------------------------------------------------------------------------
void CDotaInput::HandleKeyboardCameraMovement(float frametime)
{
    if (!m_bInCameraMode)
        return;
    
    Vector2D keyboardMove(0, 0);
    float moveSpeed = 800.0f * frametime;
    
    // Check WASD keys using proper input system
    kbutton_t *pForward = FindKey("in_forward");
    kbutton_t *pBack = FindKey("in_back"); 
    kbutton_t *pMoveLeft = FindKey("in_moveleft");
    kbutton_t *pMoveRight = FindKey("in_moveright");
    
    if (pForward && (pForward->GetPerUser().state & 1))
        keyboardMove.y += moveSpeed;
    if (pBack && (pBack->GetPerUser().state & 1))
        keyboardMove.y -= moveSpeed;
    if (pMoveLeft && (pMoveLeft->GetPerUser().state & 1))
        keyboardMove.x -= moveSpeed;
    if (pMoveRight && (pMoveRight->GetPerUser().state & 1))
        keyboardMove.x += moveSpeed;
    
    // Apply acceleration
    if (keyboardMove.Length() > 0)
    {
        keyboardMove *= dota_camera_acceleration.GetFloat();
        
        if (GetDotaCamera())
        {
            Vector currentPos = GetDotaCamera()->GetCameraPosition();
            Vector newPos = currentPos + Vector(keyboardMove.x, keyboardMove.y, 0);
            GetDotaCamera()->SetTargetPosition(newPos);
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse wheel zoom
//-----------------------------------------------------------------------------
void CDotaInput::HandleMouseWheelZoom(float frametime)
{
    if (!m_bInCameraMode)
        return;
    
    if (GetDotaCamera())
    {
        // Zoom by adjusting camera distance
        float zoomAmount = frametime > 0 ? -100.0f : 100.0f; // Negative for zoom in, positive for zoom out
        // Note: This is a simplified implementation - in a real implementation,
        // we would get the current distance and adjust it
        GetDotaCamera()->SetCameraDistance(1134.0f + zoomAmount);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Update mouse position tracking
//-----------------------------------------------------------------------------
void CDotaInput::UpdateMousePosition()
{
    GetMousePos(m_nLastMouseX, m_nLastMouseY);
}

//-----------------------------------------------------------------------------
// Purpose: Reset camera input state
//-----------------------------------------------------------------------------
void CDotaInput::ResetCameraInput()
{
    m_bMiddleMousePressed = false;
    m_bRightMousePressed = false;
    m_bLeftMousePressed = false;
    m_vMouseDelta.Init();
    m_vCameraVelocity.Init();
    m_bEdgePanningActive = false;
    m_vEdgePanDirection.Init();
}

//-----------------------------------------------------------------------------
// Purpose: Check if mouse is in edge zone for panning
//-----------------------------------------------------------------------------
bool CDotaInput::IsMouseInEdgeZone(int &edgeX, int &edgeY)
{
    int screenW, screenH;
    engine->GetScreenSize(screenW, screenH);
    
    int mouseX, mouseY;
    GetMousePos(mouseX, mouseY);
    
    int threshold = dota_camera_edge_threshold.GetInt();
    
    edgeX = 0;
    edgeY = 0;
    
    // Check horizontal edges
    if (mouseX < threshold)
        edgeX = -1;
    else if (mouseX > screenW - threshold)
        edgeX = 1;
    
    // Check vertical edges
    if (mouseY < threshold)
        edgeY = 1;
    else if (mouseY > screenH - threshold)
        edgeY = -1;
    
    return (edgeX != 0 || edgeY != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Apply smooth camera movement
//-----------------------------------------------------------------------------
void CDotaInput::SmoothCameraMovement(float frametime)
{
    if (!GetDotaCamera())
        return;
    
    // Apply smoothing to camera movement
    float smoothTime = dota_camera_smooth_time.GetFloat();
    if (smoothTime > 0.0f)
    {
        // Simple smoothing implementation
        Vector currentPos = GetDotaCamera()->GetCameraPosition();
        // Additional smoothing logic can be added here
    }
}

//-----------------------------------------------------------------------------
// Purpose: Override mouse activation to prevent FPS-style capture
//-----------------------------------------------------------------------------
void CDotaInput::ActivateMouse(void)
{
    // For RTS games, we don't want to capture/lock the mouse cursor
    // Just enable mouse input without locking cursor to center
    // Don't call BaseClass::ActivateMouse() as it locks the cursor
}

//-----------------------------------------------------------------------------
// Purpose: Override mouse deactivation
//-----------------------------------------------------------------------------
void CDotaInput::DeactivateMouse(void)
{
    // For RTS games, just disable mouse input
    BaseClass::DeactivateMouse();
}

//-----------------------------------------------------------------------------
// Purpose: Get mouse position without centering restrictions
//-----------------------------------------------------------------------------
void CDotaInput::GetFullscreenMousePos(int *mx, int *my, int *unclampedx, int *unclampedy)
{
    // Get actual mouse position without FPS-style center locking
    int x, y;
    vgui::input()->GetCursorPos(x, y);
    
    if (mx) *mx = x;
    if (my) *my = y;
    if (unclampedx) *unclampedx = x;
    if (unclampedy) *unclampedy = y;
}

//-----------------------------------------------------------------------------
// Purpose: Set mouse position without restrictions
//-----------------------------------------------------------------------------
void CDotaInput::SetFullscreenMousePos(int mx, int my)
{
    // Allow setting mouse position for RTS gameplay
    vgui::input()->SetCursorPos(mx, my);
}

//-----------------------------------------------------------------------------
// Purpose: Reset mouse without centering
//-----------------------------------------------------------------------------
void CDotaInput::ResetMouse(void)
{
    // For RTS games, don't reset mouse to center like FPS games
    // Just clear any accumulated mouse movement
    m_vMouseDelta.Init();
}

//-----------------------------------------------------------------------------
// Purpose: Override ApplyMouse to prevent view angle changes
//-----------------------------------------------------------------------------
void CDotaInput::ApplyMouse(int nSlot, QAngle& viewangles, CUserCmd *cmd, float mouse_x, float mouse_y)
{
    // For RTS games, don't apply mouse movement to view angles
    // The view angles should remain fixed for top-down camera
    // Mouse movement will be handled separately for camera position control
    
    // Store mouse delta for our custom camera movement
    m_vMouseDelta.x = mouse_x;
    m_vMouseDelta.y = mouse_y;
    
    // Don't call BaseClass::ApplyMouse() as it would change view angles
}

//-----------------------------------------------------------------------------
// Purpose: Override MouseMove to prevent view angle changes
//-----------------------------------------------------------------------------
void CDotaInput::MouseMove(int nSlot, CUserCmd *cmd)
{
    // For RTS games, don't use mouse movement for view angle changes
    // Keep the existing view angles fixed for top-down perspective
    
    // Don't call BaseClass::MouseMove() as it would apply mouse to view angles
    // Instead, our ProcessDotaInput() will handle mouse input for camera position
}

//-----------------------------------------------------------------------------
// Global input instance
//-----------------------------------------------------------------------------
static CDotaInput g_DotaInput;
IInput* input = (IInput*)&g_DotaInput; 