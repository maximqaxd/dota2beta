//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: DOTA Player Resource implementation
//
//==========================================================================//

#include "cbase.h" 
#include "dota_playerresource.h"
#include "dota_player.h"
#include "dota_gamerules.h"
#include "tier0/basetypes.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global pointer
CDOTA_PlayerResource *g_pDOTAPlayerResource = NULL;

// Removed duplicate - using the one below with correct DT name


//=============================================================================
// CMsgLeaverState Implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor (from decompiled CMsgLeaverState::CMsgLeaverState)
//-----------------------------------------------------------------------------
CMsgLeaverState::CMsgLeaverState()
{
	// Initialize leaver state data to default values
	for ( int i = 0; i < 7; i++ )
	{
		m_iLeaverStateData[i] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMsgLeaverState::~CMsgLeaverState()
{
	// TODO: Cleanup leaver state data when needed
}

//=============================================================================
// CDOTA_PlayerResource Data Tables
//=============================================================================

BEGIN_DATADESC( CDOTA_PlayerResource )
	// TODO: Add save/restore fields when needed
END_DATADESC()

// Basic server class implementation (required for linking)
IMPLEMENT_SERVERCLASS_ST( CDOTA_PlayerResource, DT_DOTA_PlayerResource )
	// TODO: Add network variable send table entries when networking system is ready
	// For now, empty table to resolve linker errors
END_SEND_TABLE()

// TODO: Re-enable when entity system is ready
// LINK_ENTITY_TO_CLASS( dota_player_resource, CDOTA_PlayerResource );

//=============================================================================
// CDOTA_PlayerResource Implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor (from decompiled CDOTA_PlayerResource::CDOTA_PlayerResource)
//-----------------------------------------------------------------------------
CDOTA_PlayerResource::CDOTA_PlayerResource() : BaseClass( false )
{
	// Initialize first player ID array to -1 (matches decompiled 0x8b8-0x8dc initialization)
	for ( int i = 0; i < MAX_DOTA_PLAYERS; i++ )
	{
		m_iPlayerIDs1[i] = -1;		// 0xffffffff in decompiled code
	}

	// Initialize second player ID array to -1 (matches decompiled 0x1020-0x1044 initialization)
	for ( int i = 0; i < MAX_DOTA_PLAYERS; i++ )
	{
		m_iPlayerIDs2[i] = -1;		// 0xffffffff in decompiled code
	}

	// Initialize player data arrays to 0 (matches decompiled 0x1a54-0x1b40 initialization)
	for ( int i = 0; i < MAX_DOTA_PLAYERS; i++ )
	{
		m_iPlayerTeams[i] = 0;
		m_iPlayerLevels[i] = 0;
		m_iPlayerGold[i] = 0;
		m_iPlayerKills[i] = 0;
		m_iPlayerDeaths[i] = 0;
		m_iPlayerAssists[i] = 0;
		m_iPlayerDenies[i] = 0;
		m_iPlayerLastHits[i] = 0;
		m_iPlayerNetWorth[i] = 0;
		m_iPlayerHeroIDs[i] = 0;
		
		// Additional data arrays
		m_iPlayerData1[i] = 0;
		m_iPlayerData2[i] = 0;
		m_iPlayerData3[i] = 0;
		m_iPlayerData4[i] = 0;
		m_iPlayerData5[i] = 0;
		
		// Player reservation state (matches decompiled 0x1b1c offset)
		m_bPlayerReservedState[i] = false;
		
		// Selected hero data initialization
		m_hSelectedHeroes[i] = NULL;			// Initialize hero handles to NULL
		m_iSelectedHeroIDs[i] = -1;				// Initialize hero IDs to -1 (no hero selected)
		m_szSelectedHeroNames[i][0] = '\0';		// Initialize hero names to empty string
	}

	// Initialize leaver states (matches decompiled 0x2b38 loop)
	// The decompiled code shows: do { CMsgLeaverState::CMsgLeaverState(...); iVar1 += 0x1c; } while (iVar1 != 0x118);
	// This means 10 objects of 0x1c (28) bytes each
	for ( int i = 0; i < MAX_DOTA_PLAYERS; i++ )
	{
		// Constructor is called automatically for array members
		// m_LeaverStates[i] is initialized by its constructor
	}

	// Initialize Steam and connection data to 0 (matches decompiled 0x2cd4-0x2df8 initialization)
	for ( int i = 0; i < MAX_DOTA_PLAYERS; i++ )
	{
		m_iPlayerSteamIDs[i] = 0;		// 0x2cd4+ area
		m_iConnectionStates[i] = 0;
		m_iPlayerFlags[i] = 0;
		m_iPlayerColors[i] = 0;
		m_iPlayerSlots[i] = 0;
	}

	// Initialize player names to empty (matches decompiled bzero 200 bytes at 0x2e04)
	memset( m_szPlayerNames, 0, sizeof(m_szPlayerNames) );

	// Initialize dynamic player data array (matches decompiled CUtlArray constructor at 0x5410)
	m_PlayerDataArray.RemoveAll();

	// Initialize final data members to 0 (matches decompiled 0x55ac-0x55bc initialization)
	m_iFinalData1 = 0;		// 0x55ac
	m_iFinalData2 = 0;		// 0x55b0
	m_iFinalData3 = 0;		// 0x55b4
	m_iFinalData4 = 0;		// 0x55b8
	m_iFinalData5 = 0;		// 0x55bc

	// Set global pointer
	g_pDOTAPlayerResource = this;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CDOTA_PlayerResource::~CDOTA_PlayerResource()
{
	// Clear global pointer
	if ( g_pDOTAPlayerResource == this )
	{
		g_pDOTAPlayerResource = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update player's team slot assignment
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::UpdateTeamSlot( int iPlayerID, int iTeamNum )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	// Update team assignment (this is called from CDOTAPlayer::ChangeTeam)
	m_iPlayerTeams[iPlayerID] = iTeamNum;

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Set player as fully joined the server
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::SetFullyJoinedServer( int iPlayerID, bool bFullyJoined )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	// Set connection state flag (this is called from CDOTAPlayer::ChangeTeam)
	if ( bFullyJoined )
	{
		m_iPlayerFlags[iPlayerID] |= 0x1;  // Set "fully joined" flag
	}
	else
	{
		m_iPlayerFlags[iPlayerID] &= ~0x1; // Clear "fully joined" flag
	}

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Get player's team number
//-----------------------------------------------------------------------------
int CDOTA_PlayerResource::GetPlayerTeam( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;

	return m_iPlayerTeams[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player team (from decompiled CDOTA_PlayerResource::GetTeam)
//-----------------------------------------------------------------------------
int CDOTA_PlayerResource::GetTeam( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;

	return m_iPlayerTeams[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Check if player is fully joined
//-----------------------------------------------------------------------------
bool CDOTA_PlayerResource::IsPlayerFullyJoined( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return false;

	return ( m_iPlayerFlags[iPlayerID] & 0x1 ) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get player's selected hero ID
//-----------------------------------------------------------------------------
int CDOTA_PlayerResource::GetSelectedHeroID( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;

	return m_iPlayerHeroIDs[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player's name
//-----------------------------------------------------------------------------
const char* CDOTA_PlayerResource::GetPlayerName( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return "";

	return m_szPlayerNames[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player's Steam ID
//-----------------------------------------------------------------------------
unsigned long long CDOTA_PlayerResource::GetSteamID( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;

	return m_iPlayerSteamIDs[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Set player's Steam ID
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::SetSteamID( int iPlayerID, unsigned long long steamID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	m_iPlayerSteamIDs[iPlayerID] = steamID;

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Set player connection state
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::SetPlayerConnectionState( int iPlayerID, int iState )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	m_iConnectionStates[iPlayerID] = iState;

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Get player connection state
//-----------------------------------------------------------------------------
int CDOTA_PlayerResource::GetPlayerConnectionState( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;

	return m_iConnectionStates[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Set connection state (from decompiled SetConnectionState)
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::SetConnectionState( int iPlayerID, int eConnectionState, int eDisconnectReason )
{
	// Validate player index (matches decompiled 0x1f < param_1 check)
	if ( iPlayerID > 31 )
		return;

	int iDisconnectReason = eDisconnectReason;
	
	// Special handling when connection state is 4 and existing state is 3
	// (matches decompiled (param_3 == 4) && (*(int *)(this + param_1 * 4 + 0xaa8) == 3))
	if ( eConnectionState == 4 && m_iPlayerConnectionStates[iPlayerID] == 3 )
	{
		// Use existing disconnect reason (matches decompiled *(int *)(this + param_1 * 4 + 0x2788))
		iDisconnectReason = m_iPlayerDisconnectReasons[iPlayerID];
	}

	// Get Steam ID for logging (matches decompiled Steam ID retrieval logic)
	unsigned long long steamID = 0;
	if ( iPlayerID < MAX_DOTA_PLAYERS )
	{
		// Get Steam ID from player data (simplified from complex decompiled logic)
		steamID = m_iPlayerSteamIDs[iPlayerID];
	}

	// Log connection state change (matches decompiled Msg call with enum names)
	// TODO: Add proper enum to string conversion when available
	// For now, use simple logging
	const char *pszConnectionState = "UNKNOWN";
	switch ( eConnectionState )
	{
		case 0: pszConnectionState = "NEVER_CONNECTED"; break;
		case 1: pszConnectionState = "CONNECTED"; break;
		case 2: pszConnectionState = "DISCONNECTED"; break;
		case 3: pszConnectionState = "ABANDONED"; break;
		case 4: pszConnectionState = "LOADING"; break;
		case 5: pszConnectionState = "FAILED"; break;
	}

	const char *pszDisconnectReason = "UNKNOWN";  
	switch ( iDisconnectReason )
	{
		case 0: pszDisconnectReason = "DISCONNECT_UNKNOWN"; break;
		case 1: pszDisconnectReason = "DISCONNECT_KICKED"; break;
		case 2: pszDisconnectReason = "DISCONNECT_TIMEOUT"; break;
		case 3: pszDisconnectReason = "DISCONNECT_LEFT"; break;
	}

	// TODO: Add proper Steam ID rendering when available
	// For now, simple logging (matches decompiled format)
	// TODO: Enable logging when Msg is available
	Msg( "PR:SetConnectionState %d:SteamID_%llu %s %s\n", iPlayerID, steamID, pszConnectionState, pszDisconnectReason );

	// Skip network notification if state hasn't changed 
	// (matches decompiled *(int *)(this + param_1 * 4 + 0xaa8) == local_14 check)
	if ( m_iPlayerConnectionStates[iPlayerID] != eConnectionState )
	{
		// TODO: Add complex network state change notification when system is ready
		// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here
		// to notify clients that the connection state has changed
		
		// For now, simple state update
		m_iPlayerConnectionStates[iPlayerID] = eConnectionState;
	}

	// Always update disconnect reason (matches decompiled final assignment)
	m_iPlayerDisconnectReasons[iPlayerID] = iDisconnectReason;
}

//-----------------------------------------------------------------------------
// Purpose: Set player reserved state (from decompiled SetPlayerReservedState)
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::SetPlayerReservedState( int iPlayerID, bool bReserved )
{
	// Validate player ID (matches decompiled check for param_1 > 9)
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	// If state is already set to the desired value, skip
	if ( m_bPlayerReservedState[iPlayerID] == bReserved )
		return;

	// TODO: Add complex network state change notification when system is ready
	// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here
	// for notifying clients of the change, but we'll skip this for now

	// Set the reserved state (matches decompiled this[param_1 + 0x1b1c] = param_2)
	m_bPlayerReservedState[iPlayerID] = bReserved;

	// Log the state change (matches decompiled Msg call at end)
	// TODO: Add Steam ID rendering when CSteamID system is available
	// For now, simple logging
	const char *pszState = bReserved ? "true" : "false";
	Msg( "PR:SetPlayerReservedState %d:%s %s\n", iPlayerID, "SteamID_Placeholder", pszState );
}

//-----------------------------------------------------------------------------
// Purpose: Get player reserved state
//-----------------------------------------------------------------------------
bool CDOTA_PlayerResource::GetPlayerReservedState( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return false;

	return m_bPlayerReservedState[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Set selected hero (from decompiled SetSelectedHero)
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::SetSelectedHero( int iPlayerID, const char* pHeroName, CBaseEntity* pHeroEntity )
{
	// Validate player ID (matches decompiled check for param_1 > 9)
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	// Handle hero entity (matches decompiled 0x8b8 offset logic)
	EHANDLE hNewHero;
	if ( pHeroEntity )
	{
		hNewHero = pHeroEntity;
	}
	int iEntityHandle = -1;
	
	if ( pHeroEntity != NULL )
	{
		// TODO: Get entity handle/index when entity system is ready
		// iEntityHandle = pHeroEntity->GetEntityIndex();
	}
	
	// Set hero entity handle if different (matches decompiled logic)
	if ( m_hSelectedHeroes[iPlayerID] != hNewHero )
	{
		// TODO: Add complex network state change notification when system is ready
		// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here
		
		m_hSelectedHeroes[iPlayerID] = hNewHero;
	}

	// Get hero ID from hero name (matches decompiled DOTAGameManager::GetHeroIDByName call)
	int iHeroID = -1;
	if ( pHeroName && pHeroName[0] != '\0' )
	{
		// TODO: Implement DOTAGameManager::GetHeroIDByName when available
		// CDOTAGameManager *pGameManager = DOTAGameManager();
		// iHeroID = pGameManager->GetHeroIDByName( pHeroName, 2 );
		
		// For now, simple placeholder logic
		if ( Q_stricmp( pHeroName, "random" ) == 0 )
		{
			iHeroID = 0; // Special ID for random
		}
		else
		{
			// TODO: Implement proper hero name to ID mapping
			iHeroID = 1; // Placeholder ID for non-random heroes
		}
	}

	// Set hero ID if different (matches decompiled 0x478 offset logic)
	if ( m_iSelectedHeroIDs[iPlayerID] != iHeroID )
	{
		// TODO: Add complex network state change notification when system is ready
		// The decompiled code has extensive CBaseEdict::GetChangeAccessor logic here
		
		m_iSelectedHeroIDs[iPlayerID] = iHeroID;
	}
	
	// Store hero name for easy access
	if ( pHeroName )
	{
		Q_strncpy( m_szSelectedHeroNames[iPlayerID], pHeroName, sizeof(m_szSelectedHeroNames[iPlayerID]) );
	}
	else
	{
		m_szSelectedHeroNames[iPlayerID][0] = '\0';
	}

	// Log the selection (matches decompiled Msg call)
	// TODO: Add Steam ID rendering when CSteamID system is available
	Msg( "PR:SetSelectedHero %d:%s %s(%d)\n", iPlayerID, "SteamID_Placeholder", pHeroName ? pHeroName : "", iHeroID );
}

//-----------------------------------------------------------------------------
// Purpose: Check if player has selected a hero
//-----------------------------------------------------------------------------
bool CDOTA_PlayerResource::HasSelectedHero( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return false;

	return m_iSelectedHeroIDs[iPlayerID] != -1;
}

//-----------------------------------------------------------------------------
// Purpose: Get selected hero name
//-----------------------------------------------------------------------------
const char* CDOTA_PlayerResource::GetSelectedHeroName( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return "";

	return m_szSelectedHeroNames[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player kill count
//-----------------------------------------------------------------------------
int CDOTA_PlayerResource::GetKills( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;

	return m_iPlayerKills[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player death count
//-----------------------------------------------------------------------------
int CDOTA_PlayerResource::GetDeaths( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;

	return m_iPlayerDeaths[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player assist count
//-----------------------------------------------------------------------------
int CDOTA_PlayerResource::GetAssists( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;

	return m_iPlayerAssists[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Increment player kill count
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::IncrementKills( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	m_iPlayerKills[iPlayerID]++;

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Increment player death count
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::IncrementDeaths( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	m_iPlayerDeaths[iPlayerID]++;

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Increment player assist count
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::IncrementAssists( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	m_iPlayerAssists[iPlayerID]++;

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Validate player ID range
//-----------------------------------------------------------------------------
bool CDOTA_PlayerResource::IsValidPlayerID( int iPlayerID )
{
	return ( iPlayerID >= 0 && iPlayerID < MAX_DOTA_PLAYERS );
}

//-----------------------------------------------------------------------------
// Purpose: Initialize player data for new player
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::InitializePlayerData( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	// Reset all stats for this player
	m_iPlayerTeams[iPlayerID] = 0;
	m_iPlayerLevels[iPlayerID] = 1;		// Start at level 1
	m_iPlayerGold[iPlayerID] = 625;		// Starting gold in DOTA
	m_iPlayerKills[iPlayerID] = 0;
	m_iPlayerDeaths[iPlayerID] = 0;
	m_iPlayerAssists[iPlayerID] = 0;
	m_iPlayerDenies[iPlayerID] = 0;
	m_iPlayerLastHits[iPlayerID] = 0;
	m_iPlayerNetWorth[iPlayerID] = 625;	// Starting net worth
	m_iPlayerHeroIDs[iPlayerID] = 0;
	m_iConnectionStates[iPlayerID] = 0;
	m_iPlayerFlags[iPlayerID] = 0;
	m_iPlayerColors[iPlayerID] = iPlayerID; // Default color based on slot
	m_iPlayerSlots[iPlayerID] = iPlayerID;

	// Clear player name
	Q_strncpy( m_szPlayerNames[iPlayerID], "", sizeof(m_szPlayerNames[iPlayerID]) );

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Reset player statistics (for new game)
//-----------------------------------------------------------------------------
void CDOTA_PlayerResource::ResetPlayerStats( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return;

	// Reset match statistics only (keep persistent data)
	m_iPlayerKills[iPlayerID] = 0;
	m_iPlayerDeaths[iPlayerID] = 0;
	m_iPlayerAssists[iPlayerID] = 0;
	m_iPlayerDenies[iPlayerID] = 0;
	m_iPlayerLastHits[iPlayerID] = 0;
	m_iPlayerGold[iPlayerID] = 625;
	m_iPlayerNetWorth[iPlayerID] = 625;
	m_iPlayerLevels[iPlayerID] = 1;

	// TODO: Add network state change notification when system is ready
}

//-----------------------------------------------------------------------------
// Purpose: Get global player resource instance
//-----------------------------------------------------------------------------
CDOTA_PlayerResource* GetDOTAPlayerResource()
{
	return g_pDOTAPlayerResource;
} 