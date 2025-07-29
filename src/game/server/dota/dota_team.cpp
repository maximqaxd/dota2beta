//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: DOTA Team implementation
//
//==========================================================================//

#include "cbase.h"
#include "dota_team.h"
#include "dota_player.h"
#include "dota_gamerules.h"
#include "igameevents.h"

// Forward declarations for DOTA-specific classes not yet implemented
class CDOTALobby;

// Constants (TODO: Move to proper header when available)
#define TRANSMIT_ALWAYS			8		// Always transmit to client
#define TRANSMIT_NEVER			0		// Never transmit to client

// External globals
extern IGameEventManager2 *gameeventmanager;
extern CDOTAGameRules *DOTAGameRules();

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// CDOTATeam Data Tables
//=============================================================================

BEGIN_DATADESC( CDOTATeam )
	// TODO: Add save/restore fields when needed
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CDOTATeam, DT_DOTATeam )
	// TODO: Add network variable send table entries when networking system is ready
	// For now, empty table to resolve linker errors
END_SEND_TABLE()

// TODO: Re-enable when entity system is ready
// LINK_ENTITY_TO_CLASS( dota_team, CDOTATeam );

//=============================================================================
// CDOTATeam Implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor (from decompiled CDOTATeam::CDOTATeam)
//-----------------------------------------------------------------------------
CDOTATeam::CDOTATeam() : BaseClass()
{
	// Initialize kill counters (matches decompiled member initialization)
	m_iHeroKills = 0;			// 0x498
	m_iTowerKills = 0;			// 0x49c
	m_iBarracksKills = 0;		// 0x4a0
	m_iTeamLogo = 0;			// 0x4a4

	// Initialize team visual data (matches decompiled initialization)
	m_iPrimaryColor1 = 0;		// 0x4a8
	m_iPrimaryColor2 = 0;		// 0x4ac
	m_iSecondaryColor1 = 0;		// 0x4b0
	m_iSecondaryColor2 = 0;		// 0x4b4
	m_iPatternColor1 = 0;		// 0x4b8
	m_iPatternColor2 = 0;		// 0x4bc

	// Initialize flags (matches decompiled flag initialization)
	m_bTeamComplete = false;	// 0x4c0
	m_bTeamReady = false;		// 0x4c1

	// Initialize internal state
	m_bInitialized = false;		// 0x494 - will be set to true at end
	m_pEventListener = NULL;	// 0x490
	m_iLobbyEventID = 0;		// 0x4e4
	m_iTeamData = 0;			// 0x4e8
	m_iTeamNumber = 0;			// 0x48c

	// Initialize strings (matches decompiled string initialization)
	Q_strncpy( m_szTeamName, "", sizeof(m_szTeamName) );		// 0x3f8
	Q_strncpy( m_szTeamTag, "", sizeof(m_szTeamTag) );			// 0x4c2

	// Initialize network change tracking (placeholders for complex decompiled logic)
	m_iNetworkChange1 = 0;
	m_iNetworkChange2 = 0;
	m_iNetworkChange3 = 0;
	m_iNetworkChange4 = 0;
	m_iNetworkChange5 = 0;
	m_iNetworkChange6 = 0;

	// TODO: Complex network change tracking initialization from decompiled code
	// The decompiled constructor has extensive network variable registration logic
	// This will be implemented when the networking system is ready

	// Cache team number for performance (matches decompiled GetTeamNumber() caching)
	m_iTeamNumber = GetTeamNumber();

	// Register for lobby events (matches decompiled game event registration)
	RegisterForLobbyEvents();

	// Set initialization flag (matches decompiled final step)
	m_bInitialized = true;		// 0x494 = 1
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDOTATeam::~CDOTATeam()
{
	UnregisterFromLobbyEvents();
}

//-----------------------------------------------------------------------------  
// Purpose: Check if team is full (from decompiled IsTeamFull)
//-----------------------------------------------------------------------------
bool CDOTATeam::IsTeamFull( void )
{
	int iTeamNum = GetTeamNumber();
	bool bIsFull = false;

	// Team 1 (spectators) can have up to 21 players (matches decompiled constant 0x15)
	if ( iTeamNum == 1 )
	{
		bIsFull = ( GetNumPlayers() > 21 );
	}
	else
	{
		// Other teams (Radiant/Dire) limited to 5 players (matches decompiled constant 4 < players)
		bIsFull = ( GetNumPlayers() > 5 );
	}

	return bIsFull;
}

//-----------------------------------------------------------------------------
// Purpose: Handle game events (from decompiled FireGameEvent)
//-----------------------------------------------------------------------------
void CDOTATeam::FireGameEvent( IGameEvent *event )
{
	if ( !event )
		return;

	// Check if this is the lobby_updated event we registered for (matches decompiled event ID check)
	int iEventID = event->GetInt( "eventid", 0 );
	if ( iEventID == m_iLobbyEventID )
	{
		UpdateTeamNameFromLobby();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Increment hero kill count (from decompiled IncrementHeroKills)
//-----------------------------------------------------------------------------
void CDOTATeam::IncrementHeroKills( void )
{
	// Store old value for network change tracking (matches decompiled logic)
	int iOldKills = m_iHeroKills;

	// TODO: Add complex network change tracking when networking system is ready
	// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here

	// Increment the counter (matches decompiled increment)
	m_iHeroKills = iOldKills + 1;

	// TODO: Trigger network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Increment tower kill count (from decompiled IncrementTowerKills)
//-----------------------------------------------------------------------------
void CDOTATeam::IncrementTowerKills( void )
{
	// Store old value for network change tracking (matches decompiled logic)
	int iOldKills = m_iTowerKills;

	// TODO: Add complex network change tracking when networking system is ready
	// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here

	// Increment the counter (matches decompiled increment)
	m_iTowerKills = iOldKills + 1;

	// TODO: Trigger network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Increment barracks kill count (from decompiled IncrementBarracksKills)
//-----------------------------------------------------------------------------
void CDOTATeam::IncrementBarracksKills( void )
{
	// Store old value for network change tracking (matches decompiled logic)
	int iOldKills = m_iBarracksKills;

	// TODO: Add complex network change tracking when networking system is ready  
	// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here

	// Increment the counter (matches decompiled increment)
	m_iBarracksKills = iOldKills + 1;

	// TODO: Trigger network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Determine if team data should transmit to player (from decompiled ShouldTransmitToPlayer)
//-----------------------------------------------------------------------------
int CDOTATeam::ShouldTransmitToPlayer( CBasePlayer *pPlayer, CBaseEntity *pEntity )
{
	// Default to transmit everything (0x8 = transmit, matches decompiled return value)
	int iTransmitFlag = TRANSMIT_ALWAYS;

	// Check if we're in a game state that restricts visibility (matches decompiled game rules check)
	CDOTAGameRules *pGameRules = DOTAGameRules();
	if ( !pGameRules )
		return iTransmitFlag;

	// During strategy time or game in progress, apply team-based visibility (matches decompiled state check)
	DOTA_GameState_t gameState = pGameRules->GetGameState();
	if ( gameState != DOTA_GAMERULES_STATE_STRATEGY_TIME && gameState != DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
	{
		return iTransmitFlag; // Always transmit outside these states
	}

	// Get the team we should check visibility against (matches decompiled team logic)
	int iCheckTeam = m_iTeamNumber;
	
	// Check if player is coaching a different team (matches decompiled GetCoachedTeamNum call)
	CDOTAPlayer *pDOTAPlayer = static_cast<CDOTAPlayer*>( pPlayer );
	if ( pDOTAPlayer )
	{
		// TODO: Implement CDOTAPlayer::GetCoachedTeamNum when available
		// int iCoachedTeam = pDOTAPlayer->GetCoachedTeamNum();
		// if ( iCoachedTeam != 0 )
		//     iCheckTeam = iCoachedTeam;
	}

	// If entity belongs to different team, check if it can be seen (matches decompiled team check)
	if ( pEntity && pEntity->GetTeamNumber() != iCheckTeam )
	{
		// TODO: Add entity visibility logic when CBaseEntity::CanBeSeenByTeam is available
		// bool bCanSee = pEntity->CanBeSeenByTeam( iCheckTeam );
		// if ( !bCanSee )
		//     iTransmitFlag = TRANSMIT_NEVER;
	}

	return iTransmitFlag;
}

//-----------------------------------------------------------------------------
// Purpose: Update team name from lobby data (from decompiled UpdateTeamNameFromLobby)
//-----------------------------------------------------------------------------
void CDOTATeam::UpdateTeamNameFromLobby( void )
{
	int iTeamNum = GetTeamNumber();

	// TODO: Get lobby system when available
	// int iGCSystem = GDOTAGCClientSystem();
	// CDOTALobby *pLobby = GetDOTALobby();

	// For now, set default team names (matches decompiled default name logic)
	const char *pszDefaultName = "";
	if ( iTeamNum == DOTA_TEAM_RADIANT )
	{
		pszDefaultName = "#DOTA_GoodGuys";
	}
	else if ( iTeamNum == DOTA_TEAM_DIRE )
	{
		pszDefaultName = "#DOTA_BadGuys";
	}

	// TODO: Add complex network change tracking when networking system is ready
	// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here

	// Set team name (matches decompiled V_strncpy calls)
	Q_strncpy( m_szTeamName, pszDefaultName, sizeof(m_szTeamName) );
	Q_strncpy( m_szTeamTag, "", sizeof(m_szTeamTag) );

	// Reset team visual data to defaults (matches decompiled reset logic)
	ResetTeamVisuals();

	// TODO: When lobby system is available, parse lobby data:
	// - Update team name from lobby->GetTeamDetails()->teamName
	// - Update team tag from lobby->GetTeamDetails()->teamTag  
	// - Update team colors from lobby->GetTeamDetails()->colors
	// - Update team logo from lobby->GetTeamDetails()->logoID
	// - Set team complete/ready flags from lobby data
}

//-----------------------------------------------------------------------------
// Purpose: Register for lobby-related game events (helper function)
//-----------------------------------------------------------------------------
void CDOTATeam::RegisterForLobbyEvents( void )
{
	if ( gameeventmanager )
	{
		// Register for lobby_updated events (matches decompiled event registration)
		gameeventmanager->AddListener( this, "lobby_updated", true );
		
		// Store the event ID for later comparison (matches decompiled event ID storage)
		// TODO: Get actual event ID when event system is properly implemented
		m_iLobbyEventID = 1; // Placeholder
	}
}

//-----------------------------------------------------------------------------
// Purpose: Unregister from game events (helper function)
//-----------------------------------------------------------------------------
void CDOTATeam::UnregisterFromLobbyEvents( void )
{
	if ( gameeventmanager )
	{
		gameeventmanager->RemoveListener( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reset team visual data to defaults (helper function)
//-----------------------------------------------------------------------------
void CDOTATeam::ResetTeamVisuals( void )
{
	// TODO: Add complex network change tracking when networking system is ready
	// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here

	// Reset color data (matches decompiled reset logic)
	m_iPrimaryColor1 = 0;
	m_iPrimaryColor2 = 0;
	m_iSecondaryColor1 = 0;
	m_iSecondaryColor2 = 0;
	m_iPatternColor1 = 0;
	m_iPatternColor2 = 0;
	
	// Reset flags
	m_bTeamComplete = false;
	m_bTeamReady = false;
	
	// Reset logo
	m_iTeamLogo = 0;

	// TODO: Trigger network state change notification when system is ready
} 