//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client-side DOTA Team
//
//=============================================================================//

#ifndef C_DOTA_TEAM_H
#define C_DOTA_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_team.h"
#include "networkvar.h"

class C_DOTATeam : public C_Team
{
public:
	DECLARE_CLASS( C_DOTATeam, C_Team );
	DECLARE_CLIENTCLASS();

	C_DOTATeam();
	virtual ~C_DOTATeam();

	// Client-side overrides
	virtual void OnDataChanged( DataUpdateType_t updateType );

	// Team statistics accessors
	int GetHeroKills() const { return m_iHeroKills; }
	int GetTowerKills() const { return m_iTowerKills; }
	int GetBarracksKills() const { return m_iBarracksKills; }

	// Team visual data
	int GetTeamLogo() const { return m_iTeamLogo; }
	int GetPrimaryColor() const { return m_iPrimaryColor1; }
	
	// Team status
	bool IsTeamComplete() const { return m_bTeamComplete; }
	bool IsTeamReady() const { return m_bTeamReady; }

private:
	// Networked team statistics
	// TODO: Add CNetworkVar declarations when networking system is ready
	int m_iHeroKills;
	int m_iTowerKills;
	int m_iBarracksKills;
	int m_iTeamLogo;
	
	// Team visual data
	int m_iPrimaryColor1;
	
	// Team status flags
	bool m_bTeamComplete;
	bool m_bTeamReady;
};

#endif // C_DOTA_TEAM_H 