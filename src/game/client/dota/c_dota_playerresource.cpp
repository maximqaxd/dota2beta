//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side DOTA Player Resource implementation
//
//=============================================================================//

#include "cbase.h"
#include "c_dota_playerresource.h"
#include "c_dota_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global pointer to the client player resource
C_DOTA_PlayerResource *g_pDOTAPlayerResourceClient = NULL;

//=============================================================================
// Data Tables
//=============================================================================

IMPLEMENT_CLIENTCLASS_DT( C_DOTA_PlayerResource, DT_DOTA_PlayerResource, CDOTA_PlayerResource )
	// TODO: Add RecvPropArray entries when networking system is ready
	// For now, empty table to resolve DT class errors
END_RECV_TABLE()

//=============================================================================
// C_DOTA_PlayerResource implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_DOTA_PlayerResource::C_DOTA_PlayerResource()
{
	// Set global pointer
	g_pDOTAPlayerResourceClient = this;
	
	// Initialize all player data
	for ( int i = 0; i < MAX_DOTA_PLAYERS; i++ )
	{
		m_iPlayerIDs[i] = -1;
		m_iPlayerTeams[i] = 0;
		m_iPlayerLevels[i] = 1;
		m_iGold[i] = 625; // Starting gold
		m_iKills[i] = 0;
		m_iDeaths[i] = 0;
		m_iAssists[i] = 0;
		m_iSelectedHeroIDs[i] = -1;
		m_iConnectionStates[i] = 0;
		m_bFullyJoinedServer[i] = false;
		m_szPlayerNames[i][0] = '\0';
	}
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
C_DOTA_PlayerResource::~C_DOTA_PlayerResource()
{
	// Clear global pointer
	if ( g_pDOTAPlayerResourceClient == this )
		g_pDOTAPlayerResourceClient = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Called when data changes on the server
//-----------------------------------------------------------------------------
void C_DOTA_PlayerResource::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Initialize client-side data when entity is created
	}
}

//-----------------------------------------------------------------------------
// Purpose: Client-side thinking
//-----------------------------------------------------------------------------
void C_DOTA_PlayerResource::ClientThink()
{
	BaseClass::ClientThink();
	
	// TODO: Update client-side player resource data
}

//-----------------------------------------------------------------------------
// Purpose: Get player's team
//-----------------------------------------------------------------------------
int C_DOTA_PlayerResource::GetPlayerTeam( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;
		
	return m_iPlayerTeams[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Is player fully joined to the server?
//-----------------------------------------------------------------------------
bool C_DOTA_PlayerResource::IsPlayerFullyJoined( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return false;
		
	return m_bFullyJoinedServer[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player's selected hero ID
//-----------------------------------------------------------------------------
int C_DOTA_PlayerResource::GetSelectedHeroID( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return -1;
		
	return m_iSelectedHeroIDs[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player's name
//-----------------------------------------------------------------------------
const char* C_DOTA_PlayerResource::GetPlayerName( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return "";
		
	return m_szPlayerNames[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player's kills
//-----------------------------------------------------------------------------
int C_DOTA_PlayerResource::GetKills( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;
		
	return m_iKills[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player's deaths
//-----------------------------------------------------------------------------
int C_DOTA_PlayerResource::GetDeaths( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;
		
	return m_iDeaths[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player's assists
//-----------------------------------------------------------------------------
int C_DOTA_PlayerResource::GetAssists( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;
		
	return m_iAssists[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Get player's connection state
//-----------------------------------------------------------------------------
int C_DOTA_PlayerResource::GetPlayerConnectionState( int iPlayerID )
{
	if ( !IsValidPlayerID( iPlayerID ) )
		return 0;
		
	return m_iConnectionStates[iPlayerID];
}

//-----------------------------------------------------------------------------
// Purpose: Is this a valid player ID?
//-----------------------------------------------------------------------------
bool C_DOTA_PlayerResource::IsValidPlayerID( int iPlayerID )
{
	return ( iPlayerID >= 0 && iPlayerID < MAX_DOTA_PLAYERS );
}

//-----------------------------------------------------------------------------
// Purpose: Get the local player's ID
//-----------------------------------------------------------------------------
int C_DOTA_PlayerResource::GetLocalPlayerID()
{
	C_DOTAPlayer *pLocalPlayer = dynamic_cast<C_DOTAPlayer*>( C_BasePlayer::GetLocalPlayer() );
	if ( pLocalPlayer )
	{
		return pLocalPlayer->GetPlayerID();
	}
	
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Global accessor function
//-----------------------------------------------------------------------------
C_DOTA_PlayerResource* GetDOTAPlayerResource()
{
	return g_pDOTAPlayerResourceClient;
} 