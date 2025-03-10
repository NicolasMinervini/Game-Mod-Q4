#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Weapon.h"

class rvWeaponGrenadeLauncher : public rvWeapon {
public:

	CLASS_PROTOTYPE( rvWeaponGrenadeLauncher );

	rvWeaponGrenadeLauncher ( void );

	virtual void			Spawn				( void );
	void					PreSave				( void );
	void					PostSave			( void );

#ifdef _XENON
	virtual bool		AllowAutoAim			( void ) const { return false; }
#endif

private:

	stateResult_t		State_Idle		( const stateParms_t& parms );
	stateResult_t		State_Fire		( const stateParms_t& parms );
	stateResult_t		State_Reload	( const stateParms_t& parms );

	const char*			GetFireAnim() const { return (!AmmoInClip()) ? "fire_empty" : "fire"; }
	const char*			GetIdleAnim() const { return (!AmmoInClip()) ? "idle_empty" : "idle"; }
	
	CLASS_STATES_PROTOTYPE ( rvWeaponGrenadeLauncher );
};

CLASS_DECLARATION( rvWeapon, rvWeaponGrenadeLauncher )
END_CLASS

/*
================
rvWeaponGrenadeLauncher::rvWeaponGrenadeLauncher
================
*/
rvWeaponGrenadeLauncher::rvWeaponGrenadeLauncher ( void ) {
}

/*
================
rvWeaponGrenadeLauncher::Spawn
================
*/
void rvWeaponGrenadeLauncher::Spawn ( void ) {
	SetState ( "Raise", 0 );	
}

/*
================
rvWeaponGrenadeLauncher::PreSave
================
*/
void rvWeaponGrenadeLauncher::PreSave ( void ) {
}

/*
================
rvWeaponGrenadeLauncher::PostSave
================
*/
void rvWeaponGrenadeLauncher::PostSave ( void ) {
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeaponGrenadeLauncher )
	STATE ( "Idle",		rvWeaponGrenadeLauncher::State_Idle)
	STATE ( "Fire",		rvWeaponGrenadeLauncher::State_Fire )
	STATE ( "Reload",	rvWeaponGrenadeLauncher::State_Reload )
END_CLASS_STATES

/*
================
rvWeaponGrenadeLauncher::State_Idle
================
*/
stateResult_t rvWeaponGrenadeLauncher::State_Idle( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	float asb = 1.0f;
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( !AmmoAvailable ( ) ) {
				SetStatus ( WP_OUTOFAMMO );
			} else {
				SetStatus ( WP_READY );
			}
		
			PlayCycle( ANIMCHANNEL_ALL, GetIdleAnim(), parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if (!holding && wsfl.attack) {
				holding = true;
				heldTime = gameLocal.time;
				//float asb = 1.0f;
				if (gameLocal.GetLocalPlayer()) {
					asb = gameLocal.GetLocalPlayer()->inventory.attackSpeedBoost;
					tripleHoldLength = 1000 * asb;
					doubleHoldLength = 500 * asb;
					minHoldLength = 100 * asb;
				}
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}		
			if (wsfl.attack && heldTime + (int)tripleHoldLength < gameLocal.time) {
				holding = false;
				SetState("Fire", 0);
				return SRESULT_DONE;
			}
			if (holding && heldTime + (int)minHoldLength < gameLocal.time && !clipSize ) {
				if ( !wsfl.attack && AmmoAvailable ( ) ) {
					holding = false;
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}
			} else { 
				if (holding && heldTime + (int)minHoldLength < gameLocal.time && gameLocal.time > nextAttackTime && !wsfl.attack && AmmoInClip ( ) ) {
					holding = false;
					SetState ( "Fire", 0 );
					return SRESULT_DONE;
				}  
						
				if ( wsfl.attack && AutoReload() && !AmmoInClip ( ) && AmmoAvailable () ) {
					SetState ( "Reload", 4 );
					return SRESULT_DONE;			
				}
				if ( wsfl.netReload || (wsfl.reload && AmmoInClip() < ClipSize() && AmmoAvailable()>AmmoInClip()) ) {
					SetState ( "Reload", 4 );
					return SRESULT_DONE;			
				}
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponGrenadeLauncher::State_Fire
================
*/
stateResult_t rvWeaponGrenadeLauncher::State_Fire ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	float asb = 1.0f;
	switch ( parms.stage ) {
		case STAGE_INIT:
			//float asb = 1.0f;
			if (gameLocal.GetLocalPlayer()) {
				asb = gameLocal.GetLocalPlayer()->inventory.attackSpeedBoost;
				tripleHoldLength = 1000 * asb;
				doubleHoldLength = 500 * asb;
				minHoldLength = 100 * asb;
			}
			nextAttackTime = gameLocal.time + (fireRate * asb * owner->PowerUpModifier ( PMOD_FIRERATE ));
			gameLocal.Printf("time:%d next:%d triple:%f double:%f min:%f\n", gameLocal.time,nextAttackTime,tripleHoldLength,doubleHoldLength,minHoldLength);
			if (gameLocal.time - heldTime > tripleHoldLength) {
				Attack(false, 3, spread + 20, 0, 1.0f);
				gameLocal.Printf("triple! %d\n", gameLocal.time - heldTime);
			}
			else if (gameLocal.time - heldTime > doubleHoldLength) {
				Attack(false, 2, spread + 10, 0, 1.0f);
				gameLocal.Printf("double! %d\n", gameLocal.time - heldTime);
			}
			else {
				Attack(false, 1, spread, 0, 1.0f);
				gameLocal.Printf("single! %d\n", gameLocal.time - heldTime);
			}
			//Attack ( false, 1, spread, 0, 1.0f );
			PlayAnim ( ANIMCHANNEL_ALL, GetFireAnim(), 0 );	
			return SRESULT_STAGE ( STAGE_WAIT );
	
		case STAGE_WAIT:		
			if (holding && heldTime + minHoldLength < gameLocal.time && !wsfl.attack && gameLocal.time >= nextAttackTime && AmmoInClip() && !wsfl.lowerWeapon ) {
				holding = false;
				SetState ( "Fire", 0 );
				return SRESULT_DONE;
			}
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetState ( "Idle", 0 );
				return SRESULT_DONE;
			}		
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeaponGrenadeLauncher::State_Reload
================
*/
stateResult_t rvWeaponGrenadeLauncher::State_Reload ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( wsfl.netReload ) {
				wsfl.netReload = false;
			} else {
				NetReload ( );
			}
			
			SetStatus ( WP_RELOAD );
			PlayAnim ( ANIMCHANNEL_ALL, "reload", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				AddToClip ( ClipSize() );
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
			
