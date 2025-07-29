//====== Copyright © 1996-2011, Valve Corporation, All rights reserved. =======
//
// Purpose: DOTA Player - Server Implementation
//
//=============================================================================

#ifndef DOTA_PLAYER_H
#define DOTA_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "basemultiplayerplayer.h"
#include "ehandle.h"
#include "networkvar.h"
#include "baseentity.h"

// Forward declarations
class CDOTAPlayerInventory;
class CDOTATeam;
class CBaseEntity;

//=============================================================================
// CDOTAPlayerInventory - Player's item inventory system
//=============================================================================
class CDOTAPlayerInventory
{
public:
	CDOTAPlayerInventory();
	~CDOTAPlayerInventory();

	// TODO: Implement inventory functionality when item system is ready

private:
	// Inventory data will be added here
	char m_InventoryData[256]; // Placeholder for inventory data
};

//=============================================================================
// CDOTAPlayer - Server-side player entity for DOTA
//=============================================================================
class CDOTAPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS( CDOTAPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CDOTAPlayer();
	virtual ~CDOTAPlayer();

	// Spawn and precache
	virtual void	Spawn( void );
	virtual void	Precache( void );

	// Player state management
	virtual void	PlayerDeathThink( void );
	virtual void	PreThink( void );
	virtual void	Think( void );
	virtual void	PostThink( void );

	// Team management
	virtual void	ChangeTeam( int iTeamNum );

	// Spawn and initialization
	virtual void	InitialSpawn( void );

	// Player events
	virtual void	Event_Killed( const CTakeDamageInfo &info );

	// Client command handling
	virtual bool	ClientCommand( const CCommand &args );

	// DOTA-specific functions
	CDOTAPlayerInventory*	GetInventory() { return &m_Inventory; }
	float			GetIdleTime( void );
	void			OnReconnect( void );
	int				GetPlayerID( void ) const;

	// DOTA-specific functions (from decompiled code)
	bool			IsFountainIdle( bool bLenient );
	bool			HasSelectedHero( void );
	void			PhysicsSimulate( void );
	void			SetAssignedHero( CBaseEntity *pHero );
	void			SetShopViewMode( int viewMode );
	bool			WantsRandomHero( void );
	void			SetLastOrderTime( float flTime, bool bUpdateIdleTime );
	void			UndoHeroSelection( void );
	void			SetWantsRandomHero( void );
	void			HandleHeroSelection( const CCommand &args );
	void			SetWantsSpecificHero( const char *pHeroName );
	void			MakeRandomHeroSelection( bool bBotGame, int iRandomSeed );
	bool			IsAFK( void );

	// Static functions
	static CDOTAPlayer *CreatePlayer( const char *className, edict_t *ed )
	{
		CDOTAPlayer::s_PlayerEdict = ed;
		return (CDOTAPlayer*)CreateEntityByName( className );
	}
	static CDOTAPlayer*	GetPlayerByPlayerID( int iPlayerID );


private:
	void			UpdatePlayerIDFromSteamID( void );

private:
	// Network variables (based on decompiled offsets)
	// 0x1338-0x1348 region
	CNetworkVar( bool, m_bDOTAPlayerFlag1 );		// 0x1338
	CNetworkVar( bool, m_bDOTAPlayerFlag2 );		// 0x1339  
	CNetworkVar( float, m_flDOTAPlayerValue1 );		// 0x133c (initialized to 0.5)
	CNetworkVar( int, m_nDOTAPlayerValue1 );		// 0x1340
	CNetworkVar( bool, m_bDOTAPlayerFlag3 );		// 0x1344
	CNetworkVar( int, m_nDOTAPlayerValue2 );		// 0x134c

	// 0x12dc-0x12f4 region - various player stats/values
	CNetworkVar( int, m_nPlayerStat1 );			// 0x12dc
	CNetworkVar( int, m_nPlayerStat2 );			// 0x12e0  
	CNetworkVar( int, m_nPlayerStat3 );			// 0x12e4
	CNetworkVar( int, m_nPlayerStat4 );			// 0x12e8 (initialized to 0xf)
	int					m_nPlayerStat5;			// 0x12ec (not networked)
	int					m_nPlayerStat6;			// 0x12f0 (not networked)
	int					m_nPlayerStat7;			// 0x12f4 (not networked)

	// 0x136c-0x1378 region - network handle and values
	CNetworkVar( EHANDLE, m_hDOTAPlayerEntity );		// 0x136c area
	CNetworkVar( bool, m_bDOTANetworkFlag );		// 0x1378

	// 0x1380-0x1394 region - additional player data
	CNetworkVar( int, m_nDOTAValue1 );			// 0x1380
	CNetworkVar( int, m_nDOTAValue2 );			// 0x1384  
	CNetworkVar( int, m_nDOTAValue3 );			// 0x138c
	int					m_nDOTAValue4;			// 0x1390 (not networked)
	int					m_nDOTAValue5;			// 0x1394 (not networked)

	// Other network variables
	int					m_nSpecialValue1;		// 0x13a8
	int					m_nSpecialValue2;		// 0x13ac (initialized to -1)
	int					m_nSpecialValue3;		// 0x13d4

	// Entity handles and references  
	EHANDLE				m_hPlayerEntity;		// 0x137c area

	// Additional data members
	int					m_nDataValue1;			// 0x13c0
	int					m_nDataValue2;			// 0x13c4
	short				m_nDataValue3;			// 0x13c8
	CNetworkVar( int, m_nPlayerID );			// 0x13cc (initialized to -1)
	int					m_nDataValue4;			// 0x13d8
	int					m_nDataValue5;			// 0x13dc
	
	// Additional timing members from decompiled code
	CNetworkVar( float, m_flLastActivityTime );	// 0x13bc - for idle time calculation
	float				m_flReconnectTime;		// 0x13c4 - reconnection time (not networked)

	// Player inventory (at offset 0x13e0)
	CDOTAPlayerInventory	m_Inventory;

	// Additional member variables
	int						m_nExtraValue1;			// 0x1490
	int						m_nExtraValue2;			// 0x1494
	
	// DOTA-specific member variables (from decompiled offsets)
	CNetworkVar( int, m_iPickerEntity );			// 0x13a4 Picker entity index
	CNetworkVar( EHANDLE, m_hAssignedHero );		// 0x13a8 Handle to assigned hero  
	CNetworkVar( float, m_flLastOrderTime );		// 0x13bc Last order time
	CNetworkVar( float, m_flIdleTime );				// 0x13c0 Idle time tracker
	CNetworkVar( bool, m_bWantsRandomHero );		// 0x13c8 Wants random hero flag
	CNetworkVar( int, m_iShopViewMode );			// 0x12f4 Shop view mode
};


#endif // DOTA_PLAYER_H 