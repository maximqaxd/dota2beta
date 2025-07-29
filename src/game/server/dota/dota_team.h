//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: DOTA Team management
//
//==========================================================================//

#ifndef DOTA_TEAM_H
#define DOTA_TEAM_H

#ifdef _WIN32
#pragma once
#endif

#include "team.h"
#include "igameevents.h"

// Forward declarations
class CBasePlayer;
class CBaseEntity;
class CDOTAPlayer;
class CDOTALobby;
class IGameEvent;

//=============================================================================
// CDOTATeam - DOTA-specific team class
//=============================================================================
class CDOTATeam : public CTeam, public IGameEventListener2
{
	DECLARE_CLASS( CDOTATeam, CTeam );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:
	CDOTATeam();
	virtual ~CDOTATeam();

	// Team capacity management
	bool			IsTeamFull( void );

	// Kill tracking
	void			IncrementHeroKills( void );
	void			IncrementTowerKills( void );
	void			IncrementBarracksKills( void );

	// Network transmission
	virtual int		ShouldTransmitToPlayer( CBasePlayer *pPlayer, CBaseEntity *pEntity );

	// Game event handling
	virtual void	FireGameEvent( IGameEvent *event );

	// Lobby integration
	void			UpdateTeamNameFromLobby( void );

	// Access functions
	int				GetHeroKills( void ) const { return m_iHeroKills; }
	int				GetTowerKills( void ) const { return m_iTowerKills; }
	int				GetBarracksKills( void ) const { return m_iBarracksKills; }

private:
	// Kill counters (network variables when system is ready)
	// TODO: Convert to CNetworkVar when networking system is ready
	int				m_iHeroKills;			// 0x498 - Hero kills by this team
	int				m_iTowerKills;			// 0x49c - Tower kills by this team  
	int				m_iBarracksKills;		// 0x4a0 - Barracks kills by this team
	int				m_iTeamLogo;			// 0x4a4 - Team logo ID

	// Team visual data (from lobby)
	int				m_iPrimaryColor1;		// 0x4a8 - Primary color part 1
	int				m_iPrimaryColor2;		// 0x4ac - Primary color part 2
	int				m_iSecondaryColor1;		// 0x4b0 - Secondary color part 1
	int				m_iSecondaryColor2;		// 0x4b4 - Secondary color part 2
	int				m_iPatternColor1;		// 0x4b8 - Pattern color part 1
	int				m_iPatternColor2;		// 0x4bc - Pattern color part 2
	
	// Team flags and data
	bool			m_bTeamComplete;		// 0x4c0 - Team setup complete flag
	bool			m_bTeamReady;			// 0x4c1 - Team ready flag

	// Team strings
	char			m_szTeamTag[33];		// 0x4c2 - Team tag/abbreviation (33 bytes)
	char			m_szTeamName[129];		// 0x3f8 - Full team name (129 bytes)

	// Internal state
	bool			m_bInitialized;			// 0x494 - Initialization flag
	void*			m_pEventListener;		// 0x490 - Event listener vtable
	int				m_iLobbyEventID;		// 0x4e4 - "lobby_updated" event ID
	int				m_iTeamData;			// 0x4e8 - Additional team data
	int				m_iTeamNumber;			// 0x48c - Cached team number

	// Network change tracking values (for future CNetworkVar conversion)
	// These match the decompiled offset patterns for network variable management
	int				m_iNetworkChange1;		// Network change tracking
	int				m_iNetworkChange2;		// Network change tracking
	int				m_iNetworkChange3;		// Network change tracking
	int				m_iNetworkChange4;		// Network change tracking
	int				m_iNetworkChange5;		// Network change tracking
	int				m_iNetworkChange6;		// Network change tracking

private:
	// Helper functions
	void			RegisterForLobbyEvents( void );
	void			UnregisterFromLobbyEvents( void );
	void			ResetTeamVisuals( void );
};

// External functions
extern CDOTALobby* GetDOTALobby( void );
extern int GDOTAGCClientSystem( void );

#endif // DOTA_TEAM_H 