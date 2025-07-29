//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side DOTA Player
//
//=============================================================================//

#ifndef C_DOTA_PLAYER_H
#define C_DOTA_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseplayer.h"
#include "networkvar.h"

class C_DOTAPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_DOTAPlayer, C_BasePlayer);
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_DOTAPlayer();
	virtual ~C_DOTAPlayer();

	// Client-side overrides
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void ClientThink();
	virtual bool ShouldDraw();

	// DOTA-specific client functions
	int GetPlayerID() const { return m_nPlayerID; }
	bool IsLocalPlayer();
	static C_DOTAPlayer* GetLocalPlayer(int nSlot = -1);


private:
	// Networked variables (matches server-side DT_DOTAPlayer)
	CNetworkVar( int, m_nPlayerID );
	
	// Player flags and state
	CNetworkVar( bool, m_bDOTAPlayerFlag1 );
	CNetworkVar( bool, m_bDOTAPlayerFlag2 );
	CNetworkVar( bool, m_bDOTAPlayerFlag3 );
	CNetworkVar( bool, m_bDOTANetworkFlag );
	CNetworkVar( bool, m_bWantsRandomHero );
	
	// Player values and stats
	CNetworkVar( float, m_flDOTAPlayerValue1 );
	CNetworkVar( int, m_nDOTAPlayerValue1 );
	CNetworkVar( int, m_nDOTAPlayerValue2 );
	CNetworkVar( int, m_nPlayerStat1 );
	CNetworkVar( int, m_nPlayerStat2 );
	CNetworkVar( int, m_nPlayerStat3 );
	CNetworkVar( int, m_nPlayerStat4 );
	CNetworkVar( int, m_iShopViewMode );
	
	// Entity handles
	CNetworkVar( EHANDLE, m_hAssignedHero );
	CNetworkVar( EHANDLE, m_hDOTAPlayerEntity );
	
	// Timing information
	CNetworkVar( float, m_flLastActivityTime );
	CNetworkVar( float, m_flLastOrderTime );
	CNetworkVar( float, m_flIdleTime );
	
	// Additional DOTA values
	CNetworkVar( int, m_nDOTAValue1 );
	CNetworkVar( int, m_nDOTAValue2 );
	CNetworkVar( int, m_nDOTAValue3 );
	CNetworkVar( int, m_iPickerEntity );
	
	// Client-only data
	bool m_bDataChanged;
};

inline C_DOTAPlayer* To_DOTAPlayer(CBaseEntity* pEntity)
{
	if (!pEntity)
		return NULL;
	Assert(dynamic_cast<C_DOTAPlayer*>(pEntity) != NULL);
	return static_cast<C_DOTAPlayer*>(pEntity);
}

#endif // C_DOTA_PLAYER_H 