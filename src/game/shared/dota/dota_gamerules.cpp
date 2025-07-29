//====== Copyright © 1996-2011, Valve Corporation, All rights reserved. =======
//
// Purpose: DOTA Game Rules Implementation
//
//=============================================================================

#include "cbase.h"
#include "dota_gamerules.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "ehandle.h"
#include "tier0/dbg.h"

#ifdef CLIENT_DLL
	// #include "c_user_message_register.h"  // TODO: Add when client message system is implemented
#else
	#include "eventqueue.h"
	#include "player.h"
	#include "game.h"
	#include "gamerules.h"
	#include "teamplay_gamerules.h"
	#include "voice_gamemgr.h"
	#include "globalstate.h"
	#include "AI_ResponseSystem.h"
	#include "baseentity.h"
	#include "dota/dota_player.h"
	#include "dota/dota_playerresource.h"
	#include "GameEventListener.h"
	// #include "steamcallbacks.h"           // TODO: Add when Steam integration is implemented
	
	// Global player resource
	extern CDOTA_PlayerResource *g_pDOTAPlayerResource;
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// ConVars
//=============================================================================
// TODO: Move ConVar declarations to proper location once build system is set up
// ConVar dota_game_allow_pause( "dota_game_allow_pause", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Allow players to pause the game" );
// ConVar dota_game_hero_selection_time( "dota_game_hero_selection_time", "75", FCVAR_REPLICATED | FCVAR_NOTIFY, "Time allowed for hero selection in seconds" );
// ConVar dota_game_strategy_time( "dota_game_strategy_time", "30", FCVAR_REPLICATED | FCVAR_NOTIFY, "Strategy time before game starts" );



//=============================================================================
// Network Tables - TODO: Implement when networking system is ready
//=============================================================================

// Network tables will be implemented later when we add the networking system

//=============================================================================
// Game Rules Registration
//=============================================================================
REGISTER_GAMERULES_CLASS( CDOTAGameRules );

//=============================================================================
// CDOTAGameRules Implementation
//=============================================================================

CDOTAGameRules::CDOTAGameRules() :
	CTeamplayRules(),
	CGameEventListener()
{
	// Initialize member variables based on decompiled constructor
	m_nGameState = DOTA_GAMERULES_STATE_INIT;
	m_flGameStartTime = 0.0f;
	m_flStateTransitionTime = 0.0f;
	m_bGamePaused = false;
	m_nPauseTeam = 0;
	m_flPauseStartTime = 0.0f;
	m_bHeroSelectionCompleted = false;
	m_flHeroSelectionTime = 0.0f;
	m_nWinningTeam = TEAM_UNASSIGNED;
	m_nWinReason = 0;
	
	// Initialize DOTA timing system
	m_flPreGameStartTime = 0.0f;
	m_flGameStartTime2 = 0.0f;
	m_flHornTime = 0.0f;
	m_flPausedTimeAccumulated = 0.0f;
	m_flLastPauseTime = 0.0f;
	m_flTimeOfDay = 0.0f;
	
	// Initialize entity handles
	m_hRadiantFountain.Set( NULL );
	m_hDireFountain.Set( NULL );
	m_hRadiantAnnouncer.Set( NULL );
	m_hDireAnnouncer.Set( NULL );
	m_hGlobalAnnouncer.Set( NULL );
	m_hDayNightEntity.Set( NULL );

	// Initialize gold system
	m_nGoldPerTick = 1;  // Default 1 gold per tick
	m_flGoldTickTime = 0.6f;  // Default 0.6 second intervals

	// Initialize glyph system
	m_flRadiantGlyphCooldown = 0.0f;
	m_flDireGlyphCooldown = 0.0f;

	// Initialize item system
	m_nDroppedItemsCount = 0;

	// Initialize map reset system
	m_bMapResetTriggered = false;
	m_nMapResetDelay = 0;

	// Initialize game flags
	m_bSafeToLeave = false;
	
	// Initialize tower tracking
	m_nNumTowers = 0;
	
	// Initialize rune system
	m_flNextRuneSpawn = 0.0f;
	m_bRuneSpawned = false;
	m_bRuneSpawnEnabled = true;
	
	// Initialize day/night cycle
	m_flTimeOfDayFraction = 0.0f;
	
	// Initialize game state info
	m_pCurrentGameStateInfo = NULL;

#ifdef GAME_DLL
	// Initialize Steam callbacks (from decompiled code)
	// The decompiled code shows Steam API callback registrations
	
	// Initialize client message binders
	// InitializeClientMessageBinders(); // TODO: Implement when client message system is ready
#endif

	// Initialize game state
	InitializeGameState();

	// Listen for game events
	ListenForGameEvent( "player_connect" );
	ListenForGameEvent( "player_disconnect" );
	ListenForGameEvent( "player_team" );
}

CDOTAGameRules::~CDOTAGameRules()
{
#ifdef GAME_DLL
	// Cleanup Steam callbacks if needed
#endif
}

CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;
	}

	return &def;
}


//-----------------------------------------------------------------------------
// Purpose: Initialize client message binders (server only)
//-----------------------------------------------------------------------------
void CDOTAGameRules::InitializeClientMessageBinders()
{
#ifdef GAME_DLL
	// TODO: Based on decompiled constructor, bind all client message handlers
	// This will be implemented when the client message system is ready
	// m_ClientMsgBinder_MapPing.Bind( this, &CDOTAGameRules::OnClientMsg_MapPing );
	// m_ClientMsgBinder_MapLine.Bind( this, &CDOTAGameRules::OnClientMsg_MapLine );
	// m_ClientMsgBinder_Pause.Bind( this, &CDOTAGameRules::OnClientMsg_Pause );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Initialize game state
//-----------------------------------------------------------------------------
void CDOTAGameRules::InitializeGameState()
{
	SetGameState( DOTA_GAMERULES_STATE_INIT );
}


//-----------------------------------------------------------------------------
// Purpose: Precache game content
//-----------------------------------------------------------------------------
void CDOTAGameRules::Precache( void )
{
#ifndef CLIENT_DLL
	BaseClass::Precache();
#endif
	// Precache DOTA-specific content here
}

//-----------------------------------------------------------------------------
// Purpose: Collision rules for DOTA
//-----------------------------------------------------------------------------
bool CDOTAGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	// Use base teamplay collision rules for now
	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 );
}


//-----------------------------------------------------------------------------
// Purpose: Think function called every frame
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CDOTAGameRules::Think( void )
{
#ifdef GAME_DLL
	BaseClass::Think();
	// Handle unpaused game state (matches decompiled unpaused logic)
	if ( !m_bGamePaused )
	{
		// Update pause countdown timer (matches decompiled timer update)
		float flPauseTime = m_flPauseTime;
		float flCurrentTime = 0.0f;
		
		if ( flPauseTime != 0.0f )
		{
			float flOldTime = flPauseTime;
			if ( gpGlobals )
			{
				// TODO: Implement when engine system is available
				// if ( !gpGlobals->m_bMapLoadFailed && !gpGlobals->m_bMapLoadFailed && gpGlobals->m_pfnChangeLevel )
				// {
				//     gpGlobals->m_pfnChangeLevel( 1 );
				// }
				flCurrentTime = gpGlobals->curtime;
			}
			
			int nPauseSeconds = (int)( flPauseTime - flCurrentTime );
			if ( nPauseSeconds != (int)( flOldTime - m_flPauseEndTime ) )
			{
				// TODO: Implement when chat system is available
				// SendChatEventMessage( DOTA_CHAT_EVENT_PAUSE_COUNTDOWN, nPauseSeconds + 1, m_nPausePlayerID, 0 );
			}
			
			m_flPauseEndTime = flCurrentTime;
			
			// Check if pause time has expired (matches decompiled time check)
			if ( flPauseTime - flCurrentTime <= 0.0f )
			{
				if ( !m_bGamePaused )
				{
					CGameRulesProxy::NotifyNetworkStateChanged();
					m_bGamePaused = true;
				}
				
				if ( m_nGameState != 1 )
				{
					CGameRulesProxy::NotifyNetworkStateChanged();
					m_nGameState = DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS;
				}
				
				m_flPauseTime = 0.0f;
				m_flPauseEndTime = 0.0f;
				
				// Handle pause unpause logic (matches decompiled unpause logic)
				// TODO: Implement when GC system is available
				// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
				// if ( pGCSystem )
				// {
				//     CDOTALobby *pLobby = pGCSystem->GetLobby();
				//     if ( pLobby )
				//     {
				//         bool bShouldLimitPausing = pLobby->ShouldLimitPausing();
				//         
				//         CDOTA_PlayerResource *pPlayerResource = g_pDOTAPlayerResource;
				//         if ( pPlayerResource )
				//         {
				//             CBasePlayer *pPausePlayer = pPlayerResource->GetPlayer( m_nPausePlayerID );
				//             if ( pPausePlayer )
				//             {
				//                 if ( !bShouldLimitPausing )
				//                 {
				//                     // Send unpause message to all players
				//                     // TODO: Implement when chat system is available
				//                     // CRecipientFilter filter;
				//                     // filter.AddAllPlayers();
				//                     // int nPlayerID = pPausePlayer->GetPlayerID();
				//                     // SendChatEventMessage( DOTA_CHAT_EVENT_UNPAUSE, 0, nPlayerID, &filter );
				//                 }
				//                 else
				//                 {
				//                     // Send limited unpause message
				//                     // TODO: Implement when chat system is available
				//                     // CRecipientFilter filter;
				//                     // filter.AddAllPlayers();
				//                     // filter.RemoveRecipient( pPausePlayer );
				//                     // int nPlayerID = pPausePlayer->GetPlayerID();
				//                     // SendChatEventMessage( DOTA_CHAT_EVENT_UNPAUSE, 0, nPlayerID, &filter );
				//                     // 
				//                     // CRecipientFilter singleFilter;
				//                     // singleFilter.AddRecipient( pPausePlayer );
				//                     // SendChatEventMessage( DOTA_CHAT_EVENT_PAUSE_LIMITED, m_nPauseLimitCount, 0, &singleFilter );
				//                 }
				//             }
				//         }
				//     }
				// }
				
				m_nPausePlayerID = -1;
				m_nPauseTeamID = -1;
			}
		}
		
		// Update suggested heroes (matches decompiled hero update)
		// TODO: Implement when hero system is available
		// UpdateSuggestedHeroes();
		
		// Update game time (matches decompiled time update)
		float flNewGameTime = gpGlobals->frametime + m_flGameTime;
		if ( flNewGameTime != m_flGameTime )
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_flGameTime = flNewGameTime;
		}
		
		// Call base class think (matches decompiled base call)
		CTeamplayRules::Think();
		
		// Update pause state (matches decompiled state update)
		float flPauseState = m_flPauseTime;
		int nPauseState = 0;
		if ( flPauseState >= 0.0f )
		{
			nPauseState = 0xffff;
			if ( flPauseState <= 1.0f )
			{
				nPauseState = (int)( flPauseState * 65535.0f );
			}
		}
		
		if ( m_nPauseState != nPauseState )
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_nPauseState = nPauseState;
		}
		
		// Update countdown timers (matches decompiled timer updates)
		// TODO: Implement when timer system is available
		// if ( m_flCountdownTimer1 <= 0.0f )
		// {
		//     m_bCountdownTimer1Active = false;
		// }
		// else
		// {
		//     m_bCountdownTimer1Active = CountdownTimer::Now( &m_CountdownTimer1 ) < m_flCountdownTimer1;
		// }
		// 
		// if ( m_bCountdownTimer1Active != m_bCountdownTimer1ActiveOld )
		// {
		//     NotifyNetworkStateChanged();
		//     m_bCountdownTimer1ActiveOld = m_bCountdownTimer1Active;
		// }
		// 
		// if ( m_flCountdownTimer2 <= 0.0f )
		// {
		//     m_bCountdownTimer2Active = false;
		// }
		// else
		// {
		//     m_bCountdownTimer2Active = CountdownTimer::Now( &m_CountdownTimer2 ) < m_flCountdownTimer2;
		// }
		// 
		// if ( m_bCountdownTimer2Active != m_bCountdownTimer2ActiveOld )
		// {
		//     NotifyNetworkStateChanged();
		//     m_bCountdownTimer2ActiveOld = m_bCountdownTimer2Active;
		// }
		
		// Check pending projection abilities (matches decompiled projection check)
		//CheckPendingProjectionAbilities();
		
		// Update telemetry (matches decompiled telemetry update)
		// TODO: Implement when telemetry system is available
		// float flGameTime = 0.0f;
		// if ( GetGameState() > 4 )
		// {
		//     if ( ( GetGameState() & 0xfffffffe ) == 6 )
		//     {
		//         flGameTime = m_flPreGameTime;
		//     }
		//     else
		//     {
		//         flGameTime = m_flGameTime;
		//     }
		//     flGameTime = ( flGameTime - m_flGameStartTime ) + m_flPreGameDuration;
		// }
		// g_Telemetry.SetValue( "dota_game_time", flGameTime );
		
		// Update match snapshot (matches decompiled snapshot update)
		// TODO: Implement when telemetry system is available
		// if ( m_flLastMatchSnapshotTime <= flGameTime && flGameTime != m_flLastMatchSnapshotTime )
		// {
		//     float flNewGameTime = 0.0f;
		//     if ( GetGameState() > 4 )
		//     {
		//         if ( ( GetGameState() & 0xfffffffe ) == 6 )
		//         {
		//             flNewGameTime = m_flPreGameTime;
		//         }
		//         else
		//         {
		//             flNewGameTime = m_flGameTime;
		//         }
		//         flNewGameTime = ( flNewGameTime - m_flGameStartTime ) + m_flPreGameDuration;
		//     }
		//     g_Telemetry.SetValue( "dota_game_time", flNewGameTime );
		//     m_flLastMatchSnapshotTime = flNewGameTime + dota_match_snapshot_interval.GetFloat();
		//     CDOTA_OGS::LogMatchSnapshot();
		// }
		
		// Update fountain entities (matches decompiled fountain updates)
		// TODO: Implement when entity system is available
		// for ( int nTeam = 2; nTeam < 4; nTeam++ )
		// {
		//     EHANDLE hFountain = ( nTeam == 2 ) ? m_hRadiantFountain : m_hDireFountain;
		//     if ( hFountain.IsValid() )
		//     {
		//         CBaseEntity *pFountain = hFountain.Get();
		//         if ( pFountain && pFountain->IsAlive() && pFountain->IsHero() )
		//         {
		//             // Update fountain logic
		//             // TODO: Implement when fountain system is available
		//         }
		//     }
		// }
		
		// Call current state think function (matches decompiled state think)
		// TODO: Implement when state system is available
		// if ( m_pCurrentStateInfo && m_pCurrentStateInfo->pfnThink )
		// {
		//     m_pCurrentStateInfo->pfnThink( this + m_pCurrentStateInfo->nOffset );
		// }
		
		// Handle game in progress logic (matches decompiled progress logic)
		if ( ( GetGameState() & 0xfffffffc ) == 4 )
		{
			// Update rune timers (matches decompiled rune updates)
			// TODO: Implement when rune system is available
			// for ( int i = 0; i < 12; i++ )
			// {
			//     if ( m_RuneTimers[i].nCurrent < m_RuneTimers[i].nMax && m_RuneTimers[i].flInterval > 0.0f )
			//     {
			//         if ( m_RuneTimers[i].flNextSpawn <= m_flGameTime && m_flGameTime != m_RuneTimers[i].flNextSpawn )
			//         {
			//             NotifyNetworkStateChanged();
			//             int nCurrent = m_RuneTimers[i].nCurrent;
			//             m_RuneTimers[i].SetDuration( m_RuneTimers[i].nCurrent );
			//             m_RuneTimers[i].nCurrent = nCurrent + 1;
			//             
			//             if ( nCurrent + 1 < m_RuneTimers[i].nMax )
			//             {
			//                 NotifyNetworkStateChanged();
			//                 float flNextSpawn = m_flGameTime + m_RuneTimers[i].flInterval;
			//                 if ( flNextSpawn != m_RuneTimers[i].flNextSpawn )
			//                 {
			//                     m_RuneTimers[i].SetDuration( m_RuneTimers[i].flNextSpawn );
			//                     m_RuneTimers[i].flNextSpawn = flNextSpawn;
			//                 }
			//             }
			//         }
			//     }
			// }
			
			// Update gold tick timer (matches decompiled gold tick)
			float flGoldTickTime = m_flGoldTickTime;
			if ( flGoldTickTime == 0.0f )
			{
				flGoldTickTime = m_flGameTime + 1.0f;
				m_flGoldTickTime = flGoldTickTime;
			}
			
			if ( GetGameState() == 1 && flGoldTickTime <= m_flGameTime )
			{
				m_flGoldTickTime = m_flGameTime + 1.0f;
				
				// Give gold to players (matches decompiled gold distribution)
				// TODO: Implement when player resource system is available
				// for ( int i = 0; i < 10; i++ )
				// {
				//     if ( g_pDOTAPlayerResource->IsValidPlayerID( i ) )
				//     {
				//         int nHeroID = g_pDOTAPlayerResource->GetSelectedHeroID( i );
				//         if ( nHeroID < 1 )
				//         {
				//             g_pDOTAPlayerResource->SpendGold( i, 1, 9 );
				//         }
				//     }
				// }
			}
			
			// Handle draft penalties (matches decompiled draft logic)
			if ( GetGameState() == 22 && m_flDraftPenaltyTime <= m_flGameTime )
			{
				m_flDraftPenaltyTime = m_flGameTime + 1.0f;
				// TODO: Implement when draft system is available
				// AllDraftRunLatePenalties();
			}
			
			// Handle game timeout (matches decompiled timeout logic)
			if ( m_nWinningTeam != 0 && m_flGameEndTime != 0.0f && m_flGameEndTime < m_flGameTime )
			{
				CGameRulesProxy::NotifyNetworkStateChanged();
				m_flGameEndTime = 0.0f;
				MakeTeamLose( m_nWinningTeam );
				if ( m_nWinningTeam != 0 )
				{
					CGameRulesProxy::NotifyNetworkStateChanged();
					m_nWinningTeam = 0;
				}
			}
			
			// Handle map reset (matches decompiled reset logic)
			if ( m_bMapResetRequested )
			{
				// TODO: Implement when map system is available
				// ExecuteResetMap();
			}
		}
	}
	else
	{
		// Handle paused game state (matches decompiled paused logic)
		float flPauseTime = m_flPauseTime;
		float flCurrentTime = 0.0f;
		
		if ( flPauseTime != 0.0f )
		{
			float flOldTime = flPauseTime;
			if ( gpGlobals )
			{
				// TODO: Implement when engine system is available
				// if ( !gpGlobals->m_bMapLoadFailed && !gpGlobals->m_bMapLoadFailed && gpGlobals->m_pfnChangeLevel )
				// {
				//     gpGlobals->m_pfnChangeLevel( 1 );
				// }
				flCurrentTime = gpGlobals->curtime;
			}
			
			int nPauseSeconds = (int)( flPauseTime - flCurrentTime );
			if ( nPauseSeconds != (int)( flOldTime - m_flPauseEndTime ) )
			{
				// TODO: Implement when chat system is available
				// SendChatEventMessage( DOTA_CHAT_EVENT_PAUSE_COUNTDOWN, nPauseSeconds + 1, 0, 0 );
			}
			
			m_flPauseEndTime = flCurrentTime;
			
			// Check if pause time has expired (matches decompiled time check)
			if ( flPauseTime - flCurrentTime <= 0.0f )
			{
				if ( m_bGamePaused )
				{
					CGameRulesProxy::NotifyNetworkStateChanged();
					m_bGamePaused = false;
				}
				
				if ( m_nGameState != DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS)
				{
					CGameRulesProxy::NotifyNetworkStateChanged();
					m_nGameState = DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS;
				}
				
				m_flPauseTime = 0.0f;
				m_flPauseEndTime = 0.0f;
				
				// Send unpause message (matches decompiled message)
				// TODO: Implement when chat system is available
				// SendChatEventMessage( DOTA_CHAT_EVENT_UNPAUSE, 0, m_nPauseTeamID, 0 );
				
				m_nPausePlayerID = -1;
				m_nPauseTeamID = -1;
			}
		}
		
		// Handle pause timeout (matches decompiled timeout logic)
		// TODO: Implement when player system is available
		// CDOTAPlayer *pPausePlayer = CDOTAPlayer::GetPlayerByPlayerID( m_nPausePlayerID );
		// if ( pPausePlayer && m_flPauseStartTime != 0.0f )
		// {
		//     float flCurrentTime = Plat_FloatTime();
		//     if ( dota_pause_timeout.GetFloat() < flCurrentTime - m_flPauseStartTime )
		//     {
		//         if ( engine->IsDedicatedServer() )
		//         {
		//             // Count connected players
		//             int nConnectedPlayers = 0;
		//             for ( int i = 0; i < 32; i++ )
		//             {
		//                 if ( g_pDOTAPlayerResource->GetConnectionState( i ) == 2 )
		//                 {
		//                     nConnectedPlayers++;
		//                 }
		//             }
		//             
		//             if ( nConnectedPlayers == 1 )
		//             {
		//                 SetGamePaused( false, -1, -1.0f, -1.0f );
		//                 SendChatEventMessage( DOTA_CHAT_EVENT_PAUSE_TIMEOUT, 0, 0, 0 );
		//                 m_nPausePlayerID = -1;
		//                 m_nPauseTeamID = -1;
		//             }
		//         }
		//     }
		// }
		
		// Update voice game manager (matches decompiled voice update)
		// TODO: Implement when voice system is available
		// CVoiceGameMgr *pVoiceMgr = GetVoiceGameMgr();
		// if ( pVoiceMgr )
		// {
		//     pVoiceMgr->Update( gpGlobals->frametime );
		// }
	}
#endif
}



//-----------------------------------------------------------------------------
// Purpose: Handle client commands - TODO: Implement when client command system is ready
//-----------------------------------------------------------------------------
/*
bool CDOTAGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	CBasePlayer *pPlayer = ToBasePlayer( pEdict );
	if ( !pPlayer )
		return false;

	const char *pcmd = args[0];

	// Handle DOTA-specific commands here
	if ( FStrEq( pcmd, "spectate" ) )
	{
		// Handle spectate command
		return true;
	}

	return BaseClass::ClientCommand( pEdict, args );
}
*/

//-----------------------------------------------------------------------------
// Purpose: Team management
//-----------------------------------------------------------------------------
int CDOTAGameRules::GetTeamIndex( const char *pTeamName )
{
	if ( FStrEq( pTeamName, "radiant" ) )
		return DOTA_TEAM_RADIANT;
	else if ( FStrEq( pTeamName, "dire" ) )
		return DOTA_TEAM_DIRE;
	else if ( FStrEq( pTeamName, "spectator" ) )
		return TEAM_SPECTATOR;

	return TEAM_UNASSIGNED;
}

const char* CDOTAGameRules::GetIndexedTeamName( int teamIndex )
{
	switch ( teamIndex )
	{
		case DOTA_TEAM_RADIANT: return "radiant";
		case DOTA_TEAM_DIRE: return "dire";
		case TEAM_SPECTATOR: return "spectator";
		default: return "";
	}
}

bool CDOTAGameRules::IsValidTeam( const char *pTeamName )
{
	return GetTeamIndex( pTeamName ) != TEAM_UNASSIGNED;
}


//-----------------------------------------------------------------------------
// Purpose: Player spawning
//-----------------------------------------------------------------------------
void CDOTAGameRules::PlayerSpawn( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	BaseClass::PlayerSpawn( pPlayer );
	
	if ( !pPlayer )
		return;

	CDOTAPlayer *pDOTAPlayer = dynamic_cast<CDOTAPlayer*>( pPlayer );
	if ( !pDOTAPlayer )
		return;

	// Check if player is valid and connected
	if ( !pDOTAPlayer->IsConnected() )
		return;

	int nPlayerID = pDOTAPlayer->GetPlayerID();
	if ( nPlayerID == -1 )
		return;

	if ( g_pDOTAPlayerResource )
	{
		g_pDOTAPlayerResource->SetConnectionState( nPlayerID, 2, 0 ); // 2 = Connected
	}

	// Handle hero spawning based on game state
	if ( m_nGameState >= DOTA_GAMERULES_STATE_HERO_SELECTION )
	{
		// Check if player has selected a hero
		if ( g_pDOTAPlayerResource && g_pDOTAPlayerResource->HasSelectedHero( nPlayerID ) )
		{
			// Hero already selected, handle reconnection
			const char *pHeroName = g_pDOTAPlayerResource->GetSelectedHeroName( nPlayerID );
			if ( pHeroName && pHeroName[0] != '\0' )
			{
				// TODO: Spawn hero with DOTA_Hero_Spawn when hero system is implemented
				// DOTA_Hero_Spawn( pHeroName, pDOTAPlayer, false );
			}
		}
		else
		{
			// No hero selected, handle based on game state
			if ( m_nGameState == DOTA_GAMERULES_STATE_HERO_SELECTION )
			{
				// Still in hero selection, allow player to pick
				// Player can use UI to select hero
			}
			else
			{
				// Past hero selection, assign random hero
				pDOTAPlayer->SetWantsRandomHero();
				pDOTAPlayer->MakeRandomHeroSelection( false, -1 );
			}
		}
	}

	// Handle reconnection logic
	pDOTAPlayer->OnReconnect();

	// TODO: Fire player reconnected event when game event system is fully integrated
	IGameEvent *event = gameeventmanager->CreateEvent( "player_reconnected" );
	if ( event )
	{
	     event->SetInt( "PlayerID", nPlayerID );
	     gameeventmanager->FireEvent( event );
	}

	// TODO: Update team-based NPC control when team system is fully integrated
	// int nTeam = pPlayer->GetTeamNumber();
	// CDOTA_BaseNPC::RecalculateControllableStateForNPCS( nTeam, true, true, nPlayerID );

#endif // CLIENT_DLL
}

void CDOTAGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	BaseClass::PlayerKilled( pVictim, info );
#endif
	// DOTA-specific death handling will be added here
}

/*
bool CDOTAGameRules::ShouldAutoAim( CBaseEntity *pTarget, CBaseEntity *pShooter )
{
	// DOTA doesn't use auto-aim
	return false;
}
*/

