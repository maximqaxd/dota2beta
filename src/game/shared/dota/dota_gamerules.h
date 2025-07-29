//====== Copyright © 1996-2011, Valve Corporation, All rights reserved. =======
//
// Purpose: DOTA Game Rules - Shared between client and server
//
//=============================================================================

#ifndef DOTA_GAMERULES_H
#define DOTA_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "teamplay_gamerules.h"
#include "convar.h"
#include "gameeventlistener.h"
#include "utlvector.h"
#include "ehandle.h"

#ifdef CLIENT_DLL
	// #include "c_user_message_register.h"  // TODO: Add when client message system is implemented
	#define CDOTAGameRules C_DOTAGameRules
	#define CDOTAGameRulesProxy C_DOTAGameRulesProxy
#else
	// #include "user_message_register.h"   // TODO: Add when server message system is implemented  
	// #include "steamcallbacks.h"          // TODO: Add when Steam integration is implemented
#endif

// Forward declarations
class CBaseEntity;

// Forward declarations
class CBasePlayer;
class CBaseEntity;
class CDOTATeam;

// Team constants
#define DOTA_TEAM_SPECTATOR		-1
//=============================================================================
// MOVE THIS BELOW INTO DOTA_SHAREDDEFS.H!
//=============================================================================
// Hero pick states
enum DOTA_HeroPickState_t
{
	DOTA_HEROPICK_STATE_NONE = 0,
	DOTA_HEROPICK_STATE_AP_SELECT = 1,
	DOTA_HEROPICK_STATE_SD_SELECT = 2,
	DOTA_HEROPICK_STATE_INTRO_SELECT_UNUSED = 3,
	DOTA_HEROPICK_STATE_RD_SELECT_UNUSED = 4,
	DOTA_HEROPICK_STATE_CM_INTRO = 5,
	DOTA_HEROPICK_STATE_CM_CAPTAINPICK = 6,
	DOTA_HEROPICK_STATE_CM_BAN1 = 7,
	DOTA_HEROPICK_STATE_CM_BAN2 = 8,
	DOTA_HEROPICK_STATE_CM_BAN3 = 9,
	DOTA_HEROPICK_STATE_CM_BAN4 = 10,
	DOTA_HEROPICK_STATE_CM_BAN5 = 11,
	DOTA_HEROPICK_STATE_CM_BAN6 = 12,
	DOTA_HEROPICK_STATE_CM_BAN7 = 13,
	DOTA_HEROPICK_STATE_CM_BAN8 = 14,
	DOTA_HEROPICK_STATE_CM_BAN9 = 15,
	DOTA_HEROPICK_STATE_CM_BAN10 = 16,
	DOTA_HEROPICK_STATE_CM_BAN11 = 17,
	DOTA_HEROPICK_STATE_CM_BAN12 = 18,
	DOTA_HEROPICK_STATE_CM_BAN13 = 19,
	DOTA_HEROPICK_STATE_CM_BAN14 = 20,
	DOTA_HEROPICK_STATE_CM_SELECT1 = 21,
	DOTA_HEROPICK_STATE_CM_SELECT2 = 22,
	DOTA_HEROPICK_STATE_CM_SELECT3 = 23,
	DOTA_HEROPICK_STATE_CM_SELECT4 = 24,
	DOTA_HEROPICK_STATE_CM_SELECT5 = 25,
	DOTA_HEROPICK_STATE_CM_SELECT6 = 26,
	DOTA_HEROPICK_STATE_CM_SELECT7 = 27,
	DOTA_HEROPICK_STATE_CM_SELECT8 = 28,
	DOTA_HEROPICK_STATE_CM_SELECT9 = 29,
	DOTA_HEROPICK_STATE_CM_SELECT10 = 30,
	DOTA_HEROPICK_STATE_CM_PICK = 31,
	DOTA_HEROPICK_STATE_AR_SELECT = 32,
	DOTA_HEROPICK_STATE_MO_SELECT = 33,
	DOTA_HEROPICK_STATE_FH_SELECT = 34,
	DOTA_HEROPICK_STATE_CD_INTRO = 35,
	DOTA_HEROPICK_STATE_CD_CAPTAINPICK = 36,
	DOTA_HEROPICK_STATE_CD_BAN1 = 37,
	DOTA_HEROPICK_STATE_CD_BAN2 = 38,
	DOTA_HEROPICK_STATE_CD_BAN3 = 39,
	DOTA_HEROPICK_STATE_CD_BAN4 = 40,
	DOTA_HEROPICK_STATE_CD_BAN5 = 41,
	DOTA_HEROPICK_STATE_CD_BAN6 = 42,
	DOTA_HEROPICK_STATE_CD_SELECT1 = 43,
	DOTA_HEROPICK_STATE_CD_SELECT2 = 44,
	DOTA_HEROPICK_STATE_CD_SELECT3 = 45,
	DOTA_HEROPICK_STATE_CD_SELECT4 = 46,
	DOTA_HEROPICK_STATE_CD_SELECT5 = 47,
	DOTA_HEROPICK_STATE_CD_SELECT6 = 48,
	DOTA_HEROPICK_STATE_CD_SELECT7 = 49,
	DOTA_HEROPICK_STATE_CD_SELECT8 = 50,
	DOTA_HEROPICK_STATE_CD_SELECT9 = 51,
	DOTA_HEROPICK_STATE_CD_SELECT10 = 52,
	DOTA_HEROPICK_STATE_CD_PICK = 53,
	DOTA_HEROPICK_STATE_BD_SELECT = 54,
	DOTA_HERO_PICK_STATE_ABILITY_DRAFT_SELECT = 55,
	DOTA_HEROPICK_STATE_ARDM_SELECT = 56,
	DOTA_HEROPICK_STATE_ALL_DRAFT_SELECT = 57,
	DOTA_HERO_PICK_STATE_CUSTOMGAME_SELECT = 58,
	DOTA_HEROPICK_STATE_SELECT_PENALTY = 59,
	DOTA_HEROPICK_STATE_CUSTOM_PICK_RULES = 60,
	DOTA_HEROPICK_STATE_SCENARIO_PICK = 61,
	DOTA_HEROPICK_STATE_COUNT = 62
};

