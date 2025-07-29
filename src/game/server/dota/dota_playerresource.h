//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: DOTA Player Resource - Central player data management
//
//==========================================================================//

#ifndef DOTA_PLAYERRESOURCE_H
#define DOTA_PLAYERRESOURCE_H

#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "utlvector.h"

// Forward declarations
class CDOTAPlayer;

// Constants
#define MAX_DOTA_PLAYERS 10

//=============================================================================
// CMsgLeaverState - Player connection state information
//=============================================================================
class CMsgLeaverState
{
public:
	CMsgLeaverState();
	~CMsgLeaverState();

private:
	// Leaver state data (28 bytes = 0x1c per decompiled size)
	int		m_iLeaverStateData[7];		// 28 bytes of leaver state information
};

//=============================================================================
// CDOTA_PlayerResource - Central player data management system
//=============================================================================
class CDOTA_PlayerResource : public CBaseEntity
{
	DECLARE_CLASS( CDOTA_PlayerResource, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:
	CDOTA_PlayerResource();
	virtual ~CDOTA_PlayerResource();

	// Player management
	void			UpdateTeamSlot( int iPlayerID, int iTeamNum );
	void			SetFullyJoinedServer( int iPlayerID, bool bFullyJoined );

	// Player data access
	int				GetPlayerTeam( int iPlayerID );
	int				GetTeam( int iPlayerID );		// Decompiled GetTeam method
	bool			IsPlayerFullyJoined( int iPlayerID );
	int				GetSelectedHeroID( int iPlayerID );
	const char*		GetPlayerName( int iPlayerID );
	
	// Hero selection (from decompiled SetSelectedHero)
	void			SetSelectedHero( int iPlayerID, const char* pHeroName, CBaseEntity* pHeroEntity );
	bool			HasSelectedHero( int iPlayerID );
	const char*		GetSelectedHeroName( int iPlayerID );

	// Steam integration
	unsigned long long		GetSteamID( int iPlayerID );
	void			SetSteamID( int iPlayerID, unsigned long long steamID );

	// Connection and reservation state  
	void			SetPlayerConnectionState( int iPlayerID, int iState );
	int				GetPlayerConnectionState( int iPlayerID );
	void			SetConnectionState( int iPlayerID, int eConnectionState, int eDisconnectReason = 0 );
	void			SetPlayerReservedState( int iPlayerID, bool bReserved );
	bool			GetPlayerReservedState( int iPlayerID );

	// Statistics
	int				GetKills( int iPlayerID );
	int				GetDeaths( int iPlayerID );
	int				GetAssists( int iPlayerID );
	void			IncrementKills( int iPlayerID );
	void			IncrementDeaths( int iPlayerID );
	void			IncrementAssists( int iPlayerID );

private:
	// Player ID arrays (matches decompiled 0x8b8-0x8dc and 0x1020-0x1044 ranges)
	// TODO: Convert to CNetworkVar when networking system is ready
	int				m_iPlayerIDs1[MAX_DOTA_PLAYERS];		// 0x8b8-0x8dc area (10 * 4 bytes)
	int				m_iPlayerIDs2[MAX_DOTA_PLAYERS];		// 0x1020-0x1044 area (10 * 4 bytes)

	// Player data arrays (matches decompiled 0x1a54-0x1b40 range)
	int				m_iPlayerTeams[MAX_DOTA_PLAYERS];		// 0x1a54+ Player team assignments
	int				m_iPlayerLevels[MAX_DOTA_PLAYERS];		// Player hero levels
	int				m_iPlayerGold[MAX_DOTA_PLAYERS];		// Player gold amounts
	int				m_iPlayerKills[MAX_DOTA_PLAYERS];		// Player kill counts
	int				m_iPlayerDeaths[MAX_DOTA_PLAYERS];		// Player death counts
	int				m_iPlayerAssists[MAX_DOTA_PLAYERS];		// Player assist counts
	int				m_iPlayerDenies[MAX_DOTA_PLAYERS];		// Player deny counts
	int				m_iPlayerLastHits[MAX_DOTA_PLAYERS];	// Player last hit counts
	int				m_iPlayerNetWorth[MAX_DOTA_PLAYERS];	// Player net worth
	int				m_iPlayerHeroIDs[MAX_DOTA_PLAYERS];		// Selected hero IDs
	
	// Additional player data (matches remaining 0x1a54-0x1b40 variables)
	int				m_iPlayerData1[MAX_DOTA_PLAYERS];
	int				m_iPlayerData2[MAX_DOTA_PLAYERS];
	int				m_iPlayerData3[MAX_DOTA_PLAYERS];
	int				m_iPlayerData4[MAX_DOTA_PLAYERS];
	int				m_iPlayerData5[MAX_DOTA_PLAYERS];

	// Steam and connection data (matches decompiled 0x2cd4-0x2df8 range)
	unsigned long long		m_iPlayerSteamIDs[MAX_DOTA_PLAYERS];	// 0x2cd4+ Steam IDs (8 bytes each)
	int				m_iConnectionStates[MAX_DOTA_PLAYERS];	// Connection states
	int				m_iPlayerFlags[MAX_DOTA_PLAYERS];		// Player flags
	int				m_iPlayerColors[MAX_DOTA_PLAYERS];		// Player colors
	int				m_iPlayerSlots[MAX_DOTA_PLAYERS];		// Player slot assignments

	// Connection state data (matches decompiled offsets)
	int				m_iPlayerConnectionStates[32];			// 0xaa8+ Connection states (32 players max)
	int				m_iPlayerDisconnectReasons[32];			// 0x2788+ Disconnect reasons

	// Leaver state data (matches decompiled 0x2b38 + loop)
	CMsgLeaverState	m_LeaverStates[MAX_DOTA_PLAYERS];		// 0x2b38+ (0x1c * 10 = 0x118 bytes)

	// Player reservation state (matches decompiled 0x1b1c offset)
	bool			m_bPlayerReservedState[MAX_DOTA_PLAYERS];	// 0x1b1c+ Player reserved states
	
	// Selected hero data (matches decompiled offsets)
	EHANDLE			m_hSelectedHeroes[MAX_DOTA_PLAYERS];		// 0x8b8+ Selected hero entity handles
	int				m_iSelectedHeroIDs[MAX_DOTA_PLAYERS];		// 0x478+ Selected hero IDs
	char			m_szSelectedHeroNames[MAX_DOTA_PLAYERS][64];	// Selected hero names
	
	// Player names (matches decompiled 0x2e04 + bzero 200 bytes)
	char			m_szPlayerNames[MAX_DOTA_PLAYERS][20];	// 0x2e04+ (200 bytes total)

	// Dynamic data array (matches decompiled 0x5410 CUtlArray)
	CUtlVector<int>	m_PlayerDataArray;						// 0x5410+ Dynamic player data

	// Final data members (matches decompiled 0x55ac-0x55bc)
	int				m_iFinalData1;							// 0x55ac
	int				m_iFinalData2;							// 0x55b0
	int				m_iFinalData3;							// 0x55b4
	int				m_iFinalData4;							// 0x55b8
	int				m_iFinalData5;							// 0x55bc

private:
	// Helper functions
	bool			IsValidPlayerID( int iPlayerID );
	void			InitializePlayerData( int iPlayerID );
	void			ResetPlayerStats( int iPlayerID );
};

// Global access
extern CDOTA_PlayerResource* GetDOTAPlayerResource();

#endif // DOTA_PLAYERRESOURCE_H 