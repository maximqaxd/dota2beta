//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side DOTA Team implementation
//
//=============================================================================//

#include "cbase.h"
#include "c_dota_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// Data Tables
//=============================================================================

IMPLEMENT_CLIENTCLASS_DT( C_DOTATeam, DT_DOTATeam, CDOTATeam )
	// TODO: Add RecvPropInt entries when networking system is ready
	// For now, empty table to resolve DT class errors
END_RECV_TABLE()

//=============================================================================
// C_DOTATeam implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_DOTATeam::C_DOTATeam()
{
	// Initialize team statistics
	m_iHeroKills = 0;
	m_iTowerKills = 0;
	m_iBarracksKills = 0;
	m_iTeamLogo = 0;
	
	// Initialize visual data
	m_iPrimaryColor1 = 0;
	
	// Initialize status flags
	m_bTeamComplete = false;
	m_bTeamReady = false;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
C_DOTATeam::~C_DOTATeam()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when data changes on the server
//-----------------------------------------------------------------------------
void C_DOTATeam::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Initialize client-side data when entity is created
	}
	else if ( updateType == DATA_UPDATE_DATATABLE_CHANGED )
	{
		// Handle team data updates (scores, status changes, etc.)
	}
} 