enum DOTA_SHOP_TYPE
{
	DOTA_SHOP_HOME = 0,
	DOTA_SHOP_SIDE = 1,
	DOTA_SHOP_SECRET = 2,
	DOTA_SHOP_GROUND = 3,
	DOTA_SHOP_SIDE2 = 4,
	DOTA_SHOP_SECRET2 = 5,
	DOTA_SHOP_CUSTOM = 6,
	DOTA_SHOP_NEUTRALS = 7,
	DOTA_SHOP_NONE = 8
};

// TODO: Remove when proper client message system is implemented
struct CUserMessageBuffer;

//=============================================================================
// DOTA Team Constants
//=============================================================================
#define DOTA_TEAM_RADIANT		2
#define DOTA_TEAM_DIRE			3

//=============================================================================
// DOTA Game States
//=============================================================================
enum DOTA_GameState_t
{
	DOTA_GAMERULES_STATE_INIT = 0,
	DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS,
	DOTA_GAMERULES_STATE_HERO_SELECTION,
	DOTA_GAMERULES_STATE_STRATEGY_TIME,
	DOTA_GAMERULES_STATE_PRE_GAME,
	DOTA_GAMERULES_STATE_GAME_IN_PROGRESS,
	DOTA_GAMERULES_STATE_POST_GAME,
	DOTA_GAMERULES_STATE_DISCONNECT,
	DOTA_GAMERULES_STATE_TEAM_SHOWCASE,
	DOTA_GAMERULES_STATE_CUSTOM_GAME_SETUP,
	DOTA_GAMERULES_STATE_LAST
};

//=============================================================================
// DOTA Game Rules Proxy - Network entity for syncing game rules
//=============================================================================
class CDOTAGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CDOTAGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

//=============================================================================
// DOTA Game Rules - Main game logic controller
//=============================================================================
class CDOTAGameRules : public CTeamplayRules, public CGameEventListener
{
public:
	DECLARE_CLASS( CDOTAGameRules, CTeamplayRules );

	CDOTAGameRules();
	virtual ~CDOTAGameRules();

	// CGameRules overrides
	virtual void		Precache( void );
	virtual bool		ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool		IsMultiplayer( void ) { return true; }
	virtual bool		IsTeamplay( void ) { return true; }
	virtual const char* GetGameDescription( void ) { return "DOTA 2"; }
	virtual void		Think( void );
	// virtual bool		ClientCommand( CBaseEntity *pEdict, const CCommand &args ); // TODO: Implement when needed

