//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side DOTA Player implementation
//
//=============================================================================//

#include "cbase.h"
#include "c_dota_player.h"
#include "engine/ivdebugoverlay.h"
#include "dt_recv.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// Data Tables
//=============================================================================

IMPLEMENT_CLIENTCLASS_DT( C_DOTAPlayer, DT_DOTAPlayer, CDOTAPlayer )
	// Core player identification
	RecvPropInt( RECVINFO( m_nPlayerID ) ),
	
	// Player flags and state
	RecvPropBool( RECVINFO( m_bDOTAPlayerFlag1 ) ),
	RecvPropBool( RECVINFO( m_bDOTAPlayerFlag2 ) ),
	RecvPropBool( RECVINFO( m_bDOTAPlayerFlag3 ) ),
	RecvPropBool( RECVINFO( m_bDOTANetworkFlag ) ),
	RecvPropBool( RECVINFO( m_bWantsRandomHero ) ),
	
	// Player values and stats
	RecvPropFloat( RECVINFO( m_flDOTAPlayerValue1 ) ),
	RecvPropInt( RECVINFO( m_nDOTAPlayerValue1 ) ),
	RecvPropInt( RECVINFO( m_nDOTAPlayerValue2 ) ),
	RecvPropInt( RECVINFO( m_nPlayerStat1 ) ),
	RecvPropInt( RECVINFO( m_nPlayerStat2 ) ),
	RecvPropInt( RECVINFO( m_nPlayerStat3 ) ),
	RecvPropInt( RECVINFO( m_nPlayerStat4 ) ),
	RecvPropInt( RECVINFO( m_iShopViewMode ) ),
	
	// Entity handles
	RecvPropEHandle( RECVINFO( m_hAssignedHero ) ),
	RecvPropEHandle( RECVINFO( m_hDOTAPlayerEntity ) ),
	
	// Timing information
	RecvPropFloat( RECVINFO( m_flLastActivityTime ) ),
	RecvPropFloat( RECVINFO( m_flLastOrderTime ) ),
	RecvPropFloat( RECVINFO( m_flIdleTime ) ),
	
	// Additional DOTA values
	RecvPropInt( RECVINFO( m_nDOTAValue1 ) ),
	RecvPropInt( RECVINFO( m_nDOTAValue2 ) ),
	RecvPropInt( RECVINFO( m_nDOTAValue3 ) ),
	RecvPropInt( RECVINFO( m_iPickerEntity ) ),
END_RECV_TABLE()

//=============================================================================
// Prediction Data Tables
//=============================================================================

BEGIN_PREDICTION_DATA( C_DOTAPlayer )
	// TODO: Add prediction fields when needed
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(player, C_DOTAPlayer);


//=============================================================================
// C_DOTAPlayer implementation
//=============================================================================

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_DOTAPlayer::C_DOTAPlayer()
{
	m_nPlayerID = -1;
	m_bDataChanged = false;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
C_DOTAPlayer::~C_DOTAPlayer()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when data changes on the server
//-----------------------------------------------------------------------------
void C_DOTAPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );
	
	if ( updateType == DATA_UPDATE_CREATED )
	{
		// Initialize client-side data when entity is created
		m_bDataChanged = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Client-side thinking
//-----------------------------------------------------------------------------
void C_DOTAPlayer::ClientThink()
{
	BaseClass::ClientThink();
	
	// TODO: Add DOTA-specific client thinking
}

//-----------------------------------------------------------------------------
// Purpose: Should this player be drawn?
//-----------------------------------------------------------------------------
bool C_DOTAPlayer::ShouldDraw()
{
	// Don't draw local player in first person
	if ( IsLocalPlayer())
		return false;
		
	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Is this the local player?
//-----------------------------------------------------------------------------
bool C_DOTAPlayer::IsLocalPlayer()
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	return ( pLocalPlayer == this );
} 


C_DOTAPlayer* C_DOTAPlayer::GetLocalPlayer(int nSlot)
{
	return dynamic_cast<C_DOTAPlayer*>(C_BasePlayer::GetLocalPlayer());
}