#include "thingdef.h"
#include "actor.h"
#include "a_inventory.h"
#include "a_keys.h"

// Since we can't easily include g_wolf/a_spearofdestiny.h due to paths,
// we will declare it manually.
class ASpearOfDestiny : public AActor {
public:
    static const ClassDef *__StaticClass;
};

class ARandomSpawner : public AActor {
public:
    static const ClassDef *__StaticClass;
};

class APatrolPoint : public AActor {
public:
    static const ClassDef *__StaticClass;
};

#define REF_CLASS(cls) { dummy_ref_var = &cls::__StaticClass; }

extern "C" {
#define REF_AF(f) bool __AF_##f(AActor *, AActor *, const Frame *, const CallArguments &, struct ActionResult *); \
                  const void * f##_ref = (const void *)&__AF_##f;

REF_AF(A_FinishDeathCam)
REF_AF(A_CustomPunch)
REF_AF(A_SmartAnimDelay)
REF_AF(A_GunAttack)
REF_AF(A_Wander)
REF_AF(A_InitSmartAnim)
REF_AF(A_WeaponReady)
REF_AF(A_Chase)
REF_AF(A_WeaponGrin)
REF_AF(A_FireCustomMissile)
REF_AF(A_Lower)
REF_AF(A_ReFire)
REF_AF(A_Look)
REF_AF(A_CustomMissile)
REF_AF(A_PlasmaGrenadeCalcDuration)
REF_AF(A_Succeed)
REF_AF(A_WolfAttack)
REF_AF(A_Dormant)
REF_AF(A_Scream)
REF_AF(A_Raise)
REF_AF(A_FaceTarget)
REF_AF(A_CallSpecial)
REF_AF(A_Light)
REF_AF(A_PlaySound)
REF_AF(A_Light1)
REF_AF(A_Pain)
REF_AF(A_ZoomFactor)
REF_AF(A_ActiveSound)
REF_AF(A_Stop)
REF_AF(A_GunFlash)
REF_AF(A_BossDeath)
REF_AF(A_Light2)
REF_AF(A_ChangeFlag)
REF_AF(A_MonsterRefire)
REF_AF(A_SpawnItem)
REF_AF(A_Jump)
REF_AF(A_SpawnItemEx)
REF_AF(A_GiveInventory)
REF_AF(A_JumpIf)
REF_AF(A_SetTics)
REF_AF(A_Explode)
REF_AF(A_Fall)
REF_AF(A_GiveExtraMan)
REF_AF(A_Light0)
REF_AF(A_AlertMonsters)
REF_AF(A_JumpIfInventory)
REF_AF(A_JumpIfCloser)
REF_AF(A_MeleeAttack)
REF_AF(A_ChangeVelocity)
REF_AF(A_ScaleVelocity)
}

volatile const void * volatile dummy_ref_var;

void RegisterNativeActors()
{
    // Native Classes
    REF_CLASS(AActor);
    REF_CLASS(AInventory);
    REF_CLASS(ASpearOfDestiny);
    REF_CLASS(ARandomSpawner);
    REF_CLASS(APatrolPoint);

    // Action Functions
    dummy_ref_var = A_FinishDeathCam_ref;
    dummy_ref_var = A_CustomPunch_ref;
    dummy_ref_var = A_SmartAnimDelay_ref;
    dummy_ref_var = A_GunAttack_ref;
    dummy_ref_var = A_Wander_ref;
    dummy_ref_var = A_InitSmartAnim_ref;
    dummy_ref_var = A_WeaponReady_ref;
    dummy_ref_var = A_Chase_ref;
    dummy_ref_var = A_WeaponGrin_ref;
    dummy_ref_var = A_FireCustomMissile_ref;
    dummy_ref_var = A_Lower_ref;
    dummy_ref_var = A_ReFire_ref;
    dummy_ref_var = A_Look_ref;
    dummy_ref_var = A_CustomMissile_ref;
    dummy_ref_var = A_PlasmaGrenadeCalcDuration_ref;
    dummy_ref_var = A_Succeed_ref;
    dummy_ref_var = A_WolfAttack_ref;
    dummy_ref_var = A_Dormant_ref;
    dummy_ref_var = A_Scream_ref;
    dummy_ref_var = A_Raise_ref;
    dummy_ref_var = A_FaceTarget_ref;
    dummy_ref_var = A_CallSpecial_ref;
    dummy_ref_var = A_Light_ref;
    dummy_ref_var = A_PlaySound_ref;
    dummy_ref_var = A_Light1_ref;
    dummy_ref_var = A_Pain_ref;
    dummy_ref_var = A_ZoomFactor_ref;
    dummy_ref_var = A_ActiveSound_ref;
    dummy_ref_var = A_Stop_ref;
    dummy_ref_var = A_GunFlash_ref;
    dummy_ref_var = A_BossDeath_ref;
    dummy_ref_var = A_Light2_ref;
    dummy_ref_var = A_ChangeFlag_ref;
    dummy_ref_var = A_MonsterRefire_ref;
    dummy_ref_var = A_SpawnItem_ref;
    dummy_ref_var = A_Jump_ref;
    dummy_ref_var = A_SpawnItemEx_ref;
    dummy_ref_var = A_GiveInventory_ref;
    dummy_ref_var = A_JumpIf_ref;
    dummy_ref_var = A_SetTics_ref;
    dummy_ref_var = A_Explode_ref;
    dummy_ref_var = A_Fall_ref;
    dummy_ref_var = A_GiveExtraMan_ref;
    dummy_ref_var = A_Light0_ref;
    dummy_ref_var = A_AlertMonsters_ref;
    dummy_ref_var = A_JumpIfInventory_ref;
    dummy_ref_var = A_JumpIfCloser_ref;
    dummy_ref_var = A_MeleeAttack_ref;
    dummy_ref_var = A_ChangeVelocity_ref;
    dummy_ref_var = A_ScaleVelocity_ref;
}