	// Team management
	virtual int			GetTeamIndex( const char *pTeamName );
	virtual const char* GetIndexedTeamName( int teamIndex );
	virtual bool		IsValidTeam( const char *pTeamName );
	virtual void		ChangePlayerTeam( CBasePlayer *pPlayer, int iTeamNum, bool bKill, bool bGib ){}

	// Player spawning (TODO: Implement when player system is ready)
	virtual void		PlayerSpawn( CBasePlayer *pPlayer );
	virtual void		PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	// virtual bool		ShouldAutoAim( CBaseEntity *pTarget, CBaseEntity *pShooter );

	// Game state management
	DOTA_GameState_t	GetGameState( void ) const { return m_nGameState; }
	void				SetGameState( DOTA_GameState_t nState );
	bool				IsGameInProgress( void ) const { return m_nGameState == DOTA_GAMERULES_STATE_GAME_IN_PROGRESS; }

	// Match timing
	float				GetGameTime( void ) const { return m_flGameStartTime > 0 ? gpGlobals->curtime - m_flGameStartTime : 0.0f; }
	void				SetGameStartTime( float flTime ) { m_flGameStartTime = flTime; }
	
	// DOTA-specific time calculation (from decompiled code)
	float				GetDOTATime( bool bIncludePregameTime = false, bool bExcludePausedTime = false, float flCurrentTime = 0.0f );
	
	// Additional DOTA functions (from decompiled code)
	class CBaseEntity*	GetFountain( int nTeam );
	float				GetGameTime();
	float				GetTransitionTime();
	float				GetCurrentTime();
	void				State_Enter( DOTA_GameState_t newState );
	void				State_Transition( DOTA_GameState_t newState );
	void				UpdateRunes();
	void				AddHeroDeath( int nPlayerID, int nKillerID, float flTime );
	class CBaseEntity*	GetAnnouncer( int nTeam );
	int					GetNumTowers();
	class CBaseEntity*	GetTeamTower( int nTeam, int nTowerIndex );
	void				MakeTeamLose( int nTeam );
	uint32				GetBarracksStatus( int nTeam );
	class CBaseEntity*	GetGameModeEntity( void );
	void				SetTimeOfDay( float flTimeOfDay );

	// Game control functions (from decompiled code)
	void				ResetDefeated( void );
	void				SetGamePaused( bool bPaused, int nPlayerID = -1, float flPauseDuration = -1.0f, float flAutoUnpauseDuration = -1.0f );
	void				SetGameWinner( int nWinningTeam );
	void				ForceGameStart( void );

	// Tower and building management
	unsigned int		GetTowerStatus( int nTeam );
	int					GetNumTeamTowers( int nTeam );
	class CBaseEntity*	GetTeamBarracks( int nTeam, int nBarracksIndex );

	// Game configuration
	void				SetGoldPerTick( int nGold );
	void				SetGoldTickTime( float flTime );
	void				SetPreGameTime( float flTime );
	void				SetPostGameTime( float flTime );
	void				SetRuneSpawnTime( float flTime );
	void				SetSafeToLeave( bool bSafe );
	void				TriggerResetMap( int nDelay );

	// Glyph system
	float				GetGlyphCooldown( int nTeam );
	void				SetGlyphCooldown( int nTeam, float flCooldown );

	// Item system
	int					NumDroppedItems( void );
	int					GetItemStockCount( int nTeam, const char *pszItemName );
	int					GetItemStockCount( int nTeam, uint16 nAbilityIndex );
	void				IncreaseItemStock( int nTeam, const char *pszItemName, int nIncrease );
	
	// Resource management
	void				PrecacheResources( void );
	void				SetTreeRegrowTime( float flTime );
	
	// Position and world management
	int					CategorizePosition( const Vector &vPosition );
	void				PostSpawnGroupLoad( void *pEvent );
	void				SetFirstTowerState( void );
	
	// Player and connection management  
	void				ClientDisconnected( int clientIndex, int nReason );
	void				DeliverXPToPlayers( void );
	const char*			GetLastUsedAbility( int nPlayerID );
	void				SetLastUsedAbility( void *pAbility );
#ifndef CLIENT_DLL
	// Damage system
	bool				Damage_IsTimeBased( int nDamageType );
	
	// Item stock management
	void				CreateItemStockInfo( int nTeam, const char *pszItemName, int nStockCount, int nMaxStock, float flStockTime );
	void				LoadStockInformation( KeyValues *pKV );
	
