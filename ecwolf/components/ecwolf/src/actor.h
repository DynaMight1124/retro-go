/*
** actor.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#ifndef __ACTOR_H__
#define __ACTOR_H__

#include "wl_def.h"
#include "actordef.h"
#include "gamemap.h"
#include "textures/textures.h"
#include "linkedlist.h"
#include "name.h"
#include "dobject.h"
#include "tflags.h"
#include "thinker.h"

enum
{
	AMETA_BASE = 0x12000,

	AMETA_Damage,
	AMETA_DropItems,
	AMETA_SecretDeathSound,
	AMETA_GibHealth,
	AMETA_DefaultHealth1,
	AMETA_DefaultHealth2,
	AMETA_DefaultHealth3,
	AMETA_DefaultHealth4,
	AMETA_DefaultHealth5,
	AMETA_DefaultHealth6,
	AMETA_DefaultHealth7,
	AMETA_DefaultHealth8,
	AMETA_DefaultHealth9,
	AMETA_ConversationID,
    AMETA_StartInventory // Missing in some versions
};

enum
{
	SPAWN_AllowReplacement = 1,
	SPAWN_Patrol = 2
};

typedef TFlags<ActorFlag> ActorFlags;
DEFINE_TFLAGS_OPERATORS (ActorFlags)

namespace Dialog { struct Page; }

class player_t;
class ClassDef;
class AInventory;

class AActor : public Thinker, public EmbeddedList<AActor>::Node
{
	DECLARE_CLASS(AActor, Thinker)
	HAS_OBJECT_POINTERS

	public:
		typedef EmbeddedList<AActor>::Iterator Iterator;
		struct DropItem
		{
			public:
				FName			className;
				unsigned int	amount;
				uint8_t			probability;
		};
		typedef LinkedList<DropItem> DropList;

		void			AddInventory(AInventory *item);
		virtual void	BeginPlay() {}
		void			ClearCounters();
		void			ClearInventory();
		virtual void	Destroy();
		virtual void	Die();
		void			EnterZone(const MapZone *zone);
		AInventory		*FindInventory(const ClassDef *cls);
		const Frame		*FindState(const FName &name) const;
		static void		FinishSpawningActors();
		int				GetDamage();
		const AActor	*GetDefault() const;
		DropList		*GetDropList() const;
		const MapZone	*GetZone() const { return soundZone; }
		bool			GiveInventory(const ClassDef *cls, int amount=0, bool allowreplacement=true);
		bool			InStateSequence(const Frame *basestate) const;
		bool			IsFast() const;
		virtual void	PostBeginPlay() {}
		void			RemoveFromWorld();
		virtual void	RemoveInventory(AInventory *item);
		void			Serialize(FArchive &arc);
		void			SetState(const Frame *state, bool norun=false);
		void			UpdateDormancy();
		void			SpawnFog();
		static AActor	*Spawn(const ClassDef *type, fixed x, fixed y, fixed z, int flags);
		int32_t			SpawnHealth() const;
		bool			Teleport(fixed x, fixed y, angle_t angle, bool nofog=false);
		virtual void	Tick();
		virtual void	Touch(AActor *toucher) {}
		virtual bool	ReceivesTouch() const { return false; }
		void			PrintInventory();

		static PointerIndexTable<ExpressionNode> damageExpressions;
		static PointerIndexTable<DropList> dropItems;
		static EmbeddedList<AActor>::List actors;
		static Iterator GetIterator() { return Iterator(actors); }
		static const TArray<AActor *> &GetTouchActors() { return touchActors; }
		struct CollisionEntry
		{
			AActor *actor;
			fixed x, y, radius;
			bool dynamic;
		};
		static const TArray<CollisionEntry> &GetCollisionActors() { return collisionActors; }
		void RefreshCollisionPosition();

		// Basic properties from objtype
		ActorFlags flags;

		int32_t	distance; // if negative, wait for that door to open
		dirtype	dir;

#if !defined(_MSC_VER) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 6)
#define COORD_PART const word
#else
#define COORD_PART word
#endif
		union
		{
			fixed x;
#ifdef __BIG_ENDIAN__
			struct { COORD_PART tilex; word fracx; };
#else
			struct { word fracx; COORD_PART tilex; };
#endif
		};
		union
		{
			fixed y;
#ifdef __BIG_ENDIAN__
			struct { COORD_PART tiley; word fracy; };
#else
			struct { word fracy; COORD_PART tiley; };
#endif
		};
#undef COORD_PART
		fixed z;
		fixed	velx, vely, velz;

		angle_t	angle;
		fixed pitch;
		int32_t	health;
		int32_t	speed, runspeed;
		int		points;
		fixed	radius;
		fixed	projectilepassheight;
        fixed   height;

		const Frame		*state;
		unsigned int	sprite;
		fixed			scaleX, scaleY;
		short			ticcount;
		FTextureID		overheadIcon;

		short       viewx;
		word        viewheight;
		fixed       transx,transy;      // in global coord

		uint16_t	sighttime;
		uint8_t		sightrandom;
		fixed		missilefrequency;
		uint16_t	minmissilechance;
		short		movecount; // Emulation of Doom's movecount
		fixed		meleerange;
		uint16_t	painchance;
		FNameNoInit	activesound, attacksound, deathsound, painsound, seesound;

		const Frame *SpawnState, *SeeState, *PathState, *PainState, *MeleeState, *MissileState;
		short       temp1,hidden;
		fixed		killerx,killery; // For deathcam

		TObjPtr<AActor> target;
        TObjPtr<AActor> lastenemy;
		player_t	*player;	// Only valid with APlayerPawn

		TObjPtr<AInventory>	inventory;

        TObjPtr<AActor> master;
        TObjPtr<AActor> tracer;

        FName MeleeSound;
        FName DeathSound;
        FName SeeSound;
        FName ActiveSound;
        fixed meleebange;
        int32 translucency;
        const Dialog::Page *conversation;

		static TArray<AActor *> SpawnedActors;
		static TArray<AActor *> touchActors;
		static TArray<CollisionEntry> collisionActors;

	protected:
		void	Init();
		void	RegisterTouchActor();
		void	UnregisterTouchActor();
		void	UpdateCollisionActor();
		void	UnregisterCollisionActor();

		const MapZone	*soundZone;
};

// Old save compatibility
// FIXME: Remove for 1.4
class AActorProxy : public Thinker
{
	DECLARE_CLASS(AActorProxy, Thinker)

public:
	void Tick() {}

	void Serialize(FArchive &arc);

	TObjPtr<AActor> actualObject;
};

#endif