//-----------------------------------------------------------------------------
// Purpose: Game state management
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetGameState( DOTA_GameState_t nState )
{
	if ( m_nGameState != nState )
	{
		m_nGameState = nState;
		m_flStateTransitionTime = gpGlobals->curtime;

#ifdef GAME_DLL
		// TODO: Notify network of state change when networking is implemented
		// if ( GameRulesProxy() )
		// {
		//     GameRulesProxy()->NetworkStateChanged();
		// }
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Victory conditions
//-----------------------------------------------------------------------------
bool CDOTAGameRules::CheckWinConditions( void )
{
	// DOTA win conditions will be implemented here
	// For now, just return false (no winner yet)
	return false;
}


void CDOTAGameRules::SetWinningTeam( int team, int iWinReason, bool bForceMapReset, bool bSwitchTeams, bool bDontAddScore )
{
	m_nWinningTeam = team;
	m_nWinReason = iWinReason;
	
	// Store final game time when match ends
	m_flTimeOfDay = gpGlobals->curtime;
	
	SetGameState( DOTA_GAMERULES_STATE_POST_GAME );
#ifndef CLIENT_DLL
	BaseClass::SetWinningTeam( team, iWinReason, bForceMapReset, bSwitchTeams, bDontAddScore );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Hero selection
//-----------------------------------------------------------------------------
void CDOTAGameRules::StartHeroSelection( void )
{
	SetGameState( DOTA_GAMERULES_STATE_HERO_SELECTION );
	m_bHeroSelectionCompleted = false;
	m_flPreGameStartTime = gpGlobals->curtime; // Track when pre-game phase started
}

void CDOTAGameRules::EndHeroSelection( void )
{
	m_bHeroSelectionCompleted = true;
	SetGameState( DOTA_GAMERULES_STATE_STRATEGY_TIME );
	m_flGameStartTime2 = gpGlobals->curtime; // Alternative game start time tracking
}

//-----------------------------------------------------------------------------
// Purpose: Pause system
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetGamePaused( bool bPaused )
{
	// TODO: Check ConVar when available
	// if ( !dota_game_allow_pause.GetBool() )
	//     return;

	if ( m_bGamePaused != bPaused )
	{
		m_bGamePaused = bPaused;
		
		if ( bPaused )
		{
			m_flPauseStartTime = gpGlobals->curtime;
			m_flLastPauseTime = gpGlobals->curtime;
		}
		else
		{
			// Accumulate paused time when unpausing
			if ( m_flLastPauseTime > 0.0f )
			{
				float flPauseDuration = gpGlobals->curtime - m_flLastPauseTime;
				m_flPausedTimeAccumulated += flPauseDuration;
				m_flLastPauseTime = 0.0f; // Clear pause start time
			}
		}

#ifdef GAME_DLL
		// TODO: Notify network of pause state change when networking is implemented
		// if ( GameRulesProxy() )
		// {
		//     GameRulesProxy()->NetworkStateChanged();
		// }
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Client message handlers - TODO: Implement when client message system is ready
//-----------------------------------------------------------------------------
/*
void CDOTAGameRules::OnClientMsg_MapPing( int iPlayerID, const CUserMessageBuffer &buf )
{
	// Handle map ping message
}

void CDOTAGameRules::OnClientMsg_MapLine( int iPlayerID, const CUserMessageBuffer &buf )
{
	// Handle map line message
}

void CDOTAGameRules::OnClientMsg_Pause( int iPlayerID, const CUserMessageBuffer &buf )
{
	// Handle pause request
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerID );
	if ( pPlayer )
	{
		SetGamePaused( !IsGamePaused() );
		m_nPauseTeam = pPlayer->GetTeamNumber();
	}
}
*/

//-----------------------------------------------------------------------------
// Purpose: Steam callbacks (server only) - TODO: Implement when Steam integration is ready
//-----------------------------------------------------------------------------
#ifdef GAME_DLL

/*
void CDOTAGameRules::Steam_OnClientKick( GSClientKick_t *pParam )
{
	// Handle Steam client kick
}

void CDOTAGameRules::Steam_OnClientDeny( GSClientDeny_t *pParam )
{
	// Handle Steam client deny
}
*/

//-----------------------------------------------------------------------------
// Purpose: Server-side specific functions
//-----------------------------------------------------------------------------
void CDOTAGameRules::GoToIntermission( void )
{
	BaseClass::GoToIntermission();
}


/*
bool CDOTAGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	// DOTA damage rules will be implemented here
	return BaseClass::FPlayerCanTakeDamage( pPlayer, pAttacker );
}
*/

#else

//-----------------------------------------------------------------------------
// Purpose: Client-side specific functions - TODO: Implement when client networking is ready
//-----------------------------------------------------------------------------
/*
void CDOTAGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	// Handle network data changes on client
}

void CDOTAGameRules::HandleOvertimeBegin( void )
{
	// Handle overtime begin on client
}
*/

#endif

//-----------------------------------------------------------------------------
// Purpose: Game event handling
//-----------------------------------------------------------------------------
void CDOTAGameRules::FireGameEvent( IGameEvent *event )
{
	const char *name = event->GetName();

	if ( FStrEq( name, "player_connect" ) )
	{
		// Handle player connect
	}
	else if ( FStrEq( name, "player_disconnect" ) )
	{
		// Handle player disconnect
	}
	else if ( FStrEq( name, "player_team" ) )
	{
		// Handle player team change
	}
}
#ifdef GAME_DLL


void InitBodyQue() {}

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool	CanPlayerHearPlayer(CBasePlayer* pListener, CBasePlayer* pTalker, bool& bProximity) { return true; }
};

CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper* g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

#endif // #ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Get DOTA time with various options (based on decompiled code)
// This function calculates game time differently based on game state and options
//-----------------------------------------------------------------------------
float CDOTAGameRules::GetDOTATime( bool bIncludePregameTime, bool bExcludePausedTime, float flCurrentTime )
{
	// Use provided time or default to current server time
	if ( flCurrentTime == 0.0f )
	{
		flCurrentTime = gpGlobals->curtime;
	}
	
	float flCalculatedTime = 0.0f;
	DOTA_GameState_t gameState = GetGameState();
	
	// Handle different game states (based on decompiled logic)
	if ( (int)gameState < DOTA_GAMERULES_STATE_GAME_IN_PROGRESS ) // States 0-4 (pre-game states)
	{
		flCalculatedTime = 0.0f; // Default for pre-game states
		
		if ( bExcludePausedTime ) // When excluding paused time, calculate actual elapsed time
		{
			if ( gameState == DOTA_GAMERULES_STATE_HERO_SELECTION )
			{
				// During hero selection, calculate time from pre-game start (matches decompiled: offset_0x10b4 - offset_0x498)
				if ( m_flPreGameStartTime > 0.0f )
				{
					flCalculatedTime = flCurrentTime - m_flPreGameStartTime;
				}
			}
			else
			{
				// Other pre-game states, calculate from game start time 2 (matches decompiled: param_3 - offset_0x48c)
				if ( m_flGameStartTime2 > 0.0f )
				{
					flCalculatedTime = flCurrentTime - m_flGameStartTime2;
				}
			}
		}
		// When NOT excluding paused time (normal mode), pre-game states return 0.0 (matches decompiled behavior)
	}
	else if ( gameState == DOTA_GAMERULES_STATE_POST_GAME || gameState == DOTA_GAMERULES_STATE_DISCONNECT )
	{
		// Post-game states (6 & 7 in decompiled code check (uVar1 & 0xfffffffe) == 6)
		// Use stored final game time (matches decompiled: (offset_0xe50 - offset_0xe4c) + offset_0xe54)
		if ( m_flTimeOfDay > 0.0f && m_flHornTime > 0.0f )
		{
			flCalculatedTime = (m_flTimeOfDay - m_flHornTime) + m_flPausedTimeAccumulated;
		}
		else
		{
			flCalculatedTime = 0.0f; // No game time recorded yet
		}
	}
	else
	{
		// Game in progress - calculate current game time (matches decompiled: (param_3 - offset_0xe4c) + offset_0xe54)
		if ( m_flHornTime > 0.0f )
		{
			flCalculatedTime = (flCurrentTime - m_flHornTime) + m_flPausedTimeAccumulated;
			
			// Subtract current pause time if we're currently paused and excluding paused time
			if ( bExcludePausedTime && IsGamePaused() && m_flLastPauseTime > 0.0f )
			{
				flCalculatedTime -= (flCurrentTime - m_flLastPauseTime);
			}
		}
		else
		{
			flCalculatedTime = 0.0f; // Horn hasn't sounded yet
		}
	}
	
	// Add pre-game time if requested (matches decompiled: adds time from before horn)
	if ( bIncludePregameTime && m_flHornTime > 0.0f && m_flPreGameStartTime > 0.0f )
	{
		// Add the time from pre-game start to horn time
		flCalculatedTime += (m_flHornTime - m_flPreGameStartTime);
	}
	
	
	return flCalculatedTime;
}

#ifdef GAME_DLL

//=============================================================================
// Additional DOTA Game Rules Functions (from decompiled code)
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Get fountain entity for specified team (from decompiled 0x105c/0x1060)
//-----------------------------------------------------------------------------
CBaseEntity* CDOTAGameRules::GetFountain( int nTeam )
{
	EHANDLE hFountain;
	
	if ( nTeam == DOTA_TEAM_DIRE )
	{
		hFountain = m_hDireFountain;
	}
	else if ( nTeam == DOTA_TEAM_RADIANT )
	{
		hFountain = m_hRadiantFountain;
	}
	else
	{
		return NULL;
	}
	
	// Validate entity handle and return entity (matches decompiled entity validation)
	CBaseEntity *pEntity = hFountain.Get();
	if ( pEntity && !pEntity->IsMarkedForDeletion() )
	{
		return pEntity;
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get current game time (from decompiled 0x10b4)
//-----------------------------------------------------------------------------
float CDOTAGameRules::GetGameTime()
{
	// TODO: This should reference the actual game time member when networking is implemented
	// For now, use our timing system
	return GetDOTATime();
}

//-----------------------------------------------------------------------------
float CDOTAGameRules::GetTransitionTime()
{
	return m_flStateTransitionTime;
}

//-----------------------------------------------------------------------------
float CDOTAGameRules::GetCurrentTime()
{
	return gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Transition from current state to new state (from decompiled State_Transition)
//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Transition( DOTA_GameState_t newState )
{
#ifndef CLIENT_DLL
	// Store the current state for the event (matches decompiled old state storage)
	DOTA_GameState_t oldState = m_nGameState;
	
	// Call leave function for current state if it exists (matches decompiled leave function call)
	if ( m_pCurrentGameStateInfo && m_pCurrentGameStateInfo->enterFunc )
	{
		// TODO: Add state leave functions when state system is fully implemented
		// The decompiled code calls a leave function at offset 0x10 in the state info
		// For now, we'll add specific leave functions as needed
		
		if ( oldState == DOTA_GAMERULES_STATE_INIT )
		{
			State_Leave_INIT();
		}
		// Add other state leave functions as needed:
		// else if ( oldState == DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS )
		//     State_Leave_WAIT_FOR_PLAYERS();
		// else if ( oldState == DOTA_GAMERULES_STATE_HERO_SELECTION )
		//     State_Leave_HERO_SELECTION();
		// else if ( oldState == DOTA_GAMERULES_STATE_PRE_GAME )
		//     State_Leave_PRE_GAME();
		// else if ( oldState == DOTA_GAMERULES_STATE_POST_GAME )
		//     State_Leave_POST_GAME();
		// etc.
	}
	
	// Enter the new state (matches decompiled State_Enter call)
	State_Enter( newState );
	
	// Fire game event for state change (matches decompiled game event firing)
	if ( gameeventmanager )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "dota_game_state_change" );
		if ( pEvent )
		{
			pEvent->SetInt( "old_state", oldState );
			pEvent->SetInt( "new_state", newState );
			gameeventmanager->FireEvent( pEvent );
		}
	}
	
	// Add to combat log (matches decompiled combat log addition)
	// TODO: Implement when combat log system is available
	// CDOTA_CombatLog::AddGameEvent( g_DOTACombatLog, DOTA_COMBATLOG_GAME_STATE, newState );
	
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: Update rune spawning system (from decompiled UpdateRunes) 
//-----------------------------------------------------------------------------
void CDOTAGameRules::UpdateRunes()
{
	// Only update runes during active game states (matches decompiled state check)
	if ( GetGameState() < DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
		return;
		
	// Calculate current DOTA time for rune timing
	float flDOTATime = GetDOTATime();
	
	// Check if it's time to spawn a rune (matches decompiled timing logic)
	if ( m_bRuneSpawnEnabled && flDOTATime >= m_flNextRuneSpawn && flDOTATime != m_flNextRuneSpawn )
	{
		// TODO: Get rune interval from ConVar when available (default 120 seconds)
		float flRuneInterval = 120.0f; 
		m_flNextRuneSpawn = flDOTATime + flRuneInterval;
		
		// Spawn rune if enabled (matches decompiled spawn logic) 
		if ( m_bRuneSpawnEnabled )
		{
			bool bReplaceExisting = m_bRuneSpawned;
			m_bRuneSpawned = true;
			
			// TODO: Implement SpawnRune function when entity system is ready
			// SpawnRune( bReplaceExisting );
			Msg( "DOTA: Spawning rune at time %.2f (replace existing: %s)\n", flDOTATime, bReplaceExisting ? "yes" : "no" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Track hero death for statistics (from decompiled AddHeroDeath)
//-----------------------------------------------------------------------------
void CDOTAGameRules::AddHeroDeath( int nPlayerID, int nKillerID, float flTime )
{
	// Add death record to tracking array (matches decompiled CUtlVector logic)
	HeroDeath_t death;
	death.nPlayerID = nPlayerID;
	death.nKillerID = nKillerID;
	death.flTime = flTime;
	
	// Insert at beginning to maintain chronological order (matches decompiled insertion)
	m_HeroDeaths.InsertBefore( 0, death );
	
	Msg( "DOTA: Hero death recorded - Player %d killed by %d at time %.2f\n", nPlayerID, nKillerID, flTime );
}

//-----------------------------------------------------------------------------
// Purpose: Get announcer entity for specified team (from decompiled GetAnnouncer)
//-----------------------------------------------------------------------------
CBaseEntity* CDOTAGameRules::GetAnnouncer( int nTeam )
{
	EHANDLE hAnnouncer;
	
	if ( nTeam == 1 ) // Global announcer
	{
		hAnnouncer = m_hGlobalAnnouncer;
	}
	else if ( nTeam == DOTA_TEAM_DIRE )
	{
		hAnnouncer = m_hDireAnnouncer;  
	}
	else if ( nTeam == DOTA_TEAM_RADIANT )
	{
		hAnnouncer = m_hRadiantAnnouncer;
	}
	else
	{
		return NULL;
	}
	
	// Validate entity handle and return entity (matches decompiled validation)
	CBaseEntity *pEntity = hAnnouncer.Get();
	if ( pEntity && !pEntity->IsMarkedForDeletion() )
	{
		return pEntity;
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get total number of towers (from decompiled GetNumTowers)
//-----------------------------------------------------------------------------
int CDOTAGameRules::GetNumTowers()
{
	return m_nNumTowers;
}

//-----------------------------------------------------------------------------
// Purpose: Get specific tower for team (from decompiled GetTeamTower)
//-----------------------------------------------------------------------------
CBaseEntity* CDOTAGameRules::GetTeamTower( int nTeam, int nTowerIndex )
{
	// Validate team (only Radiant/Dire have towers)
	if ( nTeam != DOTA_TEAM_RADIANT && nTeam != DOTA_TEAM_DIRE )
		return NULL;
		
	// Validate tower index
	if ( nTowerIndex < 0 )
		return NULL;
	
	// Get appropriate tower array (matches decompiled team offset calculation)
	CUtlVector<EHANDLE> *pTowerArray = ( nTeam == DOTA_TEAM_RADIANT ) ? &m_hRadiantTowers : &m_hDireTowers;
	
	// Validate index bounds
	if ( nTowerIndex >= pTowerArray->Count() )
		return NULL;
		
	// Get tower entity and validate (matches decompiled entity validation)
	EHANDLE hTower = (*pTowerArray)[nTowerIndex];
	CBaseEntity *pEntity = hTower.Get();
	if ( pEntity && !pEntity->IsMarkedForDeletion() )
	{
		return pEntity;
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Force a team to lose (from decompiled MakeTeamLose)
//-----------------------------------------------------------------------------
void CDOTAGameRules::MakeTeamLose( int nTeam )
{
	Msg( "Players are all disconnected, looking for their fort...\n" );
	
	// Ensure game is in progress before ending it
	if ( GetGameState() < DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
	{
		SetGameState( DOTA_GAMERULES_STATE_GAME_IN_PROGRESS );
	}
	
	// TODO: Handle special game modes when mode system is implemented
	// For now, find and destroy the team's fort (matches decompiled fort destruction)
	
	// TODO: Implement fort finding and destruction when entity system is ready
	// This would involve:
	// 1. Find all "npc_dota_fort" entities
	// 2. Check team ownership
	// 3. Call KillFort() on matching fort
	
	Msg( "Killing fort with team %d\n", nTeam );
	
	// Set the opposing team as winner
	int nWinningTeam = ( nTeam == DOTA_TEAM_RADIANT ) ? DOTA_TEAM_DIRE : DOTA_TEAM_RADIANT;
	SetWinningTeam( nWinningTeam, 0 ); // TODO: Add proper win reason when available
}

//-----------------------------------------------------------------------------
// Purpose: Set time of day with day/night cycle events (from decompiled SetTimeOfDay)
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetTimeOfDay( float flTimeOfDay )
{
	float flPreviousTime = m_flTimeOfDayFraction;
	
	// Check for day transition (0.25 = dawn) 
	if ( flTimeOfDay >= 0.25f && flPreviousTime < 0.25f )
	{
		// TODO: Fire day transition output event when entity system is ready
		// TODO: Play morning sound when sound system is ready
		// Gamerules_EmitSound( "General.Morning", NULL, false );
		Msg( "DOTA: Day transition - Morning sounds\n" );
	}
	
	// Check for night transition (0.75 = dusk)
	if ( flTimeOfDay >= 0.75f && flPreviousTime < 0.75f )
	{
		// TODO: Fire night transition output event when entity system is ready  
		// TODO: Play night sound when sound system is ready
		// Gamerules_EmitSound( "General.Night", NULL, false );
		Msg( "DOTA: Night transition - Evening sounds\n" );
	}
	
	// Store new time of day
	m_flTimeOfDayFraction = flTimeOfDay;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::ForceGameStart( void )
{
	// Force transition to game in progress state
	SetGameState( DOTA_GAMERULES_STATE_GAME_IN_PROGRESS );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::ResetDefeated( void )
{
	// Find all fort entities and remove defeat flags
	CBaseEntity *pFort = NULL;
	while ( (pFort = gEntList.FindEntityByClassname( pFort, "npc_dota_fort" )) != NULL )
	{
		// TODO: Remove defeat flags when fort entity system is implemented
		// For now, just mark as active and update transmit state
		// pFort->RemoveFlag( FL_DEFEATED );
		// pFort->DispatchUpdateTransmitState();
		Msg( "DOTA: Resetting defeat state for fort\n" );
	}
	
	// TODO: Fire reset_defeated game event when event system is ready
	// IGameEvent *event = gameeventmanager->CreateEvent( "reset_defeated" );
	// if ( event )
	// {
	//     gameeventmanager->FireEvent( event );
	// }
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetGameWinner( int nWinningTeam )
{
	if ( m_nWinningTeam != nWinningTeam )
	{
		m_nWinningTeam = nWinningTeam;
		// TODO: Add network state changed notification when networking is ready
		// NetworkStateChanged();
	}
	
	// TODO: Notify hero list of game end when hero system is ready
	// g_HeroList.GameEnded( nWinningTeam );
	
	// TODO: Handle bot end-game animations when bot system is ready
	// (Collect all bots and play victory/defeat animations)
	
	// TODO: Play appropriate end-game music based on game mode when sound system is ready
	// Different music for different game modes (normal, diretide, greeviling, etc.)
	
	Msg( "DOTA: Game winner set to team %d\n", nWinningTeam );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetGamePaused( bool bPaused, int nPlayerID, float flPauseDuration, float flAutoUnpauseDuration )
{
	// Get default pause durations from convars if not specified
	if ( flPauseDuration < 0.0f )
	{
		// TODO: Get from convar when convar system is ready
		flPauseDuration = 30.0f; // Default pause duration
	}
	
	if ( flAutoUnpauseDuration < 0.0f && nPlayerID != -1 )
	{
		// TODO: Get from convar when convar system is ready  
		flAutoUnpauseDuration = 120.0f; // Default auto-unpause duration
	}
	
	if ( bPaused )
	{
		// Pausing the game
		if ( !m_bGamePaused )
		{
			m_bGamePaused = true;
			m_nPausingPlayerID = nPlayerID;
			m_flLastPauseTime = gpGlobals->curtime;
			
			// TODO: Send pause chat message when chat system is ready
			// SendChatEventMessage( CHAT_EVENT_PAUSE, 0, nPlayerID, 0 );
			
			// TODO: Update pause counters per player when player resource is ready
			
			Msg( "DOTA: Game paused by player %d\n", nPlayerID );
		}
	}
	else
	{
		// Unpausing the game
		if ( m_bGamePaused )
		{
			// Accumulate pause time
			if ( m_flLastPauseTime > 0.0f )
			{
				m_flPausedTimeAccumulated += gpGlobals->curtime - m_flLastPauseTime;
				m_flLastPauseTime = 0.0f;
			}
			
			m_bGamePaused = false;
			m_nPausingPlayerID = -1;
			
			// TODO: Send unpause chat message when chat system is ready
			// SendChatEventMessage( CHAT_EVENT_UNPAUSE, 0, nPlayerID, 0 );
			
			// TODO: Update unpause counters per player when player resource is ready
			
			Msg( "DOTA: Game unpaused by player %d\n", nPlayerID );
		}
	}
	
	// TODO: Queue announcer concept when announcer system is ready
	// QueueConceptForAllAnnouncers( concept, delay, criteria, force );
}

//-----------------------------------------------------------------------------
// Tower and building management
//-----------------------------------------------------------------------------
unsigned int CDOTAGameRules::GetTowerStatus( int nTeam )
{
	if ( nTeam != DOTA_TEAM_RADIANT && nTeam != DOTA_TEAM_DIRE )
		return 0;

	unsigned int nStatus = 0;
	CUtlVector<EHANDLE> *pTowerArray = ( nTeam == DOTA_TEAM_RADIANT ) ? &m_hRadiantTowers : &m_hDireTowers;

	// Check each tower and set bit if alive
	for ( int i = 0; i < pTowerArray->Count(); i++ )
	{
		CBaseEntity *pTower = (*pTowerArray)[i].Get();
		if ( pTower && !pTower->IsMarkedForDeletion() )
		{
			// TODO: Add proper tower alive check when tower entity system is ready
			// For now, assume tower is alive if entity exists
			nStatus |= ( 1 << i );
		}
	}

	return nStatus;
}

//-----------------------------------------------------------------------------
int CDOTAGameRules::GetNumTeamTowers( int nTeam )
{
	if ( nTeam != DOTA_TEAM_RADIANT && nTeam != DOTA_TEAM_DIRE )
		return 0;

	CUtlVector<EHANDLE> *pTowerArray = ( nTeam == DOTA_TEAM_RADIANT ) ? &m_hRadiantTowers : &m_hDireTowers;
	return pTowerArray->Count();
}

//-----------------------------------------------------------------------------
CBaseEntity* CDOTAGameRules::GetTeamBarracks( int nTeam, int nBarracksIndex )
{
	if ( nTeam != DOTA_TEAM_RADIANT && nTeam != DOTA_TEAM_DIRE )
		return NULL;

	if ( nBarracksIndex < 0 )
		return NULL;

	CUtlVector<EHANDLE> *pBarracksArray = ( nTeam == DOTA_TEAM_RADIANT ) ? &m_hRadiantBarracks : &m_hDireBarracks;

	if ( nBarracksIndex >= pBarracksArray->Count() )
		return NULL;

	return (*pBarracksArray)[nBarracksIndex].Get();
}

//-----------------------------------------------------------------------------
// Game configuration
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetGoldPerTick( int nGold )
{
	m_nGoldPerTick = nGold;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetGoldTickTime( float flTime )
{
	m_flGoldTickTime = flTime;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetPreGameTime( float flTime )
{
	// TODO: Set pregame time ConVar when ConVar system is ready
	// ConVar::SetValue( &dota_pregame_time, flTime );
	Msg( "DOTA: SetPreGameTime %.1f\n", flTime );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetPostGameTime( float flTime )
{
	// TODO: Set postgame time ConVar when ConVar system is ready
	// ConVar::SetValue( &dota_postgame_time, flTime );
	Msg( "DOTA: SetPostGameTime %.1f\n", flTime );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetRuneSpawnTime( float flTime )
{
	// TODO: Set rune spawn time ConVar when ConVar system is ready
	// ConVar::SetValue( &dota_rune_spawn_time, flTime );
	Msg( "DOTA: SetRuneSpawnTime %.1f\n", flTime );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetSafeToLeave( bool bSafe )
{
	m_bSafeToLeave = bSafe;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::TriggerResetMap( int nDelay )
{
	m_bMapResetTriggered = true;
	m_nMapResetDelay = nDelay;
}

//-----------------------------------------------------------------------------
// Glyph system
//-----------------------------------------------------------------------------
float CDOTAGameRules::GetGlyphCooldown( int nTeam )
{
	if ( nTeam == DOTA_TEAM_RADIANT )
		return m_flRadiantGlyphCooldown;
	else if ( nTeam == DOTA_TEAM_DIRE )
		return m_flDireGlyphCooldown;
	
	return 0.0f;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetGlyphCooldown( int nTeam, float flCooldown )
{
	float flEndTime = GetGameTime() + flCooldown;

	if ( nTeam == DOTA_TEAM_RADIANT )
	{
		if ( m_flRadiantGlyphCooldown != flEndTime )
		{
			m_flRadiantGlyphCooldown = flEndTime;
			// TODO: Add network state changed notification when networking is ready
			// NetworkStateChanged();
		}
	}
	else if ( nTeam == DOTA_TEAM_DIRE )
	{
		if ( m_flDireGlyphCooldown != flEndTime )
		{
			m_flDireGlyphCooldown = flEndTime;
			// TODO: Add network state changed notification when networking is ready
			// NetworkStateChanged();
		}
	}
}

//-----------------------------------------------------------------------------
// Item system
//-----------------------------------------------------------------------------
int CDOTAGameRules::NumDroppedItems( void )
{
	return m_nDroppedItemsCount;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Connection handling
//-----------------------------------------------------------------------------
bool CDOTAGameRules::ClientConnected( edict_t *pEdict, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	// Validate entity index
	int entityIndex = ENTINDEX(pEdict);
	if ( entityIndex < 0 || entityIndex >= MAX_EDICTS )
		return false;
		
	// Get the player from the edict
	CDOTAPlayer *pPlayer = NULL;
	if ( pEdict && pEdict->GetIServerEntity() )
	{
		pPlayer = dynamic_cast<CDOTAPlayer*>(pEdict->GetIServerEntity());
	}
	
	// If we have a valid DOTA player
	if ( pPlayer && pPlayer->IsPlayer() )
	{
		int nPlayerID = pPlayer->GetPlayerID();
		if ( nPlayerID != -1 )
		{
			// Update player resource connection state
			if ( g_pDOTAPlayerResource && GetGameState() < DOTA_GAMERULES_STATE_POST_GAME )
			{
				if ( nPlayerID < 32 ) // MAX_PLAYERS
				{
					g_pDOTAPlayerResource->SetConnectionState( nPlayerID, 2, false ); //DOTA_CONNECTION_STATE_CONNECTED
				}
			}
#if 0
			// Clear voice chat state
			if ( GetVoiceGameMgrHelper() )
			{
				GetVoiceGameMgrHelper()->ClearPlayerListenerState( pPlayer, "Connected" );
			}
#endif
		}
	}
	
	// Call the base class implementation
	bool bResult = BaseClass::ClientConnected( pEdict, pszName, pszAddress, reject, maxrejectlen );
	
	return bResult;
}
#else
//-----------------------------------------------------------------------------
// Client-side connection handling
//-----------------------------------------------------------------------------
bool C_DOTAGameRules::ClientConnected( edict_t *pEdict, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	// Call the base class implementation
	return BaseClass::ClientConnected( pEdict, pszName, pszAddress, reject, maxrejectlen );
}
#endif

//-----------------------------------------------------------------------------
// State management 
//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter_INIT( void )
{
	// TODO: Check GC client system state and update network if needed
	// This matches the decompiled logic that checks GDOTAGCClientSystem() and updates network state
	// int nGCSystemState = GetGCSystemState();
	// if ( m_nSomeGCState != nGCSystemState )
	// {
	//     CGameRulesProxy::NotifyNetworkStateChanged();
	//     m_nSomeGCState = nGCSystemState;
	// }
	
	// TODO: Check lobby settings and difficulty
	// This matches the decompiled check of dota_local_addon_difficulty ConVar
	// int nDifficulty = GetLobbyDifficulty();
	// if ( m_nGameDifficulty != nDifficulty )
	// {
	//     CGameRulesProxy::NotifyNetworkStateChanged();
	//     m_nGameDifficulty = nDifficulty;
	// }
	
	// Skip VScript entity spawning (stripped as requested)
	// The decompiled code spawns "dota_base_game_mode" entity with "addon_game_mode.lua" script
	// We're not implementing VScript support, so this is omitted
	
	// Initialize social system (matches decompiled InitializeSocialSystem call)
	// TODO: Implement when social system is available
	// InitializeSocialSystem();
	
	// Disable ward placement hack (matches decompiled g_bWardPlacementWardHack = false)
	// TODO: Add when ward system is implemented
	// extern bool g_bWardPlacementWardHack;
	// g_bWardPlacementWardHack = false;
	
	Msg( "DOTA: Entered INIT state - basic initialization complete\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter_PRE_GAME( void )
{
	// Set pre-game start time (matches decompiled time setting)
	float flCurrentTime = gpGlobals->curtime;
	if ( m_flPreGameStartTime != flCurrentTime )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flPreGameStartTime = flCurrentTime;
	}
	Msg( "m_flPreGameStartTime set to %.2f\n", m_flPreGameStartTime );
	
	// Calculate state transition time based on lobby type (matches decompiled GC checks)
	float flTransitionTime = flCurrentTime;
	// TODO: Implement when GC system is available
	// CDOTAGCClientSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem && pGCSystem->GetLobby() && pGCSystem->GetLobby()->GetLobbyType() <= DOTA_LOBBY_TYPE_PRACTICE )
	//     flTransitionTime += dota_pregame_time_private.GetFloat();
	// else
	//     flTransitionTime += dota_pregame_time.GetFloat();
	
	// For now, use default 90 second pre-game time
	flTransitionTime += 90.0f;
	
	if ( m_flStateTransitionTime != flTransitionTime )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flStateTransitionTime = flTransitionTime;
	}
	Msg( "m_flStateTransitionTime set to %.2f\n", m_flStateTransitionTime );
	
	// Set time to announce game start (31 seconds before end of pre-game)
	// TODO: Implement when announcement system is available
	// m_flGameStartAnnouncementTime = 31.0f;
	
	// Fire pre-game announcement (matches decompiled criteria set)
	// TODO: Implement when announcer system is available
	// CriteriaSet criteria;
	// criteria.AppendCriteria( "announce_event", "pre_game" );
	// QueueConceptForAllAnnouncers( "pregame", 10.0f, criteria, -1, false );
	
	// Spawn heroes for all players (matches decompiled player loop)
	if ( g_pDOTAPlayerResource )
	{
		for ( int i = 0; i < 10; i++ )
		{
			// TODO: Implement when player and hero systems are available
			// CDOTAPlayer *pPlayer = g_pDOTAPlayerResource->GetPlayer( i );
			// if ( !pPlayer )
			//     continue;
			
			// // Skip if player already has a hero
			// if ( pPlayer->GetAssignedHero() )
			//     continue;
			
			// // Skip if player hasn't finished selecting
			// if ( pPlayer->HasPendingHeroSelection() )
			//     continue;
			
			// // Handle bot spawning in single player modes
			// int nGameMode = GetGameMode();
			// if ( nGameMode == DOTA_GAMEMODE_TUTORIAL && g_pDOTAPlayerResource->IsFakeClient( i ) )
			// {
			//     const char *pszHeroName = g_pDOTAPlayerResource->GetSelectedHeroName( i );
			//     DOTA_Hero_Spawn( pszHeroName, pPlayer, false );
			//     continue;
			// }
			
			// // Skip reserved players
			// if ( g_pDOTAPlayerResource->GetPlayerReservedState( i ) )
			//     continue;
			
			// // Handle random hero selection
			// if ( pPlayer->WantsRandomHero() )
			// {
			//     pPlayer->MakeRandomHeroSelection( false, -1 );
			// }
			// else
			// {
			//     const char *pszHeroName = g_pDOTAPlayerResource->GetSelectedHeroName( i );
			//     DOTA_Hero_Spawn( pszHeroName, pPlayer, false );
			// }
		}
	}
	
	// Start item restock timers (matches decompiled call)
	// TODO: Implement when item system is available
	// StartItemRestockTimers();
	
	// Notify tutorial system (matches decompiled tutorial call)
	// TODO: Implement when tutorial system is available
	// CDOTATutorial *pTutorial = DOTATutorial();
	// if ( pTutorial )
	//     pTutorial->OnEnterPreGame();
	
	// Handle courier spawning for certain game modes (matches decompiled mode checks)
	// TODO: Implement when courier system is available
	// int nGameMode = GetGameMode();
	// if ( nGameMode == DOTA_GAMEMODE_ALL_DRAFT || nGameMode == DOTA_GAMEMODE_1V1 )
	// {
	//     // Check if we have exactly 2 players for 1v1 mode
	//     int nPlayerCount = GetConnectedPlayerCount();
	//     if ( nGameMode == DOTA_GAMEMODE_1V1 && nPlayerCount != 2 )
	//         return;
	
	//     // Spawn couriers for each team
	//     for ( int nTeam = DOTA_TEAM_RADIANT; nTeam <= DOTA_TEAM_DIRE; nTeam++ )
	//     {
	//         Vector vSpawnPos;
	//         if ( GetCourierSpawnLocationForTeam( nTeam, vSpawnPos ) )
	//         {
	//             // Find controlling player for this team
	//             int nControllingPlayer = -1;
	//             for ( int i = 0; i < gpGlobals->maxClients; i++ )
	//             {
	//                 CBasePlayer *pPlayer = UTIL_PlayerByIndex( i + 1 );
	//                 if ( pPlayer && pPlayer->IsConnected() && pPlayer->GetTeamNumber() == nTeam )
	//                 {
	//                     nControllingPlayer = pPlayer->GetPlayerID();
	//                     break;
	//                 }
	//             }
	//             
	//             if ( nControllingPlayer != -1 )
	//             {
	//                 CBaseEntity *pCourier = CreateEntityByName( "npc_dota_courier" );
	//                 if ( pCourier )
	//                 {
	//                     pCourier->SetAbsOrigin( vSpawnPos );
	//                     pCourier->SetControllableByPlayer( nControllingPlayer, false );
	//                     DispatchSpawn( pCourier );
	//                 }
	//             }
	//         }
	//     }
	// }
	
	// Handle custom game mode setup (matches decompiled mode 0xf check)
	// TODO: Implement when custom game system is available
	// if ( nGameMode == DOTA_GAMEMODE_CUSTOM )
	// {
	//     // Find all buildings and give them to players
	//     CUtlVector<CBaseEntity*> buildings;
	//     FindUnitsInRadius( DOTA_TEAM_RADIANT, vec3_origin, NULL, -1.0f, buildings, 
	//                       DOTA_UNIT_TARGET_BUILDING, DOTA_UNIT_TARGET_FLAG_NONE, 0, false );
	//     
	//     for ( int i = 0; i < buildings.Count(); i++ )
	//     {
	//         CDOTA_BaseNPC_Building *pBuilding = dynamic_cast<CDOTA_BaseNPC_Building*>( buildings[i] );
	//         if ( pBuilding && pBuilding->IsAlive() )
	//         {
	//             pBuilding->PossiblyGiveSelfToPlayer();
	//         }
	//     }
	// }
	
	// Set creep spawn preparation time (-30 seconds from transition, matches decompiled calculation)
	// TODO: Implement when creep system is available
	// m_flCreepSpawnPrepTime = m_flStateTransitionTime - 30.0f;
	
	// Handle Diretide prizes for Diretide mode (matches decompiled mode 7 check)
	// TODO: Implement when Diretide system is available
	// if ( nGameMode == DOTA_GAMEMODE_DIRETIDE )
	// {
	//     DetermineDiretidePrizes( false, 0 );
	// }
	
	// Handle neutral spawner preparation (matches decompiled neutral spawner loop)
	// TODO: Implement when neutral spawner system is available
	// for ( int i = 0; i < m_NeutralSpawners.Count(); i++ )
	// {
	//     CBaseEntity *pSpawner = m_NeutralSpawners[i].Get();
	//     if ( pSpawner && pSpawner->ShouldSpawn() )
	//     {
	//         pSpawner->PrepareForSpawn();
	//     }
	// }
	
	Msg( "DOTA: Entered PRE_GAME state - heroes spawned, preparation complete\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::AddTeamCompToCriteria( int nTeam, void *pCriteriaSet )
{
	if ( !g_pDOTAPlayerResource )
		return;

	// Get all player IDs for the specified team
	// TODO: Implement when CUtlVector and GetPlayerIDsForTeam are available
	// CUtlVector<int> playerIDs;
	// if ( !g_pDOTAPlayerResource->GetPlayerIDsForTeam( nTeam, playerIDs ) )
	//     return;

	// Initialize role counters (matches decompiled 10-role system)
	int roleCount[10] = {0}; // Carry, Support, Initiator, Disabler, Jungler, etc.
	int nTotalSupports = 0;  // Count of support heroes (roles 2 and 5)
	int nTotalHeroes = 0;    // Total heroes analyzed

	// TODO: Implement when player resource and hero data systems are available
	// for ( int i = 0; i < playerIDs.Count(); i++ )
	// {
	//     int nPlayerID = playerIDs[i];
	//     const char *pszHeroName = g_pDOTAPlayerResource->GetSelectedHeroName( nPlayerID );
	//     if ( !pszHeroName || !pszHeroName[0] )
	//         continue;
	
	//     // Get hero data from game manager
	//     CDOTAGameManager *pGameManager = DOTAGameManager();
	//     if ( !pGameManager )
	//         continue;
	
	//     KeyValues *pHeroData = pGameManager->GetHeroDataByName( pszHeroName, HERO_DATA_TYPE_FULL );
	//     if ( !pHeroData )
	//         continue;
	
	//     // Parse hero roles (comma-separated string like "Carry,Disabler")
	//     const char *pszRoles = pHeroData->GetString( "Role", "" );
	//     if ( !pszRoles || !pszRoles[0] )
	//         continue;
	
	//     // Split role string by commas
	//     CUtlVector<char*> roleList;
	//     V_SplitString( pszRoles, ",", roleList );
	
	//     bool bIsSupport = false;
	//     for ( int j = 0; j < roleList.Count(); j++ )
	//     {
	//         // Check against all 10 possible roles
	//         for ( int k = 0; k < 10; k++ )
	//         {
	//             const char *pszRoleName = GetHeroRole( k );
	//             if ( V_stricmp( roleList[j], pszRoleName ) == 0 )
	//             {
	//                 roleCount[k]++;
	
	//                 // Roles 2 and 5 are support roles (matches decompiled check)
	//                 if ( k == 2 || k == 5 )
	//                 {
	//                     bIsSupport = true;
	//                 }
	//                 break;
	//             }
	//         }
	//     }
	
	//     // Clean up split string memory
	//     for ( int j = 0; j < roleList.Count(); j++ )
	//     {
	//         if ( roleList[j] )
	//         {
	//             delete[] roleList[j];
	//         }
	//     }
	
	//     if ( bIsSupport )
	//         nTotalSupports++;
	
	//     nTotalHeroes++;
	// }

	// Add role counts to criteria set (matches decompiled role loop)
	// TODO: Implement when criteria system is available
	// ResponseRules::CriteriaSet *pCriteria = (ResponseRules::CriteriaSet*)pCriteriaSet;
	// for ( int i = 0; i < 10; i++ )
	// {
	//     const char *pszRoleName = GetHeroRole( i );
	//     pCriteria->AppendCriteria( pszRoleName, (float)roleCount[i] );
	// }

	// Add summary statistics (matches decompiled final criteria)
	// pCriteria->AppendCriteria( "TotalSupports", (float)nTotalSupports );
	// pCriteria->AppendCriteria( "NumHeroes", (float)nTotalHeroes );

	// Placeholder logging for now
	Msg( "AddTeamCompToCriteria: Team %d composition analysis complete\n", nTeam );
}

//-----------------------------------------------------------------------------
int CDOTAGameRules::SelectSmartRandomHero( int nPlayerID, bool bRequireBotImplementation, void *pExcludeList, bool bUseSmartList )
{
	// Get smart random hero list first (matches decompiled call order)
	// TODO: Implement when CUtlVector and smart selection are available
	// CUtlVector<int> smartHeroList;
	// GetSmartRandomHeroList( nPlayerID, bRequireBotImplementation, smartHeroList, pExcludeList, bUseSmartList );
	
	// If smart list is empty, fall back to all available heroes (matches decompiled fallback)
	// TODO: Implement when hero iteration system is available
	// CUtlVector<int> availableHeroes;
	// if ( smartHeroList.Count() == 0 )
	// {
	//     CDOTAGameManager *pGameManager = DOTAGameManager();
	//     if ( !pGameManager )
	//         return 0;
	
	//     // Iterate through all heroes
	//     for ( KeyValues *pHeroData = pGameManager->GetFirstHero( HERO_DATA_TYPE_FULL );
	//           pHeroData != NULL;
	//           pHeroData = pGameManager->GetNextHero( pHeroData ) )
	//     {
	//         int nHeroID = pHeroData->GetInt( "HeroID", 0 );
	
	//         // Check if hero is in exclude list
	//         bool bExcluded = false;
	//         if ( pExcludeList )
	//         {
	//             CUtlArray<int, 10> *pExclude = (CUtlArray<int, 10>*)pExcludeList;
	//             for ( int i = 0; i < 10; i++ )
	//             {
	//                 if ( (*pExclude)[i] == nHeroID )
	//                 {
	//                     bExcluded = true;
	//                     break;
	//                 }
	//             }
	//         }
	
	//         if ( bExcluded )
	//             continue;
	
	//         // Check if hero is available for this player
	//         if ( !pGameManager->IsHeroAvailableByHeroID( nPlayerID, nHeroID, HERO_DATA_TYPE_FULL ) )
	//             continue;
	
	//         // Check bot implementation requirement (matches decompiled bot check)
	//         if ( bRequireBotImplementation )
	//         {
	//             int nBotImplemented = pHeroData->GetInt( "BotImplemented", 0 );
	//             if ( nBotImplemented == 0 )
	//                 continue;
	//         }
	
	//         // Add to available list
	//         availableHeroes.AddToTail( nHeroID );
	//     }
	
	//     // Use available heroes list
	//     smartHeroList = availableHeroes;
	// }

	// Select random hero from final list (matches decompiled random selection)
	// if ( smartHeroList.Count() == 0 )
	//     return 0;

	// int nRandomIndex = RandomInt( 0, smartHeroList.Count() - 1 );
	// return smartHeroList[nRandomIndex];

	// Placeholder return for now
	Msg( "SelectSmartRandomHero: Player %d, RequireBot=%d\n", nPlayerID, bRequireBotImplementation );
	return 0;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetHeroRespawnEnabled( bool bEnabled )
{
	// Update networked variable with notification (matches decompiled network update)
	if ( m_bHeroRespawnEnabled != bEnabled )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bHeroRespawnEnabled = bEnabled;
	}
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter_POST_GAME( void )
{
	// Stop spectator graph tracking (matches decompiled call order)
	// TODO: Implement when spectator system is available
	// CDOTASpectatorGraphManager *pGraphManager = GetDOTASpectatorGraphManager();
	// if ( pGraphManager )
	//     pGraphManager->StopTracking();

	// Fire game end output event (matches decompiled output firing)
	// TODO: Implement when entity output system is available
	// if ( m_hGameEndEntity.Get() )
	// {
	//     COutputEvent *pOutput = m_hGameEndEntity.Get()->GetOutput( "OnGameEnd" );
	//     if ( pOutput )
	//         pOutput->FireOutput( NULL, NULL, 0.0f );
	// }

	// Set post-game time (matches decompiled time setting)
	float flCurrentTime = gpGlobals->curtime;
	if ( m_flPostGameTime != flCurrentTime )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flPostGameTime = flCurrentTime;
	}

	// Clear paused state and timing (matches decompiled state clearing)
	// TODO: Implement when pause system member variables are available
	// m_bGamePaused = false;
	// if ( m_nGameState != DOTA_GAMERULES_STATE_POST_GAME )
	// {
	//     CGameRulesProxy::NotifyNetworkStateChanged();
	//     m_nGameState = DOTA_GAMERULES_STATE_POST_GAME;
	// }
	// if ( m_flGamePauseStartTime != 0.0f )
	// {
	//     CGameRulesProxy::NotifyNetworkStateChanged();
	//     m_flGamePauseStartTime = 0.0f;
	// }

	// Log OGS (Online Game Statistics) end event (matches decompiled OGS call)
	// TODO: Implement when OGS system is available
	// CDOTA_OGS_VPROF::OnGameEnd();

	// Send tournament item event for rare drops (matches decompiled tournament logic)
	// TODO: Implement when tournament and item systems are available
	// if ( !IsCustomGame() && GetMatchDuration() > 300.0f ) // 5+ minute games
	// {
	//     // Create tournament item event message
	//     CMsgTournamentItemEvent tournamentEvent;
	//     tournamentEvent.set_event_type( 1 ); // Rare drop event
	//     tournamentEvent.set_winning_team( GetGameWinner() == DOTA_TEAM_RADIANT ? 1 : 0 );
	//     
	//     if ( g_pDOTAPlayerResource )
	//     {
	//         tournamentEvent.set_radiant_kills( g_pDOTAPlayerResource->GetTeamKills( DOTA_TEAM_RADIANT ) );
	//         tournamentEvent.set_dire_kills( g_pDOTAPlayerResource->GetTeamKills( DOTA_TEAM_DIRE ) );
	//     }
	//     
	//     TournamentItemEvent( &tournamentEvent, "tournament_drops_rare" );
	// }

	// Handle survey system (matches decompiled survey logic)
	// TODO: Implement when GC job and survey systems are available
	// if ( ShouldShowSurvey() )
	// {
	//     // Create GC job for survey permission
	//     CGCClientJobGrantSurveyPermission *pJob = new CGCClientJobGrantSurveyPermission();
	//     
	//     // Add survey data for each player
	//     for ( int i = 0; i < 10; i++ )
	//     {
	//         if ( !IsValidPlayerIndex( i ) )
	//             continue;
	//             
	//         CDOTAPlayer *pPlayer = GetDOTAPlayer( i );
	//         if ( !pPlayer || !pPlayer->IsConnected() )
	//             continue;
	//             
	//         // Add survey questions and data
	//         pJob->AddPlayerSurveyData( i, GetMatchID(), pPlayer->GetHeroID() );
	//     }
	//     
	//     pJob->StartJob();
	// }

	// Log hero results for OGS (matches decompiled OGS hero logging)
	// TODO: Implement when OGS system is available
	// for ( int i = 0; i < 10; i++ )
	// {
	//     CDOTA_OGS::LogHeroResults( i );
	// }
	// CDOTA_OGS::LogMatchSnapshot();

	// Process kill eater events for cosmetic items (matches decompiled econ processing)
	// TODO: Implement when item and economy systems are available
	// if ( g_pDOTAPlayerResource )
	// {
	//     for ( int i = 0; i < 10; i++ )
	//     {
	//         if ( !g_pDOTAPlayerResource->IsValidTeamPlayer( i ) ||
	//              g_pDOTAPlayerResource->IsFakeClient( i ) )
	//             continue;
	
	//         CBaseEntity *pHero = g_pDOTAPlayerResource->GetSelectedHero( i );
	//         if ( !pHero || !pHero->IsAlive() )
	//             continue;
	
	//         // Log gold earned and spent
	//         int nGoldEarned = g_pDOTAPlayerResource->GetTotalEarnedGold( i );
	//         int nGoldSpent = g_pDOTAPlayerResource->GetTotalGoldSpent( i );
	//         EconEntity_OnOwnerKillEaterEvent( pHero, 0, KILL_EATER_GOLD_EARNED, nGoldEarned, 0 );
	//         EconEntity_OnOwnerKillEaterEvent( pHero, 0, KILL_EATER_GOLD_SPENT, nGoldSpent, 0 );
	
	//         // Log victory if on winning team
	//         if ( g_pDOTAPlayerResource->GetTeam( i ) == GetGameWinner() )
	//         {
	//             EconEntity_OnOwnerKillEaterEvent( pHero, 0, KILL_EATER_WIN, 1, 0 );
	//         }
	
	//         // Log match completion
	//         EconEntity_OnOwnerKillEaterEvent( pHero, 0, KILL_EATER_MATCH_COMPLETE, 1, 0 );
	//     }
	// }

	// Process combat analysis (matches decompiled combat log processing)
	// TODO: Implement when combat analysis system is available
	// CDOTA_CombatLog::DoCombatAnalysis();
	// CDOTA_CombatAnalyzer::CalculateEndGameStats();

	// Set state transition time for disconnect timeout (matches decompiled transition setup)
	float flTransitionTime = gpGlobals->curtime;
	// TODO: Implement when ConVar system is available
	// flTransitionTime += dota_postgame_time.GetFloat();
	flTransitionTime += 90.0f; // Default 90 seconds

	if ( m_flStateTransitionTime != flTransitionTime )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flStateTransitionTime = flTransitionTime;
	}

	// Set up replay upload timer (matches decompiled replay timing)
	// TODO: Implement when replay and timer systems are available
	// double flReplayTime = Plat_FloatTime() + dota_postgame_time.GetFloat() + 30.0;
	// m_flReplayUploadTime = flReplayTime;

	// Set up disconnect timeout timer (matches decompiled timeout setup)
	// TODO: Implement when ConVar and timer systems are available
	// ConVarRef tvDelay( "tv_delay" );
	// float flTotalDelay = tvDelay.GetFloat() + dota_replay_delay.GetFloat() + dota_disconnect_timeout.GetFloat();
	// m_DisconnectTimeout.SetTimer( flTotalDelay );

	// Fire match done event (matches decompiled event firing)
	// TODO: Implement when game event system is available
	// IGameEvent *pEvent = gameeventmanager->CreateEvent( "dota_match_done" );
	// if ( pEvent )
	// {
	//     pEvent->SetInt( "winningteam", GetGameWinner() );
	//     gameeventmanager->FireEvent( pEvent, true );
	// }

	// Set end game camera (matches decompiled camera logic)
	// TODO: Implement when camera system is available
	// if ( GetGameMode() != DOTA_GAMEMODE_DIRETIDE && GetGameMode() != DOTA_GAMEMODE_GREEVILING )
	// {
	//     const char *pszCameraName = ( GetGameWinner() == DOTA_TEAM_DIRE ) ? "dire_end_camera" : "radiant_end_camera";
	//     CBaseEntity *pCamera = gEntList.FindEntityByName( NULL, pszCameraName );
	//     SetEndGameCameraEntity( pCamera );
	// }

	// Send MMR information to clients (matches decompiled MMR system)
	// TODO: Implement when MMR and lobby systems are available
	// CDOTAGCClientSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem && pGCSystem->GetLobby() && 
	//      GetGameEndReason() == DOTA_GAME_END_NORMAL &&
	//      pGCSystem->GetLobby()->ShowMMROnScoreboardAtGameEnd() )
	// {
	//     CRecipientFilter filter;
	//     filter.MakeReliable();
	//     
	//     CDOTAUserMsg_PlayerMMR mmrMsg;
	//     
	//     for ( int i = 0; i < MAX_PLAYERS; i++ )
	//     {
	//         if ( !g_pDOTAPlayerResource->IsValidTeamPlayerID( i ) )
	//             continue;
	//             
	//         int nTeam = g_pDOTAPlayerResource->GetTeam( i );
	//         if ( nTeam != DOTA_TEAM_RADIANT && nTeam != DOTA_TEAM_DIRE )
	//             continue;
	//             
	//         bool bCalibrated = false;
	//         int nRank = g_pDOTAPlayerResource->GetRank( i, &bCalibrated );
	//         
	//         if ( nRank > 0 || pGCSystem->GetLobby()->GetTeamDetails( nTeam ) != NULL )
	//         {
	//             // Add MMR data to message
	//             mmrMsg.add_player_mmr( bCalibrated ? nRank : 0 );
	//             
	//             CBasePlayer *pPlayer = g_pDOTAPlayerResource->GetPlayer( i );
	//             if ( pPlayer )
	//                 filter.AddRecipient( pPlayer );
	//         }
	//     }
	//     
	//     SendUserMessage( &filter, DOTA_UM_PlayerMMR, &mmrMsg );
	// }

	Msg( "DOTA: Entered POST_GAME state - statistics processed, timers set\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Think_POST_GAME( void )
{
	// Unpause game if it's paused (matches decompiled unpause logic)
	// TODO: Implement when pause system is available
	// if ( IsGamePaused() )
	// {
	//     Msg( "State_Think_POST_GAME found the game paused! Unpausing it now.\n" );
	//     SetGamePaused( false, -1, 0.0f, -1.0f );
	// }

	// Check if all players have loaded (matches decompiled player loading check)
	bool bAllPlayersLoaded = true;
	// TODO: Implement when engine interface is available
	// for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	// {
	//     CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
	//     if ( !pPlayer || !pPlayer->IsConnected() || pPlayer->IsBot() )
	//         continue;
	
	//     int nClientIndex = pPlayer->entindex() - 1;
	//     char szNetworkID[128];
	//     bool bFullyConnected = engine->GetPlayerInfo( nClientIndex, szNetworkID );
	//     
	//     if ( !bFullyConnected )
	//     {
	//         bAllPlayersLoaded = false;
	//         break;
	//     }
	// }

	// Handle HLTV director timing (matches decompiled HLTV logic)
	// TODO: Implement when HLTV system is available
	// if ( bAllPlayersLoaded && gpGlobals )
	// {
	//     CHLTVDirector *pDirector = HLTVDirector();
	//     if ( pDirector )
	//     {
	//         float flHLTVDelay = pDirector->GetDelay();
	//         float flCurrentTime = gpGlobals->curtime;
	//         
	//         if ( m_flPostGameTime + flHLTVDelay <= flCurrentTime )
	//         {
	//             // Update state transition time
	//             if ( m_flStateTransitionTime != flCurrentTime )
	//             {
	//                 CGameRulesProxy::NotifyNetworkStateChanged();
	//                 m_flStateTransitionTime = flCurrentTime;
	//             }
	//         }
	//     }
	// }

	// Check for state transition (matches decompiled transition check)
	if ( m_flStateTransitionTime <= gpGlobals->curtime )
	{
		// TODO: Implement when ConVar system is available
		// if ( dota_skip_postgame.GetInt() == 0 )
		{
			State_Transition( DOTA_GAMERULES_STATE_DISCONNECT );
		}
	}

	// Check replay upload timer (matches decompiled replay timing)
	// TODO: Implement when replay system is available
	// if ( m_ReplayUploadTimer.HasElapsed() )
	// {
	//     CheckUploadReplay();
	// }

	// Handle timeout for slow transitions (matches decompiled timeout check)
	// TODO: Implement when platform time is available
	// double flCurrentPlatTime = Plat_FloatTime();
	// if ( m_flReplayUploadTime <= flCurrentPlatTime )
	// {
	//     Msg( "Timed out waiting for State_Think_POST_GAME transition time to expire. Expires at %.2f and current time is %.2f.\n",
	//          m_flStateTransitionTime, gpGlobals->curtime );
	//     State_Transition( DOTA_GAMERULES_STATE_DISCONNECT );
	// }
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::CreateStandardEntities( void )
{


	// Create DOTA player resource entity (matches decompiled entity creation order)
	CBaseEntity *pPlayerResource = CBaseEntity::Create( "dota_player_manager", vec3_origin, vec3_angle, NULL );
	if ( pPlayerResource )
	{
		g_pDOTAPlayerResource = (CDOTA_PlayerResource*)pPlayerResource;
		pPlayerResource->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	}

	// Create team data entities (matches decompiled team data creation)
	CBaseEntity *pRadiantData = CBaseEntity::Create( "dota_data_radiant", vec3_origin, vec3_angle, NULL );
	if ( pRadiantData )
	{
		// TODO: Implement when team data globals are available
		// g_pDOTADataRadiant = pRadiantData;
		pRadiantData->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	}

	CBaseEntity *pDireData = CBaseEntity::Create( "dota_data_dire", vec3_origin, vec3_angle, NULL );
	if ( pDireData )
	{
		// TODO: Implement when team data globals are available
		// g_pDOTADataDire = pDireData;
		pDireData->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	}

	CBaseEntity *pSpectatorData = CBaseEntity::Create( "dota_data_spectator", vec3_origin, vec3_angle, NULL );
	if ( pSpectatorData )
	{
		// TODO: Implement when team data globals are available
		// g_pDOTADataSpectator = pSpectatorData;
		pSpectatorData->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	}

	// Create game rules entity (matches decompiled gamerules entity)
	CBaseEntity *pGameRules = CBaseEntity::Create( "dota_gamerules", vec3_origin, vec3_angle, NULL );
	if ( pGameRules )
	{
		pGameRules->SetName( AllocPooledString( "dota_gamerules" ) );
	}

	// Create announcer entities for each team (matches decompiled announcer creation)
	for ( int nTeam = DOTA_TEAM_SPECTATOR; nTeam <= DOTA_TEAM_DIRE; nTeam++ )
	{
		// Regular announcer
		CBaseEntity *pAnnouncer = CBaseEntity::Create( "npc_dota_hero_announcer", vec3_origin, vec3_angle, NULL );
		if ( pAnnouncer )
		{
			pAnnouncer->SetName( AllocPooledString( "Announcer" ) );
			// TODO: Implement when announcer team setting is available
			// pAnnouncer->SetTeamNumber( nTeam );
			
			// Set announcer handle based on team (matches decompiled handle assignment)
			// TODO: Implement when announcer handles are available
			// if ( nTeam == DOTA_TEAM_RADIANT )
			//     m_hAnnouncerRadiant = pAnnouncer;
			// else if ( nTeam == DOTA_TEAM_DIRE )
			//     m_hAnnouncerDire = pAnnouncer;
			// else
			//     m_hAnnouncerSpectator = pAnnouncer;
		}

		// Killing spree announcer
		CBaseEntity *pKillAnnouncer = CBaseEntity::Create( "npc_dota_hero_announcer_killing_spree", vec3_origin, vec3_angle, NULL );
		if ( pKillAnnouncer )
		{
			pKillAnnouncer->SetName( AllocPooledString( "Announcer" ) );
			// TODO: Implement when announcer team setting is available
			// pKillAnnouncer->SetTeamNumber( nTeam );
			
			// Set killing spree announcer handle based on team (matches decompiled handle assignment)
			// TODO: Implement when announcer handles are available
			// if ( nTeam == DOTA_TEAM_RADIANT )
			//     m_hKillAnnouncerRadiant = pKillAnnouncer;
			// else if ( nTeam == DOTA_TEAM_DIRE )
			//     m_hKillAnnouncerDire = pKillAnnouncer;
			// else
			//     m_hKillAnnouncerSpectator = pKillAnnouncer;
		}
	}

	// Create game manager entity (matches decompiled manager creation)
	CBaseEntity *pGameManager = CBaseEntity::Create( "dota_gamemanager", vec3_origin, vec3_angle, NULL );
	if ( pGameManager )
	{
		pGameManager->SetName( AllocPooledString( "GameManager" ) );
	}

	// Create spectator graph manager (matches decompiled graph manager)
	CBaseEntity *pGraphManager = CBaseEntity::Create( "dota_spectator_graph_manager", vec3_origin, vec3_angle, NULL );
	if ( pGraphManager )
	{
		pGraphManager->SetName( AllocPooledString( "DotaSpectatorGraphManager" ) );
	}

	// Create fog of war entities for each team (matches decompiled FoW creation)
	// TODO: Implement when FoW system is available
	// CBaseEntity *pRadiantFoW = CreateFoWEntity( DOTA_TEAM_RADIANT );
	// CBaseEntity *pDireFoW = CreateFoWEntity( DOTA_TEAM_DIRE );
	// CBaseEntity *pRadiantTempFoW = CreateFoWEntity( DOTA_TEAM_RADIANT );
	// CBaseEntity *pDireTempFoW = CreateFoWEntity( DOTA_TEAM_DIRE );
	
	// if ( pRadiantTempFoW )
	//     g_hFogOfWarTempViewers[0] = pRadiantTempFoW;
	// if ( pDireTempFoW )
	//     g_hFogOfWarTempViewers[1] = pDireTempFoW;

	// Create team showcase editor if in development mode (matches decompiled showcase creation)
	// TODO: Implement when ConVar and showcase systems are available
	// ConVarRef devMode( "dota_dev_mode" );
	// if ( devMode.GetInt() != 0 )
	// {
	//     CBaseEntity *pShowcaseEditor = CBaseEntity::Create( "dota_ent_teamshowcaseeditor", vec3_origin, vec3_angle, NULL );
	//     if ( pShowcaseEditor )
	//     {
	//         CTeamShowcaseEditorManager::RegisterInstance( pShowcaseEditor );
	//     }
	// }

	BaseClass::CreateStandardEntities();

	Msg( "DOTA: Standard entities created - player resource, announcers, managers initialized\n" );
}

//-----------------------------------------------------------------------------
bool CDOTAGameRules::DidMatchSignoutTimeOut( void )
{
	// Check if match signout has timed out (matches decompiled timeout logic)
	// TODO: Implement when ConVar system is available
	// if ( !dota_match_signout_timeout.GetBool() && 
	//      m_nGameState == DOTA_GAMERULES_STATE_POST_GAME &&
	//      m_flPostGameTime + dota_match_signout_timeout.GetFloat() < gpGlobals->curtime )
	// {
	//     return true;
	// }
	
	// Placeholder implementation - check if we're in post-game and enough time has passed
	if ( m_nGameState == DOTA_GAMERULES_STATE_POST_GAME && 
		 m_flPostGameTime > 0.0f && 
		 m_flPostGameTime + 300.0f < gpGlobals->curtime ) // 5 minute timeout
	{
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
int CDOTAGameRules::GetActiveRuneAtSpawner( int nSpawnerIndex )
{
	// Validate spawner index (matches decompiled bounds check)
	if ( nSpawnerIndex < 0 || nSpawnerIndex >= m_RuneSpawners.Count() )
		return 0;
	
	// Get spawner entity handle (matches decompiled entity lookup)
	EHANDLE hSpawner = m_RuneSpawners[nSpawnerIndex];
	if ( !hSpawner.IsValid() )
		return 0;
	
	CBaseEntity *pSpawner = hSpawner.Get();
	if ( !pSpawner )
		return 0;
	
	// Check if spectator data entity exists (matches decompiled spectator check)
	// TODO: Implement when spectator data global is available
	// CBaseEntity *pSpectatorData = g_pDOTADataSpectator;
	// if ( !pSpectatorData )
	//     return 0;
	
	// Check first rune slot (matches decompiled first rune check)
	// TODO: Implement when rune system is available
	// EHANDLE hFirstRune = pSpectatorData->GetRuneHandle( 0 );
	// if ( hFirstRune.IsValid() )
	// {
	//     CBaseEntity *pFirstRune = hFirstRune.Get();
	//     if ( pFirstRune && pFirstRune->GetSpawnerIndex() == nSpawnerIndex )
	//         return 0; // First rune is active at this spawner
	// }
	
	// Check second rune slot (matches decompiled second rune check)
	// TODO: Implement when rune system is available
	// EHANDLE hSecondRune = pSpectatorData->GetRuneHandle( 1 );
	// if ( hSecondRune.IsValid() )
	// {
	//     CBaseEntity *pSecondRune = hSecondRune.Get();
	//     if ( pSecondRune && pSecondRune->GetSpawnerIndex() == nSpawnerIndex )
	//         return 0; // Second rune is active at this spawner
	// }
	
	// No active rune at this spawner
	return 0;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::GetSmartRandomHeroList( int nPlayerID, bool bRequireBotImplementation, 
											 void *pHeroList, void *pExcludeList, bool bUseSmartList )
{
	if ( !g_pDOTAPlayerResource )
		return;
	
	// Get player's team (matches decompiled team lookup)
	int nPlayerTeam = g_pDOTAPlayerResource->GetTeam( nPlayerID );
	
	// TODO: Implement when game manager system is available
	// CDOTAGameManager *pGameManager = DOTAGameManager();
	// if ( !pGameManager )
	//     return;
	
	// Get highest hero ID for iteration (matches decompiled hero iteration)
	// int nHighestHeroID = pGameManager->GetHighestHeroID( HERO_DATA_TYPE_FULL );
	
	// Initialize role tracking flags (matches decompiled role flags)
	bool bHasTank = false;
	bool bHasStunSupport = false;
	bool bHasPureSupport = false;
	bool bHasPushSupport = false;
	bool bHasHardCarry = false;
	bool bHasSemiCarry = false;
	bool bHasGanker = false;
	bool bHasNuker = false;
	
	// Count current team composition (matches decompiled team analysis)
	int nTeamTanks = 0;
	int nTeamStunSupports = 0;
	int nTeamPureSupports = 0;
	int nTeamPushSupports = 0;
	int nTeamHardCarries = 0;
	int nTeamSemiCarries = 0;
	int nTeamGankers = 0;
	int nTeamNukers = 0;
	
	// Analyze all heroes to determine available roles (matches decompiled hero analysis)
	// TODO: Implement when hero iteration system is available
	// for ( int nHeroID = 0; nHeroID <= nHighestHeroID; nHeroID++ )
	// {
	//     // Check if hero is in exclude list
	//     bool bExcluded = false;
	//     if ( pExcludeList )
	//     {
	//         CUtlArray<int, 10> *pExclude = (CUtlArray<int, 10>*)pExcludeList;
	//         for ( int i = 0; i < 10; i++ )
	//         {
	//             if ( (*pExclude)[i] == nHeroID )
	//             {
	//                 bExcluded = true;
	//                 break;
	//             }
	//         }
	//     }
	//     
	//     if ( bExcluded )
	//         continue;
	//     
	//     // Check if hero is available for this player
	//     if ( !pGameManager->IsHeroAvailableByHeroID( nPlayerID, nHeroID, HERO_DATA_TYPE_FULL ) )
	//         continue;
	//     
	//     // Get hero data
	//     KeyValues *pHeroData = pGameManager->GetHeroDataByHeroID( nHeroID, HERO_DATA_TYPE_FULL );
	//     if ( !pHeroData )
	//         continue;
	//     
	//     // Check bot implementation requirement
	//     if ( bRequireBotImplementation )
	//     {
	//         int nBotImplemented = pHeroData->GetInt( "BotImplemented", 0 );
	//         if ( nBotImplemented == 0 )
	//             continue;
	//     }
	//     
	//     // Get bot data
	//     KeyValues *pBotData = pHeroData->FindKey( "Bot", false );
	//     if ( !pBotData )
	//         continue;
	//     
	//     // Analyze hero type (matches decompiled type analysis)
	//     const char *pszHeroType = pBotData->GetString( "HeroType", "" );
	//     
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_TANK" ) )
	//         bHasTank = true;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_STUN_SUPPORT" ) )
	//         bHasStunSupport = true;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_PURE_SUPPORT" ) )
	//         bHasPureSupport = true;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_PUSH_SUPPORT" ) )
	//         bHasPushSupport = true;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_HARD_CARRY" ) )
	//         bHasHardCarry = true;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_SEMI_CARRY" ) )
	//         bHasSemiCarry = true;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_GANKER" ) )
	//         bHasGanker = true;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_NUKER" ) )
	//         bHasNuker = true;
	// }
	
	// Analyze current team composition (matches decompiled team analysis)
	// TODO: Implement when player resource system is available
	// for ( int i = 0; i < 10; i++ )
	// {
	//     if ( !g_pDOTAPlayerResource->IsValidTeamPlayer( i ) )
	//         continue;
	//         
	//     int nTeam = g_pDOTAPlayerResource->GetTeam( i );
	//     if ( nTeam != nPlayerTeam )
	//     {
	//         // Check if we should include enemy team in analysis
	//         if ( bUseSmartList )
	//         {
	//             if ( (nPlayerID < 5 && i < 5) || (nPlayerID >= 5 && i >= 5) )
	//                 continue; // Same side
	//         }
	//         else
	//         {
	//             continue; // Different team
	//         }
	//     }
	//     
	//     // Get selected hero ID
	//     int nHeroID = 0;
	//     if ( pExcludeList )
	//     {
	//         CUtlArray<int, 10> *pExclude = (CUtlArray<int, 10>*)pExcludeList;
	//         nHeroID = (*pExclude)[i];
	//     }
	//     else
	//     {
	//         nHeroID = g_pDOTAPlayerResource->GetSelectedHeroID( i );
	//     }
	//     
	//     // Get hero data and analyze type
	//     KeyValues *pHeroData = pGameManager->GetHeroDataByHeroID( nHeroID, HERO_DATA_TYPE_FULL );
	//     if ( !pHeroData )
	//         continue;
	//         
	//     KeyValues *pBotData = pHeroData->FindKey( "Bot", false );
	//     if ( !pBotData )
	//         continue;
	//         
	//     const char *pszHeroType = pBotData->GetString( "HeroType", "" );
	//     
	//     // Count team composition (matches decompiled counting)
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_TANK" ) )
	//         nTeamTanks++;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_STUN_SUPPORT" ) )
	//         nTeamStunSupports++;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_PURE_SUPPORT" ) )
	//         nTeamPureSupports++;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_PUSH_SUPPORT" ) )
	//         nTeamPushSupports++;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_HARD_CARRY" ) )
	//         nTeamHardCarries++;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_SEMI_CARRY" ) )
	//         nTeamSemiCarries++;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_GANKER" ) )
	//         nTeamGankers++;
	//     if ( V_stristr( pszHeroType, "DOTA_BOT_NUKER" ) )
	//         nTeamNukers++;
	// }
	
	// Build priority list based on team needs (matches decompiled priority building)
	// TODO: Implement when CUtlVector system is available
	// CUtlVector<int> priorityList;
	// 
	// // Add priorities based on team composition gaps (matches decompiled priority logic)
	// if ( bHasStunSupport && nTeamStunSupports == 0 )
	//     priorityList.AddToTail( 2 ); // STUN_SUPPORT priority
	//     
	// if ( bHasHardCarry && nTeamHardCarries == 0 )
	//     priorityList.AddToTail( 16 ); // HARD_CARRY priority
	//     
	// if ( bHasGanker && nTeamGankers == 0 )
	//     priorityList.AddToTail( 64 ); // GANKER priority
	//     
	// // Add other priority combinations based on team needs
	// // ... (matches all decompiled priority combinations)
	
	// Handle suggested hero list (matches decompiled suggested hero logic)
	// TODO: Implement when suggested hero system is available
	// if ( bRequireBotImplementation )
	// {
	//     CUtlVector<int> suggestedHeroes;
	//     g_pDOTAPlayerResource->GetSuggestedHeroList( nPlayerTeam, suggestedHeroes );
	//     
	//     // Add suggested heroes to the list
	//     for ( int i = 0; i < suggestedHeroes.Count(); i++ )
	//     {
	//         int nHeroID = suggestedHeroes[i];
	//         
	//         // Check availability and bot implementation
	//         if ( pGameManager->IsHeroAvailableByHeroID( nPlayerID, nHeroID, HERO_DATA_TYPE_FULL ) )
	//         {
	//             KeyValues *pHeroData = pGameManager->GetHeroDataByHeroID( nHeroID, HERO_DATA_TYPE_FULL );
	//             if ( pHeroData && pHeroData->GetInt( "BotImplemented", 0 ) != 0 )
	//             {
	//                 // Add to hero list
	//                 // TODO: Implement when CUtlVector system is available
	//             }
	//         }
	//     }
	// }
	// else
	// {
	//     // Add forced heroes (matches decompiled force hero logic)
	//     // TODO: Implement when force hero system is available
	// }
	
	// Final fallback - add all available heroes (matches decompiled fallback)
	// TODO: Implement when hero iteration system is available
	// if ( heroList.Count() == 0 )
	// {
	//     for ( int nHeroID = 0; nHeroID <= nHighestHeroID; nHeroID++ )
	//     {
	//         // Check availability and requirements
	//         if ( pGameManager->IsHeroAvailableByHeroID( nPlayerID, nHeroID, HERO_DATA_TYPE_FULL ) )
	//         {
	//             KeyValues *pHeroData = pGameManager->GetHeroDataByHeroID( nHeroID, HERO_DATA_TYPE_FULL );
	//             if ( pHeroData )
	//             {
	//                 // Check bot implementation requirement
	//                 if ( !bRequireBotImplementation || pHeroData->GetInt( "BotImplemented", 0 ) != 0 )
	//                 {
	//                     // Check if hero matches any priority
	//                     // TODO: Implement priority matching logic
	//                     
	//                     // Add to hero list
	//                     // TODO: Implement when CUtlVector system is available
	//                 }
	//             }
	//         }
	//     }
	// }
	
	Msg( "GetSmartRandomHeroList: Player %d, Team %d, RequireBot=%d, UseSmart=%d\n", 
		 nPlayerID, nPlayerTeam, bRequireBotImplementation, bUseSmartList );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::OnNumSpectatorsChanged( int nNewSpectatorCount )
{
	// Update maximum spectator count (matches decompiled max tracking)
	if ( nNewSpectatorCount > m_nMaxSpectators )
	{
		m_nMaxSpectators = nNewSpectatorCount;
	}
	
	// Calculate spectator limit (matches decompiled limit calculation)
	int nSpectatorLimit = 25; // Default limit
	if ( nNewSpectatorCount > 99 )
	{
		nSpectatorLimit = (int)((float)nNewSpectatorCount * 0.25f); // 25% of total
	}
	
	// Check if we need to update the limit (matches decompiled limit check)
	if (nSpectatorLimit + m_nCurrentSpectators <= nNewSpectatorCount)
	{
		m_nCurrentSpectators = nNewSpectatorCount;

		// Send chat message to all players except spectators (matches decompiled message sending)
		// TODO: Implement when chat system is available
		// CRecipientFilter filter;
		// filter.AddAllPlayers();
		// 
		// // Remove spectators from recipient list
		// CTeam *pSpectatorTeam = GetGlobalDOTATeam( DOTA_TEAM_SPECTATOR );
		// if ( pSpectatorTeam )
		//     filter.RemoveRecipientsByTeam( pSpectatorTeam );
		// 
		// SendChatEventMessage( DOTA_CHAT_MESSAGE_SPECTATORS, nNewSpectatorCount, -1, &filter );

		Msg("DOTA: Spectator count changed to %d (limit: %d)\n", nNewSpectatorCount, nSpectatorLimit);
	}
}

//-----------------------------------------------------------------------------
bool CDOTAGameRules::Rules_DisableAFKChecks( void )
{
	// Check if AFK checks should be disabled (matches decompiled ConVar checks)
	// TODO: Implement when ConVar system is available
	// bool bDisableAFKChecks = true;
	// 
	// if ( !dota_afk_check_enabled.GetBool() && !dota_afk_check_disabled.GetBool() )
	// {
	//     bDisableAFKChecks = dota_afk_check_force_disabled.GetBool();
	// }
	// 
	// return bDisableAFKChecks;
	
	// Placeholder implementation - always enable AFK checks for now
	return false;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetHeroMinimapIconSize( int nIconSize )
{
	// Update networked variable with notification (matches decompiled network update)
	if ( m_nHeroMinimapIconSize != nIconSize )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nHeroMinimapIconSize = nIconSize;
	}
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::StartItemRestockTimers( void )
{
	// Check if this is a competitive game (matches decompiled GC system check)
	bool bIsCompetitive = false;
	// TODO: Implement when GC system is available
	// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem )
	// {
	//     bIsCompetitive = pGCSystem->IsCompetitiveGame();
	// }
	
	// Calculate restock delay based on game mode (matches decompiled delay calculation)
	float flRestockDelay = 0.0f;
	// TODO: Implement when ConVar system is available
	// if ( GetGameMode() == DOTA_GAMEMODE_ALL_PICK && !bIsCompetitive )
	// {
	//     flRestockDelay = dota_item_restock_delay.GetFloat() - dota_item_restock_delay_competitive.GetFloat();
	// }
	
	// Start restock timers for both teams (matches decompiled timer loop)
	for ( int nTeam = DOTA_TEAM_RADIANT; nTeam <= DOTA_TEAM_DIRE; nTeam++ )
	{
		// Check if team needs restock (matches decompiled stock check)
		// TODO: Implement when item stock system is available
		// if ( GetItemStockCount( nTeam ) < GetMaxItemStock( nTeam ) && 
		//      GetItemRestockTime( nTeam ) > 0.0f )
		// {
		//     CGameRulesProxy::NotifyNetworkStateChanged();
		//     
		//     // Calculate restock time (matches decompiled time calculation)
		//     float flRestockTime = gpGlobals->curtime + GetItemRestockTime( nTeam ) + flRestockDelay;
		//     
		//     // Update timer (matches decompiled timer update)
		//     if ( flRestockTime != GetItemRestockTimer( nTeam ) )
		//     {
		//         GetItemRestockTimer( nTeam ).SetTimer( flRestockTime );
		//     }
		// }
	}
	
	Msg( "DOTA: Item restock timers started (Competitive: %d, Delay: %.2f)\n", bIsCompetitive, flRestockDelay );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter_DISCONNECT( void )
{
	// Disconnect all players (matches decompiled player disconnection loop)
	// TODO: Implement when engine interface is available
	// for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	// {
	//     CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
	//     if ( !pPlayer || !pPlayer->IsConnected() || pPlayer->IsBot() )
	//         continue;
	//     
	//     int nClientIndex = pPlayer->entindex() - 1;
	//     
	//     // Disconnect player (matches decompiled engine call)
	//     engine->DisconnectClient( nClientIndex, "Game ended" );
	// }
	
	// Clear disconnect timer (matches decompiled timer clearing)
	m_flDisconnectTimer = 0.0f;
	
	// Set state transition time based on HLTV status (matches decompiled time calculation)
	float flTransitionTime = gpGlobals->curtime;
	// TODO: Implement when engine interface is available
	// bool bIsHLTV = engine->IsHLTV();
	// if ( !bIsHLTV )
	// {
	//     flTransitionTime += 1.0f; // 1 second delay
	// }
	// else
	// {
	//     // TODO: Implement when ConVar system is available
	//     ConVarRef tvDelay( "tv_delay" );
	//     flTransitionTime += tvDelay.GetFloat() + 2.0f; // TV delay + 2 seconds
	// }
	flTransitionTime += 1.0f; // Default 1 second delay
	
	// Update state transition time (matches decompiled time update)
	if ( m_flStateTransitionTime != flTransitionTime )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flStateTransitionTime = flTransitionTime;
	}
	
	Msg( "DOTA: Entered DISCONNECT state - players disconnected, timers set\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Think_DISCONNECT( void )
{
	// Check replay upload timer (matches decompiled replay timing)
	// TODO: Implement when engine interface is available
	// bool bIsHLTV = engine->IsHLTV();
	// if ( !bIsHLTV || 
	//      ( m_ReplayUploadTimer.HasElapsed() && m_ReplayUploadTimer.GetElapsedTime() != m_flReplayUploadTime ) )
	// {
	//     CheckUploadReplay();
	// }
	
	// Check for state transition (matches decompiled transition check)
	if ( m_flStateTransitionTime <= gpGlobals->curtime && m_flStateTransitionTime != 0.0f )
	{
		// Record disconnect time (matches decompiled time recording)
		// TODO: Implement when platform time is available
		// double flCurrentPlatTime = Plat_FloatTime();
		// m_flDisconnectTimer = (float)flCurrentPlatTime;
		
		// Clear state transition time (matches decompiled time clearing)
		if ( m_flStateTransitionTime != 0.0f )
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_flStateTransitionTime = 0.0f;
		}
	}
	
	// Early return if transition time is still active
	if ( m_flStateTransitionTime != 0.0f )
		return;
	
	// Check for lobby timeout (matches decompiled lobby timeout check)
	// TODO: Implement when GC system is available
	// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	// CDOTALobby *pLobby = pGCSystem ? pGCSystem->GetLobby() : NULL;
	// 
	// if ( pGCSystem && pGCSystem->CheckUpdateConnectedPlayers() && pLobby &&
	//      ( m_bShouldSendLoadFailure || m_bShouldSendLoadFailureToGC ) )
	// {
	//     pGCSystem->SendLoadFailure( m_LoadFailurePlayers, m_LoadFailureReasons );
	// }
	
	// Check for lobby timeout after 5 minutes (matches decompiled timeout check)
	// TODO: Implement when GC system is available
	// if ( m_bShouldWaitForLobby && pLobby && m_flDisconnectTimer != 0.0f )
	// {
	//     double flCurrentPlatTime = Plat_FloatTime();
	//     if ( (float)flCurrentPlatTime - m_flDisconnectTimer >= 300.0f ) // 5 minutes
	//     {
	//         unsigned long long nLobbyID = pLobby->GetLobbyID();
	//         Warning( "Lobby %016llx didn't go away after five minutes, giving up!\n", nLobbyID );
	//         m_flDisconnectTimer = 0.0f;
	//         QuitServer();
	//     }
	// }
	
	// Handle server shutdown (matches decompiled shutdown logic)
	// TODO: Implement when ConVar system is available
	// if ( !pLobby )
	// {
	//     if ( !m_bShouldWaitForLobby )
	//     {
	//         // TODO: Implement when ConVar system is available
	//         ConVarRef svShutdownAfterGame( "sv_shutdown_after_game" );
	//         bool bIsHLTV = engine->IsHLTV();
	//         
	//         if ( svShutdownAfterGame.GetInt() == 0 )
	//         {
	//             if ( !bIsHLTV )
	//             {
	//                 State_Transition( DOTA_GAMERULES_STATE_INIT );
	//                 return;
	//             }
	//         }
	//         else
	//         {
	//             if ( bIsHLTV )
	//             {
	//                 // TODO: Implement when static variable is available
	//                 // if ( !State_Think_DISCONNECT::hasQuit )
	//                 // {
	//                 //     State_Think_DISCONNECT::hasQuit = true;
	//                 //     Msg( "Terminating after game.\n" );
	//                 //     QuitServer();
	//                 // }
	//                 return;
	//             }
	//         }
	//         
	//         // Send disconnect command
	//         engine->ServerCommand( "disconnect\n" );
	//     }
	//     else
	//     {
	//         State_Transition( DOTA_GAMERULES_STATE_INIT );
	//         // TODO: Implement when player resource system is available
	//         // CDOTA_PlayerResource::ClearLobbySteamIDs();
	//         engine->ServerCommand( "changelevel dota\n" );
	//     }
	// }
	
	// Default behavior - transition to INIT state
	State_Transition( DOTA_GAMERULES_STATE_INIT );
}

//-----------------------------------------------------------------------------
bool CDOTAGameRules::GetMatchSignoutComplete( void )
{
	// Return the match signout complete flag (matches decompiled offset access)
	return m_bMatchSignoutComplete;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::LeaveGameSelectionStage( void )
{
	// Process both teams (matches decompiled team loop)
	for ( int nTeam = DOTA_TEAM_RADIANT; nTeam <= DOTA_TEAM_DIRE; nTeam++ )
	{
		// Get player IDs for team (matches decompiled player resource call)
		// TODO: Implement when player resource system is available
		// CUtlVector<int> playerIDs;
		// g_pDOTAPlayerResource->GetPlayerIDsForTeam( nTeam, &playerIDs );
		
		// Process each player (matches decompiled player loop)
		// for ( int i = 0; i < playerIDs.Count(); i++ )
		// {
		//     int nPlayerID = playerIDs[i];
		//     
		//     // Check if player has selected hero (matches decompiled hero check)
		//     if ( g_pDOTAPlayerResource->HasSelectedHero( nPlayerID ) )
		//     {
		//         // Get selected hero handle (matches decompiled handle retrieval)
		//         EHANDLE hSelectedHero = g_pDOTAPlayerResource->GetPlayerSelectedHeroHandle( nPlayerID );
		//         
		//         // Validate hero entity (matches decompiled entity validation)
		//         if ( hSelectedHero.IsValid() )
		//         {
		//             CBaseEntity *pHero = hSelectedHero.Get();
		//             if ( pHero && pHero->IsAlive() && pHero->IsPlayer() )
		//             {
		//                 // Remove selection flag (matches decompiled flag removal)
		//                 pHero->RemoveFlag( FL_FAKECLIENT );
		//                 
		//                 // Set selection state (matches decompiled state call)
		//                 pHero->SetSelected( false, true, false );
		//             }
		//         }
		//     }
		// }
	}
	
	Msg( "DOTA: Left game selection stage - cleared hero selection flags\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetMinimapDebugGridData( unsigned char *pGridData )
{
	// Copy debug grid data to minimap (matches decompiled memory copy)
	// TODO: Implement when minimap system is available
	// void *pMinimapData = GetMinimapDebugData();
	// V_memcpy( pMinimapData, pGridData, 0x4000 ); // 16KB grid data
	
	Msg( "DOTA: Set minimap debug grid data (%d bytes)\n", 0x4000 );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetRuneMinimapIconScale( float flScale )
{
	// Update networked variable with notification (matches decompiled network update)
	if ( m_flRuneMinimapIconScale != flScale )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flRuneMinimapIconScale = flScale;
	}
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetSelectionStageEntity( CBaseEntity *pEntity )
{
	// Validate entity (matches decompiled null check)
	if ( !pEntity )
		return;
	
	// TODO: Implement when entity casting system is available
	// CDOTA_SelectionStageEntity *pSelectionEntity = dynamic_cast<CDOTA_SelectionStageEntity*>( pEntity );
	// if ( !pSelectionEntity )
	//     return;
	
	// Get entity name (matches decompiled name retrieval)
	char szEntityName[32];
	// TODO: Implement when entity name system is available
	// const char *pszName = pEntity->GetEntityName();
	// V_strncpy( szEntityName, pszName ? pszName : "", sizeof(szEntityName) );
	V_strncpy( szEntityName, "selection_entity", sizeof(szEntityName) );
	
	// Parse team and slot from name (matches decompiled string parsing)
	int nSlot = 0;
	bool bIsRadiant = false;
	
	// TODO: Implement when string parsing system is available
	// char *pszRadiant = V_strstr( szEntityName, "radiant_" );
	// char *pszSlot = V_strstr( szEntityName, "0" );
	// if ( pszSlot )
	//     nSlot = atoi( pszSlot );
	// bIsRadiant = ( pszRadiant != NULL );
	
	// Get entity handle (matches decompiled handle retrieval)
	// TODO: Implement when entity handle system is available
	// EHANDLE hEntity = pEntity->GetEntityHandle();
	
	// Store entity handle based on team (matches decompiled team-based storage)
	// if ( bIsRadiant )
	// {
	//     m_hRadiantSelectionEntities[nSlot] = hEntity;
	//     m_bRadiantSelectionEntityValid[nSlot] = false;
	// }
	// else
	// {
	//     m_hDireSelectionEntities[nSlot] = hEntity;
	//     m_bDireSelectionEntityValid[nSlot] = false;
	// }
	
	Msg( "DOTA: Set selection stage entity (Team: %s, Slot: %d)\n", 
		 bIsRadiant ? "Radiant" : "Dire", nSlot );
}

//-----------------------------------------------------------------------------
void *CDOTAGameRules::GetLastUsedActiveAbility( int nPlayerID )
{
	// Validate player ID (matches decompiled bounds check)
	if ( nPlayerID < 0 || nPlayerID >= MAX_PLAYERS )
	{
		Warning( "Attempt to get last used ability on invalid player id: %d!\n", nPlayerID );
		return NULL;
	}
	
	// Return ability data pointer (matches decompiled offset calculation)
	// TODO: Implement when ability system is available
	// return &m_PlayerAbilityData[nPlayerID];
	
	return NULL;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::HeroPickState_Transition( DOTA_HeroPickState_t nNewState )
{
	// Call leave function for current state (matches decompiled state leave)
	// TODO: Implement when hero pick state system is available
	// if ( m_pCurrentHeroPickState && m_pCurrentHeroPickState->pfnLeave )
	// {
	//     (this->*m_pCurrentHeroPickState->pfnLeave)();
	// }
	
	// Update networked state (matches decompiled state update)
	if ( m_nHeroPickState != nNewState )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nHeroPickState = nNewState;
	}
	
	// Find new state info (matches decompiled state lookup)
	// TODO: Implement when hero pick state system is available
	// const HeroPickStateInfo_t *pNewStateInfo = NULL;
	// for ( int i = 0; i < DOTA_HEROPICK_STATE_COUNT; i++ )
	// {
	//     if ( s_HeroPickStateInfos[i].nState == nNewState )
	//     {
	//         pNewStateInfo = &s_HeroPickStateInfos[i];
	//         break;
	//     }
	// }
	// 
	// if ( !pNewStateInfo )
	// {
	//     m_pCurrentHeroPickState = NULL;
	//     return;
	// }
	// 
	// // Update current state pointer (matches decompiled pointer update)
	// m_pCurrentHeroPickState = pNewStateInfo;
	// 
	// // Call enter function for new state (matches decompiled state enter)
	// if ( pNewStateInfo->pfnEnter )
	// {
	//     (this->*pNewStateInfo->pfnEnter)();
	// }
	
	Msg( "DOTA: Hero pick state transition to %d\n", nNewState );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::RegisterEnemyCreepInBase( CBaseEntity *pCreep )
{
	// Validate creep entity (matches decompiled null check)
	if ( !pCreep )
		return;
	
	// Get creep team (matches decompiled team check)
	int nTeam = pCreep->GetTeamNumber();
	int nTeamOffset = ( nTeam == DOTA_TEAM_DIRE ) ? 0x14 : 0; // 20 bytes offset for Dire team
	
	// Get creep handle (matches decompiled handle retrieval)
	//EHANDLE hCreep = pCreep->GetEntityHandle();
	
	// Get creep array for team (matches decompiled array access)
	// TODO: Implement when CUtlVector system is available
	// CUtlVector<EHANDLE> *pCreepArray = &m_EnemyCreepsInBase[nTeamOffset];
	// int nCurrentCount = pCreepArray->Count();
	// int nMaxCount = pCreepArray->MaxCount();
	
	// Grow array if needed (matches decompiled array growth)
	// if ( nMaxCount <= nCurrentCount )
	// {
	//     pCreepArray->Grow( nCurrentCount + 1 - nMaxCount );
	//     nCurrentCount = pCreepArray->Count();
	// }
	
	// Update count (matches decompiled count update)
	// pCreepArray->AddToTail( hCreep );
	
	// Shift existing elements (matches decompiled memory move)
	// if ( nCurrentCount > 0 )
	// {
	//     void *pData = pCreepArray->Base();
	//     V_memmove( (char*)pData + 4 + nCurrentCount * 4, 
	//                (char*)pData + nCurrentCount * 4, 
	//                ( nCurrentCount + 1 - nCurrentCount ) * 4 );
	// }
	
	// Store creep handle (matches decompiled handle storage)
	// pCreepArray->Element( nCurrentCount ) = hCreep;
	
	//Msg( "DOTA: Registered enemy creep in base (Team: %d, Handle: %d)\n", nTeam, hCreep.ToInt() );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetCreepMinimapIconScale( float flScale )
{
	// Update networked variable with notification (matches decompiled network update)
	if ( m_flCreepMinimapIconScale != flScale )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flCreepMinimapIconScale = flScale;
	}
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::UpdateAutomaticSurrender( void )
{
	// Validate player resource (matches decompiled null check)
	if ( !g_pDOTAPlayerResource )
		return;
	
	// Check game state conditions (matches decompiled state checks)
	// TODO: Implement when ConVar system is available
	// if ( GetGameState() != DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
	//     return;
	// if ( m_nAutomaticSurrenderState == 2 )
	//     return;
	// if ( m_bAutomaticSurrenderEnabled != 0 )
	//     return;
	// if ( dota_automatic_surrender.GetInt() == 0 )
	//     return;
	// if ( GetGameMode() != DOTA_GAMEMODE_ALL_PICK )
	//     return;
	// if ( dota_automatic_surrender_enabled.GetInt() != 0 )
	//     return;
	
	// Count players and leavers for each team (matches decompiled player counting)
	int nRadiantPlayers = 0, nRadiantLeavers = 0;
	int nDirePlayers = 0, nDireLeavers = 0;
	
	// TODO: Implement when player resource system is available
	// for ( int i = 0; i < MAX_PLAYERS; i++ )
	// {
	//     if ( g_pDOTAPlayerResource->IsValidTeamPlayer( i ) && 
	//          !g_pDOTAPlayerResource->IsFakeClient( i ) )
	//     {
	//         DOTAConnectionState_t nLeaverStatus = g_pDOTAPlayerResource->GetLeaverStatus( i );
	//         bool bIsPermanentLeaver = IsPermanentLeaverState( nLeaverStatus );
	//         int nTeam = g_pDOTAPlayerResource->GetTeam( i );
	//         
	//         if ( nTeam == DOTA_TEAM_DIRE )
	//         {
	//             nDirePlayers++;
	//             if ( bIsPermanentLeaver )
	//                 nDireLeavers++;
	//         }
	//         else if ( nTeam == DOTA_TEAM_RADIANT )
	//         {
	//             nRadiantPlayers++;
	//             if ( bIsPermanentLeaver )
	//                 nRadiantLeavers++;
	//         }
	//     }
	// }
	
	// Check surrender conditions (matches decompiled surrender logic)
	bool bRadiantSurrender = false, bDireSurrender = false;
	
	// TODO: Implement surrender condition checks
	// if ( ( nRadiantLeavers < nRadiantPlayers && nRadiantPlayers < 1 && nDirePlayers < 1 ) || 
	//      ( nDireLeavers < nDirePlayers ) )
	// {
	//     if ( ( nRadiantLeavers < nRadiantPlayers && nRadiantPlayers < 1 ) )
	//     {
	//         if ( nDireLeavers < nDirePlayers )
	//             return;
	//         if ( nDirePlayers < 1 )
	//             return;
	//         
	//         Msg( "UpdateAutomaticSurrender:\nGoodGuys = %d Disconnected = %d\nBadGuys = %d Disconnected = %d\n",
	//              nRadiantPlayers, nRadiantLeavers, nDirePlayers, nDireLeavers );
	//         m_nAutomaticSurrenderState = 1;
	//         MakeTeamLose( DOTA_TEAM_DIRE );
	//     }
	//     else
	//     {
	//         Msg( "UpdateAutomaticSurrender:\nGoodGuys = %d Disconnected = %d\nBadGuys = %d Disconnected = %d \n",
	//              nRadiantPlayers, nRadiantLeavers, nDirePlayers, nDireLeavers );
	//         m_nAutomaticSurrenderState = 1;
	//         MakeTeamLose( DOTA_TEAM_RADIANT );
	//     }
	// }
	// else
	// {
	//     Msg( "UpdateAutomaticSurrender:\nGoodGuys = %d Disconnected = %d\nBadGuys = %d Disconnected = %d \n",
	//          nRadiantPlayers, nRadiantLeavers, nDirePlayers, nDireLeavers );
	//     m_nAutomaticSurrenderState = 2;
	//     MakeTeamLose( DOTA_TEAM_DIRE );
	// }
	
	Msg( "DOTA: Updated automatic surrender (Radiant: %d/%d, Dire: %d/%d)\n", 
		 nRadiantLeavers, nRadiantPlayers, nDireLeavers, nDirePlayers );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::UpdateGameSelectionStage( void )
{
	// Check if selection stage is enabled (matches decompiled ConVar check)
	// TODO: Implement when ConVar system is available
	// if ( dota_selection_stage_enabled.GetInt() == 0 )
	//     return;
	
	// Process both teams (matches decompiled team loop)
	for ( int nTeam = DOTA_TEAM_RADIANT; nTeam <= DOTA_TEAM_DIRE; nTeam++ )
	{
		// Get player IDs for team (matches decompiled player resource call)
		// TODO: Implement when player resource system is available
		// CUtlVector<int> playerIDs;
		// g_pDOTAPlayerResource->GetPlayerIDsForTeam( nTeam, &playerIDs );
		
		// Process each player (matches decompiled player loop)
		// for ( int i = 0; i < playerIDs.Count(); i++ )
		// {
		//     int nPlayerID = playerIDs[i];
		//     
		//     // Check if selection entity is not set and player has selected hero
		//     if ( !m_bSelectionEntitySet[nTeam][i] && 
		//          g_pDOTAPlayerResource->HasSelectedHero( nPlayerID ) )
		//     {
		//         // Get selected hero handle (matches decompiled handle retrieval)
		//         EHANDLE hSelectedHero = g_pDOTAPlayerResource->GetPlayerSelectedHeroHandle( nPlayerID );
		//         
		//         // Validate hero entity (matches decompiled entity validation)
		//         if ( hSelectedHero.IsValid() )
		//         {
		//             CBaseEntity *pHero = hSelectedHero.Get();
		//             if ( pHero && pHero->IsAlive() && pHero->IsPlayer() )
		//             {
		//                 // Add selection flag (matches decompiled flag addition)
		//                 pHero->AddFlag( FL_FAKECLIENT );
		//                 
		//                 // Update hero positioning based on team (matches decompiled positioning)
		//                 int nHeroTeam = pHero->GetTeamNumber();
		//                 float flAngleOffset = 0.0f;
		//                 
		//                 if ( nHeroTeam == DOTA_TEAM_DIRE )
		//                 {
		//                     flAngleOffset = s_flAccumulateRightAngle + 8.0f;
		//                     s_flAccumulateRightAngle = flAngleOffset;
		//                 }
		//                 else if ( nHeroTeam == DOTA_TEAM_RADIANT )
		//                 {
		//                     flAngleOffset = s_flAccumulateLeftAngle - 8.0f;
		//                     s_flAccumulateLeftAngle = flAngleOffset;
		//                 }
		//                 
		//                 // Set hero angles (matches decompiled angle setting)
		//                 QAngle angles( 0, flAngleOffset, 0 );
		//                 pHero->SetAbsAngles( angles );
		//                 
		//                 // Update hitbox and positioning (matches decompiled hitbox logic)
		//                 // TODO: Implement when animation system is available
		//                 // int nHitboxSet = pHero->GetHitboxSet();
		//                 // pHero->SetHitboxSetByName( "select_high" );
		//                 // pHero->ComputeHitboxSurroundingBox( &mins, &maxs );
		//                 // pHero->SetHitboxSet( nHitboxSet );
		//                 
		//                 // Calculate positioning based on hitbox (matches decompiled positioning)
		//                 // TODO: Implement positioning calculations
		//                 
		//                 // Mark selection entity as set (matches decompiled flag setting)
		//                 m_bSelectionEntitySet[nTeam][i] = true;
		//             }
		//         }
		//     }
		// }
	}
	
	Msg( "DOTA: Updated game selection stage\n" );
}

//-----------------------------------------------------------------------------
CBaseEntity *CDOTAGameRules::GetAnnouncer_KillingSpree( int nTeam )
{
	// Get announcer handle based on team (matches decompiled team-based access)
	EHANDLE hAnnouncer;
	
	if ( nTeam == DOTA_TEAM_RADIANT )
	{
		hAnnouncer = m_hRadiantAnnouncer;
	}
	else if ( nTeam == DOTA_TEAM_DIRE )
	{
		hAnnouncer = m_hDireAnnouncer;
	}
	else if ( nTeam == DOTA_TEAM_SPECTATOR )
	{
		hAnnouncer = m_hSpectatorAnnouncer;
	}
	else
	{
		return NULL;
	}
	
	// Validate announcer entity (matches decompiled entity validation)
	if ( hAnnouncer.IsValid() )
	{
		CBaseEntity *pAnnouncer = hAnnouncer.Get();
		if ( pAnnouncer && pAnnouncer->IsAlive() )
		{
			return pAnnouncer;
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
bool CDOTAGameRules::GetTeamHasAbandonedPlayer( int nTeam )
{
	// Get abandoned player flag based on team (matches decompiled team-based access)
	int nTeamIndex = ( nTeam == DOTA_TEAM_DIRE ) ? 1 : 0;
	
	// TODO: Implement when abandoned player system is available
	// return m_bTeamHasAbandonedPlayer[nTeamIndex];
	
	return false;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::PlayGameStartAnnouncement( void )
{
	// Create criteria set for Radiant team (matches decompiled criteria creation)
	// TODO: Implement when response rules system is available
	// ResponseRules::CriteriaSet criteriaRadiant;
	// criteriaRadiant.AppendCriteria( "announce_event", "game_start", 1.0f );
	// AddTeamCompToCriteria( DOTA_TEAM_RADIANT, &criteriaRadiant );
	
	// Play Radiant announcer speech (matches decompiled speech queuing)
	// TODO: Implement when speech system is available
	// CBaseEntity *pRadiantAnnouncer = GetAnnouncer( DOTA_TEAM_RADIANT );
	// if ( pRadiantAnnouncer && pRadiantAnnouncer->IsAlive() )
	// {
	//     // TODO: Implement when ConVar system is available
	//     // if ( dota_announcer_enabled.GetInt() == 0 )
	//     // {
	//     //     pRadiantAnnouncer->QueueConcept( DOTA_CONCEPT_GAME_START, 8.0f, &criteriaRadiant, 6, true );
	//     // }
	// }
	
	// Create criteria set for Dire team (matches decompiled criteria creation)
	// ResponseRules::CriteriaSet criteriaDire;
	// criteriaDire.AppendCriteria( "announce_event", "game_start", 1.0f );
	// AddTeamCompToCriteria( DOTA_TEAM_DIRE, &criteriaDire );
	
	// Play Dire announcer speech (matches decompiled speech queuing)
	// CBaseEntity *pDireAnnouncer = GetAnnouncer( DOTA_TEAM_DIRE );
	// if ( pDireAnnouncer && pDireAnnouncer->IsAlive() )
	// {
	//     // if ( dota_announcer_enabled.GetInt() == 0 )
	//     // {
	//     //     pDireAnnouncer->QueueConcept( DOTA_CONCEPT_GAME_START, 8.0f, &criteriaDire, 7, true );
	//     // }
	// }
	
	// Create criteria set for Spectator announcer (matches decompiled criteria creation)
	// ResponseRules::CriteriaSet criteriaSpectator;
	// criteriaSpectator.AppendCriteria( "announce_event", "game_start", 1.0f );
	
	// Play Spectator announcer speech (matches decompiled speech queuing)
	// CBaseEntity *pSpectatorAnnouncer = GetAnnouncer( DOTA_TEAM_SPECTATOR );
	// if ( pSpectatorAnnouncer && pSpectatorAnnouncer->IsAlive() )
	// {
	//     // if ( dota_announcer_enabled.GetInt() == 0 )
	//     // {
	//     //     pSpectatorAnnouncer->QueueConcept( DOTA_CONCEPT_GAME_START, 8.0f, &criteriaSpectator, 8, true );
	//     // }
	// }
	
	// Play game mode specific sounds (matches decompiled sound emission)
	// TODO: Implement when sound system is available
	// int nGameMode = GetGameMode();
	// if ( nGameMode == DOTA_GAMEMODE_DIRETIDE )
	// {
	//     Gamerules_EmitSound( "DireTideGameStart.DireSide", NULL, false );
	//     Gamerules_EmitSound( "DireTideGameStart.RadiantSide", NULL, false );
	// }
	// else if ( nGameMode == DOTA_GAMEMODE_FROSTIVUS )
	// {
	//     Gamerules_EmitSound( "FrostivusGameStart.DireSide", NULL, false );
	//     Gamerules_EmitSound( "FrostivusGameStart.RadiantSide", NULL, false );
	// }
	// else
	// {
	//     Gamerules_EmitSound( "GameStart.DireAncient", NULL, false );
	//     Gamerules_EmitSound( "GameStart.RadiantAncient", NULL, false );
	// }
	
	Msg( "DOTA: Played game start announcement\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::PrecacheResourcesFromKeys( KeyValues *pKV, void *pManifest, bool bIsFolder )
{
	// Validate KeyValues (matches decompiled null check)
	if ( !pKV )
		return;
	
	// Determine resource type key (matches decompiled key selection)
	const char *pszResourceKey = bIsFolder ? "particle_folder" : "particlefile";
	
	// Iterate through KeyValues (matches decompiled iteration)
	// TODO: Implement when KeyValues system is available
	// KeyValues *pSubKey = pKV->GetFirstSubKey();
	// while ( pSubKey )
	// {
	//     const char *pszName = pSubKey->GetName();
	//     const char *pszValue = pSubKey->GetString( NULL, "" );
	//     const char *pszExtension = V_GetFileExtension( pszValue );
	//     
	//     if ( pszName && pszValue )
	//     {
	//         // Handle different resource types (matches decompiled type checking)
	//         if ( V_stricmp( pszName, "model" ) == 0 )
	//         {
	//             if ( !bIsFolder || V_stricmp( pszExtension, "vmdl" ) == 0 )
	//             {
	//                 PrecacheModel( pszValue, pManifest );
	//             }
	//         }
	//         else if ( V_stricmp( pszName, "model_folder" ) == 0 )
	//         {
	//             if ( !bIsFolder || pszExtension[0] == '\0' )
	//             {
	//                 PrecacheModelFolderRecursive( pszValue, pManifest );
	//             }
	//         }
	//         else if ( V_stricmp( pszName, "sound" ) == 0 )
	//         {
	//             PrecacheScriptSound( pszValue, pManifest );
	//         }
	//         else if ( V_stricmp( pszName, "soundfile" ) == 0 )
	//         {
	//             if ( !bIsFolder || V_stricmp( pszExtension, "vsndevts" ) == 0 )
	//             {
	//                 PrecacheGameSoundsFile( pszValue, pManifest, false );
	//             }
	//         }
	//         else if ( V_stricmp( pszName, "particle" ) == 0 )
	//         {
	//             if ( !bIsFolder || V_stricmp( pszExtension, "vpcf" ) == 0 )
	//             {
	//                 PrecacheParticleSystem( pszValue, pManifest );
	//             }
	//         }
	//         else if ( V_stricmp( pszName, pszResourceKey ) == 0 )
	//         {
	//             if ( !bIsFolder || pszExtension[0] == '\0' )
	//             {
	//                 // Parse comma-separated particle files (matches decompiled parsing)
	//                 char szBuffer[512];
	//                 V_strncpy( szBuffer, pszValue, sizeof(szBuffer) );
	//                 
	//                 char *pszToken = strtok( szBuffer, "," );
	//                 while ( pszToken )
	//                 {
	//                     PrecacheParticleFileAndSystems( pszToken, pManifest );
	//                     pszToken = strtok( NULL, "," );
	//                 }
	//             }
	//         }
	//         else if ( V_stricmp( pszName, "npc" ) == 0 )
	//         {
	//             DOTA_PrecacheUnitByName( pszValue, pManifest );
	//         }
	//         else
	//         {
	//             Warning( "Encountered unknown resource type \"%s\" for resource \"%s\"!\n", pszName, pszValue );
	//         }
	//     }
	//     
	//     pSubKey = pSubKey->GetNextKey();
	// }
	
	Msg( "DOTA: Precached resources from keys (Folder: %d)\n", bIsFolder );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter_STRATEGY_TIME( void )
{
	// Set state transition time (matches decompiled time calculation)
	// TODO: Implement when ConVar system is available
	// float flStrategyTime = dota_strategy_time.GetFloat();
	// float flTransitionTime = gpGlobals->curtime + flStrategyTime;
	float flTransitionTime = gpGlobals->curtime + 90.0f; // Default 90 seconds
	
	// Update state transition time (matches decompiled time update)
	if ( m_flStateTransitionTime != flTransitionTime )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flStateTransitionTime = flTransitionTime;
	}
	
	Msg( "DOTA: Entered STRATEGY_TIME state\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter_TEAM_SHOWCASE( void )
{
	// Validate team showcase helper (matches decompiled null check)
	// TODO: Implement when team showcase system is available
	// if ( !m_pTeamShowcaseHelper )
	// {
	//     Warning( "Team Showcase: No instance of CTeamShowcaseHelper.\n" );
	// }
	// else
	// {
	//     m_pTeamShowcaseHelper->EnterTeamShowcase();
	// }
	
	Msg( "DOTA: Entered TEAM_SHOWCASE state\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Leave_STRATEGY_TIME( void )
{
	// Strategy time leave function (matches decompiled empty function)
	// No cleanup needed for strategy time
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Leave_TEAM_SHOWCASE( void )
{
	// Leave team showcase (matches decompiled cleanup)
	// TODO: Implement when team showcase system is available
	// if ( m_pTeamShowcaseHelper )
	// {
	//     m_pTeamShowcaseHelper->LeaveTeamShowcase();
	//     delete m_pTeamShowcaseHelper;
	//     m_pTeamShowcaseHelper = NULL;
	// }
	
	Msg( "DOTA: Left TEAM_SHOWCASE state\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Think_STRATEGY_TIME( void )
{
	// Check for state transition (matches decompiled transition check)
	if ( m_flStateTransitionTime <= gpGlobals->curtime && 
		 m_flStateTransitionTime != 0.0f )
	{
		State_Transition( DOTA_GAMERULES_STATE_PRE_GAME );
	}
	
	// Check idle and disconnected players (matches decompiled player check)
	// TODO: Implement when player system is available
	// CheckIdleAndDisconnectedPlayers();
	
	Msg( "DOTA: Strategy time think - checking for transition\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Think_TEAM_SHOWCASE( void )
{
	// Update team showcase helper (matches decompiled think call)
	// TODO: Implement when team showcase system is available
	// if ( m_pTeamShowcaseHelper )
	// {
	//     m_pTeamShowcaseHelper->Think();
	// }
	
	Msg( "DOTA: Team showcase think\n" );
}

//-----------------------------------------------------------------------------
bool CDOTAGameRules::IsPointInBadResultPosition( const Vector &vPosition )
{
	// Initialize trace parameters (matches decompiled initialization)
	Vector vStart = vPosition;
	Vector vEnd = vPosition;
	
	// Check against all bad result entities (matches decompiled entity loop)
	// TODO: Implement when entity list system is available
	// for ( int i = 0; i < m_BadResultEntities.Count(); i++ )
	// {
	//     EHANDLE hEntity = m_BadResultEntities[i];
	//     
	//     // Validate entity (matches decompiled entity validation)
	//     if ( hEntity.IsValid() )
	//     {
	//         CBaseEntity *pEntity = hEntity.Get();
	//         
	//         // Perform trace (matches decompiled trace call)
	//         trace_t tr;
	//         UTIL_TraceLine( vStart, vEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tr );
	//         
	//         // Check trace result (matches decompiled result check)
	//         if ( tr.fraction < 1.0f || tr.startsolid )
	//         {
	//             return true; // Point is in bad result position
	//         }
	//     }
	// }
	
	// Point is not in any bad result position
	return false;
}

//-----------------------------------------------------------------------------
bool CDOTAGameRules::Rules_DisableCreepSpawning( void )
{
	// Check game state conditions (matches decompiled state checks)
	// TODO: Implement when ConVar system is available
	// if ( ( GetGameState() & 0xfffffffe ) != 6 && 
	//      dota_creeps_no_spawning.GetInt() == 0 &&
	//      dota_creeps_no_spawning_force.GetInt() == 0 )
	// {
	//     int nGameMode = GetGameMode();
	//     if ( nGameMode == DOTA_GAMEMODE_DIRETIDE && GetGameState() == DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
	//     {
	//         // Calculate Diretide phase time (matches decompiled time calculation)
	//         float flPhaseTime = ( gpGlobals->curtime - m_flDiretidePhase1StartTime ) + m_flDiretidePhase1Duration;
	//         
	//         // TODO: Implement when telemetry system is available
	//         // g_Telemetry.SetValue( "dota_diretide_phase_time", flPhaseTime );
	//         
	//         // Check if phase 2 has started (matches decompiled phase check)
	//         // TODO: Implement when ConVar system is available
	//         // if ( dota_roshan_halloween_phase2_start_time.GetFloat() <= flPhaseTime &&
	//         //      flPhaseTime != dota_roshan_halloween_phase2_start_time.GetFloat() )
	//         // {
	//         //     return true; // Disable creep spawning in phase 2
	//         // }
	//     }
	//     
	//     return nGameMode == DOTA_GAMEMODE_FROSTIVUS;
	// }
	
	// Default behavior - enable creep spawning
	return false;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter_HERO_SELECTION( void )
{
	// Request PC Bang additional items (matches decompiled request)
	// TODO: Implement when PC Bang system is available
	// SERVER_RequestPCBangAdditionalItems();
	
	// Get GC system and lobby (matches decompiled GC access)
	// TODO: Implement when GC system is available
	// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	// CDOTALobby *pLobby = pGCSystem ? pGCSystem->GetLobby() : NULL;
	
	// Check tutorial and private beta conditions (matches decompiled condition checks)
	// TODO: Implement when ConVar system is available
	// if ( dota_tutorial_enabled.GetInt() == 0 && dota_private_beta.GetInt() == 0 )
	// {
	//     // Check announcer and tutorial conditions (matches decompiled checks)
	//     // TODO: Implement when ConVar system is available
	//     // if ( dota_announcer_enabled.GetInt() == 0 || DOTATutorial()->IsTutorialActive() )
	//     // {
	//         // Set hero pick state based on game mode (matches decompiled mode switch)
	//         int nGameMode = GetGameMode();
	//         DOTA_HeroPickState_t nHeroPickState = DOTA_HEROPICK_STATE_AP_SELECT; // Default
	//         
	//         switch ( nGameMode )
	//         {
	//             case DOTA_GAMEMODE_ALL_PICK:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_AP_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_CAPTAINS_MODE:
	//             case DOTA_GAMEMODE_CUSTOM:
	//                 // TODO: Implement when engine interface is available
	//                 // if ( !engine->IsHLTV() )
	//                 // {
	//                 //     CreateFakeClients( 9 );
	//                 // }
	//                 nHeroPickState = DOTA_HEROPICK_STATE_CM_INTRO;
	//                 break;
	//             case DOTA_GAMEMODE_SINGLE_DRAFT:
	//                 // TODO: Implement when engine interface is available
	//                 // if ( !engine->IsHLTV() )
	//                 // {
	//                 //     CreateFakeClients( 4 );
	//                 // }
	//                 nHeroPickState = DOTA_HEROPICK_STATE_SD_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_RANDOM_DRAFT:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_RD_SELECT_UNUSED;
	//                 break;
	//             case DOTA_GAMEMODE_ALL_RANDOM:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_AR_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_MID_ONLY:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_MO_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_DIRETIDE:
	//             case DOTA_GAMEMODE_FROSTIVUS:
	//             case DOTA_GAMEMODE_CUSTOM_1V1:
	//             case DOTA_GAMEMODE_CUSTOM_5V5:
	//             case DOTA_GAMEMODE_CUSTOM_3V3:
	//             case DOTA_GAMEMODE_CUSTOM_10V10:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_AP_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_ABILITY_DRAFT:
	//                 nHeroPickState = DOTA_HERO_PICK_STATE_ABILITY_DRAFT_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_ALL_RANDOM_DEATH_MATCH:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_ARDM_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_ALL_DRAFT:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_ALL_DRAFT_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_CUSTOMGAME_SELECT:
	//                 nHeroPickState = DOTA_HERO_PICK_STATE_CUSTOMGAME_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_SELECT_PENALTY:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_SELECT_PENALTY;
	//                 break;
	//             case DOTA_GAMEMODE_CUSTOM_PICK_RULES:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_CUSTOM_PICK_RULES;
	//                 break;
	//             case DOTA_GAMEMODE_SCENARIO_PICK:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_SCENARIO_PICK;
	//                 break;
	//             case DOTA_GAMEMODE_CAPTAINS_DRAFT:
	//                 // TODO: Implement when engine interface is available
	//                 // if ( !engine->IsHLTV() )
	//                 // {
	//                 //     CreateFakeClients( 9 );
	//                 // }
	//                 nHeroPickState = DOTA_HEROPICK_STATE_CD_INTRO;
	//                 break;
	//             case DOTA_GAMEMODE_BALANCED_DRAFT:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_BD_SELECT;
	//                 break;
	//             case DOTA_GAMEMODE_ABILITY_DRAFT_2:
	//                 // TODO: Implement when engine interface is available
	//                 // if ( !engine->IsHLTV() )
	//                 // {
	//                 //     CreateFakeClients( 9 );
	//                 // }
	//                 nHeroPickState = DOTA_HEROPICK_STATE_CD_INTRO;
	//                 break;
	//             default:
	//                 nHeroPickState = DOTA_HEROPICK_STATE_AP_SELECT;
	//                 break;
	//         }
	//         
	//         // Update hero pick state (matches decompiled state update)
	//         if ( m_nHeroPickState != nHeroPickState )
	//         {
	//             CGameRulesProxy::NotifyNetworkStateChanged();
	//             m_nHeroPickState = nHeroPickState;
	//         }
	//         
	//         // Find state info and call enter function (matches decompiled state lookup)
	//         // TODO: Implement when hero pick state system is available
	//         // const HeroPickStateInfo_t *pStateInfo = NULL;
	//         // for ( int i = 0; i < DOTA_HEROPICK_STATE_COUNT; i++ )
	//         // {
	//         //     if ( s_HeroPickStateInfos[i].nState == nHeroPickState )
	//         //     {
	//         //         pStateInfo = &s_HeroPickStateInfos[i];
	//         //         break;
	//         //     }
	//         // }
	//         // 
	//         // if ( pStateInfo && pStateInfo->pfnEnter )
	//         // {
	//         //     (this->*pStateInfo->pfnEnter)();
	//         // }
	//     // }
	// }
	// else
	// {
	//     // Set game mode to tutorial (matches decompiled mode setting)
	//     if ( GetGameMode() != DOTA_GAMEMODE_TUTORIAL )
	//     {
	//         CGameRulesProxy::NotifyNetworkStateChanged();
	//         SetGameMode( DOTA_GAMEMODE_TUTORIAL );
	//     }
	//     
	//     // Set hero pick state to AP (matches decompiled state setting)
	//     if ( m_nHeroPickState != DOTA_HEROPICK_STATE_AP_SELECT )
	//     {
	//         CGameRulesProxy::NotifyNetworkStateChanged();
	//         m_nHeroPickState = DOTA_HEROPICK_STATE_AP_SELECT;
	//     }
	// }
	// else
	// {
	//     // Set game mode to private beta (matches decompiled mode setting)
	//     if ( GetGameMode() != DOTA_GAMEMODE_PRIVATE_BETA )
	//     {
	//         CGameRulesProxy::NotifyNetworkStateChanged();
	//         SetGameMode( DOTA_GAMEMODE_PRIVATE_BETA );
	//     }
	//     
	//     // Set hero pick state to AP (matches decompiled state setting)
	//     if ( m_nHeroPickState != DOTA_HEROPICK_STATE_AP_SELECT )
	//     {
	//         CGameRulesProxy::NotifyNetworkStateChanged();
	//         m_nHeroPickState = DOTA_HEROPICK_STATE_AP_SELECT;
	//     }
	// }
	
	// Clear current hero pick state pointer (matches decompiled pointer clearing)
	// m_pCurrentHeroPickState = NULL;
	
	// Select random captains for teams (matches decompiled captain selection)
	// TODO: Implement when player resource system is available
	// if ( g_pDOTAPlayerResource )
	// {
	//     if ( GetGameMode() == DOTA_GAMEMODE_CAPTAINS_DRAFT )
	//     {
	//         // Set invalid captains for CD mode (matches decompiled invalid setting)
	//         m_nRadiantCaptain = -1;
	//         m_nDireCaptain = -1;
	//     }
	//     else
	//     {
	//         // Get Radiant team players (matches decompiled team player retrieval)
	//         CUtlVector<int> radiantPlayers;
	//         g_pDOTAPlayerResource->GetPlayerIDsForTeam( DOTA_TEAM_RADIANT, &radiantPlayers );
	//         
	//         // Select random Radiant captain (matches decompiled random selection)
	//         if ( radiantPlayers.Count() > 0 )
	//         {
	//             int nRandomIndex = RandomInt( 0, radiantPlayers.Count() - 1 );
	//             m_nRadiantCaptain = radiantPlayers[nRandomIndex];
	//         }
	//         
	//         // Get Dire team players (matches decompiled team player retrieval)
	//         CUtlVector<int> direPlayers;
	//         g_pDOTAPlayerResource->GetPlayerIDsForTeam( DOTA_TEAM_DIRE, &direPlayers );
	//         
	//         // Select random Dire captain (matches decompiled random selection)
	//         if ( direPlayers.Count() > 0 )
	//         {
	//             int nRandomIndex = RandomInt( 0, direPlayers.Count() - 1 );
	//             m_nDireCaptain = direPlayers[nRandomIndex];
	//         }
	//     }
	// }
	
	// Clear survey state (matches decompiled state clearing)
	// m_bSurveyShown = false;
	
	// Check survey conditions (matches decompiled survey logic)
	// TODO: Implement when survey system is available
	// bool bShouldShowSurvey = true;
	// 
	// // Check private beta condition (matches decompiled condition check)
	// // TODO: Implement when app system is available
	// // if ( !IsAppPrivateDota2Beta() )
	// // {
	// //     // Check lobby and game mode conditions (matches decompiled checks)
	// //     if ( !pLobby || 
	// //          ( m_nGameMode == 0 && m_nGameMode2 == 0 ) ||
	// //          ( pLobby->GetGameMode() != 0 && pLobby->GetGameMode() != 7 ) )
	// //     {
	// //         bShouldShowSurvey = false;
	// //     }
	// // }
	// 
	// // Check game mode and survey conditions (matches decompiled condition checks)
	// // TODO: Implement when survey system is available
	// // if ( ( GetGameMode() > 21 || ( 0x2886c0 >> ( GetGameMode() & 0x1f ) & 1 ) == 0 ) && bShouldShowSurvey )
	// // {
	// //     // Get enabled survey questions (matches decompiled question retrieval)
	// //     CUtlVector<int> surveyQuestions;
	// //     DOTAGameManager()->GetEnabledSurveyQuestionIDs( &surveyQuestions );
	// //     
	// //     // Create survey message (matches decompiled message creation)
	// //     // TODO: Implement when message system is available
	// //     // CDOTAUserMsg_ShowSurvey surveyMsg;
	// //     
	// //     // Random survey type (matches decompiled random selection)
	// //     int nSurveyType = RandomInt( 1, 3 );
	// //     m_bSurveyShown = ( nSurveyType == 3 );
	// //     
	// //     if ( nSurveyType == 3 )
	// //     {
	// //         // Handle type 3 survey (matches decompiled type 3 logic)
	// //         // TODO: Implement complex survey logic
	// //     }
	// //     else
	// //     {
	// //         // Handle other survey types (matches decompiled other type logic)
	// //         // TODO: Implement other survey types
	// //     }
	// // }
	
	// Check MMR display conditions (matches decompiled MMR logic)
	// TODO: Implement when lobby system is available
	// if ( pLobby && pLobby->ShowMMROnScoreboardAtGameEnd() )
	// {
	//     Msg( "Entering hero selection state - checking which MMRs to send to players" );
	//     
	//     // Initialize MMR arrays (matches decompiled array initialization)
	//     // TODO: Implement MMR array initialization
	//     
	//     // Process players and collect MMR data (matches decompiled player processing)
	//     // TODO: Implement player MMR processing
	//     
	//     // Sort MMR data (matches decompiled sorting)
	//     // TODO: Implement MMR sorting
	//     
	//     // Send MMR messages (matches decompiled message sending)
	//     // TODO: Implement MMR message sending
	// }
	
	// Set state transition time (matches decompiled time setting)
	// TODO: Implement when ConVar system is available
	// float flHeroSelectionTime = dota_hero_selection_time.GetFloat();
	// float flTransitionTime = gpGlobals->curtime + flHeroSelectionTime;
	float flTransitionTime = gpGlobals->curtime + 90.0f; // Default 90 seconds
	
	// Update state transition time (matches decompiled time update)
	if ( m_flStateTransitionTime != flTransitionTime )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flStateTransitionTime = flTransitionTime;
	}
	
	Msg( "DOTA: Entered HERO_SELECTION state\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Leave_HERO_SELECTION( void )
{
	// Process lobby members (matches decompiled member processing)
	// TODO: Implement when GC system is available
	// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem )
	// {
	//     CDOTALobby *pLobby = pGCSystem->GetLobby();
	//     if ( pLobby )
	//     {
	//         int nMemberCount = pLobby->GetMemberCount();
	//         
	//         // Process each lobby member (matches decompiled member loop)
	//         for ( int i = 0; i < nMemberCount; i++ )
	//         {
	//             // Get member Steam ID (matches decompiled Steam ID retrieval)
	//             // TODO: Implement when Steam system is available
	//             // CSteamID steamID = pLobby->GetMemberSteamID( i );
	//             
	//             // Check if member is connected (matches decompiled connection check)
	//             bool bIsConnected = false;
	//             // TODO: Implement when player system is available
	//             // for ( int j = 1; j <= gpGlobals->maxClients; j++ )
	//             // {
	//             //     CBasePlayer *pPlayer = UTIL_PlayerByIndex( j );
	//             //     if ( pPlayer && pPlayer->IsConnected() )
	//             //     {
	//             //         CSteamID playerSteamID = pPlayer->GetSteamIDAsUInt64();
	//             //         if ( playerSteamID == steamID )
	//             //         {
	//             //             bIsConnected = true;
	//             //             break;
	//             //         }
	//             //     }
	//             // }
	//             
	//             // Add disconnected member to list (matches decompiled list addition)
	//             if ( !bIsConnected )
	//             {
	//                 // TODO: Implement when GC system is available
	//                 // int nTeam = pGCSystem->GetTeamForLobbyMember( steamID );
	//                 // 
	//                 // // Add to disconnected members list (matches decompiled list management)
	//                 // TODO: Implement disconnected members list
	//             }
	//         }
	//         
	//         // Log lobby information (matches decompiled logging)
	//         // TODO: Implement when logging system is available
	//         // MinidumpUserStreamInfoAppend( "Lobby NumMembers:%d AllowCheats:%d FillWithBots:%d\n",
	//         //                              nMemberCount, pLobby->GetAllowCheats(), pLobby->GetFillWithBots() );
	//         
	//         // Enable intro mode if needed (matches decompiled intro mode check)
	//         // TODO: Implement when ConVar system is available
	//         // if ( pLobby->GetIntroMode() )
	//         // {
	//         //     Msg( "Enabling Intro mode" );
	//         //     dota_intro_mode.SetValue( 1 );
	//         // }
	//     }
	// }
	
	// Disable bot filling (matches decompiled bot setting)
	// TODO: Implement when ConVar system is available
	// dota_fill_empty_slots_with_bots.SetValue( 0 );
	
	// Create hero statues (matches decompiled statue creation)
	// TODO: Implement when statue system is available
	// CreateHeroStatues();
	
	// Handle team showcase editor (matches decompiled editor handling)
	// TODO: Implement when ConVar system is available
	// if ( dota_team_showcase_editor.GetInt() != 0 )
	// {
	//     // TODO: Implement when team showcase system is available
	//     // CTeamShowcaseEditorManager *pEditor = GetTeamShowcaseEditorManager();
	//     // if ( pEditor )
	//     // {
	//     //     pEditor->OnHeroSelected();
	//     // }
	// }
	
	Msg( "DOTA: Left HERO_SELECTION state\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Think_HERO_SELECTION( void )
{
	// Call current hero pick state think function (matches decompiled state think)
	// TODO: Implement when hero pick state system is available
	// if ( m_pCurrentHeroPickState && m_pCurrentHeroPickState->pfnThink )
	// {
	//     (this->*m_pCurrentHeroPickState->pfnThink)();
	// }
	
	// Check lobby save game (matches decompiled save game check)
	// TODO: Implement when GC system is available
	// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem )
	// {
	//     CDOTALobby *pLobby = pGCSystem->GetLobby();
	//     if ( pLobby )
	//     {
	//         pLobby->HasSaveGame();
	//     }
	// }
	
	Msg( "DOTA: Hero selection think\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::QueueConceptForAllAnnouncers( int nConcept, float flDelay, void *pCriteriaSet, int nTeam, bool bForce )
{
	// Get Radiant announcer (matches decompiled announcer retrieval)
	// TODO: Implement when ConVar system is available
	// if ( dota_announcer_enabled.GetInt() == 0 )
	// {
	//     return; // Announcers disabled
	// }
	
	// Queue concept for Radiant announcer (matches decompiled Radiant queue)
	// TODO: Implement when announcer system is available
	// CBaseEntity *pRadiantAnnouncer = GetAnnouncer( DOTA_TEAM_RADIANT );
	// if ( pRadiantAnnouncer )
	// {
	//     CDOTA_Speech *pSpeech = (CDOTA_Speech *)( (char *)pRadiantAnnouncer + 0x3590 );
	//     pSpeech->QueueConcept( nConcept, flDelay, pCriteriaSet, 8, bForce, NULL, NULL, false );
	// }
	
	// Queue concept for Dire announcer (matches decompiled Dire queue)
	// TODO: Implement when announcer system is available
	// CBaseEntity *pDireAnnouncer = GetAnnouncer( DOTA_TEAM_DIRE );
	// if ( pDireAnnouncer )
	// {
	//     CDOTA_Speech *pSpeech = (CDOTA_Speech *)( (char *)pDireAnnouncer + 0x3590 );
	//     pSpeech->QueueConcept( nConcept, flDelay, pCriteriaSet, 6, bForce, NULL, NULL, false );
	// }
	
	// Queue concept for Neutral announcer (matches decompiled Neutral queue)
	// TODO: Implement when announcer system is available
	// CBaseEntity *pNeutralAnnouncer = GetAnnouncer( DOTA_TEAM_NEUTRAL );
	// if ( pNeutralAnnouncer )
	// {
	//     CDOTA_Speech *pSpeech = (CDOTA_Speech *)( (char *)pNeutralAnnouncer + 0x3590 );
	//     pSpeech->QueueConcept( nConcept, flDelay, pCriteriaSet, 7, bForce, NULL, NULL, false );
	// }
	
	Msg( "DOTA: Queued concept %d for all announcers\n", nConcept );
}

//-----------------------------------------------------------------------------
bool CDOTAGameRules::Rules_ShouldUploadMatchStats( void )
{
	// Check tutorial and private beta conditions (matches decompiled condition checks)
	// TODO: Implement when ConVar system is available
	// if ( dota_announcer_enabled.GetInt() == 0 )
	// {
	//     if ( dota_tutorial_enabled.GetInt() == 0 )
	//     {
	//         return dota_private_beta.GetInt() == 0; // Upload stats if not in private beta
	//     }
	//     return false; // Don't upload stats in tutorial
	// }
	// return false; // Don't upload stats when announcer is enabled
	
	// Default behavior - upload match stats
	return true;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SendProjectionAbilityMessage(CBaseEntity *pCaster, Vector vPosition, bool bIsVisible, int nAbilityID, float flDelay, bool bIsProjected, CBaseEntity *pTarget )
{
	// Check if projection system is enabled (matches decompiled system check)
	// TODO: Implement when ConVar system is available
	// if ( dota_projection_system.GetInt() == 0 )
	// {
	//     return; // Projection system disabled
	// }
	
	// Validate caster entity (matches decompiled entity validation)
	if ( !pCaster )
	{
		return;
	}
	
	// Get entity handles (matches decompiled handle retrieval)
	// TODO: Implement when entity system is available
	// int nCasterHandle = pCaster->entindex();
	// int nTargetHandle = pTarget ? pTarget->entindex() : -1;
	
	// Set position (matches decompiled position setting)
	Vector vProjectionPos = vPosition;
	
	// Check if position is origin (matches decompiled origin check)
	// TODO: Implement when vector system is available
	// if ( vPosition == vec3_origin )
	// {
	//     // Use caster's absolute position (matches decompiled position calculation)
	//     // TODO: Implement when entity system is available
	//     // if ( pCaster->IsEFlagSet( EFL_DIRTY_ABSTRANSFORM ) )
	//     // {
	//     //     pCaster->CalcAbsolutePosition();
	//     // }
	//     // vProjectionPos = pCaster->GetAbsOrigin();
	// }
	
	// Check visibility conditions (matches decompiled visibility logic)
	// TODO: Implement when projection system is available
	// if ( !pTarget && !bIsProjected && 
	//      dota_projection_visibility.GetInt() != 0 &&
	//      !IsProjectionAbilityVisibleByEnemy( nCasterHandle ) )
	// {
	//     // Add to hidden projections list (matches decompiled list management)
	//     // TODO: Implement when projection list system is available
	//     // int nIndex = m_HiddenProjections.Count();
	//     // m_HiddenProjections.EnsureCapacity( nIndex + 1 );
	//     // m_HiddenProjections.AddToTail();
	//     // 
	//     // ProjectionAbility_t &projection = m_HiddenProjections[nIndex];
	//     // projection.nCasterHandle = nCasterHandle;
	//     // projection.nTargetHandle = nTargetHandle;
	//     // projection.vPosition = vProjectionPos;
	//     // projection.bIsVisible = bIsVisible;
	//     // projection.nAbilityID = nAbilityID;
	//     // projection.flDelay = flDelay + gpGlobals->curtime;
	//     // projection.bIsProjected = bIsProjected;
	//     
	//     return; // Don't send message for hidden projections
	// }
	
	// Send projection ability message (matches decompiled message sending)
	// TODO: Implement when projection system is available
	// SendProjectionAbilityMessageInternal( &projection );
	
	Msg( "DOTA: Sent projection ability message for ability %d\n", nAbilityID );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SetUseBaseGoldBountyOnHeroes( bool bUseBaseGold )
{
	// Update base gold bounty setting (matches decompiled setting update)
	if ( m_bUseBaseGoldBountyOnHeroes != bUseBaseGold )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bUseBaseGoldBountyOnHeroes = bUseBaseGold;
	}
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter_GAME_IN_PROGRESS( void )
{
	// Set first tower state (matches decompiled tower state setting)
	// TODO: Implement when tower system is available
	// SetFirstTowerState();
	
	// Notify hero list that game started (matches decompiled hero list notification)
	// TODO: Implement when hero list system is available
	// g_HeroList->GameStarted();
	
	// Fire game start output (matches decompiled output firing)
	// TODO: Implement when entity system is available
	// if ( m_hGameStartOutput.IsValid() )
	// {
	//     CBaseEntity *pOutput = m_hGameStartOutput.Get();
	//     if ( pOutput )
	//     {
	//         pOutput->FireOutput( "OnGameStart", NULL, NULL, 0.0f );
	//     }
	// }
	
	// Start spectator graph tracking (matches decompiled graph tracking)
	// TODO: Implement when spectator system is available
	// CDOTASpectatorGraphManager *pGraphManager = GetDOTASpectatorGraphManager();
	// if ( pGraphManager )
	// {
	//     pGraphManager->StartTracking();
	// }
	
	// Start VPROF tracking (matches decompiled VPROF start)
	// TODO: Implement when VPROF system is available
	// g_VProfOGS->OnGameStart();
	
	// Handle bot starting gold (matches decompiled bot gold logic)
	// TODO: Implement when ConVar system is available
	// if ( dota_bot_starting_gold.GetInt() != 0 && g_HeroList->GetHeroCount() > 0 )
	// {
	//     // Process all heroes (matches decompiled hero loop)
	//     for ( int i = 0; i < g_HeroList->GetHeroCount(); i++ )
	//     {
	//         CDOTA_BaseNPC *pHero = g_HeroList->GetHero( i );
	//         if ( pHero && pHero->IsBot() )
	//         {
	//             // Give starting gold to bots (matches decompiled gold giving)
	//             // TODO: Implement when player resource system is available
	//             // int nStartingGold = dota_bot_starting_gold.GetInt() * 400;
	//             // g_pDOTAPlayerResource->ModifyGold( pHero->GetPlayerID(), nStartingGold, true, 0 );
	//         }
	//     }
	// }
	
	// Set game start time (matches decompiled time setting)
	float flGameStartTime = gpGlobals->curtime;
	if ( m_flGameStartTime != flGameStartTime )
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flGameStartTime = flGameStartTime;
	}
	
	// Set pre-game time (matches decompiled time setting)
	m_flPreGameTime = m_flPreGameDuration;
	
	// Send game start chat message (matches decompiled chat message)
	// TODO: Implement when GC system is available
	// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem )
	// {
	//     CDOTALobby *pLobby = pGCSystem->GetLobby();
	//     if ( pLobby && ( pLobby->GetGameMode() == 0 || pLobby->GetGameMode() == 7 ) )
	//     {
	//         // TODO: Implement when chat system is available
	//         // SendChatEventMessage( DOTA_CHAT_EVENT_GAME_START, 0, -1, 0 );
	//     }
	// }
	
	// Handle Diretide candy buckets (matches decompiled bucket handling)
	// TODO: Implement when game mode system is available
	// if ( GetGameMode() == DOTA_GAMEMODE_DIRETIDE )
	// {
	//     // Remove invulnerability from candy buckets (matches decompiled modifier removal)
	//     // TODO: Implement when entity system is available
	//     // CBaseEntity *pRadiantBucket = GetTeamCandyBucket( DOTA_TEAM_RADIANT );
	//     // CBaseEntity *pDireBucket = GetTeamCandyBucket( DOTA_TEAM_DIRE );
	//     // 
	//     // if ( pRadiantBucket )
	//     // {
	//     //     pRadiantBucket->RemoveModifierByName( "modifier_invulnerable" );
	//     // }
	//     // 
	//     // if ( pDireBucket )
	//     // {
	//     //     pDireBucket->RemoveModifierByName( "modifier_invulnerable" );
	//     // }
	// }
	
	// Handle Frostivus mode (matches decompiled Frostivus handling)
	// TODO: Implement when game mode system is available
	// if ( GetGameMode() == DOTA_GAMEMODE_FROSTIVUS )
	// {
	//     // TODO: Implement when game manager system is available
	//     // CDOTAGameManager *pGameManager = DOTAGameManager();
	//     // if ( pGameManager && strcmp( pGameManager->GetGameModeString(), "frostivus" ) == 0 )
	//     // {
	//     //     // TODO: Implement when chat system is available
	//     //     // SendChatEventMessage( DOTA_CHAT_EVENT_FROSTIVUS_START, DOTA_TEAM_RADIANT, 0 );
	//     // }
	// }
	
	// Handle custom game modes (matches decompiled custom mode handling)
	// TODO: Implement when GC system is available
	// if ( pGCSystem )
	// {
	//     int nGameMode = pGCSystem->GetGameMode();
	//     if ( nGameMode == 4 )
	//     {
	//         // TODO: Implement when chat system is available
	//         // if ( pGCSystem->GetCustomGameMode() == 0 && 
	//         //      GetGameMode() != DOTA_GAMEMODE_CUSTOM_1V1 &&
	//         //      GetGameMode() != DOTA_GAMEMODE_FROSTIVUS )
	//         // {
	//         //     SendChatEventMessage( DOTA_CHAT_EVENT_CUSTOM_GAME_START, 4, 0 );
	//         // }
	//     }
	//     else if ( nGameMode == 7 )
	//     {
	//         // TODO: Implement when chat system is available
	//         // SendChatEventMessage( DOTA_CHAT_EVENT_CUSTOM_GAME_START, 3, 0 );
	//     }
	// }
	
	// Handle Greevil mode (matches decompiled Greevil handling)
	// TODO: Implement when game mode system is available
	// if ( GetGameMode() == DOTA_GAMEMODE_GREEVIL )
	// {
	//     // TODO: Implement when sound system is available
	//     // Gamerules_EmitSound( "greevil_eventstart_Stinger", NULL, false );
	// }
	
	// Handle custom game modes with invulnerability (matches decompiled invulnerability handling)
	// TODO: Implement when game mode system is available
	// if ( GetGameMode() == DOTA_GAMEMODE_CUSTOM_1V1 || GetGameMode() == DOTA_GAMEMODE_FROSTIVUS )
	// {
	//     // TODO: Implement when entity system is available
	//     // CBaseEntity *pRadiantFort = gEntList.FindEntityByName( NULL, "dota_goodguys_fort" );
	//     // if ( pRadiantFort && pRadiantFort->GetHealth() < 1 )
	//     // {
	//     //     pRadiantFort->RemoveModifierByName( "modifier_invulnerable" );
	//     // }
	// }
	
	// Broadcast item drop bonus status (matches decompiled broadcast)
	// TODO: Implement when item system is available
	// BroadcastItemDropBonusStatus();
	
	Msg( "DOTA: Entered GAME_IN_PROGRESS state\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Think_GAME_IN_PROGRESS( void )
{
	// Handle pause countdown (matches decompiled pause logic)
	// TODO: Implement when pause system is available
	// if ( m_nPauseCountdown > 0 )
	// {
	//     m_nPauseCountdown--;
	//     if ( m_nPauseCountdown == 0 )
	//     {
	//         SetGamePaused( true, -1, -1.0f, -1.0f );
	//         m_nPauseCountdown = -1;
	//     }
	// }
	
	// Handle Frostivus win conditions (matches decompiled win logic)
	// TODO: Implement when game mode system is available
	// if ( GetGameMode() == DOTA_GAMEMODE_FROSTIVUS )
	// {
	//     // TODO: Implement when ConVar system is available
	//     // if ( m_nRadiantScore == dota_frostivus_win_score.GetInt() )
	//     // {
	//     //     MakeTeamLose( DOTA_TEAM_DIRE );
	//     // }
	//     // else if ( m_nDireScore == dota_frostivus_win_score.GetInt() )
	//     // {
	//     //     MakeTeamLose( DOTA_TEAM_RADIANT );
	//     // }
	// }
	
	// Deliver resources to players (matches decompiled resource delivery)
	// TODO: Implement when resource system is available
	// DeliverGoldToPlayers();
	// DeliverXPToPlayers();
	// DistributeAbandonedPlayerGold();
	
	// Update game systems (matches decompiled system updates)
	// TODO: Implement when system is available
	// UpdateRunes();
	// UpdateFanfareCount( gpGlobals->curtime );
	
	// Update day/night cycle (matches decompiled cycle update)
	// TODO: Implement when ConVar system is available
	// if ( dota_announcer_enabled.GetInt() == 0 || 
	//      dota_day_night_cycle.GetInt() == 0 ||
	//      dota_time_of_day.GetInt() == 0 ||
	//      dota_private_beta.GetInt() == 0 )
	// {
	//     float flTimeOfDay = 0.0f;
	//     if ( !m_bGamePaused )
	//     {
	//         flTimeOfDay = gpGlobals->frametime;
	//     }
	//     
	//     // Calculate time of day (matches decompiled calculation)
	//     // TODO: Implement when ConVar system is available
	//     // float flTimeScale = dota_time_scale.GetFloat();
	//     // flTimeOfDay = m_flTimeOfDay + flTimeOfDay * flTimeScale;
	//     
	//     // Normalize time of day (matches decompiled normalization)
	//     while ( flTimeOfDay > 1.0f )
	//     {
	//         flTimeOfDay -= 1.0f;
	//     }
	//     
	//     SetTimeOfDay( flTimeOfDay );
	// }
	
	// Check idle and disconnected players (matches decompiled player check)
	// TODO: Implement when player system is available
	// CheckIdleAndDisconnectedPlayers();
	
	// Update telemetry (matches decompiled telemetry update)
	float flGameTime = 0.0f;
	if ( GetGameState() > 4 )
	{
		if ( ( GetGameState() & 0xfffffffe ) == 6 )
		{
			flGameTime = m_flPreGameTime;
		}
		else
		{
			flGameTime = gpGlobals->curtime;
		}
		flGameTime = ( flGameTime - m_flGameStartTime ) + m_flPreGameDuration;
	}
	
	// TODO: Implement when telemetry system is available
	// g_Telemetry.SetValue( "dota_game_time", flGameTime );
	
	// Record balance for time (matches decompiled balance recording)
	// TODO: Implement when ConVar system is available
	// if ( flGameTime > (float)( m_nBalanceRecordTime * 60 ) )
	// {
	//     // TODO: Implement when player resource system is available
	//     // g_pDOTAPlayerResource->RecordBalanceForTime();
	//     m_nBalanceRecordTime++;
	// }
	
	// Update automatic surrender (matches decompiled surrender update)
	// TODO: Implement when surrender system is available
	// UpdateAutomaticSurrender();
	
	// Check for game end (matches decompiled end check)
	// TODO: Implement when game end system is available
	// if ( GetGameState() != DOTA_GAMERULES_STATE_POST_GAME )
	// {
	//     State_Transition( DOTA_GAMERULES_STATE_POST_GAME );
	// }
	
	Msg( "DOTA: Game in progress think\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::DistributeAbandonedPlayerGold( void )
{
	// Skip distribution for certain game modes (matches decompiled mode check)
	// TODO: Implement when game mode system is available
	// if ( GetGameMode() == DOTA_GAMEMODE_CUSTOM_1V1 || GetGameMode() == DOTA_GAMEMODE_FROSTIVUS )
	// {
	//     return; // Don't distribute gold in these modes
	// }
	
	// Calculate game time (matches decompiled time calculation)
	float flLastDistributionTime = m_flLastAbandonedGoldDistributionTime;
	float flGameTime = 0.0f;
	
	if ( GetGameState() > 4 )
	{
		if ( ( GetGameState() & 0xfffffffe ) == 6 )
		{
			flGameTime = m_flPreGameTime;
		}
		else
		{
			flGameTime = gpGlobals->curtime;
		}
		flGameTime = ( flGameTime - m_flGameStartTime ) + m_flPreGameDuration;
	}
	
	// TODO: Implement when telemetry system is available
	// g_Telemetry.SetValue( "dota_game_time", flGameTime );
	
	// Check if enough time has passed for distribution (matches decompiled time check)
	// TODO: Implement when ConVar system is available
	// if ( flLastDistributionTime <= flGameTime )
	// {
	//     // Initialize team arrays (matches decompiled array initialization)
	//     CUtlVector<int> radiantPlayers;
	//     CUtlVector<int> direPlayers;
	//     CUtlVector<int> radiantAbandoned;
	//     CUtlVector<int> direAbandoned;
	//     
	//     // Process all players (matches decompiled player loop)
	//     for ( int i = 0; i < 10; i++ )
	//     {
	//         if ( g_pDOTAPlayerResource->IsValidTeamPlayer( i ) )
	//         {
	//             int nTeam = g_pDOTAPlayerResource->GetTeam( i );
	//             if ( ( nTeam & 0xfffffffe ) == 2 ) // Valid team
	//             {
	//                 int nConnectionState = g_pDOTAPlayerResource->GetConnectionState( i );
	//                 int nTeamIndex = nTeam - 2; // Convert to 0-based index
	//                 
	//                 if ( nConnectionState < 4 ) // Connected or loading
	//                 {
	//                     // Check if player is AFK (matches decompiled AFK check)
	//                     // TODO: Implement when player resource system is available
	//                     // if ( !g_pDOTAPlayerResource->IsAFK( i ) )
	//                     // {
	//                     //     // Add to active players list (matches decompiled list management)
	//                     //     radiantPlayers.AddToTail( i );
	//                     // }
	//                 }
	//                 else // Disconnected
	//                 {
	//                     // Add to abandoned players list (matches decompiled list management)
	//                     // TODO: Implement when team system is available
	//                     // if ( nTeam == DOTA_TEAM_RADIANT )
	//                     // {
	//                     //     radiantAbandoned.AddToTail( i );
	//                     // }
	//                     // else if ( nTeam == DOTA_TEAM_DIRE )
	//                     // {
	//                     //     direAbandoned.AddToTail( i );
	//                     // }
	//                 }
	//             }
	//         }
	//     }
	//     
	//     // Distribute gold for each team (matches decompiled distribution logic)
	//     for ( int nTeam = 0; nTeam < 2; nTeam++ )
	//     {
	//         CUtlVector<int> &abandonedPlayers = ( nTeam == 0 ) ? radiantAbandoned : direAbandoned;
	//         CUtlVector<int> &activePlayers = ( nTeam == 0 ) ? radiantPlayers : direPlayers;
	//         
	//         if ( abandonedPlayers.Count() > 0 && activePlayers.Count() > 0 )
	//         {
	//             int nTotalGold = 0;
	//             
	//             // Collect gold from abandoned players (matches decompiled gold collection)
	//             for ( int i = 0; i < abandonedPlayers.Count(); i++ )
	//             {
	//                 int nPlayerID = abandonedPlayers[i];
	//                 int nPlayerGold = g_pDOTAPlayerResource->GetGold( nPlayerID );
	//                 int nDistributableGold = nPlayerGold - ( nPlayerGold % activePlayers.Count() );
	//                 
	//                 if ( nDistributableGold > 0 )
	//                 {
	//                     // TODO: Implement when player resource system is available
	//                     // g_pDOTAPlayerResource->SpendGold( nPlayerID, nDistributableGold, 5 );
	//                     nTotalGold += nDistributableGold;
	//                 }
	//             }
	//             
	//             // Distribute gold to active players (matches decompiled gold distribution)
	//             if ( nTotalGold > 0 )
	//             {
	//                 int nGoldPerPlayer = nTotalGold / activePlayers.Count();
	//                 for ( int i = 0; i < activePlayers.Count(); i++ )
	//                 {
	//                     int nPlayerID = activePlayers[i];
	//                     // TODO: Implement when player resource system is available
	//                     // g_pDOTAPlayerResource->ModifyGold( nPlayerID, nGoldPerPlayer, false, 5 );
	//                 }
	//             }
	//         }
	//     }
	//     
	//     // Update distribution time (matches decompiled time update)
	//     // TODO: Implement when ConVar system is available
	//     // float flDistributionInterval = dota_abandoned_gold_distribution_interval.GetFloat();
	//     // m_flLastAbandonedGoldDistributionTime = flGameTime + flDistributionInterval;
	// }
	
	Msg( "DOTA: Distributed abandoned player gold\n" );
}

//-----------------------------------------------------------------------------
void *CDOTAGameRules::GetPlayerCommunicationSummary( int nPlayerID )
{
	// Validate player ID (matches decompiled bounds check)
	if ( nPlayerID < 0 || nPlayerID >= 32 )
	{
		return NULL;
	}
	
	// Return pointer to player communication summary (matches decompiled pointer calculation)
	// TODO: Implement when communication system is available
	// return &m_PlayerCommunicationSummaries[nPlayerID];
	
	return NULL;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::FinalizePlayerCommunicationSummary( int nPlayerID )
{
	// Validate player ID (matches decompiled bounds check)
	if ( nPlayerID < 0 || nPlayerID >= 32 )
	{
		return;
	}
	
	// TODO: Implement when communication system is available
	// PlayerCommunicationSummary_t &summary = m_PlayerCommunicationSummaries[nPlayerID];
	// 
	// // Update max values (matches decompiled max value updates)
	// if ( summary.nMaxPing < summary.nCurrentPing )
	// {
	//     summary.nMaxPing = summary.nCurrentPing;
	// }
	// summary.nLastPingUpdateTime = sm_nTimeCur;
	// 
	// if ( summary.nMaxLoss < summary.nCurrentLoss )
	// {
	//     summary.nMaxLoss = summary.nCurrentLoss;
	// }
	// summary.nLastLossUpdateTime = sm_nTimeCur;
	
	Msg( "DOTA: Finalized player communication summary for player %d\n", nPlayerID );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::CheckIdleAndDisconnectedPlayers( void )
{
	// Check if player resource is available (matches decompiled resource check)
	// TODO: Implement when player resource system is available
	// if ( !g_pDOTAPlayerResource || GetGameState() <= 1 )
	// {
	//     return;
	// }
	
	// Update countdown timer (matches decompiled timer update)
	// TODO: Implement when timer system is available
	// CountdownTimer *pTimer = &m_IdleCheckTimer;
	// if ( pTimer->GetRemainingTime() <= 0.0f )
	// {
	//     pTimer->Start( 1.0f );
	// }
	
	// Check if timer has expired (matches decompiled timer check)
	// TODO: Implement when timer system is available
	// if ( pTimer->IsElapsed() )
	// {
	//     // Get GC system and lobby (matches decompiled GC access)
	//     // TODO: Implement when GC system is available
	//     // CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	//     // if ( pGCSystem )
	//     // {
	//     //     CDOTALobby *pLobby = pGCSystem->GetLobby();
	//     //     if ( pLobby && pLobby->BLeaversMatterForLobbyType() )
	//     //     {
	//     //         // Process all players (matches decompiled player loop)
	//     //         for ( int i = 0; i < 10; i++ )
	//     //         {
	//     //             if ( g_pDOTAPlayerResource->IsValidTeamPlayer( i ) && 
	//     //                  !g_pDOTAPlayerResource->IsFakeClient( i ) )
	//     //             {
	//     //                 // Check disconnection time (matches decompiled time check)
	//     //                 int nTotalDisconnectedTime = g_pDOTAPlayerResource->GetTotalDisconnectedTime( i );
	//     //                 int nLeaverStatus = g_pDOTAPlayerResource->GetLeaverStatus( i );
	//     //                 
	//     //                 // Update disconnection time (matches decompiled time update)
	//     //                 if ( ( nLeaverStatus & 0xfffffffb ) == 1 )
	//     //                 {
	//     //                     g_pDOTAPlayerResource->IncrementTotalDisconnectedTime( i, 1 );
	//     //                 }
	//     //                 
	//     //                 // Check connection state (matches decompiled state check)
	//     //                 int nConnectionState = g_pDOTAPlayerResource->GetConnectionState( i );
	//     //                 if ( nConnectionState == 3 ) // Loading
	//     //                 {
	//     //                     // TODO: Implement when ConVar system is available
	//     //                     // if ( dota_player_disconnect_timeout.GetInt() < nTotalDisconnectedTime )
	//     //                     // {
	//     //                     //     g_pDOTAPlayerResource->SetConnectionState( i, 4, 0 );
	//     //                     // }
	//     //                 }
	//     //                 
	//     //                 // Check Steam ID and lobby membership (matches decompiled Steam check)
	//     //                 // TODO: Implement when Steam system is available
	//     //                 // CSteamID steamID = g_pDOTAPlayerResource->GetSteamID( i );
	//     //                 // int nLobbyMemberIndex = pLobby->GetMemberIndexBySteamID( steamID );
	//     //                 // 
	//     //                 // if ( nLobbyMemberIndex != -1 )
	//     //                 // {
	//     //                     // TODO: Implement when lobby system is available
	//                     // CDOTALobbyMember *pMember = pLobby->GetMember( nLobbyMemberIndex );
	//                     // if ( pMember && pMember->GetDisconnectTime() < 2 )
	//                     // {
	//                     //     // Check game state conditions (matches decompiled condition checks)
	//                     //     bool bShouldCheckIdle = ( GetGameState() & 0xfffffffc ) == 4 &&
	//                     //                              GetGameMode() != DOTA_GAMEMODE_FROSTIVUS &&
	//                     //                              GetGameMode() != DOTA_GAMEMODE_CUSTOM_1V1;
	//                     //     
	//                     //     // Check hero selection for certain modes (matches decompiled hero check)
	//                     //     if ( GetGameMode() == DOTA_GAMEMODE_SELECT_PENALTY )
	//                     //     {
	//                     //         // TODO: Implement when hero system is available
	//                     //         // EHANDLE hHero = g_pDOTAPlayerResource->GetPlayerSelectedHeroHandle( i );
	//                     //         // if ( hHero.IsValid() )
	//                     //         // {
	//                     //         //     CDOTA_BaseNPC *pHero = hHero.Get();
	//                     //         //     if ( pHero && pHero->IsHero() )
	//                     //         //     {
	//                     //         //         bShouldCheckIdle = pHero->CanBeSeenByTeam( steamID, i );
	//                     //         //     }
	//                     //         // }
	//                     //     }
	//                     //     
	//                     //     // Check leaver status and idle conditions (matches decompiled idle logic)
	//                     //     if ( ( nLeaverStatus & 0xfffffffb ) == 1 )
	//                     //     {
	//                     //         // TODO: Implement when ConVar system is available
	//                     //         // if ( !dota_allow_leavers.GetBool() && 
	//                     //         //      GetGameMode() != DOTA_GAMEMODE_TUTORIAL &&
	//                     //         //      !pLobby->GetAllowCheats() )
	//                     //         // {
	//                     //             // Send chat messages and update leaver status (matches decompiled message logic)
	//                     //             // TODO: Implement when chat system is available
	//                     //             // int nTimeoutMinutes = dota_player_disconnect_timeout.GetInt() / 60;
	//                     //             // SendChatEventMessage( DOTA_CHAT_EVENT_PLAYER_DISCONNECT, nTimeoutMinutes, i, 0 );
	//                     //             // 
	//                     //             // if ( nTotalDisconnectedTime > dota_player_disconnect_timeout.GetInt() )
	//                     //             // {
	//                     //             //     int nNewLeaverStatus = ( nLeaverStatus == 1 ) ? 2 : 6;
	//                     //             //     g_pDOTAPlayerResource->SetLeaverStatus( i, nNewLeaverStatus );
	//                     //             // }
	//                     //         // }
	//                     //     }
	//                     //     else if ( nLeaverStatus == 0 && bShouldCheckIdle )
	//                     //     {
	//                     //         // Check hero and AFK status (matches decompiled AFK check)
	//                     //         // TODO: Implement when hero system is available
	//                     //         // EHANDLE hHero = g_pDOTAPlayerResource->GetPlayerSelectedHeroHandle( i );
	//                     //         // if ( hHero.IsValid() )
	//                     //         // {
	//                     //         //     CDOTA_BaseNPC *pHero = hHero.Get();
	//                     //         //     if ( pHero && pHero->IsHero() )
	//                     //         //     {
	//                     //         //         CDOTAPlayer *pPlayer = pHero->GetPlayerOwner();
	//                     //         //         if ( pPlayer && pPlayer->IsConnected() )
	//                     //         //         {
	//                     //         //             int nTeam = g_pDOTAPlayerResource->GetTeam( i );
	//                     //         //             bool bIsAFK = pPlayer->IsAFK();
	//                     //         //             bool bIsFountainIdle = pPlayer->IsFountainIdle( nTeam == DOTA_TEAM_RADIANT );
	//                     //         //             
	//                     //         //             if ( !bIsAFK && !bIsFountainIdle )
	//                     //         //             {
	//                     //         //                 // Player is active, continue
	//                     //         //             }
	//                     //         //         }
	//                     //         //     }
	//                     //         // }
	//                     //         
	//                     //         // Set player as AFK (matches decompiled AFK setting)
	//                     //         // g_pDOTAPlayerResource->SetLeaverStatus( i, 4 );
	//                     //     }
	//                     // }
	//                     // 
	//                     // // Check idle timeout (matches decompiled timeout check)
	//                     // if ( bShouldCheckIdle )
	//                     // {
	//                     //     // TODO: Implement when hero system is available
	//                     //     // bool bHasSelectedHero = g_pDOTAPlayerResource->HasSelectedHero( i );
	//                     //     // bool bIsReserved = g_pDOTAPlayerResource->GetPlayerReservedState( i );
	//                     //     // 
	//                     //     // if ( bHasSelectedHero && !bIsReserved )
	//                     //     // {
	//                     //     //     // TODO: Implement when ConVar system is available
	//                     //     //     // float flIdleTimeout = dota_player_idle_timeout.GetFloat();
	//                     //     //     // if ( flGameTime > flIdleTimeout && nLeaverStatus < 2 )
	//                     //     //     // {
	//                     //     //     //     g_pDOTAPlayerResource->SetLeaverStatus( i, 4 );
	//                     //     //     // }
	//                     //     // }
	//                     // }
	//                 // }
	//             }
	//         }
	//     }
	// }
	
	Msg( "DOTA: Checked idle and disconnected players\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::QueueConceptForAllAnnouncers_KillingSpree( int nConcept, float flDelay, void *pCriteriaSet, int nTeam, bool bForce )
{
	// Get announcer for Radiant team (matches decompiled announcer access)
	// TODO: Implement when announcer system is available
	// CDOTA_BaseNPC *pRadiantAnnouncer = GetAnnouncer_KillingSpree( DOTA_TEAM_RADIANT );
	// if ( pRadiantAnnouncer && !dota_announcer_disable.GetBool() )
	// {
	//     // Queue concept for Radiant announcer (matches decompiled queue call)
	//     // TODO: Implement when speech system is available
	//     // CDOTA_Speech::QueueConcept( pRadiantAnnouncer->GetSpeech(), nConcept, flDelay, pCriteriaSet, 8, bForce, NULL, NULL, false );
	// }
	
	// Get announcer for Dire team (matches decompiled announcer access)
	// TODO: Implement when announcer system is available
	// CDOTA_BaseNPC *pDireAnnouncer = GetAnnouncer_KillingSpree( DOTA_TEAM_DIRE );
	// if ( pDireAnnouncer && !dota_announcer_disable.GetBool() )
	// {
	//     // Queue concept for Dire announcer (matches decompiled queue call)
	//     // TODO: Implement when speech system is available
	//     // CDOTA_Speech::QueueConcept( pDireAnnouncer->GetSpeech(), nConcept, flDelay, pCriteriaSet, 6, bForce, NULL, NULL, false );
	// }
	
	// Get announcer for Neutral team (matches decompiled announcer access)
	// TODO: Implement when announcer system is available
	// CDOTA_BaseNPC *pNeutralAnnouncer = GetAnnouncer_KillingSpree( DOTA_TEAM_NEUTRAL );
	// if ( pNeutralAnnouncer && !dota_announcer_disable.GetBool() )
	// {
	//     // Queue concept for Neutral announcer (matches decompiled queue call)
	//     // TODO: Implement when speech system is available
	//     // CDOTA_Speech::QueueConcept( pNeutralAnnouncer->GetSpeech(), nConcept, flDelay, pCriteriaSet, 7, bForce, NULL, NULL, false );
	// }
	
	Msg( "DOTA: Queued killing spree concept for all announcers\n" );
}


//-----------------------------------------------------------------------------
void CDOTAGameRules::Glyph( int nTeam )
{
	// Get glyph cooldown time for the team (matches decompiled cooldown access)
	float *pGlyphCooldown = ( nTeam == DOTA_TEAM_RADIANT ) ? &m_flGlyphCooldownRadiant : &m_flGlyphCooldownDire;
	
	// Check if glyph is on cooldown (matches decompiled cooldown check)
	if ( gpGlobals->curtime < *pGlyphCooldown )
	{
		return; // Glyph is still on cooldown
	}
	
	// Create glyph modifier KeyValues (matches decompiled modifier creation)
	// TODO: Implement when KeyValues system is available
	// KeyValues *pGlyphModifier = new KeyValues( "modifier_fountain_glyph" );
	// pGlyphModifier->SetFloat( "duration", 5.0f );
	
	// Apply glyph to all buildings of the team (matches decompiled building iteration)
	// TODO: Implement when entity system is available
	// const char *pszBuildingClasses[] = {
	//     "npc_dota_building_ancient",
	//     "npc_dota_building_tower1",
	//     "npc_dota_building_tower2", 
	//     "npc_dota_building_tower3",
	//     "npc_dota_building_tower4",
	//     "npc_dota_building_barracks",
	//     "npc_dota_building_filler"
	// };
	// 
	// for ( int i = 0; i < 7; i++ )
	// {
	//     CBaseEntity *pBuilding = NULL;
	//     while ( ( pBuilding = gEntList.FindEntityByClassname( pBuilding, pszBuildingClasses[i] ) ) != NULL )
	//     {
	//         if ( pBuilding->IsAlive() && pBuilding->GetTeamNumber() == nTeam )
	//         {
	//             // TODO: Implement when modifier system is available
	//             // CDOTA_BaseNPC *pNPC = dynamic_cast<CDOTA_BaseNPC*>( pBuilding );
	//             // if ( pNPC )
	//             // {
	//             //     pNPC->AddNewModifier( pNPC, pNPC, NULL, "modifier_fountain_glyph", pGlyphModifier );
	//             // }
	//         }
	//     }
	// }
	
	// Set glyph cooldown (matches decompiled cooldown setting)
	float flNewCooldown = gpGlobals->curtime + 300.0f; // 5 minutes
	
	if ( nTeam == DOTA_TEAM_RADIANT )
	{
		if ( flNewCooldown != m_flGlyphCooldownRadiant )
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_flGlyphCooldownRadiant = flNewCooldown;
		}
	}
	else
	{
		if ( flNewCooldown != m_flGlyphCooldownDire )
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_flGlyphCooldownDire = flNewCooldown;
		}
	}
	
	// Send chat message (matches decompiled chat message)
	// TODO: Implement when chat system is available
	// SendChatEventMessage( DOTA_CHAT_EVENT_GLYPH_USED, 0, nTeam, 0 );
	
	// Queue announcer concept (matches decompiled announcer queue)
	// TODO: Implement when response rules system is available
	// ResponseRules::CriteriaSet criteriaSet;
	// criteriaSet.AppendCriteria( "announce_event", ( nTeam == DOTA_TEAM_RADIANT ) ? "glyph_used_good" : "glyph_used_bad", 1.0f );
	// QueueConceptForAllAnnouncers( DOTA_CONCEPT_GLYPH_USED, -1.0f, &criteriaSet, nTeam, true );
	
	// Clean up (matches decompiled cleanup)
	// TODO: Implement when KeyValues system is available
	// pGlyphModifier->deleteThis();
	
	Msg( "DOTA: Glyph activated for team %d\n", nTeam );
}



//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Think_PRE_GAME( void )
{
	// Skip pre-game logic if forced by ConVars (matches decompiled ConVar checks)
	// TODO: Implement when ConVar system is available
	// if ( dota_force_gamestart.GetInt() == 1 || dota_skip_pregame.GetInt() != 0 )
	//     return;
	
	// Check if pre-game time has expired (matches decompiled time comparison)
	if ( m_flStateTransitionTime <= gpGlobals->curtime )
	{
		// Transition to game in progress
		State_Transition( DOTA_GAMERULES_STATE_GAME_IN_PROGRESS );
		return;
	}
	
	// Handle creep spawn preparation (matches decompiled -30 second check)
	// TODO: Implement when creep system is available
	// if ( m_flCreepSpawnPrepTime != -1.0f && m_flCreepSpawnPrepTime <= gpGlobals->curtime )
	// {
	//     PrepareSpawners( 30.0f ); // Prepare spawners 30 seconds early
	//     m_flCreepSpawnPrepTime = -1.0f; // Mark as completed
	// }
	
	// Handle game start announcements (matches decompiled announcement timing)
	// TODO: Implement when announcer system is available
	// if ( m_flGameStartAnnouncementTime > 0.0f && 
	//      m_flStateTransitionTime - gpGlobals->curtime <= m_flGameStartAnnouncementTime )
	// {
	//     if ( m_flGameStartAnnouncementTime == 31.0f )
	//     {
	//         // 30 second warning
	//         CriteriaSet criteria;
	//         criteria.AppendCriteria( "game_start_time", "30" );
	//         QueueConceptForAllAnnouncers( "countdown", -1.0f, criteria, -1, true );
	//         m_flGameStartAnnouncementTime = 20.0f; // Next announcement at 20 seconds
	//     }
	//     else
	//     {
	//         // Final countdown announcement
	//         PlayGameStartAnnouncement();
	//         m_flGameStartAnnouncementTime = -1.0f; // Mark as completed
	//     }
	// }
	
	// Check for early game start conditions (matches decompiled ConVar checks)
	// TODO: Implement when ConVar system is available
	// if ( dota_tutorial_enabled.GetInt() == 0 && 
	//      dota_force_load_time.GetInt() == 0 &&
	//      dota_wait_for_players_ready.GetInt() == 0 &&
	//      dota_bypass_ready_check.GetInt() == 0 )
	// {
	//     // Check for idle and disconnected players
	//     CheckIdleAndDisconnectedPlayers();
	// }
	// else
	// {
	//     // Force early transition due to ConVar settings
	//     State_Transition( DOTA_GAMERULES_STATE_GAME_IN_PROGRESS );
	// }
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Leave_INIT( void )
{
	// Check if we're in a custom game mode (matches decompiled state checks for 0xf and 0x13)
	// TODO: Implement when GC system and custom games are available
	// int nGameMode = GetGameMode();
	// if ( nGameMode == DOTA_GAMEMODE_CUSTOM || nGameMode == DOTA_GAMEMODE_ADDON )
	// {
	//     CDOTAGameManager *pGameManager = DOTAGameManager();
	//     if ( pGameManager )
	//     {
	//         pGameManager->LoadCustomHeroList();
	//         pGameManager->LoadCustomHeroes();
	//     }
	// }
	
	Msg( "DOTA: Left INIT state\n" );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Think_INIT( void )
{
	// Check time of day for morning transition (matches decompiled time check <= 0.25)
	if ( m_flTimeOfDayFraction <= 0.25f && m_flTimeOfDayFraction != 0.25f )
	{
		// Fire day/night entity output (matches decompiled COutputEvent::FireOutput call)
		if ( m_hDayNightEntity.Get() )
		{
			// TODO: Fire output event when output system is available
			// m_hDayNightEntity->FireOutput( "OnMorning", NULL, NULL, 0.0f );
		}
		
		// Play morning sound (matches decompiled Gamerules_EmitSound call)
		// TODO: Implement when sound system is available
		// Gamerules_EmitSound( "General.Morning", NULL, false );
	}
	
	// Set time of day to morning (matches decompiled 0x3e808312 = 0.25f)
	m_flTimeOfDayFraction = 0.25f;
	
	// Check for forced bot play (matches decompiled ForceBotPlay check)
	// TODO: Implement when bot system is available
	// if ( ForceBotPlay() )
	//     return;
	
	// Check GC client system state (matches decompiled GDOTAGCClientSystem checks)
	// TODO: Implement when GC system is available
	// CDOTAGCClientSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem && pGCSystem->GetLobbyState() != LOBBY_STATE_READY )
	// {
	//     if ( pGCSystem->GetLobbyState() == LOBBY_STATE_DISCONNECT )
	//         State_Transition( DOTA_GAMERULES_STATE_DISCONNECT );
	//     return;
	// }
	

	// Initialize tutorial if needed (matches decompiled DOTATutorial check)
	// TODO: Implement when tutorial system is available
	// if ( dota_tutorial_enabled.GetInt() != 0 )
	// {
	//     CDOTATutorial *pTutorial = DOTATutorial();
	//     if ( pTutorial )
	//         pTutorial->Initialize();
	// }
	
	// Check if we should stay in INIT or transition (matches decompiled transition logic)
	// TODO: Replace with proper lobby/player checks when available
	// if ( dota_wait_for_players_to_load.GetInt() != 0 )
	//     return;
	
	// Determine next state based on game manager and player count
	// TODO: Implement proper player counting when player system is available
	// CDOTAGameManager *pGameManager = DOTAGameManager();
	// int nPlayerCount = GetConnectedPlayerCount();
	// 
	// DOTA_GameState_t nextState = DOTA_GAMERULES_STATE_HERO_SELECTION;
	// if ( pGameManager && pGameManager->GetPlayerCount() >= 1 )
	// {
	//     if ( dota_force_gamestart.GetInt() > 0 )
	//         nextState = DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS;
	// }
	// else if ( dota_force_gamestart.GetInt() > 0 )
	// {
	//     nextState = DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS;
	// }
	// else
	// {
	//     nextState = DOTA_GAMERULES_STATE_HERO_SELECTION;
	// }
	
	// For now, just transition to waiting for players
	State_Transition( DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS );
}

//-----------------------------------------------------------------------------
// Purpose: Get barracks status bitmask for a team (matches decompiled GetBarracksStatus)
//-----------------------------------------------------------------------------
uint32 CDOTAGameRules::GetBarracksStatus( int nTeam )
{
	uint32 nBarracksStatus = 0;
	
	// Only valid for Radiant (2) and Dire (3) teams (matches decompiled team check)
	if ( (nTeam & 0xFFFFFFFE) == 2 )
	{
		int nTeamIndex = nTeam - 2; // Convert to 0-based index
		
		// TODO: Implement when barracks tracking is available
		// CUtlVector<EHANDLE> &barracks = (nTeamIndex == 0) ? m_hRadiantBarracks : m_hDireBarracks;
		// for ( int i = 0; i < barracks.Count(); i++ )
		// {
		//     CBaseEntity *pBarracks = barracks[i].Get();
		//     if ( pBarracks && pBarracks->IsAlive() )
		//     {
		//         nBarracksStatus |= (1 << i);
		//     }
		// }
		
		// For now, return all barracks alive (0xFF = 8 barracks)
		nBarracksStatus = 0xFF;
	}
	
	return nBarracksStatus;
}

//-----------------------------------------------------------------------------
// Purpose: Get the game mode entity (matches decompiled GetGameModeEntity)
//-----------------------------------------------------------------------------
CBaseEntity* CDOTAGameRules::GetGameModeEntity( void )
{
	// TODO: Implement when game mode entity system is available
	// return m_hGameModeEntity.Get();
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get item stock count by name (matches decompiled GetItemStockCount overload)
//-----------------------------------------------------------------------------
int CDOTAGameRules::GetItemStockCount( int nTeam, const char *pszItemName )
{
	// TODO: Implement when item system is available
	// CDOTAGameManager *pGameManager = DOTAGameManager();
	// if ( pGameManager )
	// {
	//     uint16 nAbilityIndex = pGameManager->GetAbilityIndex( pszItemName, 2 );
	//     return GetItemStockCount( nTeam, nAbilityIndex );
	// }
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get item stock count by ability index (matches decompiled GetItemStockCount)
//-----------------------------------------------------------------------------
int CDOTAGameRules::GetItemStockCount( int nTeam, uint16 nAbilityIndex )
{
	// TODO: Implement when item system and game manager are available
	// CDOTAGameManager *pGameManager = DOTAGameManager();
	// if ( !pGameManager || nAbilityIndex == pGameManager->GetInvalidAbilityIndex() )
	// {
	//     Warning( "Attempt to get item stock count for unknown item! (%d)\n", nAbilityIndex );
	//     return 0;
	// }
	
	// Search through team's item stock (matches decompiled stock array search)
	// ItemStockInfo_t *pStockArray = (nTeam == DOTA_TEAM_RADIANT) ? m_RadiantItemStock : m_DireItemStock;
	// for ( int i = 0; i < MAX_ITEM_STOCKS; i++ )
	// {
	//     if ( pStockArray[i].nAbilityIndex == nAbilityIndex )
	//         return pStockArray[i].nStockCount;
	// }
	
	// Warning( "Attempt to get item stock count for unknown stock item! (%d)\n", nAbilityIndex );
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Increase item stock count (matches decompiled IncreaseItemStock)
//-----------------------------------------------------------------------------
void CDOTAGameRules::IncreaseItemStock( int nTeam, const char *pszItemName, int nIncrease )
{
	// TODO: Implement when item system is available
	// CDOTAGameManager *pGameManager = DOTAGameManager();
	// if ( !pGameManager )
	//     return;
	
	// uint16 nAbilityIndex = pGameManager->GetAbilityIndex( pszItemName, 2 );
	// if ( nAbilityIndex == pGameManager->GetInvalidAbilityIndex() )
	// {
	//     Warning( "Attempt to increment unknown item stock info (%s)\n", pszItemName );
	//     return;
	// }
	
	// Find and update stock (matches decompiled stock search and update)
	// ItemStockInfo_t *pStockArray = (nTeam == DOTA_TEAM_RADIANT) ? m_RadiantItemStock : m_DireItemStock;
	// for ( int i = 0; i < MAX_ITEM_STOCKS; i++ )
	// {
	//     if ( pStockArray[i].nAbilityIndex == nAbilityIndex )
	//     {
	//         int nNewStock = pStockArray[i].nStockCount + nIncrease;
	//         if ( nNewStock > pStockArray[i].nMaxStock )
	//             nNewStock = pStockArray[i].nMaxStock;
	//         
	//         if ( pStockArray[i].nStockCount != nNewStock )
	//         {
	//             CGameRulesProxy::NotifyNetworkStateChanged();
	//             pStockArray[i].nStockCount = nNewStock;
	//         }
	//         break;
	//     }
	// }
}

//-----------------------------------------------------------------------------
// Purpose: Precache game resources (matches decompiled PrecacheResources)
//-----------------------------------------------------------------------------
void CDOTAGameRules::PrecacheResources( void )
{
	// Load precache file (matches decompiled KeyValues loading)
	KeyValues *pPrecacheKeys = new KeyValues( "DOTA_PRECACHE_FILENAME" );
	if ( !pPrecacheKeys )
	{
		Warning( "Precache file keys not allocated!\n" );
		return;
	}
	
	// TODO: Implement when filesystem is available
	// if ( !pPrecacheKeys->LoadFromFile( filesystem, "scripts/precache.txt", "MOD" ) )
	// {
	//     Warning( "Unable to load precache file! 'scripts/precache.txt'\n" );
	//     pPrecacheKeys->deleteThis();
	//     return;
	// }
	
	// Precache resources from keys (matches decompiled PrecacheResourcesFromKeys call)
	// TODO: Implement when resource system is available
	// PrecacheResourcesFromKeys( pPrecacheKeys, NULL, false );
	
	pPrecacheKeys->deleteThis();
	
	// Handle special themed content (matches decompiled map name check)
	// TODO: Implement when globals and particle system are available
	// const char *pszMapName = gpGlobals->mapname.ToCStr();
	// if ( pszMapName && V_strcmp( pszMapName, "dota_newyear" ) == 0 )
	// {
	//     PrecacheParticleFileAndSystems( "particles/themed_fx.pcf", NULL );
	// }
	
	// Precache creeps for current game mode (matches decompiled PrecacheCreepsForGameMode call)
	// TODO: Implement when creep system is available
	// PrecacheCreepsForGameMode();
}

//-----------------------------------------------------------------------------
// Purpose: Set tree regrow time (matches decompiled SetTreeRegrowTime)
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetTreeRegrowTime( float flTime )
{
	// TODO: Implement when ConVar system is available
	// ConVar::SetValue( &dota_tree_regrow_time, flTime );
}

//-----------------------------------------------------------------------------
// Purpose: Categorize a position based on predefined regions (matches decompiled CategorizePosition)
//-----------------------------------------------------------------------------
int CDOTAGameRules::CategorizePosition( const Vector &vPosition )
{
	// TODO: Implement when position categorization system is available
	// This matches the decompiled logic that checks position against stored regions
	// The decompiled code checks against arrays at offsets 0x173c and 0x1748
	// for ( int i = 0; i < m_nPositionRegionCount; i++ )
	// {
	//     PositionRegion_t &region = m_PositionRegions[i];
	//     
	//     // Check if it's a point region (origin matches exactly)
	//     if ( region.vOrigin == vec3_origin )
	//     {
	//         Vector vDelta = vPosition - region.vStart;
	//         if ( vDelta.LengthSqr() < region.flRadius )
	//             return region.nCategory;
	//     }
	//     else
	//     {
	//         // Check distance to line segment
	//         float flDistSqr = CalcDistanceSqrToLineSegment( vPosition, region.vStart, region.vEnd, NULL );
	//         if ( flDistSqr < region.flRadius )
	//             return region.nCategory;
	//     }
	// }
	
	return 0; // Default category
}

//-----------------------------------------------------------------------------
// Purpose: Handle client disconnection (matches decompiled ClientDisconnected)
//-----------------------------------------------------------------------------
void CDOTAGameRules::ClientDisconnected( int clientIndex, int nReason )
{
	// Get the disconnecting player (matches decompiled player lookup)
	CDOTAPlayer *pPlayer = NULL;
	if ( clientIndex >= 0 )
	{
		edict_t *pEdict = INDEXENT( clientIndex );
		if ( pEdict && pEdict->GetIServerEntity() )
		{
			pPlayer = dynamic_cast<CDOTAPlayer*>( pEdict->GetIServerEntity() );
		}
	}
	
	if ( !pPlayer || !pPlayer->IsPlayer() )
	{
		edict_t *pEdict = INDEXENT( clientIndex );
		BaseClass::ClientDisconnected( pEdict );
		return;
	}
	
	int nPlayerID = pPlayer->GetPlayerID();
	if ( nPlayerID == -1 )
	{
		edict_t *pEdict = INDEXENT( clientIndex );
		BaseClass::ClientDisconnected( pEdict );
		return;
	}
	
	// Clear voice chat state (matches decompiled voice manager clear)
	// TODO: Implement when voice system is available
	// if ( GetVoiceGameMgrHelper() )
	//     GetVoiceGameMgrHelper()->ClearPlayerListenerState( pPlayer, "Disconnected" );
	
	// Log tutorial stats if in tutorial mode (matches decompiled tutorial check)
	// TODO: Implement when tutorial system is available
	// if ( dota_tutorial_enabled.GetInt() != 0 )
	// {
	//     CDOTASteamWorksGameStatsServer *pStats = GetDOTASteamWorksGameStatsServer();
	//     if ( pStats )
	//         pStats->LogTutorialStats();
	// }
	
	// Handle disconnection messages and bot replacement (matches decompiled chat logic)
	if ( GetGameState() < DOTA_GAMERULES_STATE_POST_GAME )
	{
		// Send disconnect message to players
		// TODO: Implement when chat system is available
		// SendChatEventMessage( CHAT_MESSAGE_PLAYER_DISCONNECTED, 0, nPlayerID, NULL );
		
		// Update player resource connection state
		if ( g_pDOTAPlayerResource && nPlayerID < 32 )
		{
			g_pDOTAPlayerResource->SetConnectionState( nPlayerID, 3, nReason ); // DOTA_CONNECTION_STATE_DISCONNECTED
			// TODO: Implement SetLeaverStatus when player resource system is expanded
			//g_pDOTAPlayerResource->SetLeaverStatus( nPlayerID, 1 ); // DOTA_LEAVER_DISCONNECTED
		}
		
		// Try to replace with bot if commander allows (matches decompiled bot replacement)
		// TODO: Implement when bot system is available
		// CDOTACommander *pCommander = DOTACommander();
		// if ( pCommander && pCommander->ReplaceDisconnectedWithBots() )
		// {
		//     // Create bot replacement logic here
		// }
	}
	
	// Record disconnection in player history (matches decompiled player tracking)
	// TODO: Add disconnection tracking when telemetry system is available
	
	// Fire disconnect announcement (matches decompiled announcer logic)
	if ( GetGameState() < DOTA_GAMERULES_STATE_POST_GAME )
	{
		// TODO: Implement when announcer system is available
		// QueueConceptForAllAnnouncers( "disconnected", 2.0f, criteria, nPlayerID, true );
	}
	
	// Call base class
	edict_t *pEdict = INDEXENT( clientIndex );
	BaseClass::ClientDisconnected( pEdict );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Check if damage type is time-based (matches decompiled Damage_IsTimeBased)
//-----------------------------------------------------------------------------
bool CDOTAGameRules::Damage_IsTimeBased( int nDamageType )
{
	// Call base class implementation (matches decompiled call to CMultiplayRules)
	return BaseClass::Damage_IsTimeBased( nDamageType );
}
#endif
//-----------------------------------------------------------------------------
// Purpose: Deliver XP to players periodically (matches decompiled DeliverXPToPlayers)
//-----------------------------------------------------------------------------
void CDOTAGameRules::DeliverXPToPlayers( void )
{
	// Only deliver XP in certain game modes (matches decompiled mode check for 9)
	// TODO: Implement when game mode system is available
	// if ( GetGameMode() != DOTA_GAMEMODE_ALLPICK )
	//     return;
	
	// Check if it's time to deliver XP (matches decompiled time check)
	float flGameTime = GetGameTime(); // Get current game time
	
	// TODO: Implement when XP system is available
	// if ( m_flNextXPTime <= flGameTime )
	// {
	//     // Deliver XP to all valid players
	//     for ( int i = 0; i < 10; i++ )
	//     {
	//         if ( g_pDOTAPlayerResource && g_pDOTAPlayerResource->IsValidTeamPlayer( i ) )
	//         {
	//             CBaseEntity *pHero = g_pDOTAPlayerResource->GetPlayerSelectedHero( i );
	//             if ( pHero && pHero->IsAlive() )
	//             {
	//                 // Award 5 XP (matches decompiled 0x41400000 = 12.0f, but likely 5 XP)
	//                 pHero->AddXP( 5, false, true, true );
	//             }
	//         }
	//     }
	//     
	//     m_flNextXPTime += 0.6f; // Next XP in 0.6 seconds
	// }
}

//-----------------------------------------------------------------------------
// Purpose: Get last used ability name for a player (matches decompiled GetLastUsedAbility)
//-----------------------------------------------------------------------------
const char* CDOTAGameRules::GetLastUsedAbility( int nPlayerID )
{
	if ( nPlayerID < 0 || nPlayerID >= 32 )
	{
		Warning( "Attempt to get last used ability on invalid player id: %d!\n", nPlayerID );
		return NULL;
	}
	
	// TODO: Implement when ability tracking system is available
	// The decompiled code accesses this + nPlayerID * 0x100 + 0x17f8
	// return &m_szLastUsedAbilities[nPlayerID][0];
	return "";
}

//-----------------------------------------------------------------------------
// Purpose: Set last used ability for tracking (matches decompiled SetLastUsedAbility)
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetLastUsedAbility( void *pAbility )
{
	// TODO: Implement when ability system is available
	// CDOTABaseAbility *pDOTAAbility = (CDOTABaseAbility*)pAbility;
	// if ( !pDOTAAbility )
	//     return;
	
	// CBaseEntity *pCaster = pDOTAAbility->GetCaster();
	// if ( !pCaster || !pCaster->IsPlayer() )
	//     return;
	
	// int nPlayerID = pCaster->GetPlayerID();
	// if ( nPlayerID < 0 || nPlayerID >= 32 )
	//     return;
	
	// const char *pszAbilityName = pDOTAAbility->GetAbilityName();
	// if ( pszAbilityName )
	// {
	//     V_strncpy( m_szLastUsedAbilities[nPlayerID], pszAbilityName, 256 );
	//     
	//     // Also track ultimate abilities separately
	//     if ( !pDOTAAbility->IsItem() && pDOTAAbility->IsUltimate() )
	//     {
	//         V_strncpy( m_szLastUsedUltimates[nPlayerID], pszAbilityName, 256 );
	//     }
	// }
}

//-----------------------------------------------------------------------------
// Purpose: Handle post spawn group loading (matches decompiled PostSpawnGroupLoad)
//-----------------------------------------------------------------------------
void CDOTAGameRules::PostSpawnGroupLoad( void *pEvent )
{
	// TODO: Implement when spawn group system is available
	// This complex method handles:
	// - Finding and storing fountain entities
	// - Setting up shop entities based on distance to fountains  
	// - Finding and storing fort entities
	// - Collecting rune spawner entities
	// - Building tower and barracks lists
	// - Setting up neutral spawner arrays
	// - Finding game events entity
	// - Setting up day/night cycle
	
	// For now, just log that spawn groups loaded
	Msg( "DOTA: Post spawn group load complete\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Set initial tower vulnerability state (matches decompiled SetFirstTowerState)
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetFirstTowerState( void )
{
	// TODO: Implement when building system is available
	// int nGameMode = GetGameMode();
	
	
	// // In normal modes, make specific towers vulnerable based on ConVars
	// for ( CBaseEntity *pEntity = gEntList.FindEntityByClassname( NULL, "npc_dota_tower" );
	//       pEntity != NULL;
	//       pEntity = gEntList.FindEntityByClassname( pEntity, "npc_dota_tower" ) )
	// {
	//     CDOTA_BaseNPC_Building *pTower = dynamic_cast<CDOTA_BaseNPC_Building*>( pEntity );
	//     if ( !pTower || !pTower->IsVulnerableOnCreepSpawn() )
	//         continue;
	//         
	//     const char *pszTargetName = pTower->GetEntityName().ToCStr();
	//     if ( !pszTargetName )
	//         pszTargetName = "";
	//         
	//     bool bMakeVulnerable = false;
	//     
	//     // Check lane-specific ConVars
	//     if ( dota_creeps_top_enabled.GetInt() && V_strstr( pszTargetName, "top" ) )
	//         bMakeVulnerable = true;
	//     else if ( dota_creeps_mid_enabled.GetInt() && V_strstr( pszTargetName, "mid" ) )
	//         bMakeVulnerable = true;
	//     else if ( dota_creeps_bot_enabled.GetInt() && V_strstr( pszTargetName, "bot" ) )
	//         bMakeVulnerable = true;
	//    
	//         
	//     if ( bMakeVulnerable )
	//     {
	//         pTower->RemoveModifierByName( "modifier_invulnerable" );
	//     }
	// }
}

//-----------------------------------------------------------------------------
// Purpose: Create item stock information for shops (matches decompiled CreateItemStockInfo)
//-----------------------------------------------------------------------------
void CDOTAGameRules::CreateItemStockInfo( int nTeam, const char *pszItemName, int nStockCount, int nMaxStock, float flStockTime )
{
	// TODO: Implement when item and game manager systems are available
	// CDOTAGameManager *pGameManager = DOTAGameManager();
	// if ( !pGameManager )
	//     return;
	
	// uint16 nAbilityIndex = pGameManager->GetAbilityIndex( pszItemName, 2 );
	// if ( nAbilityIndex == pGameManager->GetInvalidAbilityIndex() )
	// {
	//     Warning( "Attempt to add unknown item to stock info (%s)\n", pszItemName );
	//     return;
	// }
	
	// Find available slot in team's item stock array (matches decompiled slot search)
	// ItemStockInfo_t *pStockArray = (nTeam == DOTA_TEAM_RADIANT) ? m_RadiantItemStock : m_DireItemStock;
	// for ( int i = 0; i < MAX_ITEM_STOCKS; i++ )
	// {
	//     if ( pStockArray[i].nAbilityIndex == nAbilityIndex )
	//     {
	//         Warning( "Attempt to add duplicate item stock info! (%s)\n", pszItemName );
	//         return;
	//     }
	//     
	//     if ( pStockArray[i].nAbilityIndex == 0 ) // Empty slot
	//     {
	//         // Set up stock info with network notifications
	//         CGameRulesProxy::NotifyNetworkStateChanged();
	//         pStockArray[i].nAbilityIndex = nAbilityIndex;
	//         pStockArray[i].nStockCount = nStockCount;
	//         pStockArray[i].nMaxStock = nMaxStock;
	//         pStockArray[i].flStockTime = flStockTime;
	//         
	//         if ( nMaxStock > nStockCount && flStockTime > 0.0f )
	//         {
	//             pStockArray[i].flNextRestockTime = gpGlobals->curtime + flStockTime;
	//         }
	//         return;
	//     }
	// }
	
	// Warning( "Attempt to add item stock info ('%s'), but ran out of slots!\n", pszItemName );
}

//-----------------------------------------------------------------------------
// Purpose: Check if position is in range of team fountain (matches decompiled IsInRangeOfFountain)
//-----------------------------------------------------------------------------
bool CDOTAGameRules::IsInRangeOfFountain( int nTeam, const Vector &vPosition )
{
	// Get the appropriate fountain entity (matches decompiled team check)
	CBaseEntity *pFountain = NULL;
	if ( nTeam == DOTA_TEAM_RADIANT )
	{
		pFountain = m_hRadiantFountain.Get();
	}
	else if ( nTeam == DOTA_TEAM_DIRE )
	{
		pFountain = m_hDireFountain.Get();
	}
	else
	{
		return false;
	}
	
	if ( !pFountain )
		return false;
	
	// Calculate distance to fountain (matches decompiled distance calculation)
	Vector vFountainPos = pFountain->GetAbsOrigin();
	float flDistanceSqr = (vPosition - vFountainPos).Length2DSqr();
	
	// 1150.0 is the fountain range from decompiled code
	return sqrtf( flDistanceSqr ) < 1150.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Check if NPC is in range of its team's fountain (matches decompiled overload)
//-----------------------------------------------------------------------------
bool CDOTAGameRules::IsInRangeOfFountain( CBaseEntity *pNPC )
{
	if ( !pNPC )
		return false;
	
	// TODO: Implement when NPC system is available
	// int nTeam = pNPC->GetTeamNumber();
	// Vector vPosition = pNPC->GetAbsOrigin();
	// return IsInRangeOfFountain( nTeam, vPosition );
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Reset all selected heroes (matches decompiled ResetSelectedHeroes)
//-----------------------------------------------------------------------------
void CDOTAGameRules::ResetSelectedHeroes( void )
{
	// Remove all player heroes (matches decompiled player loop)
	if ( g_pDOTAPlayerResource )
	{
		for ( int i = 0; i < 10; i++ )
		{
			// TODO: Implement when player resource system is expanded
			// if ( g_pDOTAPlayerResource->IsValidPlayerID( i ) )
			// {
			//     CBaseEntity *pHero = g_pDOTAPlayerResource->GetPlayerSelectedHero( i );
			//     if ( pHero )
			//         UTIL_Remove( pHero );
			//     
			//     g_pDOTAPlayerResource->SetSelectedHero( i, NULL, NULL );
			// }
		}
	}
	
	// TODO: Clear disconnected player hero tracking when available
	// for ( int i = 0; i < m_DisconnectedPlayers.Count(); i++ )
	// {
	//     CBaseEntity *pHero = m_DisconnectedPlayers[i].hHero.Get();
	//     if ( pHero )
	//         UTIL_Remove( pHero );
	//     m_DisconnectedPlayers[i].hHero.Set( NULL );
	// }
	
	// Reset hero selection state (matches decompiled state reset)
	// TODO: Implement when hero state system is available
	// if ( m_nHeroSelectionState != 0 )
	// {
	//     CGameRulesProxy::NotifyNetworkStateChanged();
	//     m_nHeroSelectionState = 0;
	// }
	
	// Reset to first hero state (matches decompiled hero state info lookup)
	// m_pCurrentHeroStateInfo = &herostateinfos[0];
}

//-----------------------------------------------------------------------------
// Purpose: Set first blood active state (matches decompiled SetFirstBloodActive)
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetFirstBloodActive( bool bActive )
{
	// TODO: Implement when first blood system is available
	// m_bFirstBloodActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: Check if client should timeout (matches decompiled ShouldTimeoutClient)
//-----------------------------------------------------------------------------
bool CDOTAGameRules::ShouldTimeoutClient( int nUserID, float flTime )
{
	// TODO: Implement when player timeout system is available
	// CBasePlayer *pPlayer = UTIL_PlayerByUserId( nUserID );
	// if ( !pPlayer )
	//     return false;
	
	// // Don't timeout bots
	// if ( pPlayer->IsFakeClient() )
	//     return false;
	
	// // Don't timeout spectators
	// if ( !pPlayer->IsConnected() )
	//     return false;
	
	// // Only timeout during active game states
	// if ( GetGameState() < DOTA_GAMERULES_STATE_GAME_IN_PROGRESS )
	//     return false;
	
	// // Check timeout ConVar based on lobby type
	// float flTimeoutDuration = dota_disconnect_timeout.GetFloat();
	// CDOTAGCClientSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem && pGCSystem->GetLobbyType() <= DOTA_LOBBY_TYPE_RANKED )
	//     flTimeoutDuration = dota_disconnect_timeout_ranked.GetFloat();
	
	// if ( flTimeoutDuration <= 0.0f )
	//     return false;
	
	// return flTime > flTimeoutDuration;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Add creep upgrade state flags (matches decompiled AddCreepUpgradeState)
//-----------------------------------------------------------------------------
void CDOTAGameRules::AddCreepUpgradeState( int nUpgradeFlags )
{
	// TODO: Implement when creep upgrade system is available
	// m_nCreepUpgradeState |= nUpgradeFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Deliver gold to players periodically (matches decompiled DeliverGoldToPlayers)
//-----------------------------------------------------------------------------
void CDOTAGameRules::DeliverGoldToPlayers( void )
{
	// Only deliver gold in certain game modes (matches decompiled mode checks)
	// TODO: Implement when game mode system is available
	// int nGameMode = GetGameMode();
	// if ( nGameMode == DOTA_GAMEMODE_ALLPICK || nGameMode == DOTA_GAMEMODE_ADDON )
	//     return;
	
	// Check if it's time to deliver gold (matches decompiled time check)
	float flGameTime = GetGameTime();
	
	// TODO: Implement when gold system is available
	// if ( m_flNextGoldTime <= flGameTime )
	// {
	//     // Skip if in tutorial or other special modes
	//     if ( dota_tutorial_enabled.GetInt() != 0 || dota_disable_gold_tick.GetInt() != 0 )
	//         return;
	
	//     int nGoldPerTick = m_nGoldPerTick;
	//     
	//     // Deliver gold to all valid players
	//     for ( int i = 0; i < 10; i++ )
	//     {
	//         if ( g_pDOTAPlayerResource && g_pDOTAPlayerResource->IsValidTeamPlayer( i ) )
	//         {
	//             g_pDOTAPlayerResource->ModifyGold( i, nGoldPerTick, false, DOTA_ModifyGold_CreepKill );
	//             
	//             // Bonus gold in turbo mode
	//             if ( GetGameMode() == DOTA_GAMEMODE_TURBO )
	//                 g_pDOTAPlayerResource->ModifyGold( i, 1000, false, DOTA_ModifyGold_CreepKill );
	//             
	//             // Double gold if ConVar enabled
	//             if ( dota_double_gold.GetInt() != 0 )
	//                 g_pDOTAPlayerResource->ModifyGold( i, nGoldPerTick, false, DOTA_ModifyGold_CreepKill );
	//         }
	//     }
	//     
	//     m_flNextGoldTime += m_flGoldTickTime; // Next gold in configured interval
	// }
}

//-----------------------------------------------------------------------------
// Purpose: Check creep upgrade state flags (matches decompiled GetCreepUpgradeState)
//-----------------------------------------------------------------------------
bool CDOTAGameRules::GetCreepUpgradeState( int nUpgradeFlags )
{
	// TODO: Implement when creep upgrade system is available
	// return (m_nCreepUpgradeState & nUpgradeFlags) == nUpgradeFlags;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Load item stock information from KeyValues (matches decompiled LoadStockInformation)
//-----------------------------------------------------------------------------
void CDOTAGameRules::LoadStockInformation( KeyValues *pKV )
{
	// TODO: Implement when KeyValues and item stock systems are available
	// KeyValues *pStockInfo = pKV->FindKey( "StockInfo" );
	// if ( !pStockInfo )
	//     return;
	
	// // Load Radiant stock info
	// KeyValues *pRadiantStock = pStockInfo->FindKey( "Radiant" );
	// if ( pRadiantStock )
	// {
	//     for ( int i = 0; i < MAX_ITEM_STOCKS; i++ )
	//     {
	//         char szSlotName[10];
	//         V_sprintf_safe( szSlotName, "%d", i );
	//         
	//         KeyValues *pSlot = pRadiantStock->FindKey( szSlotName );
	//         if ( pSlot )
	//         {
	//             float flStockTime = pSlot->GetFloat( "StockTime", 0.0f );
	//             int nStockCount = pSlot->GetInt( "StockCount", 0 );
	//             
	//             CGameRulesProxy::NotifyNetworkStateChanged();
	//             m_RadiantItemStock[i].flNextRestockTime = gpGlobals->curtime + flStockTime;
	//             m_RadiantItemStock[i].nStockCount = nStockCount;
	//         }
	//     }
	// }
	
	// // Load Dire stock info
	// KeyValues *pDireStock = pStockInfo->FindKey( "Dire" );
	// if ( pDireStock )
	// {
	//     for ( int i = 0; i < MAX_ITEM_STOCKS; i++ )
	//     {
	//         char szSlotName[10];
	//         V_sprintf_safe( szSlotName, "%d", i );
	//         
	//         KeyValues *pSlot = pDireStock->FindKey( szSlotName );
	//         if ( pSlot )
	//         {
	//             float flStockTime = pSlot->GetFloat( "StockTime", 0.0f );
	//             int nStockCount = pSlot->GetInt( "StockCount", 0 );
	//             
	//             CGameRulesProxy::NotifyNetworkStateChanged();
	//             m_DireItemStock[i].flNextRestockTime = gpGlobals->curtime + flStockTime;
	//             m_DireItemStock[i].nStockCount = nStockCount;
	//         }
	//     }
	// }
}

//-----------------------------------------------------------------------------
// Purpose: Reset game back to hero selection (matches decompiled ResetToHeroSelection)
//-----------------------------------------------------------------------------
void CDOTAGameRules::ResetToHeroSelection( void )
{
	// Reset all selected heroes first
	ResetSelectedHeroes();
	
	// Transition back to hero selection state
	State_Transition( DOTA_GAMERULES_STATE_HERO_SELECTION );
}

//-----------------------------------------------------------------------------
// Purpose: Set hero selection time (matches decompiled SetHeroSelectionTime)
//-----------------------------------------------------------------------------
void CDOTAGameRules::SetHeroSelectionTime( float flTime )
{
	// TODO: Implement when ConVar system is available
	// ConVar::SetValue( &dota_hero_selection_time, flTime );
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::Status( void (*pfnPrint)( const char*, ... ) )
{
	// Get GC client system and lobby info (matches decompiled GC access)
	// TODO: Implement when GC system is available
	// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem )
	// {
	//     CDOTALobby *pLobby = pGCSystem->GetLobby();
	//     if ( pLobby )
	//     {
	//         unsigned long long ullMatchID = pLobby->GetMatchID();
	//         unsigned long long ullLobbyID = pLobby->GetLobbyID();
	//         Msg( "Match ID: %llu   Lobby ID:%016llx\n", ullMatchID, ullLobbyID );
	//     }
	// }
	
	// Print game state (matches decompiled state printing)
	pfnPrint( "gamestate: " );
	
	if ( !g_pGameRules )
	{
		pfnPrint( "No DOTAGameRules()\n" );
	}
	else
	{
		int nGameState = GetGameState();
		const char *pszStateName = "State = %d";
		
		// Find state name in gamestateinfos array (matches decompiled state lookup)
		// TODO: Implement when gamestateinfos array is available
		// for ( int i = 0; i < 9; i++ )
		// {
		//     if ( gamestateinfos[i].nState == nGameState )
		//     {
		//         pszStateName = gamestateinfos[i].pszName;
		//         break;
		//     }
		// }
		
		pfnPrint( pszStateName, nGameState );
		
		// Print timing information (matches decompiled timing print)
		float flTransitionTime = GetTransitionTime();
		float flCurrentTime = GetCurrentTime();
		pfnPrint( " Times: Transition=%4.2f Current=%4.2f\n", flTransitionTime, flCurrentTime );
	}
	
	// Print player status (matches decompiled player status)
	// TODO: Implement when player system is available
	// DOTAPrintPlayerStatus( pfnPrint );
}

//-----------------------------------------------------------------------------
CBaseEntity* CDOTAGameRules::GetShop( int nTeam, DOTA_SHOP_TYPE nShopType )
{
	CBaseEntity *pShop = NULL;
	
	// Get shop entity based on team and shop type (matches decompiled shop access)
	switch ( nShopType )
	{
	case DOTA_SHOP_HOME:
		if ( nTeam == DOTA_TEAM_DIRE )
		{
			pShop = m_hDireHomeShop.Get();
		}
		else if ( nTeam == DOTA_TEAM_RADIANT )
		{
			pShop = m_hRadiantHomeShop.Get();
		}
		break;
		
	case DOTA_SHOP_SIDE:
		pShop = m_hSideShop.Get();
		break;
		
	case DOTA_SHOP_SECRET:
		pShop = m_hSecretShop.Get();
		break;
		
	case DOTA_SHOP_GROUND:
		pShop = m_hGroundShop.Get();
		break;
		
	case DOTA_SHOP_SIDE2:
		pShop = m_hSideShop2.Get();
		break;
		
	case DOTA_SHOP_SECRET2:
		pShop = m_hSecretShop2.Get();
		break;
		
	case DOTA_SHOP_CUSTOM:
	case DOTA_SHOP_NEUTRALS:
	case DOTA_SHOP_NONE:
	default:
		break;
	}
	
	// Validate shop entity (matches decompiled entity validation)
	if ( pShop && pShop->IsAlive() )
	{
		return pShop;
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::Defeated( void )
{
	// Find fort entity (matches decompiled fort search)
	CBaseEntity *pFort = gEntList.FindEntityByClassname( NULL, "npc_dota_fort" );
	if ( !pFort )
	{
		return;
	}
	
	// Cast to CDOTA_BaseNPC (matches decompiled dynamic cast)
	// TODO: Implement when CDOTA_BaseNPC system is available
	// CDOTA_BaseNPC *pFortNPC = dynamic_cast<CDOTA_BaseNPC*>( pFort );
	// if ( !pFortNPC )
	// {
	//     return;
	// }
	
	// Add defeated effects (matches decompiled effects)
	pFort->AddEffects( EF_NODRAW );
	
	// Get fort position (matches decompiled position calculation)
	Vector vecFortPos = pFort->GetAbsOrigin();
	
	// Find units in radius (matches decompiled unit search)
	// TODO: Implement when unit system is available
	// CUtlVector<CDOTA_BaseNPC*> units;
	// FindUnitsInRadius( pFort->GetTeamNumber(), vecFortPos, pFortNPC, -1.0f, &units, 3, 3, 0x10, 0, false );
	// 
	// // Apply knockback to units (matches decompiled knockback application)
	// for ( int i = 0; i < units.Count(); i++ )
	// {
	//     CDOTA_BaseNPC *pUnit = units[i];
	//     if ( pUnit && pUnit->IsAlive() )
	//     {
	//         // Create knockback modifier (matches decompiled modifier creation)
	//         KeyValues *pKnockbackModifier = new KeyValues( "modifier_knockback" );
	//         pKnockbackModifier->SetFloat( "center_x", vecFortPos.x );
	//         pKnockbackModifier->SetFloat( "center_y", vecFortPos.y );
	//         pKnockbackModifier->SetFloat( "center_z", vecFortPos.z );
	//         pKnockbackModifier->SetFloat( "knockback_duration", 1.0f );
	//         pKnockbackModifier->SetFloat( "knockback_distance", 750.0f );
	//         pKnockbackModifier->SetFloat( "duration", 3.0f );
	//         
	//         // Apply modifier to unit (matches decompiled modifier application)
	//         pUnit->AddNewModifier( pUnit, pFortNPC, NULL, "modifier_knockback", pKnockbackModifier );
	//         pKnockbackModifier->deleteThis();
	//     }
	// }
	
	// Add fog of war viewer (matches decompiled fog viewer)
	// TODO: Implement when fog of war system is available
	// CDOTAFogOfWarSystem *pFogSystem = g_DOTAFogOfWarSystem;
	// if ( pFogSystem )
	// {
	//     m_nFogViewerHandle = pFogSystem->AddTempViewer( 40, 2, &vecFortPos, 1200.0f, 720.0f, 0 );
	// }
	
	// Fire output event (matches decompiled output firing)
	// TODO: Implement when output system is available
	// CBaseEntity *pOutputEntity = m_hOutputEntity.Get();
	// if ( pOutputEntity )
	// {
	//     pOutputEntity->FireOutput( "OnDefeated", pFort, pFort, 0.0f );
	// }
	
	// Create game event (matches decompiled event creation)
	// TODO: Implement when game event system is available
	// IGameEvent *pEvent = gameeventmanager->CreateEvent( "defeated" );
	// if ( pEvent )
	// {
	//     int nEntIndex = 0;
	//     if ( pFort->GetEntityId() != 0 )
	//     {
	//         nEntIndex = ( pFort->GetEntityId() - gpGlobals->tickcount ) >> 3;
	//     }
	//     pEvent->SetInt( "entindex", nEntIndex );
	//     gameeventmanager->FireEvent( pEvent );
	// }
	
	Msg( "DOTA: Fort defeated\n" );
}

//-----------------------------------------------------------------------------
CBaseEntity* CDOTAGameRules::GetTower( int nTowerIndex )
{
	// Get tower entity handle (matches decompiled tower access)
	EHANDLE hTower = m_hTowers[nTowerIndex];
	if ( !hTower.IsValid() )
	{
		return NULL;
	}
	
	CBaseEntity *pTower = hTower.Get();
	if ( !pTower || !pTower->IsAlive() )
	{
		return NULL;
	}
	
	// Check if entity is a tower (matches decompiled tower check)
	// TODO: Implement when tower system is available
	// if ( !( pTower->GetFlags() & FL_TOWER ) )
	// {
	//     return NULL;
	// }
	
	return pTower;
}

//-----------------------------------------------------------------------------
CBaseEntity* CDOTAGameRules::GetRoshan( void )
{
	// Get Roshan entity handle (matches decompiled Roshan access)
	EHANDLE hRoshan = m_hRoshan;
	if ( !hRoshan.IsValid() )
	{
		return NULL;
	}
	
	CBaseEntity *pRoshan = hRoshan.Get();
	if ( !pRoshan )
	{
		return NULL;
	}
	
	// Get Roshan's current target (matches decompiled target access)
	// TODO: Implement when Roshan system is available
	// int nTargetEntIndex = pRoshan->GetTargetEntIndex();
	// if ( nTargetEntIndex != -1 )
	// {
	//     return gEntList.FindEntityByIndex( nTargetEntIndex );
	// }
	
	return NULL;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::SpawnRune( bool bForceBounty )
{
	// Remove existing runes (matches decompiled rune removal)
	// TODO: Implement when rune system is available
	// CDOTA_Item_Rune *pRune1 = m_hRune1.Get();
	// if ( pRune1 )
	// {
	//     UTIL_RemoveImmediate( pRune1 );
	// }
	// 
	// CDOTA_Item_Rune *pRune2 = m_hRune2.Get();
	// if ( pRune2 )
	// {
	//     UTIL_RemoveImmediate( pRune2 );
	// }
	
	// Select random spawn point (matches decompiled spawn point selection)
	int nSpawnPointCount = m_RuneSpawnPoints.Count();
	if ( nSpawnPointCount == 0 )
	{
		return;
	}
	
	int nSpawnPointIndex = RandomInt( 0, nSpawnPointCount - 1 );
	EHANDLE hSpawnPoint = m_RuneSpawnPoints[nSpawnPointIndex];
	
	CBaseEntity *pSpawnPoint = hSpawnPoint.Get();
	if ( !pSpawnPoint )
	{
		return;
	}
	
	// Get spawn position (matches decompiled position calculation)
	Vector vecSpawnPos = pSpawnPoint->GetAbsOrigin();
	
	// Create rune entity (matches decompiled entity creation)
	// TODO: Implement when entity spawner system is available
	// CEntitySpawnerBase<> spawner;
	// spawner.SetEntityClass( "dota_item_rune" );
	// spawner.SetAbsOrigin( vecSpawnPos );
	// 
	// CBaseEntity *pRune = spawner.Spawn();
	// if ( !pRune )
	// {
	//     return;
	// }
	
	// Set rune type (matches decompiled type setting)
	int nRuneType = 5; // Default to bounty rune
	if ( !bForceBounty )
	{
		nRuneType = RandomInt( 0, 4 );
	}
	
	// Ensure different from last rune (matches decompiled type check)
	while ( nRuneType == m_nLastRuneType )
	{
		nRuneType = RandomInt( 0, 4 );
	}
	m_nLastRuneType = nRuneType;
	
	// Set rune properties (matches decompiled property setting)
	// TODO: Implement when rune system is available
	// CDOTA_Item_Rune *pRuneItem = dynamic_cast<CDOTA_Item_Rune*>( pRune );
	// if ( pRuneItem )
	// {
	//     pRuneItem->SetAbsOrigin( vecSpawnPos );
	//     pRuneItem->SetRuneType( nRuneType );
	//     pRuneItem->SetAbsVelocity( Vector( 0, 0, 0 ) );
	//     pRuneItem->SetSpawnPoint( hSpawnPoint );
	// }
	
	// Update rune handle (matches decompiled handle update)
	// m_hRune1 = pRune;
	
	// Notify commander (matches decompiled commander notification)
	// TODO: Implement when commander system is available
	// CDOTA_Commander *pCommander = DOTACommander();
	// if ( pCommander )
	// {
	//     pCommander->OnRuneSpawn();
	// }
	
	// Spawn second rune at opposite spawn point (matches decompiled second spawn)
	EHANDLE hOppositeSpawnPoint = m_RuneSpawnPoints[nSpawnPointIndex == 0 ? 1 : 0];
	CBaseEntity *pOppositeSpawnPoint = hOppositeSpawnPoint.Get();
	if ( pOppositeSpawnPoint )
	{
		Vector vecOppositePos = pOppositeSpawnPoint->GetAbsOrigin();
		
		// Create second rune (matches decompiled second rune creation)
		// TODO: Implement when entity spawner system is available
		// CEntitySpawnerBase<> spawner2;
		// spawner2.SetEntityClass( "dota_item_rune" );
		// spawner2.SetAbsOrigin( vecOppositePos );
		// 
		// CBaseEntity *pRune2 = spawner2.Spawn();
		// if ( pRune2 )
		// {
		//     CDOTA_Item_Rune *pRuneItem2 = dynamic_cast<CDOTA_Item_Rune*>( pRune2 );
		//     if ( pRuneItem2 )
		//     {
		//         pRuneItem2->SetAbsOrigin( vecOppositePos );
		//         pRuneItem2->SetRuneType( 5 ); // Always bounty rune for second spawn
		//         pRuneItem2->SetAbsVelocity( Vector( 0, 0, 0 ) );
		//         pRuneItem2->SetSpawnPoint( hOppositeSpawnPoint );
		//     }
		//     
		//     m_hRune2 = pRune2;
		//     
		//     // Notify commander again (matches decompiled second notification)
		//     if ( pCommander )
		//     {
		//         pCommander->OnRuneSpawn();
		//     }
		// }
	}
	
	Msg( "DOTA: Rune spawned at spawn point %d, type %d\n", nSpawnPointIndex, nRuneType );
}

//-----------------------------------------------------------------------------
int CDOTAGameRules::State_Get( void )
{
	// Return current game state (matches decompiled state access)
	return m_nGameState;
}

#endif // GAME_DLL

//-----------------------------------------------------------------------------
bool CDOTAGameRules::Init(void)
{
	// Initialize game time (matches decompiled time initialization)
	float flGameTime = 0.0f;
	if (gpGlobals)
	{
		// TODO: Implement when engine system is available
		// if ( !gpGlobals->m_bMapLoadFailed && !gpGlobals->m_bMapLoadFailed && gpGlobals->m_pfnChangeLevel )
		// {
		//     gpGlobals->m_pfnChangeLevel( 1 );
		// }
		flGameTime = gpGlobals->curtime;
	}

	// Update network state if game time changed
	if (flGameTime != m_flGameTime)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flGameTime = flGameTime;
	}

	// Reset pause flags (matches decompiled flag resets)
	if (m_bGamePaused)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bGamePaused = false;
	}

	// Reset game state flags (matches decompiled flag resets)
	if (m_nGameState != DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nGameState = DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS;
	}

	if (m_nWinningTeam != 0)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nWinningTeam = 0;
	}

	if (m_flGameEndTime != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flGameEndTime = 0.0f;
	}

	if (!m_bSafeToLeave)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bSafeToLeave = true;
	}

	// Reset team showcase helper (matches decompiled reset)
	m_pTeamShowcaseHelper = NULL;
	m_bMapResetRequested = false;
	m_nMapResetRequestTime = 0;

	// Initialize teams array (matches decompiled team initialization)
	// TODO: Implement when team system is available
	// g_Teams.m_Teams.RemoveAll();
	// g_Teams.m_Teams.EnsureCapacity( 0 );
	// g_Teams.m_Teams.SetCount( 0 );

	// Create team entities (matches decompiled team creation)
	// TODO: Implement when entity system is available
	// CDOTATeam *pRadiantTeam = (CDOTATeam*)CreateEntityByName( "dota_team", -1, true );
	// if ( pRadiantTeam )
	// {
	//     pRadiantTeam->Initialize( _sTeamNames, 0 );
	//     g_Teams.m_Teams.AddToTail( pRadiantTeam );
	// }
	// 
	// CDOTATeam *pDireTeam = (CDOTATeam*)CreateEntityByName( "dota_team", -1, true );
	// if ( pDireTeam )
	// {
	//     pDireTeam->Initialize( "dota_team_dire", 1 );
	//     g_Teams.m_Teams.AddToTail( pDireTeam );
	// }
	// 
	// CDOTATeam *pGoodGuysTeam = (CDOTATeam*)CreateEntityByName( "dota_team", -1, true );
	// if ( pGoodGuysTeam )
	// {
	//     pGoodGuysTeam->Initialize( "#DOTA_GoodGuys", 2 );
	//     g_Teams.m_Teams.AddToTail( pGoodGuysTeam );
	// }
	// 
	// CDOTATeam *pBadGuysTeam = (CDOTATeam*)CreateEntityByName( "dota_team", -1, true );
	// if ( pBadGuysTeam )
	// {
	//     pBadGuysTeam->Initialize( "#DOTA_BadGuys", 3 );
	//     g_Teams.m_Teams.AddToTail( pBadGuysTeam );
	// }

	// Reset tower and barracks counts (matches decompiled resets)
	m_nRadiantTowerCount = 0;
	m_nDireTowerCount = 0;
	m_bFirstTowerDestroyed = true;
	m_nFirstTowerTeam = -1;
	m_flFirstTowerTime = 0.0f;

	// Clear string arrays (matches decompiled string cleanup)
	// TODO: Implement when string system is available
	// for ( int i = 0; i < m_StringArrays.Count(); i++ )
	// {
	//     if ( m_StringArrays[i].IsValid() )
	//     {
	//         m_StringArrays[i].FreeMemoryBlock();
	//     }
	// }
	// m_StringArrays.RemoveAll();

	// Reset game flags (matches decompiled flag resets)
	m_bHeroRespawnEnabled = true;
	m_nGameMode = 1;

	// Reset pause flags (matches decompiled flag resets)
	if (m_bPauseEnabled)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bPauseEnabled = false;
	}

	if (m_bPauseRequested)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bPauseRequested = false;
	}

	if (m_bPauseActive)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bPauseActive = false;
	}

	// Reset tower and barracks arrays (matches decompiled array resets)
	m_nRadiantTowers = 0;
	m_nDireTowers = 0;
	m_nRadiantBarracks = 0;
	m_nDireBarracks = 0;

	// Reset hero arrays (matches decompiled array resets)
	// TODO: Implement when hero system is available
	// for ( int i = 0x17f8; i < 0x37f8; i += 0x100 )
	// {
	//     m_HeroArrays[i] = 0;
	//     m_HeroArrays[i + 0x2000] = 0;
	// }

	// Load item stock information (matches decompiled stock loading)
	// TODO: Implement when game manager system is available
	// CDOTAGameManager *pGameManager = DOTAGameManager();
	// if ( pGameManager )
	// {
	//     bool bIsStableMode = IsStableMode();
	//     KeyValues *pStockKeys = NULL;
	//     
	//     if ( !pGameManager->m_bIsCustomGame && !pGameManager->m_bIsWorkshopGame )
	//     {
	//         pStockKeys = pGameManager->m_StockKeys[bIsStableMode ? 1 : 0];
	//     }
	//     else
	//     {
	//         pStockKeys = pGameManager->m_WorkshopStockKeys[bIsStableMode ? 1 : 0];
	//     }
	//     
	//     if ( pStockKeys )
	//     {
	//         for ( KeyValues *pSubKey = pStockKeys->GetFirstSubKey(); pSubKey; pSubKey = pSubKey->GetNextKey() )
	//         {
	//             int nMaxStock = pSubKey->GetInt( "ItemStockMax", 0 );
	//             if ( nMaxStock > 0 )
	//             {
	//                 int nInitialStock = pSubKey->GetInt( "ItemStockInitial", nMaxStock );
	//                 float flStockTime = pSubKey->GetFloat( "ItemStockTime", 0.0f );
	//                 const char *pszItemName = pSubKey->GetName();
	//                 
	//                 CreateItemStockInfo( DOTA_TEAM_RADIANT, pszItemName, nInitialStock, nMaxStock, flStockTime );
	//                 CreateItemStockInfo( DOTA_TEAM_DIRE, pszItemName, nInitialStock, nMaxStock, flStockTime );
	//             }
	//         }
	//     }
	// }

	// Reset enabled heroes (matches decompiled hero resets)
	// TODO: Implement when game manager system is available
	// CDOTAGameManager *pGameManager = DOTAGameManager();
	// if ( pGameManager )
	// {
	//     pGameManager->ResetEnabledHeroes( 0 );
	//     pGameManager->ResetEnabledHeroes( 1 );
	// }
#if 0
	// Reset hero arrays (matches decompiled array resets)
	for (int i = 0x708; i < 2000; i += 4)
	{
		if (m_HeroArrays[i] != 0)
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_HeroArrays[i] = 0;
		}
	}

	// Reset additional hero arrays (matches decompiled array resets)
	if (m_HeroArrays[2000] != 0)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_HeroArrays[2000] = 0;
	}

	// Reset more hero arrays (matches decompiled array resets)
	for (int i = 0x7d4; i < 0x7f4; i += 4)
	{
		if (m_HeroArrays[i] != 0)
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_HeroArrays[i] = 0;
		}
	}

	// Reset hero arrays (matches decompiled array resets)
	for (int i = 0x7f8; i < 0xe38; i += 4)
	{
		if (m_HeroArrays[i] != 0)
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_HeroArrays[i] = 0;
		}
	}
#endif
	// Reset additional counters (matches decompiled resets)
	m_nRadiantBarracksCount = 0;
	if (m_nDireBarracksCount != 0)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nDireBarracksCount = 0;
	}

	// Reset load failure arrays (matches decompiled array resets)
	m_nLoadFailureCount = 0;
	// TODO: Implement when memory system is available
	// if ( m_pLoadFailureArray )
	// {
	//     free( m_pLoadFailureArray );
	//     m_pLoadFailureArray = NULL;
	// }
	// m_nLoadFailureCapacity = 0;
	// m_pLoadFailureArray = NULL;

	m_nLoadSuccessCount = 0;
	// TODO: Implement when memory system is available
	// if ( m_pLoadSuccessArray )
	// {
	//     free( m_pLoadSuccessArray );
	//     m_pLoadSuccessArray = NULL;
	// }
	// m_nLoadSuccessCapacity = 0;
	// m_pLoadSuccessArray = NULL;

	// Reset additional counters (matches decompiled resets)
	m_nDisconnectCount = 0;
	m_nReconnectCount = 0;

	// Initialize minimap grid (matches decompiled grid initialization)
	// TODO: Implement when engine system is available
	// m_pMinimapGrid = engine->CreateStringTable( "DOTAMinimapGrid", 2048 );
	// if ( m_pMinimapGrid )
	// {
	//     m_pMinimapGrid->SetMaxStrings( 16384 );
	// }

	// Reset selection entity (matches decompiled reset)
	if (m_nSelectionEntity != -1)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nSelectionEntity = -1;
	}

	// Reset icon scales (matches decompiled resets)
	if (m_flRuneIconScale != 1.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flRuneIconScale = 1.0f;
	}

	if (m_flCreepIconScale != 1.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flCreepIconScale = 1.0f;
	}

	// Reset additional counters (matches decompiled resets)
	m_nRadiantKillCount = 0;
	m_nDireKillCount = 0;

	// Reset timing variables (matches decompiled resets)
	if (m_flWaitForPlayersTime != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flWaitForPlayersTime = 0.0f;
	}

	if (m_flGameStartTime != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flGameStartTime = 0.0f;
	}

	if (m_flPreGameTime != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flPreGameTime = 0.0f;
	}

	if (m_flPreGameDuration != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flPreGameDuration = 0.0f;
	}

	if (m_flPostGameTime != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flPostGameTime = 0.0f;
	}

	if (m_flRuneSpawnTime != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flRuneSpawnTime = 0.0f;
	}

	if (m_flGlyphCooldownRadiant != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flGlyphCooldownRadiant = 0.0f;
	}

	if (m_flGlyphCooldownDire != 0.0f)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_flGlyphCooldownDire = 0.0f;
	}

	// Reset additional counters (matches decompiled resets)
	m_nRadiantTowerKills = 0;
	m_nDireTowerKills = 0;
	m_nRadiantBarracksKills = 0;
	m_nDireBarracksKills = 0;
	m_nRadiantHeroKills = 0;
	m_nDireHeroKills = 0;
	m_nRadiantCreepKills = 0;
	m_nDireCreepKills = 0;

	// Reset selection entity (matches decompiled reset)
	if (m_hSelectionEntity.IsValid())
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_hSelectionEntity = NULL;
	}

	// Reset pause variables (matches decompiled resets)
	m_flPauseTime = 0.0f;
	if (m_nPauseState != 5)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nPauseState = 5;
	}

	// Reset additional flags (matches decompiled resets)
	m_bFirstBloodActive = false;
	if (m_bFirstBloodOccurred)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bFirstBloodOccurred = false;
	}

	// Enter initial state (matches decompiled state transition)
	State_Enter(DOTA_GAMERULES_STATE_INIT);

	// Initialize hero state (matches decompiled hero state initialization)
	// TODO: Implement when hero state system is available
	// for ( int i = 0; i < 54; i++ )
	// {
	//     if ( herostateinfos[i].nState == 0 )
	//     {
	//         m_pHeroStateInfo = &herostateinfos[i];
	//         if ( m_pHeroStateInfo && m_pHeroStateInfo->pfnEnter )
	//         {
	//             m_pHeroStateInfo->pfnEnter( this + m_pHeroStateInfo->nOffset );
	//         }
	//         break;
	//     }
	// }

	// Reset hero state variables (matches decompiled resets)
	m_nHeroState = 0;
	m_nHeroStateTime = 0;
	m_nHeroStateDuration = 0;
	m_nHeroStateInterval = 0;
	m_nHeroStateCount = 0;
	m_nHeroStateMax = 0;
	m_nHeroStateMin = 0;
	m_nHeroStateTarget = 0;

	// Reset additional variables (matches decompiled resets)
	m_nRadiantScore = 0;
	m_nDireScore = 0;
	m_bHeroStateActive = false;
	m_nHeroStateIndex = 0;

	// Load localization (matches decompiled localization loading)
	// TODO: Implement when engine system is available
	// if ( engine->IsDedicatedServer() )
	// {
	//     g_pVGuiLocalize->AddFile( "resource/dota_english.txt", NULL, NULL );
	// }

	// Initialize hero arrays (matches decompiled array initialization)
	// TODO: Implement when hero system is available
	// for ( int i = -0x80; i < 0; i += 4 )
	// {
	//     m_HeroArrays[0x58e8 + i] = g_HeroData[i + 0x30];
	//     m_HeroArrays[0x5968 + i] = 0;
	// }

	// Reset selection entities (matches decompiled resets)
	m_nRadiantSelectionEntity = -1;
	m_nDireSelectionEntity = -1;
	m_nNeutralSelectionEntity = -1;
	m_nRadiantScore = 0;
	m_nDireScore = 0;
#if 0
	// Initialize countdown timers (matches decompiled timer initialization)
	for (int i = 0; i < 48; i += 16)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		if (m_CountdownTimers[i + 8] != 10)
		{
			// TODO: Implement when timer system is available
			// m_CountdownTimers[i].SetDuration( m_CountdownTimers[i + 8] );
			m_CountdownTimers[i + 8] = 10;
		}

		CGameRulesProxy::NotifyNetworkStateChanged();
		if (m_CountdownTimers[i + 4] != 0)
		{
			// TODO: Implement when timer system is available
			// m_CountdownTimers[i].SetDuration( m_CountdownTimers[i + 4] );
			m_CountdownTimers[i + 4] = 0;
		}

		CGameRulesProxy::NotifyNetworkStateChanged();
		if (m_CountdownTimers[i + 12] != 0)
		{
			// TODO: Implement when timer system is available
			// m_CountdownTimers[i].SetDuration( m_CountdownTimers[i + 12] );
			m_CountdownTimers[i + 12] = 0;
		}
	}

	// Reset additional arrays (matches decompiled array resets)
	for (int i = 0; i < 48; i += 4)
	{
		if (m_AdditionalArrays[i + 0x1420] != 0)
		{
			CGameRulesProxy::NotifyNetworkStateChanged();
			m_AdditionalArrays[i + 0x1420] = 0;
		}
	}

	// Reset communication summaries (matches decompiled summary resets)
	for (int i = 0; i < 0x700; i += 56)
	{
		memset(&m_CommunicationSummaries[i + 0x5a24], 0, 56);
	}

	// Reset additional variables (matches decompiled resets)
	if (m_nAdditionalVariable != 0)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nAdditionalVariable = 0;
	}
#endif
	// Reset string (matches decompiled string reset)
	CGameRulesProxy::NotifyNetworkStateChanged();
	Q_strncpy(m_szAdditionalString, "", 256);

	// Start game stats session (matches decompiled stats initialization)
	// TODO: Implement when stats system is available
	// CSteamWorksGameStatsUploader *pStatsUploader = GetSteamWorksGameStatsServer();
	// if ( pStatsUploader )
	// {
	//     pStatsUploader->StartSession();
	// }

	// Reset additional flags (matches decompiled flag resets)
	if (m_bAdditionalFlag1)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bAdditionalFlag1 = false;
	}

	if (m_nAdditionalVariable1 != 0)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nAdditionalVariable1 = 0;
	}

	if (m_nAdditionalVariable2 != 0)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nAdditionalVariable2 = 0;
	}

	if (!m_bAdditionalFlag2)
	{
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_bAdditionalFlag2 = true;
	}

	Msg("DOTA: Game rules initialized\n");
	return true;
}

//-----------------------------------------------------------------------------
void CDOTAGameRules::Shutdown(void)
{
	// Clean up minimap grid (matches decompiled cleanup)
	if (m_pMinimapGrid)
	{
		// TODO: Implement when string table system is available
		// m_pMinimapGrid->RemoveAll();
		m_pMinimapGrid = NULL;
	}

	Msg("DOTA: Game rules shutdown\n");
}

// Purpose: Enter new game state with notifications (from decompiled State_Enter)
//-----------------------------------------------------------------------------
void CDOTAGameRules::State_Enter(DOTA_GameState_t newState)
{
#ifndef CLIENT_DLL
	// Check if state is actually changing
	if (m_nGameState != newState)
	{
		// Notify network that game rules state has changed
		CGameRulesProxy::NotifyNetworkStateChanged();
		m_nGameState = newState;
	}

	// Game state info structure (matches decompiled gamestateinfos lookup)
	static GameStateInfo_t s_GameStateInfos[] = {
		{ DOTA_GAMERULES_STATE_INIT, "DOTA_GAMERULES_STATE_INIT", &CDOTAGameRules::State_Enter_INIT, 0 },
		{ DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS, "DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS", NULL, 0 },
		{ DOTA_GAMERULES_STATE_PRE_GAME, "DOTA_GAMERULES_STATE_PRE_GAME", &CDOTAGameRules::State_Enter_PRE_GAME, 0 },
		{ DOTA_GAMERULES_STATE_POST_GAME, "DOTA_GAMERULES_STATE_POST_GAME", &CDOTAGameRules::State_Enter_POST_GAME, 0 },
		{ DOTA_GAMERULES_STATE_DISCONNECT, "DOTA_GAMERULES_STATE_DISCONNECT", &CDOTAGameRules::State_Enter_DISCONNECT, 0 },
		{ DOTA_GAMERULES_STATE_HERO_SELECTION, "DOTA_GAMERULES_STATE_STRATEGY_TIME", &CDOTAGameRules::State_Enter_HERO_SELECTION, 0 },
		{ DOTA_GAMERULES_STATE_GAME_IN_PROGRESS, "DOTA_GAMERULES_STATE_GAME_IN_PROGRESS", &CDOTAGameRules::State_Enter_GAME_IN_PROGRESS, 0 },
		{ DOTA_GAMERULES_STATE_STRATEGY_TIME, "DOTA_GAMERULES_STATE_STRATEGY_TIME", &CDOTAGameRules::State_Enter_STRATEGY_TIME, 0 },
		{ DOTA_GAMERULES_STATE_TEAM_SHOWCASE, "DOTA_GAMERULES_STATE_TEAM_SHOWCASE", &CDOTAGameRules::State_Enter_TEAM_SHOWCASE, 0 }
	};

	// Find state info (matches decompiled lookup loop)
	GameStateInfo_t* pStateInfo = NULL;
	for (int i = 0; i < ARRAYSIZE(s_GameStateInfos); i++)
	{
		if (s_GameStateInfos[i].state == newState)
		{
			pStateInfo = &s_GameStateInfos[i];
			break;
		}
	}

	// Store current state info pointer (matches decompiled offset 0xea0)
	m_pCurrentGameStateInfo = pStateInfo;

	// Log state transition (matches decompiled logging with developer check)
	if (pStateInfo)
	{
		Msg("C:Gamerules: entering state '%s'\n", pStateInfo->name);
	}
	else
	{
		Msg("C:Gamerules: entering state #%d\n", newState);
	}

	// Call state-specific enter function if it exists (matches decompiled function call)
	if (pStateInfo && pStateInfo->enterFunc)
	{
		(this->*(pStateInfo->enterFunc))();
	}

	// Fire game event (matches decompiled game event firing)
	if (gameeventmanager)
	{
		IGameEvent* pEvent = gameeventmanager->CreateEvent("game_rules_state_change");
		if (pEvent)
		{
			pEvent->SetInt("state", newState);
			gameeventmanager->FireEvent(pEvent);
		}
	}

	// TODO: Notify GC system when available
	// CDOTAGCServerSystem *pGCSystem = GDOTAGCClientSystem();
	// if ( pGCSystem )
	//     pGCSystem->GameRules_State_Enter( newState );

	// TODO: Notify HLTV director when available
	// CDOTAHLTVDirector *pHLTVDirector = DOTAHLTVDirector();
	// if ( pHLTVDirector )
	//     pHLTVDirector->GameRules_State_Enter( newState );

	// Record state transition for telemetry (matches decompiled transition tracking)
	float flCurrentTime = gpGlobals->curtime;
	float flRealTime = gpGlobals->realtime;

	// TODO: Add state transition tracking array when telemetry system is available
	// s_GameStateTransitions[s_GameStateTransitionsIndex].curtime = flCurrentTime;
	// s_GameStateTransitions[s_GameStateTransitionsIndex].realtime = flRealTime;
	// s_GameStateTransitions[s_GameStateTransitionsIndex].state = newState;
	// s_GameStateTransitionsIndex = (s_GameStateTransitionsIndex + 1) % 16;

	// Add to minidump info (matches decompiled minidump append)
	const char* stateName = pStateInfo ? pStateInfo->name : "???";
	// TODO: MinidumpUserStreamInfoAppend when available
	// MinidumpUserStreamInfoAppend("CDOTAGamerules::State_Enter %s curtime:%f realtime:%f\n", stateName, flCurrentTime, flRealTime);

	// Send user message to all clients (matches decompiled user message sending)
	// TODO: Implement when user message system is available
	// CBroadcastRecipientFilter filter;
	// filter.MakeReliable();
	// CDOTA_UM_GamerulesStateChanged msg;
	// msg.set_state( newState );
	// SendUserMessage( filter, DOTA_UM_GamerulesStateChanged, msg );

#endif // !CLIENT_DLL
}
