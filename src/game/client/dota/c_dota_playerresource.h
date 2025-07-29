//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side DOTA Player Resource
//
//=============================================================================//

#ifndef C_DOTA_PLAYERRESOURCE_H
#define C_DOTA_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "networkvar.h"

#define MAX_DOTA_PLAYERS 10

class C_DOTA_PlayerResource : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_DOTA_PlayerResource, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_DOTA_PlayerResource();
	virtual ~C_DOTA_PlayerResource();

	// Client-side overrides
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ClientThink();

	// Player data accessors
	int GetPlayerTeam( int iPlayerID );
	bool IsPlayerFullyJoined( int iPlayerID );
	int GetSelectedHeroID( int iPlayerID );
	const char* GetPlayerName( int iPlayerID );
	int GetKills( int iPlayerID );
	int GetDeaths( int iPlayerID );
	int GetAssists( int iPlayerID );
	int GetPlayerConnectionState( int iPlayerID );

	// Utility functions
	bool IsValidPlayerID( int iPlayerID );
	int GetLocalPlayerID();

private:
	// Player data arrays (networked from server)
	// TODO: Add CNetworkArray declarations when networking system is ready
	int m_iPlayerIDs[MAX_DOTA_PLAYERS];
	int m_iPlayerTeams[MAX_DOTA_PLAYERS];
	int m_iPlayerLevels[MAX_DOTA_PLAYERS];
	int m_iGold[MAX_DOTA_PLAYERS];
	int m_iKills[MAX_DOTA_PLAYERS];
	int m_iDeaths[MAX_DOTA_PLAYERS];
	int m_iAssists[MAX_DOTA_PLAYERS];
	int m_iSelectedHeroIDs[MAX_DOTA_PLAYERS];
	int m_iConnectionStates[MAX_DOTA_PLAYERS];
	bool m_bFullyJoinedServer[MAX_DOTA_PLAYERS];
	
	// Player names (simplified)
	char m_szPlayerNames[MAX_DOTA_PLAYERS][64];
};

// Global accessor
extern C_DOTA_PlayerResource* GetDOTAPlayerResource();

#endif // C_DOTA_PLAYERRESOURCE_H 