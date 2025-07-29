//====== Copyright © 1996-2011, Valve Corporation, All rights reserved. =======
//
// Purpose: DOTA Player - Server Implementation
//
//=============================================================================

#include "cbase.h"
#include "dota_player.h"
#include "baseanimating.h"
#include "entitylist.h"
#include "eiface.h"
#include "dota_gamerules.h"
#include "dota_team.h"
#include "dota_playerresource.h"
#include "team.h"
#include "tier0/dbg.h"
#include "engine/iserverplugin.h"

// Constants
#define TEAM_SPECTATOR 1

// External globals
extern CDOTAGameRules *DOTAGameRules();
extern CTeam *GetGlobalTeam( int iIndex );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// Data Tables
//=============================================================================

// Server class implementation with networking
IMPLEMENT_SERVERCLASS_ST( CDOTAPlayer, DT_DOTAPlayer )
	// Core player identification
	SendPropInt( SENDINFO( m_nPlayerID ), 8, SPROP_UNSIGNED ),
	
	// Player flags and state
	SendPropBool( SENDINFO( m_bDOTAPlayerFlag1 ) ),
	SendPropBool( SENDINFO( m_bDOTAPlayerFlag2 ) ),
	SendPropBool( SENDINFO( m_bDOTAPlayerFlag3 ) ),
	SendPropBool( SENDINFO( m_bDOTANetworkFlag ) ),
	SendPropBool( SENDINFO( m_bWantsRandomHero ) ),
	
	// Player values and stats
	SendPropFloat( SENDINFO( m_flDOTAPlayerValue1 ) ),
	SendPropInt( SENDINFO( m_nDOTAPlayerValue1 ) ),
	SendPropInt( SENDINFO( m_nDOTAPlayerValue2 ) ),
	SendPropInt( SENDINFO( m_nPlayerStat1 ) ),
	SendPropInt( SENDINFO( m_nPlayerStat2 ) ),
	SendPropInt( SENDINFO( m_nPlayerStat3 ) ),
	SendPropInt( SENDINFO( m_nPlayerStat4 ) ),
	SendPropInt( SENDINFO( m_iShopViewMode ) ),
	
	// Entity handles
	SendPropEHandle( SENDINFO( m_hAssignedHero ) ),
	SendPropEHandle( SENDINFO( m_hDOTAPlayerEntity ) ),
	
	// Timing information
	SendPropFloat( SENDINFO( m_flLastActivityTime ) ),
	SendPropFloat( SENDINFO( m_flLastOrderTime ) ),
	SendPropFloat( SENDINFO( m_flIdleTime ) ),
	
	// Additional DOTA values
	SendPropInt( SENDINFO( m_nDOTAValue1 ) ),
	SendPropInt( SENDINFO( m_nDOTAValue2 ) ),
	SendPropInt( SENDINFO( m_nDOTAValue3 ) ),
	SendPropInt( SENDINFO( m_iPickerEntity ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CDOTAPlayer )
	// TODO: Add save/restore fields when needed
END_DATADESC()

// TODO: Re-enable when entity system is ready
LINK_ENTITY_TO_CLASS( player, CDOTAPlayer );

//=============================================================================
// CDOTAPlayer Implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor (based on decompiled code)
//-----------------------------------------------------------------------------
CDOTAPlayer::CDOTAPlayer() : CBaseMultiplayerPlayer()
{
	// Initialize network variables based on decompiled constructor
	// 0x1338-0x1348 region
	m_bDOTAPlayerFlag1 = false;			// 0x1338
	m_bDOTAPlayerFlag2 = false;			// 0x1339
	m_flDOTAPlayerValue1 = 0.5f;		// 0x133c (initialized to 0.5)
	m_nDOTAPlayerValue1 = 0;			// 0x1340
	m_bDOTAPlayerFlag3 = false;			// 0x1344
	m_nDOTAPlayerValue2 = 0;			// 0x134c

	// 0x12dc-0x12f4 region - various player stats/values
	m_nPlayerStat1 = 0;					// 0x12dc
	m_nPlayerStat2 = 0;					// 0x12e0
	m_nPlayerStat3 = 0;					// 0x12e4
	m_nPlayerStat4 = 0xf;				// 0x12e8 (initialized to 15)
	m_nPlayerStat5 = 0;					// 0x12ec
	m_nPlayerStat6 = 0;					// 0x12f0
	m_nPlayerStat7 = 0;					// 0x12f4

	// 0x136c-0x1378 region - network handle and values  
	m_hDOTAPlayerEntity.Set( NULL );	// 0x136c area
	m_bDOTANetworkFlag = false;			// 0x1378

	// 0x1380-0x1394 region - additional player data
	m_nDOTAValue1 = 0;					// 0x1380
	m_nDOTAValue2 = 0;					// 0x1384
	m_nDOTAValue3 = 0;					// 0x138c
	m_nDOTAValue4 = 0;					// 0x1390
	m_nDOTAValue5 = 0;					// 0x1394

	// Other network variables
	m_nSpecialValue1 = 0;				// 0x13a8
	m_nSpecialValue2 = -1;				// 0x13ac (initialized to -1)
	m_nSpecialValue3 = 0;				// 0x13d4

	// Entity handles and references
	m_hPlayerEntity.Set( NULL );		// 0x137c area

	// Additional data members
	m_nDataValue1 = 0;					// 0x13c0
	m_nDataValue2 = 0;					// 0x13c4
	m_nDataValue3 = 0;					// 0x13c8
	m_nPlayerID = -1;					// 0x13cc (initialized to -1)
	m_nDataValue4 = 0;					// 0x13d8
	m_nDataValue5 = 0;					// 0x13dc

	// Additional member variables
	m_nExtraValue1 = 0;					// 0x1490
	m_nExtraValue2 = 0;					// 0x1494

	// Additional timing members
	m_flLastActivityTime = 0.0f;		// 0x13bc - for idle time calculation
	m_flReconnectTime = 0.0f;			// 0x13c4 - reconnection time
	
	// Initialize DOTA-specific members
	m_iPickerEntity = -1;				// 0x13a4 - picker entity index
	m_hAssignedHero = NULL;				// 0x13a8 - assigned hero handle
	m_flLastOrderTime = 0.0f;			// 0x13bc - last order time
	m_flIdleTime = 0.0f;				// 0x13c0 - idle time tracker
	m_bWantsRandomHero = false;			// 0x13c8 - wants random hero flag
	m_iShopViewMode = 0;				// 0x12f4 - shop view mode

	// Use client-side animation (matches decompiled code)
	// TODO: Fix this call when animation system is ready
	// UseClientSideAnimation();

	// TODO: When networking system is implemented, add the complex network change tracking
	// The decompiled code has extensive network variable registration logic here
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDOTAPlayer::~CDOTAPlayer()
{
	// Cleanup will be added here as needed
}

//-----------------------------------------------------------------------------
// Purpose: Precache resources
//-----------------------------------------------------------------------------
void CDOTAPlayer::Precache( void )
{
	BaseClass::Precache();
	
	PrecacheModel ( "models/player.mdl" );

	// Precache DOTA player-specific resources
	// TODO: Precache hero models when hero system is available
	// TODO: Precache player UI sounds
	// TODO: Precache shop/inventory sounds
	// TODO: Precache team-specific sounds and effects
	
	// Initialize inventory system
	// TODO: Precache inventory item models and effects when item system ready
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the player
//-----------------------------------------------------------------------------
void CDOTAPlayer::Spawn( void )
{
	SetModel("models/player.mdl");
	BaseClass::Spawn();
	
	// Initialize DOTA-specific spawn logic
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( pGameRules )
	{
		// Update activity time to current game time
		m_flLastActivityTime = pGameRules->GetGameTime();
		m_flIdleTime = m_flLastActivityTime;
		m_flLastOrderTime = m_flLastActivityTime;
	}
	
	// Update player ID from Steam ID if needed
	if ( GetPlayerID() == -1 )
	{
		UpdatePlayerIDFromSteamID();
	}
	
	// Update player resource with spawn info
	CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
	if ( pPlayerResource && GetPlayerID() != -1 )
	{
		// TODO: Set connection state when SetConnectionState is available
		 pPlayerResource->SetConnectionState( GetPlayerID(), 2 ); // DOTA_CONNECTION_STATE_CONNECTED
		
		// Update team slot if we have a team
		// TODO: Fix GetTeamNumber() when base class is properly included
		// int iTeam = GetTeamNumber();
		// if ( iTeam > 0 )
		// {
		//     pPlayerResource->UpdateTeamSlot( GetPlayerID(), iTeam );
		// }
	}
	
	// Initialize hero selection state if in hero selection phase
	if ( pGameRules && pGameRules->GetGameState() == DOTA_GAMERULES_STATE_HERO_SELECTION )
	{
		// Player is ready for hero selection
		m_bWantsRandomHero = false;
		m_hAssignedHero = NULL;
	}
	
	// Set initial shop view mode
	m_iShopViewMode = 0; // Default shop view
}

//-----------------------------------------------------------------------------
// Purpose: Think while dead
//-----------------------------------------------------------------------------
void CDOTAPlayer::PlayerDeathThink( void )
{
	BaseClass::PlayerDeathThink();
	
	// DOTA-specific death thinking
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return;
		
	// Only process death logic during active game
	if ( pGameRules->GetGameState() != DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
		return;
	
	// Update death time tracking (matches decompiled death handling)
	float flGameTime = pGameRules->GetGameTime();
	
	// TODO: Handle respawn timer calculation based on level and game time
	// TODO: Handle buyback availability and costs
	// TODO: Handle death camera target selection
	// TODO: Handle spectating teammates while dead
	
	// Update idle time even while dead (prevent AFK while watching)
	m_flLastActivityTime = flGameTime;
	
	// TODO: Check for respawn conditions:
	// - Natural respawn timer expired
	// - Buyback used
	// - Game state changes (aegis, reincarnation, etc.)
}

//-----------------------------------------------------------------------------
// Purpose: Pre-think processing
//-----------------------------------------------------------------------------
void CDOTAPlayer::PreThink( void )
{
	BaseClass::PreThink();
	
	// DOTA-specific pre-think logic
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return;
	
	float flGameTime = pGameRules->GetGameTime();
	
	// Update activity tracking for idle detection
	// TODO: Track actual input activity when input system is available
	// For now, assume any PreThink call indicates activity
	if ( !IsAlive() || pGameRules->GetGameState() == DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
	{
		// Only update activity during game or when dead (spectating counts as activity)
		m_flLastActivityTime = flGameTime;
	}
	
	// Check for AFK status and handle kick logic
	if ( IsAFK() )
	{
		// TODO: Handle AFK player kick when admin system is available
		// TODO: Show AFK warning to player
	}
	
	// Check fountain idle status for potential abandon detection
	if ( IsFountainIdle( false ) )
	{
		// TODO: Handle fountain idle warnings and potential abandonment
		// TODO: Update player resource idle state
	}
	
	// Update hero-related pre-think logic
	if ( m_hAssignedHero.Get() )
	{
		// TODO: Update hero activity time when hero system available
		// Hero activity should also update player activity
	}
}

//-----------------------------------------------------------------------------
// Purpose: Think processing
//-----------------------------------------------------------------------------
void CDOTAPlayer::Think( void )
{
	// TODO: Call BaseClass::Think() when base class supports it
	// BaseClass::Think();
	
	// DOTA-specific think logic
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return;
	
	// Update order time tracking for idle detection
	float flGameTime = pGameRules->GetGameTime();
	
	// Check game state and update player accordingly
	DOTA_GameState_t gameState = pGameRules->GetGameState();
	
	// Handle hero selection phase logic
	if ( gameState == DOTA_GAMERULES_STATE_HERO_SELECTION )
	{
		// Update hero selection activity time
		SetLastOrderTime( flGameTime, true );
		
		// TODO: Handle hero selection UI updates
		// TODO: Check for selection timeouts
	}
	
	// Handle active game logic
	if ( gameState == DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
	{
		// Monitor for idle behavior in fountain
		if ( IsFountainIdle( false ) )
		{
			// TODO: Increment idle warnings
			// TODO: Trigger abandon detection if idle too long
		}
		
		// Update assigned hero if we have one
		if ( m_hAssignedHero.Get() )
		{
			// TODO: Sync hero position for idle detection
			// TODO: Update hero-based activity tracking
		}
	}
	
	// Handle inventory updates
	// TODO: Process inventory changes when item system ready
	// m_Inventory.Think();
	
	// TODO: Set next think time when SetNextThink is available
	// SetNextThink( gpGlobals->curtime + 0.1f ); // Think every 100ms
}

//-----------------------------------------------------------------------------
// Purpose: Post-think processing  
//-----------------------------------------------------------------------------
void CDOTAPlayer::PostThink( void )
{
	BaseClass::PostThink();
	
	// DOTA-specific post-think logic
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return;
		
	// Update player resource with current player state
	CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
	if ( pPlayerResource && GetPlayerID() != -1 )
	{
		// Update connection and activity state
		// TODO: Update last seen time when networking ready
		// pPlayerResource->SetLastSeenTime( GetPlayerID(), pGameRules->GetGameTime() );
		
		// Update AFK status
		bool bIsAFK = IsAFK();
		// TODO: Update AFK state in player resource when available
		// pPlayerResource->SetAFKState( GetPlayerID(), bIsAFK );
		
		// Update idle status during game
		if ( pGameRules->GetGameState() == DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
		{
			bool bIsFountainIdle = IsFountainIdle( false );
			// TODO: Update fountain idle state when available
			//pPlayerResource->SetFountainIdleState( GetPlayerID(), bIsFountainIdle );
		}
	}
	
	// Handle hero selection timeout during hero selection phase
	if ( pGameRules->GetGameState() == DOTA_GAMERULES_STATE_HERO_SELECTION )
	{
		// TODO: Check if hero selection time is running out
		// TODO: Auto-select random hero if time expires and no selection made
		if ( !HasSelectedHero() && WantsRandomHero() )
		{
			// TODO: Trigger random hero selection when hero system ready
			// MakeRandomHeroSelection( false, -1 );
		}
	}
	
	// Update shop view mode and inventory state
	if ( m_iShopViewMode != 0 )
	{
		// TODO: Update shop UI state when shop system is available
		// TODO: Handle shop item purchases and recommendations
	}
	
	// Handle inventory management
	// TODO: Process pending inventory updates when item system ready
	// m_Inventory.PostThink();
}

//=============================================================================
// CDOTAPlayerInventory Implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor (based on decompiled code reference)
//-----------------------------------------------------------------------------
CDOTAPlayerInventory::CDOTAPlayerInventory()
{
	// Initialize inventory data
	memset( m_InventoryData, 0, sizeof(m_InventoryData) );
	
	// TODO: Initialize inventory slots, items, etc. when item system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDOTAPlayerInventory::~CDOTAPlayerInventory()
{
	// TODO: Cleanup inventory items when item system is ready
}

//=============================================================================
// Additional CDOTAPlayer Functions (from decompiled code)
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Handle team changes (from decompiled ChangeTeam)
//-----------------------------------------------------------------------------
void CDOTAPlayer::ChangeTeam( int iTeamNum )
{
	// Get the team and validate it's not full (matches decompiled team validation)
	CTeam *pTeam = GetGlobalTeam( iTeamNum );
	if ( pTeam )
	{
		// Check if team is full (matches decompiled team validation)
		CDOTATeam *pDOTATeam = dynamic_cast<CDOTATeam*>( pTeam );
		if ( pDOTATeam && pDOTATeam->IsTeamFull() )
		{
			return; // Team is full, cannot join
		}

		// Call base class team change (matches decompiled base call)
		BaseClass::ChangeTeam( iTeamNum );

		// Update player ID from Steam ID if needed (matches decompiled logic)
		if ( GetPlayerID() == -1 )
		{
			UpdatePlayerIDFromSteamID();
		}

		// Update team slot in player resource (matches decompiled PlayerResource calls)
		if ( GetPlayerID() != -1 )
		{
			CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
			if ( pPlayerResource )
			{
				pPlayerResource->UpdateTeamSlot( GetPlayerID(), iTeamNum );
				
				// Set fully joined if conditions are met (matches decompiled logic)
				if ( m_nDataValue3 != 0 && GetTeamNumber() > 1 )
				{
					pPlayerResource->SetFullyJoinedServer( GetPlayerID(), true );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get player idle time (from decompiled GetIdleTime)
//-----------------------------------------------------------------------------
float CDOTAPlayer::GetIdleTime( void )
{
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return 0.0f;

	// Only calculate idle time during active game state (matches decompiled state check)
	if ( pGameRules->GetGameState() != DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
		return 0.0f;

	// Return current game time minus last activity time (matches decompiled calculation)
	float flGameTime = pGameRules->GetGameTime();
	return flGameTime - m_flLastActivityTime;
}

//-----------------------------------------------------------------------------
// Purpose: Handle player reconnection (from decompiled OnReconnect)
//-----------------------------------------------------------------------------
void CDOTAPlayer::OnReconnect( void )
{
	// Set reconnect time to current game time (matches decompiled logic)
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( pGameRules )
	{
		m_flReconnectTime = pGameRules->GetGameTime();
	}
	else
	{
		m_flReconnectTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static function to create player entity (from decompiled CreatePlayer)
//-----------------------------------------------------------------------------

// Static global to track player edict index (matches decompiled PTR_s_nPlayerEdictIndex_013fb9dc)
static int s_nPlayerEdictIndex = -1;

//-----------------------------------------------------------------------------
// Purpose: Static function to get player by player ID (from decompiled GetPlayerByPlayerID)
//-----------------------------------------------------------------------------
CDOTAPlayer* CDOTAPlayer::GetPlayerByPlayerID( int iPlayerID )
{
	// TODO: Implement proper player iteration when entity system is ready
	// For now, return NULL as placeholder
	/*
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CDOTAPlayer *pPlayer = (CDOTAPlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->IsConnected() && pPlayer->GetPlayerID() == iPlayerID )
		{
			return pPlayer;
		}
	}
	*/
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Check if player is idle in fountain (from decompiled IsFountainIdle)
//-----------------------------------------------------------------------------
bool CDOTAPlayer::IsFountainIdle( bool bLenient )
{
	// Get game rules (matches decompiled PTR__g_pGameRules_013fb0c4 access)
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return false;
	
	if ( pGameRules->GetGameState() != DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
		return false;
	
	// Get current game time (matches decompiled *(float *)(this_00 + 0x10b4))
	float flGameTime = pGameRules->GetGameTime();
	
	// Check against reconnect idle buffer time (matches decompiled check)
	// fVar4 - *(float *)(this + 0x13c4) compared to dota_reconnect_idle_buffer_time
	float flReconnectIdleBufferTime = 90.0f; // TODO: Get from dota_reconnect_idle_buffer_time ConVar
	float flTimeSinceLastActivity = flGameTime - m_flLastActivityTime; // this + 0x13c4
	
	if ( flTimeSinceLastActivity <= flReconnectIdleBufferTime )
	{
		return false;
	}
	
	// Check DOTA time against fountain idle minimum time (matches decompiled GetDOTATime call)
	float flDOTATime = pGameRules->GetDOTATime( false, false, 0.0f );
	float flFountainIdleMinimumTime = 60.0f; // TODO: Get from dota_fountain_idle_minimum_time ConVar
	
	if ( flDOTATime <= flFountainIdleMinimumTime )
	{
		return false;
	}
	
	// Final check against idle time ConVars (matches decompiled logic)
	// Uses either dota_idle_time or dota_lenient_idle_time based on param_1 (bLenient)
	float flIdleTimeThreshold;
	if ( bLenient )
	{
		flIdleTimeThreshold = 300.0f; // TODO: Get from dota_lenient_idle_time ConVar (5 minutes)
	}
	else
	{
		flIdleTimeThreshold = 180.0f; // TODO: Get from dota_idle_time ConVar (3 minutes)  
	}
	
	// Check time since last order/idle time (matches decompiled this + 0x13c0)
	float flTimeSinceLastOrder = flGameTime - m_flIdleTime; // this + 0x13c0 is m_flIdleTime
	
	// Return true if idle time exceeds threshold (matches decompiled <= and != checks)
	return ( flTimeSinceLastOrder > flIdleTimeThreshold );
}

//-----------------------------------------------------------------------------
// Purpose: Check if player has selected a hero (from decompiled HasSelectedHero)
//-----------------------------------------------------------------------------
bool CDOTAPlayer::HasSelectedHero( void )
{
	int iPlayerID = GetPlayerID();
	if ( iPlayerID == -1 )
		return false;
		
	CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
	if ( !pPlayerResource )
		return false;
		
	return pPlayerResource->GetSelectedHeroID( iPlayerID ) != -1;
}

//-----------------------------------------------------------------------------
// Purpose: Physics simulation (from decompiled PhysicsSimulate)
//-----------------------------------------------------------------------------
void CDOTAPlayer::PhysicsSimulate( void )
{
	// TODO: Check if ShouldRunPhysicsThink() when available
	// if ( ShouldRunPhysicsThink() )
	//	PhysicsRunThink( TICK_NEVER_THINK );
		
	BaseClass::PhysicsSimulate();
}

//-----------------------------------------------------------------------------
// Purpose: Set assigned hero (from decompiled SetAssignedHero)
//-----------------------------------------------------------------------------
void CDOTAPlayer::SetAssignedHero( CBaseEntity *pHero )
{
	m_hAssignedHero = pHero;
	
	CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
	if ( pPlayerResource )
	{
		int iPlayerID = GetPlayerID();
		pPlayerResource->SetPlayerReservedState( iPlayerID, false );
	}
	
	// Update last order and idle times
	CDOTAGameRules *pGameRules = DOTAGameRules();
	float flGameTime = pGameRules ? pGameRules->GetGameTime() : 0.0f;
	
	m_flLastOrderTime = flGameTime;
	m_flIdleTime = flGameTime;
}

//-----------------------------------------------------------------------------
// Purpose: Set shop view mode (from decompiled SetShopViewMode)
//-----------------------------------------------------------------------------
void CDOTAPlayer::SetShopViewMode( int viewMode )
{
	if ( m_iShopViewMode == viewMode )
		return;
		
	m_iShopViewMode = viewMode;
	
	// TODO: Add network state change notification when system is ready
	// NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Check if player wants random hero (from decompiled WantsRandomHero)
//-----------------------------------------------------------------------------
bool CDOTAPlayer::WantsRandomHero( void )
{
	return m_bWantsRandomHero;
}


//-----------------------------------------------------------------------------
// Purpose: Set last order time (from decompiled SetLastOrderTime)
//-----------------------------------------------------------------------------
void CDOTAPlayer::SetLastOrderTime( float flTime, bool bUpdateIdleTime )
{
	m_flLastOrderTime = flTime;
	
	if ( bUpdateIdleTime )
	{
		m_flIdleTime = flTime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Undo hero selection (from decompiled UndoHeroSelection)
//-----------------------------------------------------------------------------
void CDOTAPlayer::UndoHeroSelection( void )
{
	CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
	if ( !pPlayerResource )
		return;
		
	int iPlayerID = GetPlayerID();
	
	// Check if player has already undone selection (matches decompiled 0xe5c + iVar7 check)
	// TODO: Add proper has_undone_selection flag when networking system is ready
	// The decompiled code checks *(char *)(iVar16 + 0xe5c + iVar7) == '\x01'
	
	// TODO: Add complex network state change notification when system is ready
	// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here
	// to notify clients that the undo flag has been set
	
	// Get assigned hero entity (matches decompiled this + 0x13a8)
	// TODO: Fix EHANDLE access when entity system is ready
	/*
	CBaseEntity *pHeroEntity = m_hAssignedHero.Get();
	
	if ( pHeroEntity )
	{
		// The decompiled code does complex entity list validation:
		// - Checks entity handle validity in g_pEntityList  
		// - Validates entity pointers and flags
		// For now, simple validation
		
		// UTIL_Remove( pHeroEntity );
		
		// Clear our hero handle
		m_hAssignedHero = NULL;
	}
	*/
	
	// Clear selected hero in player resource (matches decompiled SetSelectedHero call)
	pPlayerResource->SetSelectedHero( iPlayerID, "", NULL );
	
	// Fire game event for hero undo selection (matches decompiled game event logic)
	// TODO: Implement when IGameEventManager2 is available
	/*
	IGameEvent *pEvent = gameeventmanager->CreateEvent( "dota_hero_undoselection" );
	if ( pEvent )
	{
		pEvent->SetInt( "playerid1", iPlayerID );
		gameeventmanager->FireEvent( pEvent );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: Set wants random hero (from decompiled SetWantsRandomHero)
//-----------------------------------------------------------------------------
void CDOTAPlayer::SetWantsRandomHero( void )
{
	m_bWantsRandomHero = true;
	
	CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
	if ( pPlayerResource )
	{
		int iPlayerID = GetPlayerID();
		// TODO: Call SetSelectedHero with "random" when available
		// pPlayerResource->SetSelectedHero( iPlayerID, "random", NULL );
	}
	
	// TODO: Set network flag for wants random hero when networking ready
}


//-----------------------------------------------------------------------------
// Purpose: Handle hero selection command (from decompiled HandleHeroSelection)
//-----------------------------------------------------------------------------
void CDOTAPlayer::HandleHeroSelection( const CCommand &args )
{
	if ( args.ArgC() < 2 )
		return;
		
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return;
		
	// TODO: Check if game mode allows hero selection
	// TODO: Check game state
	// TODO: Implement full hero selection logic including:
	//       - Repick handling with gold costs
	//       - Random hero selection
	//       - Specific hero selection
	//       - Reserve hero functionality
	//       - Captain's mode validation
	//       - Hero availability checks
	
	const char *pHeroName = args.Arg(1);
	
	// Simple placeholder logic
	if ( Q_stricmp( pHeroName, "random" ) == 0 )
	{
		SetWantsRandomHero();
		MakeRandomHeroSelection( false, -1 );
	}
	else
	{
		SetWantsSpecificHero( pHeroName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set wants specific hero (from decompiled SetWantsSpecificHero)
//-----------------------------------------------------------------------------
void CDOTAPlayer::SetWantsSpecificHero( const char *pHeroName )
{
	m_bWantsRandomHero = false;
	
	CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
	if ( pPlayerResource )
	{
		int iPlayerID = GetPlayerID();
		pPlayerResource->SetSelectedHero( iPlayerID, pHeroName, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make random hero selection (from decompiled MakeRandomHeroSelection)
//-----------------------------------------------------------------------------
void CDOTAPlayer::MakeRandomHeroSelection( bool bBotGame, int iRandomSeed )
{
	// TODO: Implement full random hero selection logic including:
	//       - Getting available heroes from DOTAGameManager
	//       - Filtering by bot implementation if needed
	//       - Filtering by game mode restrictions
	//       - Filtering by already selected heroes
	//       - Using random seed or random selection
	//       - Setting player resource data
	//       - Firing game events
	//       - Granting random gold bonus
	
	// For now, simple placeholder
	CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
	if ( pPlayerResource )
	{
		int iPlayerID = GetPlayerID();
		// TODO: Set selected hero to a random available hero
		// TODO: Set player reserved state
		// TODO: Grant random gold bonus
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if player is AFK (from decompiled IsAFK)
//-----------------------------------------------------------------------------
bool CDOTAPlayer::IsAFK( void )
{
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return false;
		
	// TODO: Check Rules_DisableAFKChecks() when available
	// TODO: Check if player is bot
	// TODO: Check against dota_reconnect_idle_buffer_time ConVar
	// TODO: Check against dota_idle_time ConVar
	
	// Simple AFK check based on activity time
	float flGameTime = pGameRules->GetGameTime();
	float flIdleThreshold = 300.0f; // 5 minutes
	
	return (flGameTime - m_flLastActivityTime) > flIdleThreshold;
}

//-----------------------------------------------------------------------------
// Purpose: Handle player death (from decompiled Event_Killed)
//-----------------------------------------------------------------------------
void CDOTAPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	// Update death and activity times (matches decompiled time setting)
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( pGameRules )
	{
		float flGameTime = pGameRules->GetGameTime();
		m_flLastActivityTime = flGameTime;  // 0x13bc
		m_nDataValue1 = (int)flGameTime;    // 0x13c0 - death time
	}

	// Call base class event (matches decompiled base call)
	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Handle initial spawn (from decompiled InitialSpawn)
//-----------------------------------------------------------------------------
void CDOTAPlayer::InitialSpawn( void )
{
	// Call base class initial spawn (matches decompiled base call)
	BaseClass::InitialSpawn();

	// Handle Steam inventory request (matches decompiled Steam logic)
	if ( !IsFakeClient() )
	{
		// TODO: Request Steam inventory when Steam integration is ready
		// if ( steamgameserverapicontext && steamgameserverapicontext->SteamGameServer() )
		// {
		//     CSteamID steamID;
		//     if ( GetSteamID( &steamID ) )
		//     {
		//         DOTAInventoryManager()->SteamRequestInventory( &m_Inventory, steamID, 0 );
		//     }
		// }
	}

	// Reset network change tracking values (matches decompiled network variable resets)
	if ( m_nExtraValue1 != 0 )
	{
		// TODO: Add complex network change tracking when networking system is ready
		// The decompiled code has extensive network variable registration logic here
		m_nExtraValue1 = 0;
	}

	if ( m_nExtraValue2 != 0 )
	{
		// TODO: Add complex network change tracking when networking system is ready
		m_nExtraValue2 = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update player ID from Steam ID (helper function)
//-----------------------------------------------------------------------------
void CDOTAPlayer::UpdatePlayerIDFromSteamID( void )
{
	// TODO: Implement Steam ID to player ID mapping when Steam integration is ready
	// This would involve:
	// 1. Get Steam ID from player
	// 2. Map to internal player ID system
	// 3. Update m_nPlayerID member
	
	// For now, use a simple placeholder
	m_nPlayerID = entindex(); // Use entity index as temporary player ID
}

//-----------------------------------------------------------------------------
// Purpose: Get player ID (helper function)
//-----------------------------------------------------------------------------
int CDOTAPlayer::GetPlayerID( void ) const
{
	return m_nPlayerID;
}

//-----------------------------------------------------------------------------
// Purpose: Handle client commands (from decompiled ClientCommand)
//-----------------------------------------------------------------------------
bool CDOTAPlayer::ClientCommand( const CCommand &args )
{
	// Get the command name (matches decompiled command extraction)
	const char *pszCommand = args.Arg(0);
	if ( !pszCommand || !pszCommand[0] )
		return false;

	int iArgCount = args.ArgC();

	// Hero selection commands
	if ( !Q_stricmp( pszCommand, "possible_hero" ) )
	{
		// Set possible hero selection (matches decompiled possible_hero handling)
		int iPlayerID = GetPlayerID();
		if ( iPlayerID == -1 )
			return true;

		CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
		if ( !pPlayerResource )
			return true;

		// TODO: Check if player has already selected hero
		// bool bHasSelected = pPlayerResource->HasSelectedHero( iPlayerID );
		// if ( bHasSelected )
		//     return true;

		if ( iArgCount == 2 )
		{
			const char *pszHeroName = args.Arg(1);
			// TODO: Implement SetPossibleHeroSelection when available
			// pPlayerResource->SetPossibleHeroSelection( iPlayerID, pszHeroName );
			// TODO: Add proper logging when Msg is available
			// Msg( "CDOTAPlayer::ClientCommand - possible_hero: %s\n", pszHeroName );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "suggest_hero" ) )
	{
		// Set suggested hero selection (matches decompiled suggest_hero handling)
		if ( iArgCount == 2 )
		{
			int iPlayerID = GetPlayerID();
			const char *pszHeroName = args.Arg(1);
			// TODO: Implement SetSuggestedHeroSelection when available
			// CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
			// if ( pPlayerResource )
			//     pPlayerResource->SetSuggestedHeroSelection( iPlayerID, pszHeroName );
			Msg( "CDOTAPlayer::ClientCommand - suggest_hero: %s\n", pszHeroName );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_select_starting_position" ) )
	{
		// Set starting position (matches decompiled starting position logic) 
		if ( iArgCount == 2 )
		{
			int iTeamNum = GetTeamNumber();
			if ( iTeamNum == DOTA_TEAM_RADIANT || iTeamNum == DOTA_TEAM_DIRE )
			{
				const char *pszPosition = args.Arg(1);
				int iPosition = atoi( pszPosition );
				if ( iPosition < 6 ) // Max 6 positions (matches decompiled validation)
				{
					int iPlayerID = GetPlayerID();
					// TODO: Implement SetPlayerStartingPosition when team data system available
					// CDOTA_DataNonSpectator *pTeamData = GetTeamData( iTeamNum );
					// if ( pTeamData )
					//     pTeamData->SetPlayerStartingPosition( iPlayerID, iPosition );
					Msg( "CDOTAPlayer::ClientCommand - starting position: %d\n", iPosition );
				}
			}
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_select_hero" ) )
	{
		// Main hero selection (matches decompiled hero selection logic)
		// TODO: Check dota_force_pick_allow ConVar
		// if ( !dota_force_pick_allow->GetBool() )
		// {
		//     CDOTAGameRules *pGameRules = DOTAGameRules();
		//     if ( pGameRules && !pGameRules->HeroPickState_IsPlayerIDInControl( GetPlayerID() ) )
		//         return true;
		// }

		// TODO: Implement HandleHeroSelection when hero system is available
		// HandleHeroSelection( this, args );
		Msg( "CDOTAPlayer::ClientCommand - dota_select_hero\n" );
		return true;
	}

	// Captain's Mode commands
	if ( !Q_stricmp( pszCommand, "dota_captain_select_captain" ) )
	{
		// Select captain in Captain's Mode (matches decompiled captain selection)
		if ( GetTeamNumber() != TEAM_SPECTATOR )
		{
			// TODO: Check if player is coach
			// CSteamID steamID;
			// if ( GetSteamID( &steamID ) && IsPlayerCoach( steamID ) )
			//     return true;

			CDOTAGameRules *pGameRules = DOTAGameRules();
			if ( pGameRules )
			{
				// TODO: Implement captain mode functions when available
				// int iCurrentCaptain = pGameRules->CM_GetCaptain( GetTeamNumber() );
				// if ( iCurrentCaptain == -1 )
				// {
				//     pGameRules->CM_SetCaptain( GetTeamNumber(), GetPlayerID() );
				// }
				Msg( "CDOTAPlayer::ClientCommand - captain select\n" );
			}
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_captain_select_hero" ) )
	{
		// Captain selects hero (matches decompiled captain hero selection)
		// TODO: Check dota_force_pick_allow and player control
		CDOTAGameRules *pGameRules = DOTAGameRules();
		if ( pGameRules && iArgCount >= 2 )
		{
			const char *pszHeroName = args.Arg(1);
			// TODO: Implement CM_AddSelectedHero when available
			// pGameRules->CM_AddSelectedHero( pszHeroName );
			Msg( "CDOTAPlayer::ClientCommand - captain select hero: %s\n", pszHeroName );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_captain_ban_hero" ) )
	{
		// Captain bans hero (matches decompiled captain hero banning)
		CDOTAGameRules *pGameRules = DOTAGameRules();
		if ( pGameRules && iArgCount >= 2 )
		{
			const char *pszHeroName = args.Arg(1);
			// TODO: Implement CM_AddBannedHero when available
			// pGameRules->CM_AddBannedHero( pszHeroName );
			Msg( "CDOTAPlayer::ClientCommand - captain ban hero: %s\n", pszHeroName );
		}
		return true;
	}


	if ( !Q_stricmp( pszCommand, "location_ping" ) )
	{
		// Location ping (matches decompiled ping handling)
		// TODO: Implement ping system when available
		return true;
	}

	// Cheat commands (require cheat mode)
	CDOTAGameRules *pGameRules = DOTAGameRules();
#if 1
	bool bCheatMode = true;
#else
	bool bCheatMode = pGameRules ? pGameRules->IsCheatMode() : false; // TODO: implement isCheatMode
#endif
	if ( !Q_stricmp( pszCommand, "dota_all_vision_enable" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Set dota_all_vision ConVar when available
			// ConVar *pConVar = FindConVar( "dota_all_vision" );
			// if ( pConVar ) pConVar->SetValue( 1 );
			Msg( "CDOTAPlayer::ClientCommand - all vision enabled\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_all_vision_disable" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Set dota_all_vision ConVar when available
			// ConVar *pConVar = FindConVar( "dota_all_vision" );
			// if ( pConVar ) pConVar->SetValue( 0 );
			Msg( "CDOTAPlayer::ClientCommand - all vision disabled\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_clear_wards" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Find and remove all ward entities when entity system available
			// CBaseEntity *pWard = gEntList.FindEntityByClassname( NULL, "npc_dota_ward_base" );
			// while ( pWard )
			// {
			//     UTIL_Remove( pWard );
			//     pWard = gEntList.FindEntityByClassname( pWard, "npc_dota_ward_base" );
			// }
			Msg( "CDOTAPlayer::ClientCommand - wards cleared\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_start_game" ) || !Q_stricmp( pszCommand, "dota_takephotos" ) )
	{
		if ( bCheatMode && pGameRules )
		{
			// TODO: Check if game is already in progress
			// if ( pGameRules->GetGameState() != DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
			// {
			//     pGameRules->ForceGameStart();
			//     pGameRules->PrepareSpawners( 1.0f );
			// }
			Msg( "CDOTAPlayer::ClientCommand - force start game\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_give_gold" ) )
	{
		if ( bCheatMode )
		{
			if ( iArgCount < 2 )
			{
				Msg( "Missing gold amount\n" );
				return true;
			}

			const char *pszAmount = args.Arg(1);
			int iGoldAmount = atoi( pszAmount );
			
			CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
			if ( pPlayerResource )
			{
				// TODO: Implement ModifyGold when available
				// pPlayerResource->ModifyGold( GetPlayerID(), iGoldAmount, false, 8 );
				Msg( "CDOTAPlayer::ClientCommand - give gold: %d\n", iGoldAmount );
			}
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_hero_respawn" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Get controlled hero and respawn when hero system available
			// CDOTA_BaseNPC_Hero *pHero = GetControlledHero();
			// if ( pHero && pHero->IsAlive() == false )
			// {
			//     pHero->RespawnHero( false, false, false );
			// }
			Msg( "CDOTAPlayer::ClientCommand - hero respawn\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_hero_refresh" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Refresh hero health, mana, and cooldowns when hero system available
			Msg( "CDOTAPlayer::ClientCommand - hero refresh\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_hero_level" ) )
	{
		if ( bCheatMode )
		{
			if ( iArgCount < 2 )
			{
				Msg( "Missing level count\n" );
				return true;
			}

			const char *pszLevels = args.Arg(1);
			int iLevels = atoi( pszLevels );
			if ( iLevels < 1 )
				return true;

			// TODO: Level up hero when available
			// CDOTA_BaseNPC_Hero *pHero = GetControlledHero();
			// if ( pHero )
			// {
			//     for ( int i = 0; i < iLevels; i++ )
			//     {
			//         if ( pHero->GetLevel() >= GetHeroMaxLevel() )
			//             break;
			//         int iXPNeeded = GetXPNeededToReachNextLevel( pHero->GetLevel() );
			//         pHero->AddExperience( iXPNeeded, false, false, true );
			//     }
			// }
			Msg( "CDOTAPlayer::ClientCommand - hero level up: %d levels\n", iLevels );
		}
		return true;
	}

	// Economy/item commands
	if ( !Q_stricmp( pszCommand, "use_item" ) )
	{
		if ( iArgCount >= 2 )
		{
			const char *pszItem = args.Arg(1);
			const char *pszParam = iArgCount >= 3 ? args.Arg(2) : "";
			// TODO: Implement UseEconItem when economy system available
			// UseEconItem( pszItem, pszParam, this );
			Msg( "CDOTAPlayer::ClientCommand - use item: %s %s\n", pszItem, pszParam );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "use_item_id" ) )
	{
		if ( iArgCount >= 2 )
		{
			const char *pszItemID = args.Arg(1);
			// TODO: Implement UseEconItemID when economy system available
			// UseEconItemID( pszItemID, this );
			Msg( "CDOTAPlayer::ClientCommand - use item id: %s\n", pszItemID );
		}
		return true;
	}

	// ConVar cheat commands
	if ( !Q_stricmp( pszCommand, "dota_ability_debug_enable" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Set dota_ability_debug ConVar when available
			Msg( "CDOTAPlayer::ClientCommand - ability debug enabled\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_ability_debug_disable" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Set dota_ability_debug ConVar when available  
			Msg( "CDOTAPlayer::ClientCommand - ability debug disabled\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_creeps_no_spawning_enable" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Set dota_creeps_no_spawning ConVar when available
			Msg( "CDOTAPlayer::ClientCommand - creep spawning disabled\n" );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dota_creeps_no_spawning_disable" ) )
	{
		if ( bCheatMode )
		{
			// TODO: Set dota_creeps_no_spawning ConVar when available
			Msg( "CDOTAPlayer::ClientCommand - creep spawning enabled\n" );
		}
		return true;
	}

	// Debug commands
	if ( !Q_stricmp( pszCommand, "dump_modifiers" ) )
	{
		// TODO: Check sv_cheats ConVar when available
		// Dump modifier information to file (simplified from complex decompiled version)
		Msg( "CDOTAPlayer::ClientCommand - dump modifiers\n" );
		return true;
	}

	// Simple cheat command for giving gold (alternate version)
	if ( !Q_stricmp( pszCommand, "givegold" ) )
	{
		// TODO: Check sv_cheats ConVar when available
		if ( iArgCount >= 2 )
		{
			const char *pszAmount = args.Arg(1);
			int iGoldAmount = atoi( pszAmount );
			
			CDOTA_PlayerResource *pPlayerResource = GetDOTAPlayerResource();
			if ( pPlayerResource )
			{
				// TODO: Implement ModifyGold when available
				// pPlayerResource->ModifyGold( GetPlayerID(), iGoldAmount, false, 0 );
				Msg( "CDOTAPlayer::ClientCommand - givegold: %d\n", iGoldAmount );
			}
		}
		return true;
	}

	// If we didn't handle the command, pass it to the base class
	return BaseClass::ClientCommand( args );
} 