	// Fountain system
	bool				IsInRangeOfFountain( int nTeam, const Vector &vPosition );
	bool				IsInRangeOfFountain( CBaseEntity *pNPC );
	
	// Hero management
	void				ResetSelectedHeroes( void );
	void				ResetToHeroSelection( void );
	void				SetHeroSelectionTime( float flTime );
	
	// Game mechanics
	void				SetFirstBloodActive( bool bActive );
	bool				ShouldTimeoutClient( int nUserID, float flTime );
	void				AddCreepUpgradeState( int nUpgradeFlags );
	bool				GetCreepUpgradeState( int nUpgradeFlags );
	void				DeliverGoldToPlayers( void );
	// Connection handling
	virtual bool		ClientConnected( edict_t *pEdict, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
#endif
	// State management
	void				State_Enter_INIT( void );
	void				State_Leave_INIT( void );
	void				State_Think_INIT( void );
	void				State_Enter_PRE_GAME( void );
	void				State_Think_PRE_GAME( void );

	// Team composition analysis
	void				AddTeamCompToCriteria( int nTeam, void *pCriteriaSet );

	// Hero selection
	int					SelectSmartRandomHero( int nPlayerID, bool bRequireBotImplementation, void *pExcludeList, bool bUseSmartList );
	void				SetHeroRespawnEnabled( bool bEnabled );

	// Post-game state management
	void				State_Enter_POST_GAME( void );
	void				State_Think_POST_GAME( void );

	// Entity creation
	void				CreateStandardEntities( void );

	// Match management
	bool				DidMatchSignoutTimeOut( void );
	int					GetActiveRuneAtSpawner( int nSpawnerIndex );
	void				GetSmartRandomHeroList( int nPlayerID, bool bRequireBotImplementation, 
											   void *pHeroList, void *pExcludeList, bool bUseSmartList );
	void				OnNumSpectatorsChanged( int nNewSpectatorCount );

	// Rules and configuration
	bool				Rules_DisableAFKChecks( void );
	void				SetHeroMinimapIconSize( int nIconSize );
	void				StartItemRestockTimers( void );

	// Disconnect state management
	void				State_Enter_DISCONNECT( void );
	void				State_Think_DISCONNECT( void );

	// Match and selection management
	bool				GetMatchSignoutComplete( void );
	void				LeaveGameSelectionStage( void );
	void				SetMinimapDebugGridData( unsigned char *pGridData );
	void				SetRuneMinimapIconScale( float flScale );
	void				SetSelectionStageEntity( CBaseEntity *pEntity );
	void				*GetLastUsedActiveAbility( int nPlayerID );
	void				HeroPickState_Transition( DOTA_HeroPickState_t nNewState );

	// Creep and minimap management
	void				RegisterEnemyCreepInBase( CBaseEntity *pCreep );
	void				SetCreepMinimapIconScale( float flScale );

	// Automatic surrender system
	void				UpdateAutomaticSurrender( void );

	// Game selection stage
	void				UpdateGameSelectionStage( void );

	// Announcer and team management
	CBaseEntity			*GetAnnouncer_KillingSpree( int nTeam );
	bool				GetTeamHasAbandonedPlayer( int nTeam );
	void				PlayGameStartAnnouncement( void );

	// Resource management
	void				PrecacheResourcesFromKeys( KeyValues *pKV, void *pManifest, bool bIsFolder );

	// Additional state management
	void				State_Enter_STRATEGY_TIME( void );
	void				State_Enter_TEAM_SHOWCASE( void );
	void				State_Leave_STRATEGY_TIME( void );
	void				State_Leave_TEAM_SHOWCASE( void );
	void				State_Think_STRATEGY_TIME( void );
	void				State_Think_TEAM_SHOWCASE( void );

	// Position and creep management
	bool				IsPointInBadResultPosition( const Vector &vPosition );
	bool				Rules_DisableCreepSpawning( void );

	// Hero selection state management
	void				State_Enter_HERO_SELECTION( void );
	void				State_Leave_HERO_SELECTION( void );
	void				State_Think_HERO_SELECTION( void );

	// Announcer and projection systems
	void				QueueConceptForAllAnnouncers( int nConcept, float flDelay, void *pCriteriaSet, int nTeam, bool bForce );
	bool				Rules_ShouldUploadMatchStats( void );
	void				SendProjectionAbilityMessage( CBaseEntity *pCaster, Vector vPosition, bool bIsVisible, int nAbilityID, float flDelay, bool bIsProjected, CBaseEntity *pTarget );
	void				SetUseBaseGoldBountyOnHeroes( bool bUseBaseGold );

	// Game in progress state management
	void				State_Enter_GAME_IN_PROGRESS( void );
	void				State_Think_GAME_IN_PROGRESS( void );

	// Player management and communication
	void				DistributeAbandonedPlayerGold( void );
	void				*GetPlayerCommunicationSummary( int nPlayerID );
	void				FinalizePlayerCommunicationSummary( int nPlayerID );
	void				CheckIdleAndDisconnectedPlayers( void );

	// Projection system
	void				CheckPendingProjectionAbilities( void );
	bool				IsProjectionAbilityVisibleByEnemy( void *pProjectionAbility );
	void				SendProjectionAbilityMessageInternal( void *pProjectionAbility );
	bool				DoesRayIntersectBadResultPosition( const Ray_t &ray );

	// Hero pick state management
	void				HeroPickState_Enter_AP_SELECT( void );
	void				HeroPickState_Think_AP_SELECT( void );
	void				HeroPickState_Enter_INTRO_SELECT( void );
	void				HeroPickState_Think_INTRO_SELECT( void );
	bool				HeroPickState_IsPlayerIDInControl( int nPlayerID );

	// Wait for players state management
	void				State_Enter_WAIT_FOR_PLAYERS_TO_LOAD( void );
	void				State_Leave_WAIT_FOR_PLAYERS_TO_LOAD( void );
	void				State_Think_WAIT_FOR_PLAYERS_TO_LOAD( void );

	// Announcer system
	void				QueueConceptForAllAnnouncers_KillingSpree( int nConcept, float flDelay, void *pCriteriaSet, int nTeam, bool bForce );

	// Core game functions
	bool				Init( void );
	void				Glyph( int nTeam );
	
	// Additional methods from decompiled code
	void				Status( void (*pfnPrint)( const char*, ... ) );
	CBaseEntity*		GetShop( int nTeam, DOTA_SHOP_TYPE nShopType );
	void				Defeated( void );
	CBaseEntity*		GetTower( int nTowerIndex );
	void				Shutdown( void );
	CBaseEntity*		GetRoshan( void );
	void				SpawnRune( bool bForceBounty );
	int					State_Get( void );

	// Victory conditions
	virtual void		SetWinningTeam( int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false );
	virtual bool		CheckWinConditions( void );

	// Hero selection
	void				StartHeroSelection( void );
	void				EndHeroSelection( void );
	bool				IsInHeroSelection( void ) const { return m_nGameState == DOTA_GAMERULES_STATE_HERO_SELECTION; }

	// Pause system
	void				SetGamePaused( bool bPaused );
	bool				IsGamePaused( void ) const { return m_bGamePaused; }


	// Client message handlers (TODO: Implement when client message system is ready)
	// void				OnClientMsg_MapPing( int iPlayerID, const CUserMessageBuffer &buf );
	// void				OnClientMsg_MapLine( int iPlayerID, const CUserMessageBuffer &buf );
	// void				OnClientMsg_Pause( int iPlayerID, const CUserMessageBuffer &buf );
	// ... Additional client message handlers will be added

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();

	// Client-side specific (TODO: Implement when client networking is ready)
	// virtual void		OnDataChanged( DataUpdateType_t updateType );
	// virtual void		HandleOvertimeBegin( void );

#else
	DECLARE_SERVERCLASS_NOBASE();

	// Server-side specific
	virtual void		GoToIntermission( void );
	// TODO: Add FPlayerCanTakeDamage when CBaseEntity is available

	// Steam callbacks (TODO: Implement when Steam integration is ready)
	// STEAM_CALLBACK( CDOTAGameRules, Steam_OnClientKick, GSClientKick_t );
	// STEAM_CALLBACK( CDOTAGameRules, Steam_OnClientDeny, GSClientDeny_t );

	// Network vars (server only) - TODO: Implement when networking system is ready
	// CNetworkVar( DOTA_GameState_t, m_nGameState );
	// CNetworkVar( float, m_flGameStartTime );
	// CNetworkVar( bool, m_bGamePaused );
	// CNetworkVar( int, m_nPauseTeam );
	// CNetworkVar( bool, m_bHeroSelectionCompleted );

#endif

	// CGameEventListener
	virtual void		FireGameEvent( IGameEvent *event );

public: // Made public for testing access
	// Game state
	DOTA_GameState_t	m_nGameState;
	float				m_flGameStartTime;
	float				m_flStateTransitionTime;
	float				m_flGameTime;


	// Match settings
	bool				m_bGamePaused;
	int					m_nPauseTeam;
	float				m_flPauseStartTime;
	float				m_flPauseEndTime;
	float				m_flPauseTime;
	int					m_nPauseTeamID;
	int					m_nPausePlayerID;
	// Hero selection
	bool				m_bHeroSelectionCompleted;
	float				m_flHeroSelectionTime;
	
	// DOTA timing system (based on decompiled offsets)
	float				m_flPreGameStartTime;		// Time when pre-game started
	float				m_flGameStartTime2;			// Alternative game start time 
	float				m_flHornTime;				// Time when horn sounds (game actually starts)
	float				m_flPausedTimeAccumulated;	// Total time spent paused
	float				m_flLastPauseTime;			// When last pause started
	float				m_flTimeOfDay;				// Current in-game time of day

	// Entity tracking (from decompiled offsets)
	EHANDLE				m_hRadiantFountain;			// 0x105c - Radiant fountain entity
	EHANDLE				m_hDireFountain;			// 0x1060 - Dire fountain entity
	EHANDLE				m_hRadiantAnnouncer;		// 0x106c - Radiant announcer entity  
	EHANDLE				m_hDireAnnouncer;			// 0x1070 - Dire announcer entity
	EHANDLE				m_hGlobalAnnouncer;			// 0x1074 - Global announcer entity
	
	// Tower tracking
	int					m_nNumTowers;				// 0x175c - Total number of towers
	CUtlVector<EHANDLE>	m_hRadiantTowers;			// 0x1764 - Radiant tower entities
	CUtlVector<EHANDLE>	m_hDireTowers;				// 0x1768 - Dire tower entities (calculated offset)
	
	// Hero death tracking (0x15d4-0x15e4)
	struct HeroDeath_t
	{
		int nPlayerID;
		int nKillerID; 
		float flTime;
	};
	CUtlVector<HeroDeath_t> m_HeroDeaths;			// Hero death history
	
	// Rune system
	float				m_flNextRuneSpawn;			// 0x16cc - Next rune spawn time
	bool				m_bRuneSpawned;				// 0x16d0 - Rune spawn flag
	bool				m_bRuneSpawnEnabled;		// 0x1638 - Rune spawning enabled
	
	// Day/Night cycle
	EHANDLE				m_hDayNightEntity;			// 0x1738 - Day/night controller entity
	float				m_flTimeOfDayFraction;		// 0x1084 - Time of day (0.0-1.0)
	
	// Game state info structure (for State_Enter)
	struct GameStateInfo_t
	{
		DOTA_GameState_t state;
		const char *name;
		void (CDOTAGameRules::*enterFunc)(void);
		int offset; // Member function offset for virtual calls
	};
	GameStateInfo_t*	m_pCurrentGameStateInfo;	// 0xea0 - Current game state info

	// Gold system (decompiled offsets 0x268, 0x26c)
	int					m_nGoldPerTick;				// 0x268 - Gold awarded per tick
	float				m_flGoldTickTime;			// 0x26c - Time between gold ticks

	// Glyph system (decompiled offsets 0xe80, 0xe84)
	float				m_flRadiantGlyphCooldown;	// 0xe80 - Radiant glyph cooldown end time
	float				m_flDireGlyphCooldown;		// 0xe84 - Dire glyph cooldown end time

	// Item system (decompiled offset 0x171c)
	int					m_nDroppedItemsCount;		// 0x171c - Number of dropped items

	// Barracks tracking (similar pattern to towers)
	CUtlVector<EHANDLE>	m_hRadiantBarracks;			// Radiant barracks entities
	CUtlVector<EHANDLE>	m_hDireBarracks;			// Dire barracks entities

	// Map reset system (decompiled offsets 0x59b4, 0x59b8)
	bool				m_bMapResetTriggered;		// 0x59b4 - Map reset flag
	int					m_nMapResetDelay;			// 0x59b8 - Map reset delay

	// Game flags (decompiled offset 0x5837)
	bool				m_bSafeToLeave;				// 0x5837 - Safe to leave flag

	// Pause system
	int					m_nPauseState;				// Current pause state
	bool				m_bPauseEnabled;			// Pause system enabled
	bool				m_bPauseRequested;			// Pause requested
	bool				m_bPauseActive;				// Pause currently active

	// Game timing
	float				m_flGameEndTime;			// Game end time
	float				m_flDraftPenaltyTime;		// Draft penalty time
	float				m_flPreGameTime;			// Pre-game time
	float				m_flPreGameDuration;		// Pre-game duration
	float				m_flPostGameTime;			// Post-game time
	float				m_flLastAbandonedGoldDistributionTime; // Last gold distribution time

	// Map reset system
	bool				m_bMapResetRequested;		// Map reset requested
	int					m_nMapResetRequestTime;		// Map reset request time

	// Hero system
	bool				m_bHeroRespawnEnabled;		// Hero respawn enabled
	int					m_nHeroPickState;			// Hero pick state
	int					m_nHeroMinimapIconSize;		// Hero minimap icon size

	// Spectator system
	int					m_nMaxSpectators;			// Maximum spectators
	int					m_nCurrentSpectators;		// Current spectator count
	EHANDLE				m_hSpectatorAnnouncer;		// Spectator announcer

	// Minimap system
	float				m_flRuneMinimapIconScale;	// Rune minimap icon scale
	float				m_flCreepMinimapIconScale;	// Creep minimap icon scale

	// Game mode and settings
	int					m_nGameMode;				// Current game mode
	bool				m_bUseBaseGoldBountyOnHeroes; // Use base gold bounty on heroes

	// Tower and barracks tracking
	int					m_nRadiantTowers;			// Radiant tower count
	int					m_nDireTowers;				// Dire tower count
	int					m_nRadiantBarracks;			// Radiant barracks count
	int					m_nDireBarracks;			// Dire barracks count
	int					m_nRadiantTowerCount;		// Radiant tower count
	int					m_nDireTowerCount;			// Dire tower count
	int					m_nRadiantBarracksCount;	// Radiant barracks count
	int					m_nDireBarracksCount;		// Dire barracks count

	// First tower tracking
	bool				m_bFirstTowerDestroyed;		// First tower destroyed
	int					m_nFirstTowerTeam;			// First tower team
	float				m_flFirstTowerTime;			// First tower time

	// Rune system
	CUtlVector<EHANDLE>	m_RuneSpawners;			// Rune spawner entities

	// Hero arrays
	CUtlVector<int>		m_HeroArrays;				// Hero arrays

	// Match system
	bool				m_bMatchSignoutComplete;		// Match signout complete
	float				m_flDisconnectTimer;		// Disconnect timer

	// Load tracking
	int					m_nLoadFailureCount;		// Load failure count
	int					m_nLoadSuccessCount;		// Load success count
	int					m_nDisconnectCount;			// Disconnect count

	// Team showcase
	void*				m_pTeamShowcaseHelper;		// Team showcase helper

	// Additional missing variables
	int					m_nReconnectCount;			// Reconnect count
	int					m_nSelectionEntity;			// Selection entity
	float				m_flRuneIconScale;			// Rune icon scale
	float				m_flCreepIconScale;			// Creep icon scale
	int					m_nRadiantKillCount;		// Radiant kill count
	int					m_nDireKillCount;			// Dire kill count
	float				m_flWaitForPlayersTime;		// Wait for players time
	float				m_flRuneSpawnTime;			// Rune spawn time
	float				m_flGlyphCooldownRadiant;	// Radiant glyph cooldown
	float				m_flGlyphCooldownDire;		// Dire glyph cooldown
	int					m_nRadiantTowerKills;		// Radiant tower kills
	int					m_nDireTowerKills;			// Dire tower kills
	int					m_nRadiantBarracksKills;	// Radiant barracks kills
	int					m_nDireBarracksKills;		// Dire barracks kills
	int					m_nRadiantHeroKills;		// Radiant hero kills
	int					m_nDireHeroKills;			// Dire hero kills
	int					m_nRadiantCreepKills;		// Radiant creep kills
	int					m_nDireCreepKills;			// Dire creep kills
	EHANDLE				m_hSelectionEntity;			// Selection entity handle
	bool				m_bFirstBloodActive;		// First blood active
	bool				m_bFirstBloodOccurred;		// First blood occurred
	int					m_nHeroState;				// Hero state
	int					m_nHeroStateTime;			// Hero state time
	int					m_nHeroStateDuration;		// Hero state duration
	int					m_nHeroStateInterval;		// Hero state interval
	int					m_nHeroStateCount;			// Hero state count
	int					m_nHeroStateMax;			// Hero state max
	int					m_nHeroStateMin;			// Hero state min
	int					m_nHeroStateTarget;			// Hero state target
	int					m_nRadiantScore;			// Radiant score
	int					m_nDireScore;				// Dire score
	bool				m_bHeroStateActive;			// Hero state active
	int					m_nHeroStateIndex;			// Hero state index
	int					m_nRadiantSelectionEntity;	// Radiant selection entity
	int					m_nDireSelectionEntity;		// Dire selection entity
	int					m_nNeutralSelectionEntity;	// Neutral selection entity
	int					m_CountdownTimers[48];		// Countdown timers array
	int					m_AdditionalArrays[100];		// Additional arrays
	void*				m_CommunicationSummaries[10]; // Communication summaries
	int					m_nAdditionalVariable;		// Additional variable
	char				m_szAdditionalString[256];	// Additional string
	bool				m_bAdditionalFlag1;			// Additional flag 1
	int					m_nAdditionalVariable1;		// Additional variable 1
	int					m_nAdditionalVariable2;		// Additional variable 2
	bool				m_bAdditionalFlag2;			// Additional flag 2

	// Shop handles
	EHANDLE				m_hDireHomeShop;			// Dire home shop
	EHANDLE				m_hRadiantHomeShop;			// Radiant home shop
	EHANDLE				m_hSideShop;				// Side shop
	EHANDLE				m_hSecretShop;				// Secret shop
	EHANDLE				m_hGroundShop;				// Ground shop
	EHANDLE				m_hSideShop2;				// Side shop 2
	EHANDLE				m_hSecretShop2;				// Secret shop 2

	// Tower handles
	EHANDLE				m_hTowers[12];				// Tower handles

	// Minimap system
	void*				m_pMinimapGrid;				// Minimap grid

	// Roshan handle
	EHANDLE				m_hRoshan;					// Roshan handle

	// Rune system
	CUtlVector<EHANDLE>	m_RuneSpawnPoints;			// Rune spawn points
	int					m_nLastRuneType;			// Last rune type

private:
	
	// Victory tracking
	int					m_nWinningTeam;
	int					m_nWinReason;
	int					m_nPausingPlayerID;

	// Statistics tracking
	struct PlayerStats_t
	{
		int kills;
		int deaths;
		int assists;
		int lastHits;
		int denies;
		int gold;
		int netWorth;
	};
	
	// Client message binders (server only) - TODO: Implement when client message system is ready
#ifndef CLIENT_DLL
	/*
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_MapPing;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_MapLine;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_Pause;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_AspectRatio;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_UnitsAutoAttack;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_AutoPurchaseItems;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_TestItems;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_SearchString;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_ShopViewMode;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_SetUnitShareFlag;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_SwapRequest;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_SwapAccept;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_WorldLine;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_RequestGraphUpdate;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_ItemAlert;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_EnemyItemAlert;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_ModifierAlert;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_ClickedBuff;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_HPManaAlert;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_GlyphAlert;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_WillPurchaseAlert;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_ChatWheel;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_SendStatPopup;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_BeginLastHitChallenge;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_UpdateQuickBuy;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_UpdateCoachListen;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_CoachHUDPing;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_UnitsAutoAttackAfterSpell;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_PlayerShowCase;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_TeleportRequiresHalt;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_CameraZoomAmount;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_BroadcasterUsingCamerman;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_BroadcasterUsingAssistedCameraOperator;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_FreeInventory;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_BuyBackStateAlert;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_QuickBuyAlert;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_HeroStatueLike;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_TeamShowcaseEditor;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_TeamShowcaseClientData;
	CClientMessageBinder<CDOTAGameRules>	m_ClientMsgBinder_TeamShowcasePlayTeamShowcase;
	*/
#endif

	// Initialization helpers
	void				InitializeClientMessageBinders( void );
	void				InitializeGameState( void );
};

//-----------------------------------------------------------------------------
// Gets us at the DOTA game rules
//-----------------------------------------------------------------------------
inline CDOTAGameRules* DOTAGameRules()
{
	return static_cast<CDOTAGameRules*>(g_pGameRules);
}

// Convars
extern ConVar dota_game_allow_pause;  
extern ConVar dota_game_hero_selection_time;
extern ConVar dota_game_strategy_time;

#endif // DOTA_GAMERULES_H 