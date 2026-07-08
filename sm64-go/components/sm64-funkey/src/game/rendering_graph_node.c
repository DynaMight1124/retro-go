#include <PR/ultratypes.h>
#include <rg_system.h>
#include <math.h>

#include "area.h"
#include "behavior_data.h"
#include "engine/math_util.h"
#include "game_init.h"
#include "gfx_dimensions.h"
#include "level_table.h"
#include "main.h"
#include "memory.h"
#include "model_ids.h"
#include "object_list_processor.h"
#include "print.h"
#include "rendering_graph_node.h"
#include "shadow.h"
#include "sm64.h"

#define GEO_RENDER_DIAG 0
#if !GEO_RENDER_DIAG
#undef RG_LOGI
#define RG_LOGI(...) ((void)0)
#endif

/**
 * This file contains the code that processes the scene graph for rendering.
 * The scene graph is responsible for drawing everything except the HUD / text boxes.
 * First the root of the scene graph is processed when geo_process_root
 * is called from level_script.c. The rest of the tree is traversed recursively
 * using the function geo_process_node_and_siblings, which switches over all
 * geo node types and calls a specialized function accordingly.
 * The types are defined in engine/graph_node.h
 *
 * The scene graph typically looks like:
 * - Root (viewport)
 *  - Master list
 *   - Ortho projection
 *    - Background (skybox)
 *  - Master list
 *   - Perspective
 *    - Camera
 *     - <area-specific display lists>
 *     - Object parent
 *      - <group with 240 object nodes>
 *  - Master list
 *   - Script node (Cannon overlay)
 *
 */

#define GEO_MAT_STACK_SIZE 64

s16 gMatStackIndex;
Mat4 gMatStack[GEO_MAT_STACK_SIZE];
Mtx *gMatStackFixed[GEO_MAT_STACK_SIZE];

/**
 * Animation nodes have state in global variables, so this struct captures
 * the animation state so a 'context switch' can be made when rendering the
 * held object.
 */
struct GeoAnimState {
    /*0x00*/ u8 type;
    /*0x01*/ u8 enabled;
    /*0x02*/ s16 frame;
    /*0x04*/ f32 translationMultiplier;
    /*0x08*/ u16 *attribute;
    /*0x0C*/ s16 *data;
};

// For some reason, this is a GeoAnimState struct, but the current state consists
// of separate global variables. It won't match EU otherwise.
struct GeoAnimState gGeoTempState;

u8 gCurAnimType;
u8 gCurAnimEnabled;
s16 gCurrAnimFrame;
f32 gCurAnimTranslationMultiplier;
u16 *gCurrAnimAttribute;
s16 *gCurAnimData;

struct AllocOnlyPool *gDisplayListHeap;

struct RenderModeContainer {
    u32 modes[8];
};

#define GEO_MENU_OBJECT_DIAG 0
#define GEO_MENU_OBJECT_DIAG_LIMIT 0
#define GEO_MENU_CAMERA_DIAG_LIMIT 0
#define GEO_GAMEPLAY_OBJECT_DIAG_LIMIT 0
#define GEO_OBJECT_MATRIX_FALLBACK_DIAG_LIMIT 0
#define GEO_ANIM_PART_DIAG_LIMIT 0
#define GEO_OBJECT_MATRIX_TRY_DIAG_LIMIT 0
#define GEO_ANIM_PARENT_REPAIR_DIAG_LIMIT 0
#define GEO_ANIM_CHILD_REPAIR_DIAG_LIMIT 0
#define GEO_ANIM_DISPLAY_SKIP_DIAG_LIMIT 0
#define GEO_RAW_FALLBACK_OBJECT_DIAG_LIMIT 0
#define GEO_RAW_FALLBACK_ROOT_DRAW_DIAG_LIMIT 0
#define GEO_CHARACTER_OWNER_OBJECT_DIAG_LIMIT 0
#define GEO_CHARACTER_CHILD_DRAW_DIAG_LIMIT 0
#define GEO_CHARACTER_TRACE_DIAG_LIMIT 0
#define GEO_COURSE_ACTOR_TRACE_DIAG_LIMIT 0
#define GEO_MARIO_TRACE_DIAG_LIMIT 0
#define GEO_DOOR_TRACE_DIAG_LIMIT 0
#define GEO_DISPLAY_MATRIX_REFRESH_DIAG_LIMIT 0
#define GEO_TARGET_ANIM_REPAIR_DIAG_LIMIT 0
#define GEO_NODE_TRANSFORM_REPAIR_DIAG_LIMIT 0
#define GEO_RENDERER_MENU_MODEL_DIAG_ENABLED 0
#define GEO_RENDERER_CHARACTER_MODEL_DIAG_ENABLED 0
#define GEO_MIN_USABLE_BASIS 0.01f

static s32 sGeoMenuObjectDiagCount;
static s32 sGeoMenuCameraDiagCount;
static s32 sGeoGameplayObjectDiagCount;
static s32 sGeoObjectMatrixFallbackDiagCount;
static s32 sGeoAnimPartDiagCount;
static s32 sGeoObjectMatrixTryDiagCount;
static s32 sGeoAnimParentRepairDiagCount;
static s32 sGeoAnimChildRepairDiagCount;
static s32 sGeoAnimDisplaySkipDiagCount;
static s32 sGeoRawFallbackObjectDiagCount;
static s32 sGeoRawFallbackRootDrawDiagCount;
static s32 sGeoCharacterOwnerObjectDiagCount;
static s32 sGeoCharacterChildDrawDiagCount;
static s32 sGeoCharacterTraceDiagCount;
static s32 sGeoCourseActorTraceDiagCount;
static s32 sGeoMarioTraceDiagCount;
static s32 sGeoDoorTraceDiagCount;
static s32 sGeoDisplayMatrixRefreshDiagCount;
static s32 sGeoTargetAnimRepairDiagCount;
static s32 sGeoNodeTransformRepairDiagCount;
static Mat4 sGeoRawCameraMatrix;
static Mat4 sGeoCurrentObjectRootMatrix;
static s32 sGeoRawCameraMatrixValid;
static s32 sGeoCurrentObjectRootMatrixValid;
static s32 sGeoCurrentObjectUsedRawCameraFallback;
static s32 sGeoCurrentObjectRootStackIndex;
static s32 sGeoCurrentObjectModelId;
static const BehaviorScript *sGeoCurrentObjectBehavior;
static u32 sGeoCurrentObjectBehParams;
static s32 sGeoCurrentObjectBehParam2;
static s32 sGeoCurrentObjectAnimId;
static s32 sGeoCurrentObjectProbeRenderer;

extern void gfx_debug_arm_menu_model(int shared_type, int pos_x, int pos_y, int pos_z,
                                     int scale_x, int scale_y, int scale_z);
extern void gfx_debug_arm_character_model(int shared_type, int row_x, int row_y, int row_z);

static s32 geo_matrix_is_finite(Mat4 mtx) {
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (!isfinite(mtx[i][j])) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static f32 geo_matrix_basis_sum(Mat4 mtx) {
    return
        mtx[0][0] * mtx[0][0] + mtx[0][1] * mtx[0][1] + mtx[0][2] * mtx[0][2] +
        mtx[1][0] * mtx[1][0] + mtx[1][1] * mtx[1][1] + mtx[1][2] * mtx[1][2] +
        mtx[2][0] * mtx[2][0] + mtx[2][1] * mtx[2][1] + mtx[2][2] * mtx[2][2];
}

static s32 geo_matrix_has_valid_basis(Mat4 mtx) {
    f32 basis = geo_matrix_basis_sum(mtx);

    return geo_matrix_is_finite(mtx) && isfinite(basis)
        && mtx[3][3] > 0.99f && mtx[3][3] < 1.01f
        && basis > GEO_MIN_USABLE_BASIS && basis < 1000000000000.0f;
}

static s32 geo_matrix_has_plausible_object_transform(Mat4 mtx) {
    return geo_matrix_has_valid_basis(mtx)
        && mtx[3][0] > -1000000.0f && mtx[3][0] < 1000000.0f
        && mtx[3][1] > -1000000.0f && mtx[3][1] < 1000000.0f
        && mtx[3][2] > -1000000.0f && mtx[3][2] < 1000000.0f;
}

static s32 geo_current_shared_child_type(void) {
    return gCurGraphNodeObject != NULL && gCurGraphNodeObject->sharedChild != NULL
        ? gCurGraphNodeObject->sharedChild->type
        : -1;
}

static s32 geo_find_model_id(struct GraphNode *sharedChild) {
    s32 i;

    if (sharedChild == NULL || gLoadedGraphNodes == NULL) {
        return -1;
    }
    for (i = 0; i < 0x100; i++) {
        if (gLoadedGraphNodes[i] == sharedChild) {
            return i;
        }
    }
    return -1;
}

static const char *geo_model_diag_name(s32 modelId) {
    if (modelId >= MODEL_LEVEL_GEOMETRY_03 && modelId <= MODEL_LEVEL_GEOMETRY_16) {
        return "levelgeo";
    }
    if (gCurrLevelNum == LEVEL_BOB) {
        switch (modelId) {
            case MODEL_KING_BOBOMB:
                return "king_bobomb";
            case MODEL_ENEMY_LAKITU:
                return "enemy_lakitu";
            case MODEL_CHAIN_CHOMP:
                return "chain_chomp";
            case MODEL_METALLIC_BALL:
                return "metallic_ball";
            case MODEL_KOOPA_WITH_SHELL:
                return "koopa_shell_body";
            case MODEL_KOOPA_FLAG:
                return "koopa_flag";
            case MODEL_WOODEN_POST:
                return "wooden_post";
            default:
                break;
        }
    }

    switch (modelId) {
        case MODEL_MARIO:
            return "mario";
        case MODEL_CASTLE_CASTLE_DOOR_UNUSED:
            return "door_unused";
        case MODEL_CASTLE_DOOR_0_STARS:
            return "door_0_star";
        case MODEL_CASTLE_DOOR_1_STAR:
            return "door_1_star";
        case MODEL_CASTLE_DOOR_3_STARS:
            return "door_3_star";
        case MODEL_CASTLE_KEY_DOOR:
            return "key_door";
        case MODEL_CASTLE_CASTLE_DOOR:
            return "castle_door";
        case MODEL_CASTLE_WOODEN_DOOR:
            return "wooden_door";
        case MODEL_CASTLE_METAL_DOOR:
            return "metal_door";
        case MODEL_CASTLE_GROUNDS_VCUTM_GRILL:
            return "cg_grill";
        case MODEL_CASTLE_GROUNDS_FLAG:
            return "cg_flag";
        case MODEL_CASTLE_GROUNDS_CANNON_GRILL:
            return "cg_cannon";
        case MODEL_BIRDS:
            return "birds";
        case MODEL_YOSHI:
            return "yoshi";
        case MODEL_LAKITU:
            return "lakitu";
        case MODEL_BUTTERFLY:
            return "butterfly";
        case MODEL_MARIOS_WINGED_METAL_CAP:
            return "wing_metal_cap";
        case MODEL_MARIOS_METAL_CAP:
            return "metal_cap";
        case MODEL_MARIOS_WING_CAP:
            return "wing_cap";
        case MODEL_MARIOS_CAP:
            return "cap";
        case MODEL_EXCLAMATION_BOX:
            return "exclamation";
        case MODEL_TOAD:
            return "toad";
        case MODEL_PEACH:
            return "peach";
        case MODEL_BLACK_BOBOMB:
            return "black_bobomb";
        case MODEL_KOOPA_SHELL:
            return "koopa_shell";
        case MODEL_KOOPA_WITHOUT_SHELL:
            return "koopa_no_shell";
        case MODEL_GOOMBA:
            return "goomba";
        case MODEL_AMP:
            return "amp";
        case MODEL_BOBOMB_BUDDY:
            return "bobomb_buddy";
        case MODEL_SNUFIT:
            return "snufit";
        case MODEL_FLYGUY:
            return "flyguy";
        case MODEL_CHUCKYA:
            return "chuckya";
        case MODEL_CASTLE_STAR_DOOR_30_STARS:
            return "star_door_30";
        case MODEL_CASTLE_STAR_DOOR_50_STARS:
            return "star_door_50";
        case MODEL_CASTLE_STAR_DOOR_8_STARS:
            return "star_door_8";
        case MODEL_CASTLE_STAR_DOOR_70_STARS:
            return "star_door_70";
        default:
            return "other";
    }
}

static s32 geo_is_course_actor_model(s32 modelId) {
    if (gCurrLevelNum == LEVEL_BOB) {
        switch (modelId) {
            case MODEL_KING_BOBOMB:
            case MODEL_ENEMY_LAKITU:
            case MODEL_CHAIN_CHOMP:
            case MODEL_METALLIC_BALL:
            case MODEL_KOOPA_WITH_SHELL:
            case MODEL_KOOPA_FLAG:
            case MODEL_WOODEN_POST:
                return TRUE;
            default:
                break;
        }
    }

    switch (modelId) {
        case MODEL_BLACK_BOBOMB:
        case MODEL_KOOPA_SHELL:
        case MODEL_KOOPA_WITHOUT_SHELL:
        case MODEL_GOOMBA:
        case MODEL_AMP:
        case MODEL_BOBOMB_BUDDY:
        case MODEL_SNUFIT:
        case MODEL_FLYGUY:
        case MODEL_CHUCKYA:
            return TRUE;
        default:
            return FALSE;
    }
}

static s32 geo_is_character_like_model(s32 modelId) {
    switch (modelId) {
        case MODEL_MARIO:
        case MODEL_BIRDS:
        case MODEL_YOSHI:
        case MODEL_LAKITU:
        case MODEL_MARIOS_WINGED_METAL_CAP:
        case MODEL_MARIOS_METAL_CAP:
        case MODEL_MARIOS_WING_CAP:
        case MODEL_MARIOS_CAP:
        case MODEL_BUTTERFLY:
        case MODEL_TOAD:
        case MODEL_PEACH:
            return TRUE;
        default:
            return FALSE;
    }
}

static s32 geo_is_character_like_behavior(const BehaviorScript *behavior) {
    return behavior == bhvMario
        || behavior == bhvBeginningPeach
        || behavior == bhvBeginningLakitu
        || behavior == bhvBird
        || behavior == bhvCameraLakitu
        || behavior == bhvYoshi;
}

static s32 geo_is_course_actor_behavior(const BehaviorScript *behavior) {
    return behavior == bhvKingBobomb
        || behavior == bhvBobomb
        || behavior == bhvBobombBuddy
        || behavior == bhvBobombBuddyOpensCannon
        || behavior == bhvKoopa
        || behavior == bhvKoopaShell
        || behavior == bhvKoopaFlag
        || behavior == bhvGoomba
        || behavior == bhvChainChomp
        || behavior == bhvChainChompChainPart
        || behavior == bhvWoodenPost
        || behavior == bhvEnemyLakitu
        || behavior == bhvSpiny
        || behavior == bhvMontyMole
        || behavior == bhvMontyMoleRock
        || behavior == bhvFlyGuy
        || behavior == bhvChuckya
        || behavior == bhvSpindrift
        || behavior == bhvPiranhaPlant
        || behavior == bhvWhompKingBoss
        || behavior == bhvBoo
        || behavior == bhvBooInCastle
        || behavior == bhvSkeeter
        || behavior == bhvSnufit
        || behavior == bhvBreakableBox
        || behavior == bhvBreakableBoxSmall;
}

static s32 geo_is_character_like_object(struct Object *node) {
    return node == gMarioObject
        || geo_is_character_like_model(sGeoCurrentObjectModelId)
        || geo_is_character_like_behavior(node->behavior);
}

static s32 geo_is_target_actor_object(struct Object *node) {
    return geo_is_character_like_object(node)
        || geo_is_course_actor_model(sGeoCurrentObjectModelId)
        || geo_is_course_actor_behavior(node->behavior);
}

static s32 geo_is_door_like_model(s32 modelId) {
    switch (modelId) {
        case MODEL_CASTLE_CASTLE_DOOR_UNUSED:
        case MODEL_CASTLE_DOOR_0_STARS:
        case MODEL_CASTLE_DOOR_1_STAR:
        case MODEL_CASTLE_DOOR_3_STARS:
        case MODEL_CASTLE_KEY_DOOR:
        case MODEL_CASTLE_CASTLE_DOOR:
        case MODEL_CASTLE_WOODEN_DOOR:
        case MODEL_CASTLE_METAL_DOOR:
        case MODEL_CASTLE_STAR_DOOR_30_STARS:
        case MODEL_CASTLE_STAR_DOOR_50_STARS:
        case MODEL_CASTLE_STAR_DOOR_8_STARS:
        case MODEL_CASTLE_STAR_DOOR_70_STARS:
            return TRUE;
        default:
            return FALSE;
    }
}

static s32 geo_is_door_like_behavior(const BehaviorScript *behavior) {
    return behavior == bhvDoor
        || behavior == bhvDoorWarp
        || behavior == bhvStarDoor
        || behavior == bhvTowerDoor;
}

static s32 geo_is_door_like_object(struct Object *node) {
    return geo_is_door_like_model(sGeoCurrentObjectModelId)
        || geo_is_door_like_behavior(node->behavior);
}

static s32 geo_should_probe_renderer_object_raw(void) {
    return sGeoCurrentObjectModelId == MODEL_MARIO
        || sGeoCurrentObjectBehavior == bhvMario
        || geo_is_character_like_model(sGeoCurrentObjectModelId)
        || geo_is_character_like_behavior(sGeoCurrentObjectBehavior)
        || geo_is_course_actor_model(sGeoCurrentObjectModelId)
        || geo_is_course_actor_behavior(sGeoCurrentObjectBehavior)
        || geo_is_door_like_model(sGeoCurrentObjectModelId)
        || geo_is_door_like_behavior(sGeoCurrentObjectBehavior);
}

static s32 geo_should_probe_renderer_object(void) {
    return sGeoCurrentObjectProbeRenderer;
}

static const char *geo_behavior_diag_name(const BehaviorScript *behavior) {
    if (behavior == bhvMario) {
        return "bhvMario";
    }
    if (behavior == bhvBeginningPeach) {
        return "bhvBeginningPeach";
    }
    if (behavior == bhvBeginningLakitu) {
        return "bhvBeginningLakitu";
    }
    if (behavior == bhvBird) {
        return "bhvBird";
    }
    if (behavior == bhvCameraLakitu) {
        return "bhvCameraLakitu";
    }
    if (behavior == bhvYoshi) {
        return "bhvYoshi";
    }
    if (behavior == bhvKingBobomb) {
        return "bhvKingBobomb";
    }
    if (behavior == bhvBobomb) {
        return "bhvBobomb";
    }
    if (behavior == bhvBobombBuddy) {
        return "bhvBobombBuddy";
    }
    if (behavior == bhvBobombBuddyOpensCannon) {
        return "bhvBobombBuddyOpensCannon";
    }
    if (behavior == bhvKoopa) {
        return "bhvKoopa";
    }
    if (behavior == bhvKoopaShell) {
        return "bhvKoopaShell";
    }
    if (behavior == bhvKoopaFlag) {
        return "bhvKoopaFlag";
    }
    if (behavior == bhvGoomba) {
        return "bhvGoomba";
    }
    if (behavior == bhvChainChomp) {
        return "bhvChainChomp";
    }
    if (behavior == bhvChainChompChainPart) {
        return "bhvChainChompChainPart";
    }
    if (behavior == bhvWoodenPost) {
        return "bhvWoodenPost";
    }
    if (behavior == bhvEnemyLakitu) {
        return "bhvEnemyLakitu";
    }
    if (behavior == bhvSpiny) {
        return "bhvSpiny";
    }
    if (behavior == bhvMontyMole) {
        return "bhvMontyMole";
    }
    if (behavior == bhvMontyMoleRock) {
        return "bhvMontyMoleRock";
    }
    if (behavior == bhvFlyGuy) {
        return "bhvFlyGuy";
    }
    if (behavior == bhvChuckya) {
        return "bhvChuckya";
    }
    if (behavior == bhvSpindrift) {
        return "bhvSpindrift";
    }
    if (behavior == bhvPiranhaPlant) {
        return "bhvPiranhaPlant";
    }
    if (behavior == bhvWhompKingBoss) {
        return "bhvWhompKingBoss";
    }
    if (behavior == bhvBoo) {
        return "bhvBoo";
    }
    if (behavior == bhvBooInCastle) {
        return "bhvBooInCastle";
    }
    if (behavior == bhvSkeeter) {
        return "bhvSkeeter";
    }
    if (behavior == bhvSnufit) {
        return "bhvSnufit";
    }
    if (behavior == bhvBreakableBox) {
        return "bhvBreakableBox";
    }
    if (behavior == bhvBreakableBoxSmall) {
        return "bhvBreakableBoxSmall";
    }
    if (behavior == bhvCastleFlagWaving) {
        return "bhvCastleFlagWaving";
    }
    if (behavior == bhvMoatGrills) {
        return "bhvMoatGrills";
    }
    if (behavior == bhvHiddenAt120Stars) {
        return "bhvHiddenAt120Stars";
    }
    if (behavior == bhvExclamationBox) {
        return "bhvExclamationBox";
    }
    if (behavior == bhvIntroScene) {
        return "bhvIntroScene";
    }
    if (behavior == bhvDoor) {
        return "bhvDoor";
    }
    if (behavior == bhvDoorWarp) {
        return "bhvDoorWarp";
    }
    if (behavior == bhvStarDoor) {
        return "bhvStarDoor";
    }
    if (behavior == bhvTowerDoor) {
        return "bhvTowerDoor";
    }
    return "other";
}

static void geo_normalize_object_basis(Mat4 mtx, Vec3f scale) {
    s32 i;

    for (i = 0; i < 3; i++) {
        f32 len = sqrtf(mtx[i][0] * mtx[i][0] + mtx[i][1] * mtx[i][1] + mtx[i][2] * mtx[i][2]);
        f32 targetLen = fabsf(scale[i]);

        if (targetLen < 0.0001f) {
            targetLen = 1.0f;
        }
        if (isfinite(len) && len > 0.0001f && isfinite(targetLen)) {
            f32 factor = targetLen / len;

            mtx[i][0] *= factor;
            mtx[i][1] *= factor;
            mtx[i][2] *= factor;
        }
        mtx[i][3] = 0.0f;
    }
    mtx[3][3] = 1.0f;
}

static s32 geo_matrix_basis_needs_repair(Mat4 mtx) {
    f32 basis = geo_matrix_basis_sum(mtx);

    return !geo_matrix_has_valid_basis(mtx) || !isfinite(basis)
        || basis < GEO_MIN_USABLE_BASIS || basis > 16.0f;
}

static void geo_repair_matrix_basis_keep_position(Mat4 mtx, Mat4 rootMtx) {
    Vec3f pos;

    vec3f_set(pos, mtx[3][0], mtx[3][1], mtx[3][2]);
    if (rootMtx != NULL &&
        (pos[0] < -50000.0f || pos[0] > 50000.0f ||
         pos[1] < -50000.0f || pos[1] > 50000.0f ||
         pos[2] < -50000.0f || pos[2] > 50000.0f)) {
        vec3f_set(pos, rootMtx[3][0], rootMtx[3][1], rootMtx[3][2]);
    }
    mtxf_identity(mtx);
    mtx[3][0] = pos[0];
    mtx[3][1] = pos[1];
    mtx[3][2] = pos[2];
}

static s32 geo_matrix_position_far_from_root(Mat4 mtx, Mat4 rootMtx) {
    f32 dx;
    f32 dy;
    f32 dz;

    if (rootMtx == NULL) {
        return FALSE;
    }
    dx = mtx[3][0] - rootMtx[3][0];
    dy = mtx[3][1] - rootMtx[3][1];
    dz = mtx[3][2] - rootMtx[3][2];

    return !geo_matrix_is_finite(mtx)
        || dx < -20000.0f || dx > 20000.0f
        || dy < -20000.0f || dy > 20000.0f
        || dz < -20000.0f || dz > 20000.0f;
}

static s32 geo_matrix_row_matches_root(Mat4 mtx, Mat4 rootMtx) {
    f32 dx;
    f32 dy;
    f32 dz;

    if (rootMtx == NULL) {
        return FALSE;
    }
    dx = mtx[3][0] - rootMtx[3][0];
    dy = mtx[3][1] - rootMtx[3][1];
    dz = mtx[3][2] - rootMtx[3][2];

    return dx > -1.0f && dx < 1.0f
        && dy > -1.0f && dy < 1.0f
        && dz > -1.0f && dz < 1.0f;
}

static s32 geo_matrix_position_far_from_anchor(Mat4 mtx, Mat4 anchorMtx) {
    f32 dx;
    f32 dy;
    f32 dz;

    if (anchorMtx == NULL) {
        return FALSE;
    }
    dx = mtx[3][0] - anchorMtx[3][0];
    dy = mtx[3][1] - anchorMtx[3][1];
    dz = mtx[3][2] - anchorMtx[3][2];

    return !geo_matrix_is_finite(mtx)
        || dx < -5000.0f || dx > 5000.0f
        || dy < -5000.0f || dy > 5000.0f
        || dz < -5000.0f || dz > 5000.0f;
}

static void geo_set_anchored_translation(Mat4 dest, Mat4 anchorMtx, f32 localX, f32 localY, f32 localZ) {
    dest[3][0] = anchorMtx[3][0]
        + localX * anchorMtx[0][0] + localY * anchorMtx[1][0] + localZ * anchorMtx[2][0];
    dest[3][1] = anchorMtx[3][1]
        + localX * anchorMtx[0][1] + localY * anchorMtx[1][1] + localZ * anchorMtx[2][1];
    dest[3][2] = anchorMtx[3][2]
        + localX * anchorMtx[0][2] + localY * anchorMtx[1][2] + localZ * anchorMtx[2][2];
}

static void geo_rebuild_child_transform(Mat4 childMtx, Mat4 localMtx, Mat4 anchorMtx) {
    s32 i;

    for (i = 0; i < 3; i++) {
        childMtx[i][0] = localMtx[i][0] * anchorMtx[0][0]
                       + localMtx[i][1] * anchorMtx[1][0]
                       + localMtx[i][2] * anchorMtx[2][0];
        childMtx[i][1] = localMtx[i][0] * anchorMtx[0][1]
                       + localMtx[i][1] * anchorMtx[1][1]
                       + localMtx[i][2] * anchorMtx[2][1];
        childMtx[i][2] = localMtx[i][0] * anchorMtx[0][2]
                       + localMtx[i][1] * anchorMtx[1][2]
                       + localMtx[i][2] * anchorMtx[2][2];
        childMtx[i][3] = 0.0f;
    }
    geo_set_anchored_translation(childMtx, anchorMtx,
                                 localMtx[3][0], localMtx[3][1], localMtx[3][2]);
    childMtx[3][3] = 1.0f;
}

static void geo_repair_target_node_transform(Mat4 childMtx, Mat4 localMtx, const char *tag) {
    Mat4 anchorMtx;
    s32 repaired = FALSE;
    s32 oldX;
    s32 oldY;
    s32 oldZ;
    f32 basisBefore;

    if (!geo_should_probe_renderer_object()
        || !sGeoCurrentObjectRootMatrixValid
        || gCurGraphNodeObject == NULL
        || !sGeoCurrentObjectUsedRawCameraFallback
        || (geo_matrix_has_valid_basis(childMtx)
            && !geo_matrix_position_far_from_root(childMtx, sGeoCurrentObjectRootMatrix))) {
        return;
    }

    oldX = (s32) childMtx[3][0];
    oldY = (s32) childMtx[3][1];
    oldZ = (s32) childMtx[3][2];
    basisBefore = geo_matrix_basis_sum(childMtx);

    if (geo_matrix_has_valid_basis(gMatStack[gMatStackIndex])
        && !geo_matrix_position_far_from_root(gMatStack[gMatStackIndex], sGeoCurrentObjectRootMatrix)) {
        mtxf_copy(anchorMtx, gMatStack[gMatStackIndex]);
    } else {
        mtxf_copy(anchorMtx, sGeoCurrentObjectRootMatrix);
    }

    if (geo_matrix_has_valid_basis(anchorMtx) && geo_matrix_has_valid_basis(localMtx)) {
        geo_rebuild_child_transform(childMtx, localMtx, anchorMtx);
        repaired = TRUE;
    }

    if (repaired && sGeoNodeTransformRepairDiagCount < GEO_NODE_TRANSFORM_REPAIR_DIAG_LIMIT) {
        RG_LOGI("geo_node_transform_repair[%d]: tag=%s model=%d/%s beh=%s type=%d basis=%.3e old=%d,%d,%d anchor=%d,%d,%d local=%d,%d,%d new=%d,%d,%d after=%.3e",
                (int) sGeoNodeTransformRepairDiagCount,
                tag,
                (int) sGeoCurrentObjectModelId,
                geo_model_diag_name(sGeoCurrentObjectModelId),
                geo_behavior_diag_name(sGeoCurrentObjectBehavior),
                (int) geo_current_shared_child_type(),
                (double) basisBefore,
                (int) oldX,
                (int) oldY,
                (int) oldZ,
                (int) anchorMtx[3][0],
                (int) anchorMtx[3][1],
                (int) anchorMtx[3][2],
                (int) localMtx[3][0],
                (int) localMtx[3][1],
                (int) localMtx[3][2],
                (int) childMtx[3][0],
                (int) childMtx[3][1],
                (int) childMtx[3][2],
                (double) geo_matrix_basis_sum(childMtx));
        sGeoNodeTransformRepairDiagCount++;
    }
}

static void geo_repair_target_scale_transform(Mat4 childMtx, f32 scale) {
    Mat4 localMtx;

    if (!geo_should_probe_renderer_object()
        || !sGeoCurrentObjectRootMatrixValid
        || gCurGraphNodeObject == NULL
        || !sGeoCurrentObjectUsedRawCameraFallback
        || (geo_matrix_has_valid_basis(childMtx)
            && !geo_matrix_position_far_from_root(childMtx, sGeoCurrentObjectRootMatrix))) {
        return;
    }

    mtxf_identity(localMtx);
    localMtx[0][0] = scale;
    localMtx[1][1] = scale;
    localMtx[2][2] = scale;
    geo_repair_target_node_transform(childMtx, localMtx, "scale");
}

static s32 geo_is_raw_fallback_character_child(void) {
    return gCurGraphNodeObject != NULL
        && gCurrLevelNum == LEVEL_CASTLE_GROUNDS
        && sGeoCurrentObjectRootMatrixValid
        && gMatStackIndex > sGeoCurrentObjectRootStackIndex
        && sGeoCurrentObjectUsedRawCameraFallback
        && gCurGraphNodeObject->sharedChild != NULL
        && gCurGraphNodeObject->sharedChild->type == 47
        && (gCurGraphNodeObject->node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0
        && (gCurGraphNodeObject->node.flags & GRAPH_RENDER_BILLBOARD) == 0;
}

static s32 geo_is_raw_fallback_character_root(void) {
    return gCurGraphNodeObject != NULL
        && gCurrLevelNum == LEVEL_CASTLE_GROUNDS
        && sGeoCurrentObjectRootMatrixValid
        && gMatStackIndex == sGeoCurrentObjectRootStackIndex
        && sGeoCurrentObjectUsedRawCameraFallback
        && gCurGraphNodeObject->sharedChild != NULL
        && gCurGraphNodeObject->sharedChild->type == 47
        && (gCurGraphNodeObject->node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0
        && (gCurGraphNodeObject->node.flags & GRAPH_RENDER_BILLBOARD) == 0;
}

static void geo_reanchor_anim_child_to_root(Mat4 childMtx, Mat4 localMtx, Mat4 rootMtx) {
    f32 localX = localMtx[3][0];
    f32 localY = localMtx[3][1];
    f32 localZ = localMtx[3][2];
    s32 i;

    if (rootMtx != NULL && geo_matrix_has_valid_basis(rootMtx) && geo_matrix_has_valid_basis(localMtx)) {
        mtxf_mul(childMtx, localMtx, rootMtx);
        if (!geo_matrix_position_far_from_root(childMtx, rootMtx)) {
            return;
        }
    }
    if (rootMtx != NULL && geo_matrix_has_valid_basis(rootMtx)) {
        if (geo_matrix_has_valid_basis(localMtx)) {
            geo_rebuild_child_transform(childMtx, localMtx, rootMtx);
        } else {
            for (i = 0; i < 3; i++) {
                childMtx[i][0] = rootMtx[i][0];
                childMtx[i][1] = rootMtx[i][1];
                childMtx[i][2] = rootMtx[i][2];
                childMtx[i][3] = 0.0f;
            }
            childMtx[3][3] = 1.0f;
        }
    } else if (geo_matrix_has_valid_basis(localMtx)) {
        for (i = 0; i < 3; i++) {
            childMtx[i][0] = localMtx[i][0];
            childMtx[i][1] = localMtx[i][1];
            childMtx[i][2] = localMtx[i][2];
            childMtx[i][3] = 0.0f;
        }
        childMtx[3][3] = 1.0f;
    } else {
        geo_repair_matrix_basis_keep_position(childMtx, rootMtx);
    }
    if (rootMtx != NULL &&
        localX > -4000.0f && localX < 4000.0f &&
        localY > -4000.0f && localY < 4000.0f &&
        localZ > -4000.0f && localZ < 4000.0f) {
        geo_set_anchored_translation(childMtx, rootMtx, localX, localY, localZ);
    }
}

static void geo_reanchor_target_anim_child(Mat4 childMtx, Mat4 localMtx, Mat4 parentMtx, Mat4 rootMtx) {
    f32 localX = localMtx[3][0];
    f32 localY = localMtx[3][1];
    f32 localZ = localMtx[3][2];
    Mat4 anchorMtx;
    s32 i;

    if (geo_matrix_has_valid_basis(parentMtx) && !geo_matrix_position_far_from_root(parentMtx, rootMtx)) {
        mtxf_copy(anchorMtx, parentMtx);
    } else {
        mtxf_copy(anchorMtx, rootMtx);
    }

    if (geo_matrix_has_valid_basis(anchorMtx) && geo_matrix_has_valid_basis(localMtx)) {
        mtxf_mul(childMtx, localMtx, anchorMtx);
        if (geo_matrix_has_valid_basis(childMtx)
            && !geo_matrix_position_far_from_root(childMtx, rootMtx)
            && !geo_matrix_position_far_from_anchor(childMtx, anchorMtx)) {
            return;
        }
    }

    if (geo_matrix_has_valid_basis(anchorMtx) && geo_matrix_has_valid_basis(localMtx)) {
        geo_rebuild_child_transform(childMtx, localMtx, anchorMtx);
        return;
    }

    for (i = 0; i < 3; i++) {
        childMtx[i][0] = anchorMtx[i][0];
        childMtx[i][1] = anchorMtx[i][1];
        childMtx[i][2] = anchorMtx[i][2];
        childMtx[i][3] = 0.0f;
    }
    childMtx[3][3] = 1.0f;
    if (localX > -4000.0f && localX < 4000.0f &&
        localY > -4000.0f && localY < 4000.0f &&
        localZ > -4000.0f && localZ < 4000.0f) {
        geo_set_anchored_translation(childMtx, anchorMtx, localX, localY, localZ);
    } else {
        childMtx[3][0] = anchorMtx[3][0];
        childMtx[3][1] = anchorMtx[3][1];
        childMtx[3][2] = anchorMtx[3][2];
    }
}

static void geo_build_object_transform_no_camera(Mat4 dest, struct GraphNodeObject *node) {
    Mat4 mtxf;

    mtxf_rotate_zxy_and_translate(mtxf, node->pos, node->angle);
    mtxf_scale_vec3f(dest, mtxf, node->scale);
}

/* Rendermode settings for cycle 1 for all 8 layers. */
struct RenderModeContainer renderModeTable_1Cycle[2] = { { {
    G_RM_OPA_SURF,
    G_RM_AA_OPA_SURF,
    G_RM_AA_OPA_SURF,
    G_RM_AA_OPA_SURF,
    G_RM_AA_TEX_EDGE,
    G_RM_AA_XLU_SURF,
    G_RM_AA_XLU_SURF,
    G_RM_AA_XLU_SURF,
    } },
    { {
    /* z-buffered */
    G_RM_ZB_OPA_SURF,
    G_RM_AA_ZB_OPA_SURF,
    G_RM_AA_ZB_OPA_DECAL,
    G_RM_AA_ZB_OPA_INTER,
    G_RM_AA_ZB_TEX_EDGE,
    G_RM_AA_ZB_XLU_SURF,
    G_RM_AA_ZB_XLU_DECAL,
    G_RM_AA_ZB_XLU_INTER,
    } } };

/* Rendermode settings for cycle 2 for all 8 layers. */
struct RenderModeContainer renderModeTable_2Cycle[2] = { { {
    G_RM_OPA_SURF2,
    G_RM_AA_OPA_SURF2,
    G_RM_AA_OPA_SURF2,
    G_RM_AA_OPA_SURF2,
    G_RM_AA_TEX_EDGE2,
    G_RM_AA_XLU_SURF2,
    G_RM_AA_XLU_SURF2,
    G_RM_AA_XLU_SURF2,
    } },
    { {
    /* z-buffered */
    G_RM_ZB_OPA_SURF2,
    G_RM_AA_ZB_OPA_SURF2,
    G_RM_AA_ZB_OPA_DECAL2,
    G_RM_AA_ZB_OPA_INTER2,
    G_RM_AA_ZB_TEX_EDGE2,
    G_RM_AA_ZB_XLU_SURF2,
    G_RM_AA_ZB_XLU_DECAL2,
    G_RM_AA_ZB_XLU_INTER2,
    } } };

struct GraphNodeRoot *gCurGraphNodeRoot = NULL;
struct GraphNodeMasterList *gCurGraphNodeMasterList = NULL;
struct GraphNodePerspective *gCurGraphNodeCamFrustum = NULL;
struct GraphNodeCamera *gCurGraphNodeCamera = NULL;
struct GraphNodeObject *gCurGraphNodeObject = NULL;
struct GraphNodeHeldObject *gCurGraphNodeHeldObject = NULL;
u16 gAreaUpdateCounter = 0;

static s32 geo_is_file_select_camera(void) {
    return gCurGraphNodeRoot != NULL
        && gCurGraphNodeRoot->areaIndex == 1
        && gCurGraphNodeCamera != NULL
        && gCurGraphNodeCamera->pos[0] == 0
        && gCurGraphNodeCamera->pos[1] == 0
        && gCurGraphNodeCamera->pos[2] == 1000
        && gCurGraphNodeCamera->focus[0] == 0
        && gCurGraphNodeCamera->focus[1] == 0
        && gCurGraphNodeCamera->focus[2] == 0;
}

static void geo_build_object_transform_from_camera(Mat4 dest, struct GraphNodeObject *node, Mat4 cameraMtx) {
    Mat4 mtxf;
    s32 i;

    if (node->node.flags & GRAPH_RENDER_BILLBOARD) {
        mtxf_billboard(dest, cameraMtx, node->pos, gCurGraphNodeCamera->roll);
    } else {
        mtxf_rotate_zxy_and_translate(mtxf, node->pos, node->angle);
        for (i = 0; i < 3; i++) {
            dest[i][0] = mtxf[i][0] * cameraMtx[0][0] + mtxf[i][1] * cameraMtx[1][0] + mtxf[i][2] * cameraMtx[2][0];
            dest[i][1] = mtxf[i][0] * cameraMtx[0][1] + mtxf[i][1] * cameraMtx[1][1] + mtxf[i][2] * cameraMtx[2][1];
            dest[i][2] = mtxf[i][0] * cameraMtx[0][2] + mtxf[i][1] * cameraMtx[1][2] + mtxf[i][2] * cameraMtx[2][2];
            dest[i][3] = 0.0f;
        }
        dest[3][0] = node->pos[0] * cameraMtx[0][0] + node->pos[1] * cameraMtx[1][0] +
                     node->pos[2] * cameraMtx[2][0] + cameraMtx[3][0];
        dest[3][1] = node->pos[0] * cameraMtx[0][1] + node->pos[1] * cameraMtx[1][1] +
                     node->pos[2] * cameraMtx[2][1] + cameraMtx[3][1];
        dest[3][2] = node->pos[0] * cameraMtx[0][2] + node->pos[1] * cameraMtx[1][2] +
                     node->pos[2] * cameraMtx[2][2] + cameraMtx[3][2];
        dest[3][3] = 1.0f;
    }
    mtxf_scale_vec3f(dest, dest, node->scale);
}

#ifdef F3DEX_GBI_2
LookAt lookAt;
#endif

/**
 * Process a master list node.
 */
static void geo_process_master_list_sub(struct GraphNodeMasterList *node) {
    struct DisplayListNode *currList;
    s32 i;
    s32 enableZBuffer = (node->node.flags & GRAPH_RENDER_Z_BUFFER) != 0;
    struct RenderModeContainer *modeList = &renderModeTable_1Cycle[enableZBuffer];
    struct RenderModeContainer *mode2List = &renderModeTable_2Cycle[enableZBuffer];

    // @bug This is where the LookAt values should be calculated but aren't.
    // As a result, environment mapping is broken on Fast3DEX2 without the
    // changes below.
#ifdef F3DEX_GBI_2
    Mtx lMtx;
    guLookAtReflect(&lMtx, &lookAt, 0, 0, 0, /* eye */ 0, 0, 1, /* at */ 1, 0, 0 /* up */);
#endif

    if (enableZBuffer != 0) {
        gDPPipeSync(gDisplayListHead++);
        gSPSetGeometryMode(gDisplayListHead++, G_ZBUFFER);
    }

    for (i = 0; i < GFX_NUM_MASTER_LISTS; i++) {
        if ((currList = node->listHeads[i]) != NULL) {
            gDPSetRenderMode(gDisplayListHead++, modeList->modes[i], mode2List->modes[i]);
            while (currList != NULL) {
                gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(currList->transform),
                          G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                gSPDisplayList(gDisplayListHead++, currList->displayList);
                currList = currList->next;
            }
        }
    }
    if (enableZBuffer != 0) {
        gDPPipeSync(gDisplayListHead++);
        gSPClearGeometryMode(gDisplayListHead++, G_ZBUFFER);
    }
}

/**
 * Appends the display list to one of the master lists based on the layer
 * parameter. Look at the RenderModeContainer struct to see the corresponding
 * render modes of layers.
 */
static void geo_append_display_list(void *displayList, s16 layer) {

#ifdef F3DEX_GBI_2
    gSPLookAt(gDisplayListHead++, &lookAt);
#endif
    if (gCurGraphNodeMasterList != 0) {
        if (geo_is_raw_fallback_character_child()) {
            if (geo_is_character_like_model(sGeoCurrentObjectModelId)) {
                if (sGeoCharacterChildDrawDiagCount < GEO_CHARACTER_CHILD_DRAW_DIAG_LIMIT) {
                    RG_LOGI("geo_char_child_draw[%d]: model=%d/%s beh=%s type=%d row=%d,%d,%d root=%d,%d,%d rawfb=%d",
                            (int) sGeoCharacterChildDrawDiagCount,
                            (int) sGeoCurrentObjectModelId,
                            geo_model_diag_name(sGeoCurrentObjectModelId),
                            geo_behavior_diag_name(sGeoCurrentObjectBehavior),
                            (int) geo_current_shared_child_type(),
                            (int) gMatStack[gMatStackIndex][3][0],
                            (int) gMatStack[gMatStackIndex][3][1],
                            (int) gMatStack[gMatStackIndex][3][2],
                            (int) sGeoCurrentObjectRootMatrix[3][0],
                            (int) sGeoCurrentObjectRootMatrix[3][1],
                            (int) sGeoCurrentObjectRootMatrix[3][2],
                            (int) sGeoCurrentObjectUsedRawCameraFallback);
                    sGeoCharacterChildDrawDiagCount++;
                }
            } else {
                if (sGeoAnimDisplaySkipDiagCount < GEO_ANIM_DISPLAY_SKIP_DIAG_LIMIT) {
                    RG_LOGI("geo_anim_display_skip[%d]: model=%d/%s beh=%s type=%d row=%d,%d,%d root=%d,%d,%d rawfb=%d",
                            (int) sGeoAnimDisplaySkipDiagCount,
                            (int) sGeoCurrentObjectModelId,
                            geo_model_diag_name(sGeoCurrentObjectModelId),
                            geo_behavior_diag_name(sGeoCurrentObjectBehavior),
                            (int) geo_current_shared_child_type(),
                            (int) gMatStack[gMatStackIndex][3][0],
                            (int) gMatStack[gMatStackIndex][3][1],
                            (int) gMatStack[gMatStackIndex][3][2],
                            (int) sGeoCurrentObjectRootMatrix[3][0],
                            (int) sGeoCurrentObjectRootMatrix[3][1],
                            (int) sGeoCurrentObjectRootMatrix[3][2],
                            (int) sGeoCurrentObjectUsedRawCameraFallback);
                    sGeoAnimDisplaySkipDiagCount++;
                }
                return;
            }
        }
        if (geo_is_raw_fallback_character_root()
            && sGeoRawFallbackRootDrawDiagCount < GEO_RAW_FALLBACK_ROOT_DRAW_DIAG_LIMIT) {
            RG_LOGI("geo_raw_root_draw[%d]: model=%d/%s beh=%s type=%d behp=%p bp=%08x bp2=%d anim=%d row=%d,%d,%d root=%d,%d,%d",
                    (int) sGeoRawFallbackRootDrawDiagCount,
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    geo_behavior_diag_name(sGeoCurrentObjectBehavior),
                    (int) geo_current_shared_child_type(),
                    (void *) sGeoCurrentObjectBehavior,
                    (unsigned int) sGeoCurrentObjectBehParams,
                    (int) sGeoCurrentObjectBehParam2,
                    (int) sGeoCurrentObjectAnimId,
                    (int) gMatStack[gMatStackIndex][3][0],
                    (int) gMatStack[gMatStackIndex][3][1],
                    (int) gMatStack[gMatStackIndex][3][2],
                    (int) sGeoCurrentObjectRootMatrix[3][0],
                    (int) sGeoCurrentObjectRootMatrix[3][1],
                    (int) sGeoCurrentObjectRootMatrix[3][2]);
            sGeoRawFallbackRootDrawDiagCount++;
        }
#if GEO_RENDERER_CHARACTER_MODEL_DIAG_ENABLED
        if (gCurGraphNodeObject != NULL &&
            gMatStackIndex > 1 &&
            gCurGraphNodeObject->sharedChild != NULL &&
            geo_should_probe_renderer_object() &&
            (gCurGraphNodeObject->node.flags & GRAPH_RENDER_BILLBOARD) == 0) {
            gfx_debug_arm_character_model(gCurGraphNodeObject->sharedChild != NULL
                                              ? gCurGraphNodeObject->sharedChild->type
                                              : -1,
                                          (s32) gMatStack[gMatStackIndex][3][0],
                                          (s32) gMatStack[gMatStackIndex][3][1],
                                          (s32) gMatStack[gMatStackIndex][3][2]);
        }
#endif
        struct DisplayListNode *listNode =
            alloc_only_pool_alloc(gDisplayListHeap, sizeof(struct DisplayListNode));
        Mtx *queuedTransform = NULL;

        if (geo_should_probe_renderer_object()) {
            queuedTransform = alloc_display_list(sizeof(*queuedTransform));
        }
        if (queuedTransform != NULL) {
            mtxf_to_mtx(queuedTransform, gMatStack[gMatStackIndex]);
            listNode->transform = queuedTransform;
            if (sGeoDisplayMatrixRefreshDiagCount < GEO_DISPLAY_MATRIX_REFRESH_DIAG_LIMIT) {
                RG_LOGI("geo_dl_mtx_refresh[%d]: model=%d/%s beh=%s type=%d row=%d,%d,%d stack=%d",
                        (int) sGeoDisplayMatrixRefreshDiagCount,
                        (int) sGeoCurrentObjectModelId,
                        geo_model_diag_name(sGeoCurrentObjectModelId),
                        geo_behavior_diag_name(sGeoCurrentObjectBehavior),
                        (int) geo_current_shared_child_type(),
                        (int) gMatStack[gMatStackIndex][3][0],
                        (int) gMatStack[gMatStackIndex][3][1],
                        (int) gMatStack[gMatStackIndex][3][2],
                        (int) gMatStackIndex);
                sGeoDisplayMatrixRefreshDiagCount++;
            }
        } else {
            listNode->transform = gMatStackFixed[gMatStackIndex];
        }
        listNode->displayList = displayList;
        listNode->next = 0;
        if (gCurGraphNodeMasterList->listHeads[layer] == 0) {
            gCurGraphNodeMasterList->listHeads[layer] = listNode;
        } else {
            gCurGraphNodeMasterList->listTails[layer]->next = listNode;
        }
        gCurGraphNodeMasterList->listTails[layer] = listNode;
    }
}

/**
 * Process the master list node.
 */
static void geo_process_master_list(struct GraphNodeMasterList *node) {
    s32 i;
    UNUSED s32 sp1C;

    if (gCurGraphNodeMasterList == NULL && node->node.children != NULL) {
        gCurGraphNodeMasterList = node;
        for (i = 0; i < GFX_NUM_MASTER_LISTS; i++) {
            node->listHeads[i] = NULL;
        }
        geo_process_node_and_siblings(node->node.children);
        geo_process_master_list_sub(node);
        gCurGraphNodeMasterList = NULL;
    }
}

/**
 * Process an orthographic projection node.
 */
static void geo_process_ortho_projection(struct GraphNodeOrthoProjection *node) {
    if (node->node.children != NULL) {
        Mtx *mtx = alloc_display_list(sizeof(*mtx));
        f32 left = (gCurGraphNodeRoot->x - gCurGraphNodeRoot->width) / 2.0f * node->scale;
        f32 right = (gCurGraphNodeRoot->x + gCurGraphNodeRoot->width) / 2.0f * node->scale;
        f32 top = (gCurGraphNodeRoot->y - gCurGraphNodeRoot->height) / 2.0f * node->scale;
        f32 bottom = (gCurGraphNodeRoot->y + gCurGraphNodeRoot->height) / 2.0f * node->scale;

        guOrtho(mtx, left, right, bottom, top, -2.0f, 2.0f, 1.0f);
        gSPPerspNormalize(gDisplayListHead++, 0xFFFF);
        gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(mtx), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);

        geo_process_node_and_siblings(node->node.children);
    }
}

/**
 * Process a perspective projection node.
 */
static void geo_process_perspective(struct GraphNodePerspective *node) {
    if (node->fnNode.func != NULL) {
        node->fnNode.func(GEO_CONTEXT_RENDER, &node->fnNode.node, gMatStack[gMatStackIndex]);
    }
    if (node->fnNode.node.children != NULL) {
        u16 perspNorm;
        Mtx *mtx = alloc_display_list(sizeof(*mtx));

#ifdef VERSION_EU
        f32 aspect = ((f32) gCurGraphNodeRoot->width / (f32) gCurGraphNodeRoot->height) * 1.1f;
#else
        f32 aspect = (f32) gCurGraphNodeRoot->width / (f32) gCurGraphNodeRoot->height;
#endif

        guPerspective(mtx, &perspNorm, node->fov, aspect, node->near, node->far, 1.0f);
        gSPPerspNormalize(gDisplayListHead++, perspNorm);

        gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(mtx), G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);

        gCurGraphNodeCamFrustum = node;
        geo_process_node_and_siblings(node->fnNode.node.children);
        gCurGraphNodeCamFrustum = NULL;
    }
}

/**
 * Process a level of detail node. From the current transformation matrix,
 * the perpendicular distance to the camera is extracted and the children
 * of this node are only processed if that distance is within the render
 * range of this node.
 */
static void geo_process_level_of_detail(struct GraphNodeLevelOfDetail *node) {
#ifdef GBI_FLOATS
    Mtx *mtx = gMatStackFixed[gMatStackIndex];
    s16 distanceFromCam = (s32) -mtx->m[3][2]; // z-component of the translation column
#else
    // The fixed point Mtx type is defined as 16 longs, but it's actually 16
    // shorts for the integer parts followed by 16 shorts for the fraction parts
    Mtx *mtx = gMatStackFixed[gMatStackIndex];
    s16 distanceFromCam = -GET_HIGH_S16_OF_32(mtx->m[1][3]); // z-component of the translation column
#endif

#ifndef TARGET_N64
    // We assume modern hardware is powerful enough to draw the most detailed variant
    distanceFromCam = 0;
#endif

    if (node->minDistance <= distanceFromCam && distanceFromCam < node->maxDistance) {
        if (node->node.children != 0) {
            geo_process_node_and_siblings(node->node.children);
        }
    }
}

/**
 * Process a switch case node. The node's selection function is called
 * if it is 0, and among the node's children, only the selected child is
 * processed next.
 */
static void geo_process_switch(struct GraphNodeSwitchCase *node) {
    struct GraphNode *selectedChild = node->fnNode.node.children;
    s32 i;

    if (node->fnNode.func != NULL) {
        node->fnNode.func(GEO_CONTEXT_RENDER, &node->fnNode.node, gMatStack[gMatStackIndex]);
    }
    for (i = 0; selectedChild != NULL && node->selectedCase > i; i++) {
        selectedChild = selectedChild->next;
    }
    if (selectedChild != NULL) {
        geo_process_node_and_siblings(selectedChild);
    }
}

/**
 * Process a camera node.
 */
static void geo_process_camera(struct GraphNodeCamera *node) {
    Mat4 cameraTransform;
    Mtx *rollMtx = alloc_display_list(sizeof(*rollMtx));
    Mtx *mtx = alloc_display_list(sizeof(*mtx));
    s32 usedFallback;

    if (node->fnNode.func != NULL) {
        node->fnNode.func(GEO_CONTEXT_RENDER, &node->fnNode.node, gMatStack[gMatStackIndex]);
    }
    mtxf_rotate_xy(rollMtx, node->rollScreen);

    gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(rollMtx), G_MTX_PROJECTION | G_MTX_MUL | G_MTX_NOPUSH);

    mtxf_lookat(cameraTransform, node->pos, node->focus, node->roll);
    mtxf_copy(sGeoRawCameraMatrix, cameraTransform);
    sGeoRawCameraMatrixValid = geo_matrix_has_valid_basis(sGeoRawCameraMatrix);
    mtxf_mul(gMatStack[gMatStackIndex + 1], cameraTransform, gMatStack[gMatStackIndex]);
    usedFallback = 0;
    if (!geo_matrix_has_valid_basis(gMatStack[gMatStackIndex + 1])) {
        mtxf_copy(gMatStack[gMatStackIndex + 1], cameraTransform);
        usedFallback = 1;
    }
    gMatStackIndex++;
    mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
    gMatStackFixed[gMatStackIndex] = mtx;
    if (node->fnNode.node.children != 0) {
        gCurGraphNodeCamera = node;
        node->matrixPtr = &gMatStack[gMatStackIndex];
        if (GEO_MENU_OBJECT_DIAG && usedFallback && geo_is_file_select_camera()
            && sGeoMenuCameraDiagCount < GEO_MENU_CAMERA_DIAG_LIMIT) {
            RG_LOGI("geo_menu_camera[%d]: fallback=1 pos=%.1f,%.1f,%.1f focus=%.1f,%.1f,%.1f row3=%.1f,%.1f,%.1f,%.1f",
                    (int) sGeoMenuCameraDiagCount,
                    (double) node->pos[0], (double) node->pos[1], (double) node->pos[2],
                    (double) node->focus[0], (double) node->focus[1], (double) node->focus[2],
                    (double) gMatStack[gMatStackIndex][3][0],
                    (double) gMatStack[gMatStackIndex][3][1],
                    (double) gMatStack[gMatStackIndex][3][2],
                    (double) gMatStack[gMatStackIndex][3][3]);
            sGeoMenuCameraDiagCount++;
        }
        geo_process_node_and_siblings(node->fnNode.node.children);
        gCurGraphNodeCamera = NULL;
        sGeoRawCameraMatrixValid = FALSE;
    }
    gMatStackIndex--;
}

/**
 * Process a translation / rotation node. A transformation matrix based
 * on the node's translation and rotation is created and pushed on both
 * the float and fixed point matrix stacks.
 * For the rest it acts as a normal display list node.
 */
static void geo_process_translation_rotation(struct GraphNodeTranslationRotation *node) {
    Mat4 mtxf;
    Vec3f translation;
    Mtx *mtx = alloc_display_list(sizeof(*mtx));

    vec3s_to_vec3f(translation, node->translation);
    mtxf_rotate_zxy_and_translate(mtxf, translation, node->rotation);
    mtxf_mul(gMatStack[gMatStackIndex + 1], mtxf, gMatStack[gMatStackIndex]);
    geo_repair_target_node_transform(gMatStack[gMatStackIndex + 1], mtxf, "tr");
    gMatStackIndex++;
    mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
    gMatStackFixed[gMatStackIndex] = mtx;
    if (node->displayList != NULL) {
        geo_append_display_list(node->displayList, node->node.flags >> 8);
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
    gMatStackIndex--;
}

/**
 * Process a translation node. A transformation matrix based on the node's
 * translation is created and pushed on both the float and fixed point matrix stacks.
 * For the rest it acts as a normal display list node.
 */
static void geo_process_translation(struct GraphNodeTranslation *node) {
    Mat4 mtxf;
    Vec3f translation;
    Mtx *mtx = alloc_display_list(sizeof(*mtx));

    vec3s_to_vec3f(translation, node->translation);
    mtxf_rotate_zxy_and_translate(mtxf, translation, gVec3sZero);
    mtxf_mul(gMatStack[gMatStackIndex + 1], mtxf, gMatStack[gMatStackIndex]);
    geo_repair_target_node_transform(gMatStack[gMatStackIndex + 1], mtxf, "trans");
    gMatStackIndex++;
    mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
    gMatStackFixed[gMatStackIndex] = mtx;
    if (node->displayList != NULL) {
        geo_append_display_list(node->displayList, node->node.flags >> 8);
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
    gMatStackIndex--;
}

/**
 * Process a rotation node. A transformation matrix based on the node's
 * rotation is created and pushed on both the float and fixed point matrix stacks.
 * For the rest it acts as a normal display list node.
 */
static void geo_process_rotation(struct GraphNodeRotation *node) {
    Mat4 mtxf;
    Mtx *mtx = alloc_display_list(sizeof(*mtx));

    mtxf_rotate_zxy_and_translate(mtxf, gVec3fZero, node->rotation);
    mtxf_mul(gMatStack[gMatStackIndex + 1], mtxf, gMatStack[gMatStackIndex]);
    geo_repair_target_node_transform(gMatStack[gMatStackIndex + 1], mtxf, "rot");
    gMatStackIndex++;
    mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
    gMatStackFixed[gMatStackIndex] = mtx;
    if (node->displayList != NULL) {
        geo_append_display_list(node->displayList, node->node.flags >> 8);
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
    gMatStackIndex--;
}

/**
 * Process a scaling node. A transformation matrix based on the node's
 * scale is created and pushed on both the float and fixed point matrix stacks.
 * For the rest it acts as a normal display list node.
 */
static void geo_process_scale(struct GraphNodeScale *node) {
    UNUSED Mat4 transform;
    Vec3f scaleVec;
    Mtx *mtx = alloc_display_list(sizeof(*mtx));

    vec3f_set(scaleVec, node->scale, node->scale, node->scale);
    mtxf_scale_vec3f(gMatStack[gMatStackIndex + 1], gMatStack[gMatStackIndex], scaleVec);
    geo_repair_target_scale_transform(gMatStack[gMatStackIndex + 1], node->scale);
    gMatStackIndex++;
    mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
    gMatStackFixed[gMatStackIndex] = mtx;
    if (node->displayList != NULL) {
        geo_append_display_list(node->displayList, node->node.flags >> 8);
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
    gMatStackIndex--;
}

/**
 * Process a billboard node. A transformation matrix is created that makes its
 * children face the camera, and it is pushed on the floating point and fixed
 * point matrix stacks.
 * For the rest it acts as a normal display list node.
 */
static void geo_process_billboard(struct GraphNodeBillboard *node) {
    Vec3f translation;
    Mtx *mtx = alloc_display_list(sizeof(*mtx));

    gMatStackIndex++;
    vec3s_to_vec3f(translation, node->translation);
    mtxf_billboard(gMatStack[gMatStackIndex], gMatStack[gMatStackIndex - 1], translation,
                   gCurGraphNodeCamera->roll);
    if (gCurGraphNodeHeldObject != NULL) {
        mtxf_scale_vec3f(gMatStack[gMatStackIndex], gMatStack[gMatStackIndex],
                         gCurGraphNodeHeldObject->objNode->header.gfx.scale);
    } else if (gCurGraphNodeObject != NULL) {
        mtxf_scale_vec3f(gMatStack[gMatStackIndex], gMatStack[gMatStackIndex],
                         gCurGraphNodeObject->scale);
    }

    mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
    gMatStackFixed[gMatStackIndex] = mtx;
    if (node->displayList != NULL) {
        geo_append_display_list(node->displayList, node->node.flags >> 8);
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
    gMatStackIndex--;
}

/**
 * Process a display list node. It draws a display list without first pushing
 * a transformation on the stack, so all transformations are inherited from the
 * parent node. It processes its children if it has them.
 */
static void geo_process_display_list(struct GraphNodeDisplayList *node) {
    if (node->displayList != NULL) {
        geo_append_display_list(node->displayList, node->node.flags >> 8);
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
}

/**
 * Process a generated list. Instead of storing a pointer to a display list,
 * the list is generated on the fly by a function.
 */
static void geo_process_generated_list(struct GraphNodeGenerated *node) {
    if (node->fnNode.func != NULL) {
        Gfx *list = node->fnNode.func(GEO_CONTEXT_RENDER, &node->fnNode.node,
                                     (struct AllocOnlyPool *) gMatStack[gMatStackIndex]);

        if (list != 0) {
            Mtx *freshTransform = alloc_display_list(sizeof(*freshTransform));
            Mtx *savedTransform = gMatStackFixed[gMatStackIndex];
            if (freshTransform != NULL) {
                mtxf_to_mtx(freshTransform, gMatStack[gMatStackIndex]);
                gMatStackFixed[gMatStackIndex] = freshTransform;
            }
            geo_append_display_list((void *) VIRTUAL_TO_PHYSICAL(list), node->fnNode.node.flags >> 8);
            gMatStackFixed[gMatStackIndex] = savedTransform;
        }
    }
    if (node->fnNode.node.children != NULL) {
        geo_process_node_and_siblings(node->fnNode.node.children);
    }
}

/**
 * Process a background node. Tries to retrieve a background display list from
 * the function of the node. If that function is null or returns null, a black
 * rectangle is drawn instead.
 */
static void geo_process_background(struct GraphNodeBackground *node) {
    Gfx *list = NULL;

    if (node->fnNode.func != NULL) {
        list = node->fnNode.func(GEO_CONTEXT_RENDER, &node->fnNode.node,
                                 (struct AllocOnlyPool *) gMatStack[gMatStackIndex]);
    }
    if (list != 0) {
        geo_append_display_list((void *) VIRTUAL_TO_PHYSICAL(list), node->fnNode.node.flags >> 8);
    } else if (gCurGraphNodeMasterList != NULL) {
#ifndef F3DEX_GBI_2E
        Gfx *gfxStart = alloc_display_list(sizeof(Gfx) * 7);
#else
        Gfx *gfxStart = alloc_display_list(sizeof(Gfx) * 8);
#endif
        Gfx *gfx = gfxStart;

        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_FILL);
        gDPSetFillColor(gfx++, node->background);
        gDPFillRectangle(gfx++, GFX_DIMENSIONS_RECT_FROM_LEFT_EDGE(0), BORDER_HEIGHT,
        GFX_DIMENSIONS_RECT_FROM_RIGHT_EDGE(0) - 1, SCREEN_HEIGHT - BORDER_HEIGHT - 1);
        gDPPipeSync(gfx++);
        gDPSetCycleType(gfx++, G_CYC_1CYCLE);
        gSPEndDisplayList(gfx++);

        geo_append_display_list((void *) VIRTUAL_TO_PHYSICAL(gfxStart), 0);
    }
    if (node->fnNode.node.children != NULL) {
        geo_process_node_and_siblings(node->fnNode.node.children);
    }
}

/**
 * Render an animated part. The current animation state is not part of the node
 * but set in global variables. If an animated part is skipped, everything afterwards desyncs.
 */
static void geo_process_animated_part(struct GraphNodeAnimatedPart *node) {
    Mat4 matrix;
    Vec3s rotation;
    Vec3f translation;
    Vec3f baseTranslation;
    s32 animTypeBefore = gCurAnimType;
    Mtx *matrixPtr = alloc_display_list(sizeof(*matrixPtr));

    vec3s_copy(rotation, gVec3sZero);
    vec3f_set(translation, node->translation[0], node->translation[1], node->translation[2]);
    vec3f_copy(baseTranslation, translation);
    if (gCurAnimType == ANIM_TYPE_TRANSLATION) {
        translation[0] += gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)]
                          * gCurAnimTranslationMultiplier;
        translation[1] += gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)]
                          * gCurAnimTranslationMultiplier;
        translation[2] += gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)]
                          * gCurAnimTranslationMultiplier;
        gCurAnimType = ANIM_TYPE_ROTATION;
    } else {
        if (gCurAnimType == ANIM_TYPE_LATERAL_TRANSLATION) {
            translation[0] +=
                gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)]
                * gCurAnimTranslationMultiplier;
            gCurrAnimAttribute += 2;
            translation[2] +=
                gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)]
                * gCurAnimTranslationMultiplier;
            gCurAnimType = ANIM_TYPE_ROTATION;
        } else {
            if (gCurAnimType == ANIM_TYPE_VERTICAL_TRANSLATION) {
                gCurrAnimAttribute += 2;
                translation[1] +=
                    gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)]
                    * gCurAnimTranslationMultiplier;
                gCurrAnimAttribute += 2;
                gCurAnimType = ANIM_TYPE_ROTATION;
            } else if (gCurAnimType == ANIM_TYPE_NO_TRANSLATION) {
                gCurrAnimAttribute += 6;
                gCurAnimType = ANIM_TYPE_ROTATION;
            }
        }
    }

    if (gCurAnimType == ANIM_TYPE_ROTATION) {
        rotation[0] = gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)];
        rotation[1] = gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)];
        rotation[2] = gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)];
    }
    if (gCurrLevelNum == LEVEL_CASTLE_GROUNDS
        && geo_current_shared_child_type() == 47
        && sGeoCurrentObjectUsedRawCameraFallback
        && (gCurGraphNodeObject->node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0
        && geo_matrix_basis_needs_repair(gMatStack[gMatStackIndex])) {
        f32 basisBefore = geo_matrix_basis_sum(gMatStack[gMatStackIndex]);
        Mat4 *rootMtx = sGeoCurrentObjectRootMatrixValid ? &sGeoCurrentObjectRootMatrix : gCurGraphNodeObject->throwMatrix;

        geo_repair_matrix_basis_keep_position(gMatStack[gMatStackIndex], rootMtx != NULL ? *rootMtx : NULL);
        if (sGeoAnimParentRepairDiagCount < GEO_ANIM_PARENT_REPAIR_DIAG_LIMIT) {
            RG_LOGI("geo_anim_parent_repair[%d]: model=%d/%s type=%d rawfb=%d basis=%.3e row=%d,%d,%d root=%d,%d,%d after=%.3e",
                    (int) sGeoAnimParentRepairDiagCount,
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    (int) geo_current_shared_child_type(),
                    (int) sGeoCurrentObjectUsedRawCameraFallback,
                    (double) basisBefore,
                    (int) gMatStack[gMatStackIndex][3][0],
                    (int) gMatStack[gMatStackIndex][3][1],
                    (int) gMatStack[gMatStackIndex][3][2],
                    rootMtx != NULL ? (int) (*rootMtx)[3][0] : 0,
                    rootMtx != NULL ? (int) (*rootMtx)[3][1] : 0,
                    rootMtx != NULL ? (int) (*rootMtx)[3][2] : 0,
                    (double) geo_matrix_basis_sum(gMatStack[gMatStackIndex]));
            sGeoAnimParentRepairDiagCount++;
        }
    }
    mtxf_rotate_xyz_and_translate(matrix, translation, rotation);
    mtxf_mul(gMatStack[gMatStackIndex + 1], matrix, gMatStack[gMatStackIndex]);
    if (geo_should_probe_renderer_object()
        && sGeoCurrentObjectRootMatrixValid
        && gCurGraphNodeObject != NULL
        && (gCurGraphNodeObject->node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0
        && (!geo_matrix_has_valid_basis(gMatStack[gMatStackIndex + 1])
            || geo_matrix_position_far_from_root(gMatStack[gMatStackIndex + 1],
                                                 sGeoCurrentObjectRootMatrix))) {
        s32 oldX = (s32) gMatStack[gMatStackIndex + 1][3][0];
        s32 oldY = (s32) gMatStack[gMatStackIndex + 1][3][1];
        s32 oldZ = (s32) gMatStack[gMatStackIndex + 1][3][2];
        f32 parentBasis = geo_matrix_basis_sum(gMatStack[gMatStackIndex]);
        f32 childBasis = geo_matrix_basis_sum(gMatStack[gMatStackIndex + 1]);

        geo_reanchor_target_anim_child(gMatStack[gMatStackIndex + 1], matrix,
                                       gMatStack[gMatStackIndex],
                                       sGeoCurrentObjectRootMatrix);
        if (sGeoTargetAnimRepairDiagCount < GEO_TARGET_ANIM_REPAIR_DIAG_LIMIT) {
            RG_LOGI("geo_target_anim_repair[%d]: model=%d/%s beh=%s type=%d pb=%.3e cb=%.3e anim=%d frame=%d local=%d,%d,%d rot=%d,%d,%d old=%d,%d,%d root=%d,%d,%d new=%d,%d,%d",
                    (int) sGeoTargetAnimRepairDiagCount,
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    geo_behavior_diag_name(sGeoCurrentObjectBehavior),
                    (int) geo_current_shared_child_type(),
                    (double) parentBasis,
                    (double) childBasis,
                    (int) animTypeBefore,
                    (int) gCurrAnimFrame,
                    (int) matrix[3][0],
                    (int) matrix[3][1],
                    (int) matrix[3][2],
                    (int) rotation[0],
                    (int) rotation[1],
                    (int) rotation[2],
                    (int) oldX,
                    (int) oldY,
                    (int) oldZ,
                    (int) sGeoCurrentObjectRootMatrix[3][0],
                    (int) sGeoCurrentObjectRootMatrix[3][1],
                    (int) sGeoCurrentObjectRootMatrix[3][2],
                    (int) gMatStack[gMatStackIndex + 1][3][0],
                    (int) gMatStack[gMatStackIndex + 1][3][1],
                    (int) gMatStack[gMatStackIndex + 1][3][2]);
            sGeoTargetAnimRepairDiagCount++;
        }
    }
    if (gCurrLevelNum == LEVEL_CASTLE_GROUNDS
        && geo_current_shared_child_type() == 47
        && sGeoCurrentObjectUsedRawCameraFallback
        && (gCurGraphNodeObject->node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0) {
        Mat4 *rootMtx = sGeoCurrentObjectRootMatrixValid ? &sGeoCurrentObjectRootMatrix : gCurGraphNodeObject->throwMatrix;

        if (rootMtx != NULL && geo_matrix_position_far_from_root(gMatStack[gMatStackIndex + 1], *rootMtx)) {
            f32 basisBefore = geo_matrix_basis_sum(gMatStack[gMatStackIndex + 1]);
            f32 parentBasis = geo_matrix_basis_sum(gMatStack[gMatStackIndex]);
            f32 localBasis = geo_matrix_basis_sum(matrix);
            s32 oldX = (s32) gMatStack[gMatStackIndex + 1][3][0];
            s32 oldY = (s32) gMatStack[gMatStackIndex + 1][3][1];
            s32 oldZ = (s32) gMatStack[gMatStackIndex + 1][3][2];

            geo_reanchor_anim_child_to_root(gMatStack[gMatStackIndex + 1], matrix, *rootMtx);
            if (sGeoAnimChildRepairDiagCount < GEO_ANIM_CHILD_REPAIR_DIAG_LIMIT) {
                RG_LOGI("geo_anim_child_repair[%d]: model=%d/%s type=%d rawfb=%d pb=%.3e lb=%.3e cb=%.3e old=%d,%d,%d root=%d,%d,%d local=%d,%d,%d new=%d,%d,%d after=%.3e",
                        (int) sGeoAnimChildRepairDiagCount,
                        (int) sGeoCurrentObjectModelId,
                        geo_model_diag_name(sGeoCurrentObjectModelId),
                        (int) geo_current_shared_child_type(),
                        (int) sGeoCurrentObjectUsedRawCameraFallback,
                        (double) parentBasis,
                        (double) localBasis,
                        (double) basisBefore,
                        (int) oldX,
                        (int) oldY,
                        (int) oldZ,
                        (int) (*rootMtx)[3][0],
                        (int) (*rootMtx)[3][1],
                        (int) (*rootMtx)[3][2],
                        (int) matrix[3][0],
                        (int) matrix[3][1],
                        (int) matrix[3][2],
                        (int) gMatStack[gMatStackIndex + 1][3][0],
                        (int) gMatStack[gMatStackIndex + 1][3][1],
                        (int) gMatStack[gMatStackIndex + 1][3][2],
                        (double) geo_matrix_basis_sum(gMatStack[gMatStackIndex + 1]));
                sGeoAnimChildRepairDiagCount++;
            }
        }
    }
    gMatStackIndex++;
    mtxf_to_mtx(matrixPtr, gMatStack[gMatStackIndex]);
    gMatStackFixed[gMatStackIndex] = matrixPtr;
    if (gCurrLevelNum == LEVEL_CASTLE_GROUNDS
        && geo_current_shared_child_type() == 47
        && (gCurGraphNodeObject->node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0
        && sGeoAnimPartDiagCount < GEO_ANIM_PART_DIAG_LIMIT) {
        RG_LOGI("geo_anim_part[%d]: model=%d/%s type=%d node=%04x anim=%d frame=%d mult=%d base=%d,%d,%d trans=%d,%d,%d rot=%d,%d,%d parent=%d,%d,%d row=%d,%d,%d",
                (int) sGeoAnimPartDiagCount,
                (int) sGeoCurrentObjectModelId,
                geo_model_diag_name(sGeoCurrentObjectModelId),
                (int) geo_current_shared_child_type(),
                (unsigned int) node->node.flags,
                (int) animTypeBefore,
                (int) gCurrAnimFrame,
                (int) (gCurAnimTranslationMultiplier * 1000.0f),
                (int) baseTranslation[0],
                (int) baseTranslation[1],
                (int) baseTranslation[2],
                (int) translation[0],
                (int) translation[1],
                (int) translation[2],
                (int) rotation[0],
                (int) rotation[1],
                (int) rotation[2],
                (int) gMatStack[gMatStackIndex - 1][3][0],
                (int) gMatStack[gMatStackIndex - 1][3][1],
                (int) gMatStack[gMatStackIndex - 1][3][2],
                (int) gMatStack[gMatStackIndex][3][0],
                (int) gMatStack[gMatStackIndex][3][1],
                (int) gMatStack[gMatStackIndex][3][2]);
        sGeoAnimPartDiagCount++;
    }
    if (node->displayList != NULL) {
        geo_append_display_list(node->displayList, node->node.flags >> 8);
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
    gMatStackIndex--;
}

/**
 * Initialize the animation-related global variables for the currently drawn
 * object's animation.
 */
void geo_set_animation_globals(struct GraphNodeObject_sub *node, s32 hasAnimation) {
    struct Animation *anim = node->curAnim;

    if (hasAnimation != 0) {
        node->animFrame = geo_update_animation_frame(node, &node->animFrameAccelAssist);
    }
    node->animTimer = gAreaUpdateCounter;
    if (anim->flags & ANIM_FLAG_HOR_TRANS) {
        gCurAnimType = ANIM_TYPE_VERTICAL_TRANSLATION;
    } else if (anim->flags & ANIM_FLAG_VERT_TRANS) {
        gCurAnimType = ANIM_TYPE_LATERAL_TRANSLATION;
    } else if (anim->flags & ANIM_FLAG_6) {
        gCurAnimType = ANIM_TYPE_NO_TRANSLATION;
    } else {
        gCurAnimType = ANIM_TYPE_TRANSLATION;
    }

    gCurrAnimFrame = node->animFrame;
    gCurAnimEnabled = (anim->flags & ANIM_FLAG_5) == 0;
    gCurrAnimAttribute = segmented_to_virtual((void *) anim->index);
    gCurAnimData = segmented_to_virtual((void *) anim->values);

    if (anim->unk02 == 0) {
        gCurAnimTranslationMultiplier = 1.0f;
    } else {
        gCurAnimTranslationMultiplier = (f32) node->animYTrans / (f32) anim->unk02;
    }
}

/**
 * Process a shadow node. Renders a shadow under an object offset by the
 * translation of the first animated component and rotated according to
 * the floor below it.
 */
static void geo_process_shadow(struct GraphNodeShadow *node) {
    Gfx *shadowList;
    Mat4 mtxf;
    Vec3f shadowPos;
    Vec3f animOffset;
    f32 objScale;
    f32 shadowScale;
    f32 sinAng;
    f32 cosAng;
    struct GraphNode *geo;
    Mtx *mtx;

    if (gCurGraphNodeCamera != NULL && gCurGraphNodeObject != NULL) {
        if (gCurGraphNodeHeldObject != NULL) {
            get_pos_from_transform_mtx(shadowPos, gMatStack[gMatStackIndex],
                                       *gCurGraphNodeCamera->matrixPtr);
            shadowScale = node->shadowScale;
        } else {
            vec3f_copy(shadowPos, gCurGraphNodeObject->pos);
            shadowScale = node->shadowScale * gCurGraphNodeObject->scale[0];
        }

        objScale = 1.0f;
        if (gCurAnimEnabled != 0) {
            if (gCurAnimType == ANIM_TYPE_TRANSLATION
                || gCurAnimType == ANIM_TYPE_LATERAL_TRANSLATION) {
                geo = node->node.children;
                if (geo != NULL && geo->type == GRAPH_NODE_TYPE_SCALE) {
                    objScale = ((struct GraphNodeScale *) geo)->scale;
                }
                animOffset[0] =
                    gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)]
                    * gCurAnimTranslationMultiplier * objScale;
                animOffset[1] = 0.0f;
                gCurrAnimAttribute += 2;
                animOffset[2] =
                    gCurAnimData[retrieve_animation_index(gCurrAnimFrame, &gCurrAnimAttribute)]
                    * gCurAnimTranslationMultiplier * objScale;
                gCurrAnimAttribute -= 6;

                // simple matrix rotation so the shadow offset rotates along with the object
                sinAng = sins(gCurGraphNodeObject->angle[1]);
                cosAng = coss(gCurGraphNodeObject->angle[1]);

                shadowPos[0] += animOffset[0] * cosAng + animOffset[2] * sinAng;
                shadowPos[2] += -animOffset[0] * sinAng + animOffset[2] * cosAng;
            }
        }

        shadowList = create_shadow_below_xyz(shadowPos[0], shadowPos[1], shadowPos[2], shadowScale,
                                             node->shadowSolidity, node->shadowType);
        if (shadowList != NULL) {
            mtx = alloc_display_list(sizeof(*mtx));
            gMatStackIndex++;
            mtxf_translate(mtxf, shadowPos);
            mtxf_mul(gMatStack[gMatStackIndex], mtxf, *gCurGraphNodeCamera->matrixPtr);
            mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
            gMatStackFixed[gMatStackIndex] = mtx;
            if (gShadowAboveWaterOrLava == 1) {
                geo_append_display_list((void *) VIRTUAL_TO_PHYSICAL(shadowList), 4);
            } else if (gMarioOnIceOrCarpet == 1) {
                geo_append_display_list((void *) VIRTUAL_TO_PHYSICAL(shadowList), 5);
            } else {
                geo_append_display_list((void *) VIRTUAL_TO_PHYSICAL(shadowList), 6);
            }
            gMatStackIndex--;
        }
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
}

/**
 * Check whether an object is in view to determine whether it should be drawn.
 * This is known as frustum culling.
 * It checks whether the object is far away, very close / behind the camera,
 * or horizontally out of view. It does not check whether it is vertically
 * out of view. It assumes a sphere of 300 units around the object's position
 * unless the object has a culling radius node that specifies otherwise.
 *
 * The matrix parameter should be the top of the matrix stack, which is the
 * object's transformation matrix times the camera 'look-at' matrix. The math
 * is counter-intuitive, but it checks column 3 (translation vector) of this
 * matrix to determine where the origin (0,0,0) in object space will be once
 * transformed to camera space (x+ = right, y+ = up, z = 'coming out the screen').
 * In 3D graphics, you typically model the world as being moved in front of a
 * static camera instead of a moving camera through a static world, which in
 * this case simplifies calculations. Note that the perspective matrix is not
 * on the matrix stack, so there are still calculations with the fov to compute
 * the slope of the lines of the frustum.
 *
 *        z-
 *
 *  \     |     /
 *   \    |    /
 *    \   |   /
 *     \  |  /
 *      \ | /
 *       \|/
 *        C       x+
 *
 * Since (0,0,0) is unaffected by rotation, columns 0, 1 and 2 are ignored.
 */
static int obj_is_in_view(struct GraphNodeObject *node, Mat4 matrix) {
    s16 cullingRadius;
    s16 halfFov; // half of the fov in in-game angle units instead of degrees
    struct GraphNode *geo;
    f32 hScreenEdge;

    if (node->node.flags & GRAPH_RENDER_INVISIBLE) {
        return FALSE;
    }

    geo = node->sharedChild;

    // ! @bug The aspect ratio is not accounted for. When the fov value is 45,
    // the horizontal effective fov is actually 60 degrees, so you can see objects
    // visibly pop in or out at the edge of the screen.
    halfFov = (gCurGraphNodeCamFrustum->fov / 2.0f + 1.0f) * 32768.0f / 180.0f + 0.5f;

    hScreenEdge = -matrix[3][2] * sins(halfFov) / coss(halfFov);
    // -matrix[3][2] is the depth, which gets multiplied by tan(halfFov) to get
    // the amount of units between the center of the screen and the horizontal edge
    // given the distance from the object to the camera.

#ifdef WIDESCREEN
    // This multiplication should really be performed on 4:3 as well,
    // but the issue will be more apparent on widescreen.
    hScreenEdge *= GFX_DIMENSIONS_ASPECT_RATIO;
#endif

    if (geo != NULL && geo->type == GRAPH_NODE_TYPE_CULLING_RADIUS) {
        cullingRadius =
            (f32)((struct GraphNodeCullingRadius *) geo)->cullingRadius; //! Why is there a f32 cast?
    } else {
        cullingRadius = 300;
    }

    // Don't render if the object is close to or behind the camera
    if (matrix[3][2] > -100.0f + cullingRadius) {
        return FALSE;
    }

    //! This makes the HOLP not update when the camera is far away, and it
    //  makes PU travel safe when the camera is locked on the main map.
    //  If Mario were rendered with a depth over 65536 it would cause overflow
    //  when converting the transformation matrix to a fixed point matrix.
    if (matrix[3][2] < -20000.0f - cullingRadius) {
        return FALSE;
    }

    // Check whether the object is horizontally in view
    if (matrix[3][0] > hScreenEdge + cullingRadius) {
        return FALSE;
    }
    if (matrix[3][0] < -hScreenEdge - cullingRadius) {
        return FALSE;
    }
    return TRUE;
}

/**
 * Process an object node.
 */
static void geo_process_object(struct Object *node) {
    Mat4 mtxf;
    s32 hasAnimation = (node->header.gfx.node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0;
    struct GraphNodeObject *savedGraphNodeObject = gCurGraphNodeObject;
    Mat4 savedObjectRootMatrix;
    s32 savedObjectRootMatrixValid = sGeoCurrentObjectRootMatrixValid;
    s32 savedObjectUsedRawCameraFallback = sGeoCurrentObjectUsedRawCameraFallback;
    s32 savedObjectRootStackIndex = sGeoCurrentObjectRootStackIndex;
    s32 savedObjectModelId = sGeoCurrentObjectModelId;
    const BehaviorScript *savedObjectBehavior = sGeoCurrentObjectBehavior;
    u32 savedObjectBehParams = sGeoCurrentObjectBehParams;
    s32 savedObjectBehParam2 = sGeoCurrentObjectBehParam2;
    s32 savedObjectAnimId = sGeoCurrentObjectAnimId;
    s32 savedObjectProbeRenderer = sGeoCurrentObjectProbeRenderer;

    mtxf_copy(savedObjectRootMatrix, sGeoCurrentObjectRootMatrix);

    if (node->header.gfx.unk18 == gCurGraphNodeRoot->areaIndex) {
        s32 inView;
        s32 usedObjectFallback = 0;
        s32 hadThrowMatrix = node->header.gfx.throwMatrix != NULL;
        s32 objectMatrixValid;

        if (hadThrowMatrix) {
            mtxf_mul(gMatStack[gMatStackIndex + 1], *node->header.gfx.throwMatrix,
                     gMatStack[gMatStackIndex]);
        } else if (node->header.gfx.node.flags & GRAPH_RENDER_BILLBOARD) {
            mtxf_billboard(gMatStack[gMatStackIndex + 1], gMatStack[gMatStackIndex],
                           node->header.gfx.pos,
                           gCurGraphNodeCamera != NULL ? gCurGraphNodeCamera->roll : 0);
        } else {
            mtxf_rotate_zxy_and_translate(mtxf, node->header.gfx.pos, node->header.gfx.angle);
            mtxf_mul(gMatStack[gMatStackIndex + 1], mtxf, gMatStack[gMatStackIndex]);
        }

        mtxf_scale_vec3f(gMatStack[gMatStackIndex + 1], gMatStack[gMatStackIndex + 1],
                         node->header.gfx.scale);
        if (!geo_matrix_has_plausible_object_transform(gMatStack[gMatStackIndex + 1])) {
            if (gCurrLevelNum == LEVEL_CASTLE_GROUNDS
                && node->header.gfx.sharedChild != NULL
                && sGeoObjectMatrixTryDiagCount < GEO_OBJECT_MATRIX_TRY_DIAG_LIMIT) {
                RG_LOGI("geo_obj_mtx_try[%d]: type=%d flags=%04x throw=%d raw=%d basis=%.3e pos=%.1f,%.1f,%.1f scale=%.3f row3=%.1f,%.1f,%.1f,%.3f parent3=%.1f,%.1f,%.1f,%.3f",
                        (int) sGeoObjectMatrixTryDiagCount,
                        (int) node->header.gfx.sharedChild->type,
                        (unsigned int) node->header.gfx.node.flags,
                        (int) hadThrowMatrix,
                        (int) sGeoRawCameraMatrixValid,
                        (double) geo_matrix_basis_sum(gMatStack[gMatStackIndex + 1]),
                        (double) node->header.gfx.pos[0],
                        (double) node->header.gfx.pos[1],
                        (double) node->header.gfx.pos[2],
                        (double) node->header.gfx.scale[0],
                        (double) gMatStack[gMatStackIndex + 1][3][0],
                        (double) gMatStack[gMatStackIndex + 1][3][1],
                        (double) gMatStack[gMatStackIndex + 1][3][2],
                        (double) gMatStack[gMatStackIndex + 1][3][3],
                        (double) gMatStack[gMatStackIndex][3][0],
                        (double) gMatStack[gMatStackIndex][3][1],
                        (double) gMatStack[gMatStackIndex][3][2],
                        (double) gMatStack[gMatStackIndex][3][3]);
                sGeoObjectMatrixTryDiagCount++;
            }
            if (node->header.gfx.node.flags & GRAPH_RENDER_BILLBOARD) {
                mtxf_billboard(gMatStack[gMatStackIndex + 1], gMatStack[gMatStackIndex],
                               node->header.gfx.pos,
                               gCurGraphNodeCamera != NULL ? gCurGraphNodeCamera->roll : 0);
            } else {
                mtxf_rotate_zxy_and_translate(mtxf, node->header.gfx.pos, node->header.gfx.angle);
                mtxf_mul(gMatStack[gMatStackIndex + 1], mtxf, gMatStack[gMatStackIndex]);
            }
            mtxf_scale_vec3f(gMatStack[gMatStackIndex + 1], gMatStack[gMatStackIndex + 1],
                             node->header.gfx.scale);
            if (!geo_matrix_has_plausible_object_transform(gMatStack[gMatStackIndex + 1]) &&
                geo_is_file_select_camera()) {
                geo_build_object_transform_no_camera(gMatStack[gMatStackIndex + 1], &node->header.gfx);
            }
            if (!geo_matrix_has_plausible_object_transform(gMatStack[gMatStackIndex + 1]) &&
                !geo_is_file_select_camera() && gCurGraphNodeCamera != NULL && sGeoRawCameraMatrixValid) {
                geo_build_object_transform_from_camera(gMatStack[gMatStackIndex + 1], &node->header.gfx,
                                                       sGeoRawCameraMatrix);
                geo_normalize_object_basis(gMatStack[gMatStackIndex + 1], node->header.gfx.scale);
                usedObjectFallback = 2;
            } else {
                usedObjectFallback = 1;
            }
        }
        objectMatrixValid = geo_matrix_has_plausible_object_transform(gMatStack[gMatStackIndex + 1]);
        node->header.gfx.throwMatrix = &gMatStack[++gMatStackIndex];
        mtxf_copy(sGeoCurrentObjectRootMatrix, gMatStack[gMatStackIndex]);
        sGeoCurrentObjectRootMatrixValid = TRUE;
        sGeoCurrentObjectUsedRawCameraFallback = usedObjectFallback == 2;
        sGeoCurrentObjectRootStackIndex = gMatStackIndex;
        sGeoCurrentObjectModelId = node->header.gfx.sharedChild != NULL
            ? geo_find_model_id(node->header.gfx.sharedChild)
            : -1;
        sGeoCurrentObjectBehavior = node->behavior;
        sGeoCurrentObjectBehParams = node->oBehParams;
        sGeoCurrentObjectBehParam2 = node->oBehParams2ndByte;
        sGeoCurrentObjectAnimId = node->header.gfx.unk38.animID;
        sGeoCurrentObjectProbeRenderer = geo_should_probe_renderer_object_raw();
        node->header.gfx.cameraToObject[0] = gMatStack[gMatStackIndex][3][0];
        node->header.gfx.cameraToObject[1] = gMatStack[gMatStackIndex][3][1];
        node->header.gfx.cameraToObject[2] = gMatStack[gMatStackIndex][3][2];

        // FIXME: correct types
        if (node->header.gfx.unk38.curAnim != NULL) {
            geo_set_animation_globals(&node->header.gfx.unk38, hasAnimation);
        }
        inView = objectMatrixValid && obj_is_in_view(&node->header.gfx, gMatStack[gMatStackIndex]);
        if (node == gMarioObject
            && node->header.gfx.sharedChild != NULL
            && sGeoMarioTraceDiagCount < GEO_MARIO_TRACE_DIAG_LIMIT) {
            RG_LOGI("geo_mario_trace[%d]: level=%d area=%d valid=%d in=%d fb=%d throw=%d type=%d flags=%04x pos=%.1f,%.1f,%.1f model=%d/%s beh=%s/%p anim=%d row=%d,%d,%d stack=%d",
                    (int) sGeoMarioTraceDiagCount,
                    (int) gCurrLevelNum,
                    (int) gCurrAreaIndex,
                    (int) objectMatrixValid,
                    (int) inView,
                    (int) usedObjectFallback,
                    (int) hadThrowMatrix,
                    (int) node->header.gfx.sharedChild->type,
                    (unsigned int) node->header.gfx.node.flags,
                    (double) node->header.gfx.pos[0],
                    (double) node->header.gfx.pos[1],
                    (double) node->header.gfx.pos[2],
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    geo_behavior_diag_name(node->behavior),
                    (void *) node->behavior,
                    (int) node->header.gfx.unk38.animID,
                    (int) gMatStack[gMatStackIndex][3][0],
                    (int) gMatStack[gMatStackIndex][3][1],
                    (int) gMatStack[gMatStackIndex][3][2],
                    (int) gMatStackIndex);
            sGeoMarioTraceDiagCount++;
        }
        if (geo_is_door_like_object(node)
            && node->header.gfx.sharedChild != NULL
            && sGeoDoorTraceDiagCount < GEO_DOOR_TRACE_DIAG_LIMIT) {
            RG_LOGI("geo_door_trace[%d]: level=%d area=%d valid=%d in=%d fb=%d throw=%d type=%d flags=%04x pos=%.1f,%.1f,%.1f model=%d/%s beh=%s/%p bp=%08x bp2=%d anim=%d row=%d,%d,%d stack=%d",
                    (int) sGeoDoorTraceDiagCount,
                    (int) gCurrLevelNum,
                    (int) gCurrAreaIndex,
                    (int) objectMatrixValid,
                    (int) inView,
                    (int) usedObjectFallback,
                    (int) hadThrowMatrix,
                    (int) node->header.gfx.sharedChild->type,
                    (unsigned int) node->header.gfx.node.flags,
                    (double) node->header.gfx.pos[0],
                    (double) node->header.gfx.pos[1],
                    (double) node->header.gfx.pos[2],
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    geo_behavior_diag_name(node->behavior),
                    (void *) node->behavior,
                    (unsigned int) node->oBehParams,
                    (int) node->oBehParams2ndByte,
                    (int) node->header.gfx.unk38.animID,
                    (int) gMatStack[gMatStackIndex][3][0],
                    (int) gMatStack[gMatStackIndex][3][1],
                    (int) gMatStack[gMatStackIndex][3][2],
                    (int) gMatStackIndex);
            sGeoDoorTraceDiagCount++;
        }
        if (geo_is_target_actor_object(node)
            && node->header.gfx.sharedChild != NULL
            && sGeoCharacterTraceDiagCount < GEO_CHARACTER_TRACE_DIAG_LIMIT) {
            RG_LOGI("geo_character_trace[%d]: level=%d area=%d mario=%d valid=%d in=%d fb=%d throw=%d type=%d flags=%04x pos=%.1f,%.1f,%.1f model=%d/%s beh=%s/%p anim=%d row=%d,%d,%d stack=%d",
                    (int) sGeoCharacterTraceDiagCount,
                    (int) gCurrLevelNum,
                    (int) gCurrAreaIndex,
                    (int) (node == gMarioObject),
                    (int) objectMatrixValid,
                    (int) inView,
                    (int) usedObjectFallback,
                    (int) hadThrowMatrix,
                    (int) node->header.gfx.sharedChild->type,
                    (unsigned int) node->header.gfx.node.flags,
                    (double) node->header.gfx.pos[0],
                    (double) node->header.gfx.pos[1],
                    (double) node->header.gfx.pos[2],
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    geo_behavior_diag_name(node->behavior),
                    (void *) node->behavior,
                    (int) node->header.gfx.unk38.animID,
                    (int) gMatStack[gMatStackIndex][3][0],
                    (int) gMatStack[gMatStackIndex][3][1],
                    (int) gMatStack[gMatStackIndex][3][2],
                    (int) gMatStackIndex);
            sGeoCharacterTraceDiagCount++;
        }
        if (gCurrLevelNum == LEVEL_BOB
            && geo_is_target_actor_object(node)
            && node->header.gfx.sharedChild != NULL
            && sGeoCourseActorTraceDiagCount < GEO_COURSE_ACTOR_TRACE_DIAG_LIMIT) {
            RG_LOGI("geo_course_actor[%d]: level=%d area=%d mario=%d valid=%d in=%d fb=%d throw=%d type=%d flags=%04x pos=%.1f,%.1f,%.1f model=%d/%s beh=%s/%p anim=%d row=%d,%d,%d stack=%d",
                    (int) sGeoCourseActorTraceDiagCount,
                    (int) gCurrLevelNum,
                    (int) gCurrAreaIndex,
                    (int) (node == gMarioObject),
                    (int) objectMatrixValid,
                    (int) inView,
                    (int) usedObjectFallback,
                    (int) hadThrowMatrix,
                    (int) node->header.gfx.sharedChild->type,
                    (unsigned int) node->header.gfx.node.flags,
                    (double) node->header.gfx.pos[0],
                    (double) node->header.gfx.pos[1],
                    (double) node->header.gfx.pos[2],
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    geo_behavior_diag_name(node->behavior),
                    (void *) node->behavior,
                    (int) node->header.gfx.unk38.animID,
                    (int) gMatStack[gMatStackIndex][3][0],
                    (int) gMatStack[gMatStackIndex][3][1],
                    (int) gMatStack[gMatStackIndex][3][2],
                    (int) gMatStackIndex);
            sGeoCourseActorTraceDiagCount++;
        }
        if (gCurrLevelNum == LEVEL_CASTLE_GROUNDS
            && sGeoGameplayObjectDiagCount < GEO_GAMEPLAY_OBJECT_DIAG_LIMIT
            && node->header.gfx.sharedChild != NULL) {
            s32 sharedType = node->header.gfx.sharedChild->type;

            RG_LOGI("geo_game_obj[%d]: valid=%d in=%d fb=%d throw=%d type=%d flags=%04x pos=%.1f,%.1f,%.1f scale=%.3f row3=%.1f,%.1f,%.1f,%.3f",
                    (int) sGeoGameplayObjectDiagCount,
                    (int) objectMatrixValid,
                    (int) inView,
                    (int) usedObjectFallback,
                    (int) hadThrowMatrix,
                    (int) sharedType,
                    (unsigned int) node->header.gfx.node.flags,
                    (double) node->header.gfx.pos[0],
                    (double) node->header.gfx.pos[1],
                    (double) node->header.gfx.pos[2],
                    (double) node->header.gfx.scale[0],
                    (double) gMatStack[gMatStackIndex][3][0],
                    (double) gMatStack[gMatStackIndex][3][1],
                    (double) gMatStack[gMatStackIndex][3][2],
                    (double) gMatStack[gMatStackIndex][3][3]);
            sGeoGameplayObjectDiagCount++;
        }
        if (usedObjectFallback && !objectMatrixValid && !geo_is_file_select_camera()
            && sGeoObjectMatrixFallbackDiagCount < GEO_OBJECT_MATRIX_FALLBACK_DIAG_LIMIT) {
            s32 sharedType = node->header.gfx.sharedChild != NULL ? node->header.gfx.sharedChild->type : -1;

            RG_LOGI("geo_obj_mtx_fb[%d]: type=%d flags=%04x fb=%d raw=%d basis=%.3e pos=%.1f,%.1f,%.1f scale=%.3f row3=%.1f,%.1f,%.1f,%.3f raw3=%.1f,%.1f,%.1f,%.3f",
                    (int) sGeoObjectMatrixFallbackDiagCount,
                    (int) sharedType,
                    (unsigned int) node->header.gfx.node.flags,
                    (int) usedObjectFallback,
                    (int) sGeoRawCameraMatrixValid,
                    (double) geo_matrix_basis_sum(gMatStack[gMatStackIndex]),
                    (double) node->header.gfx.pos[0],
                    (double) node->header.gfx.pos[1],
                    (double) node->header.gfx.pos[2],
                    (double) node->header.gfx.scale[0],
                    (double) gMatStack[gMatStackIndex][3][0],
                    (double) gMatStack[gMatStackIndex][3][1],
                    (double) gMatStack[gMatStackIndex][3][2],
                    (double) gMatStack[gMatStackIndex][3][3],
                    (double) sGeoRawCameraMatrix[3][0],
                    (double) sGeoRawCameraMatrix[3][1],
                    (double) sGeoRawCameraMatrix[3][2],
                    (double) sGeoRawCameraMatrix[3][3]);
            sGeoObjectMatrixFallbackDiagCount++;
        }
        if (usedObjectFallback == 2
            && gCurrLevelNum == LEVEL_CASTLE_GROUNDS
            && node->header.gfx.sharedChild != NULL
            && node->header.gfx.sharedChild->type == 47
            && geo_is_character_like_model(sGeoCurrentObjectModelId)
            && sGeoCharacterOwnerObjectDiagCount < GEO_CHARACTER_OWNER_OBJECT_DIAG_LIMIT) {
            RG_LOGI("geo_char_obj[%d]: in=%d flags=%04x pos=%.1f,%.1f,%.1f model=%d/%s beh=%s/%p bp=%08x bp2=%d anim=%d row=%d,%d,%d stack=%d",
                    (int) sGeoCharacterOwnerObjectDiagCount,
                    (int) inView,
                    (unsigned int) node->header.gfx.node.flags,
                    (double) node->header.gfx.pos[0],
                    (double) node->header.gfx.pos[1],
                    (double) node->header.gfx.pos[2],
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    geo_behavior_diag_name(node->behavior),
                    (void *) node->behavior,
                    (unsigned int) node->oBehParams,
                    (int) node->oBehParams2ndByte,
                    (int) node->header.gfx.unk38.animID,
                    (int) gMatStack[gMatStackIndex][3][0],
                    (int) gMatStack[gMatStackIndex][3][1],
                    (int) gMatStack[gMatStackIndex][3][2],
                    (int) gMatStackIndex);
            sGeoCharacterOwnerObjectDiagCount++;
        }
        if (usedObjectFallback == 2
            && gCurrLevelNum == LEVEL_CASTLE_GROUNDS
            && node->header.gfx.sharedChild != NULL
            && node->header.gfx.sharedChild->type == 47
            && (node->header.gfx.node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0
            && sGeoRawFallbackObjectDiagCount < GEO_RAW_FALLBACK_OBJECT_DIAG_LIMIT) {
            RG_LOGI("geo_raw_obj[%d]: in=%d flags=%04x pos=%.1f,%.1f,%.1f model=%d/%s beh=%s/%p bp=%08x bp2=%d anim=%d row=%d,%d,%d stack=%d",
                    (int) sGeoRawFallbackObjectDiagCount,
                    (int) inView,
                    (unsigned int) node->header.gfx.node.flags,
                    (double) node->header.gfx.pos[0],
                    (double) node->header.gfx.pos[1],
                    (double) node->header.gfx.pos[2],
                    (int) sGeoCurrentObjectModelId,
                    geo_model_diag_name(sGeoCurrentObjectModelId),
                    geo_behavior_diag_name(node->behavior),
                    (void *) node->behavior,
                    (unsigned int) node->oBehParams,
                    (int) node->oBehParams2ndByte,
                    (int) node->header.gfx.unk38.animID,
                    (int) gMatStack[gMatStackIndex][3][0],
                    (int) gMatStack[gMatStackIndex][3][1],
                    (int) gMatStack[gMatStackIndex][3][2],
                    (int) gMatStackIndex);
            sGeoRawFallbackObjectDiagCount++;
        }
        if (GEO_MENU_OBJECT_DIAG && geo_is_file_select_camera() && sGeoMenuObjectDiagCount < GEO_MENU_OBJECT_DIAG_LIMIT
            && (node->header.gfx.sharedChild != NULL || node->header.gfx.pos[2] < -1000.0f)) {
            s32 sharedType = node->header.gfx.sharedChild != NULL ? node->header.gfx.sharedChild->type : -1;

            RG_LOGI("geo_menu_obj[%d]: fb=%d type=%d in=%d flags=%04x pos=%d,%d,%d scale=%d,%d,%d cam=%d,%d,%d",
                    (int) sGeoMenuObjectDiagCount,
                    (int) usedObjectFallback,
                    (int) sharedType,
                    (int) inView,
                    (unsigned int) node->header.gfx.node.flags,
                    (int) node->header.gfx.pos[0],
                    (int) node->header.gfx.pos[1],
                    (int) node->header.gfx.pos[2],
                    (int) (node->header.gfx.scale[0] * 1000.0f),
                    (int) (node->header.gfx.scale[1] * 1000.0f),
                    (int) (node->header.gfx.scale[2] * 1000.0f),
                    (int) node->header.gfx.cameraToObject[0],
                    (int) node->header.gfx.cameraToObject[1],
                    (int) node->header.gfx.cameraToObject[2]);
            sGeoMenuObjectDiagCount++;
        }
        if (inView) {
            Mtx *mtx = alloc_display_list(sizeof(*mtx));

            mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
            gMatStackFixed[gMatStackIndex] = mtx;
            if (node->header.gfx.sharedChild != NULL) {
                gCurGraphNodeObject = &node->header.gfx;
                node->header.gfx.sharedChild->parent = &node->header.gfx.node;
#if GEO_RENDERER_MENU_MODEL_DIAG_ENABLED
                if (geo_is_file_select_camera()) {
                    s32 sharedType = node->header.gfx.sharedChild->type;
                    gfx_debug_arm_menu_model(sharedType,
                                             (s32) node->header.gfx.pos[0],
                                             (s32) node->header.gfx.pos[1],
                                             (s32) node->header.gfx.pos[2],
                                             (s32) (node->header.gfx.scale[0] * 1000.0f),
                                             (s32) (node->header.gfx.scale[1] * 1000.0f),
                                             (s32) (node->header.gfx.scale[2] * 1000.0f));
                }
#endif
                geo_process_node_and_siblings(node->header.gfx.sharedChild);
                node->header.gfx.sharedChild->parent = NULL;
                gCurGraphNodeObject = &node->header.gfx;
            }
            if (node->header.gfx.node.children != NULL) {
                geo_process_node_and_siblings(node->header.gfx.node.children);
            }
        }

        gMatStackIndex--;
        gCurAnimType = ANIM_TYPE_NONE;
        node->header.gfx.throwMatrix = NULL;
    }
    gCurGraphNodeObject = savedGraphNodeObject;
    mtxf_copy(sGeoCurrentObjectRootMatrix, savedObjectRootMatrix);
    sGeoCurrentObjectRootMatrixValid = savedObjectRootMatrixValid;
    sGeoCurrentObjectUsedRawCameraFallback = savedObjectUsedRawCameraFallback;
    sGeoCurrentObjectRootStackIndex = savedObjectRootStackIndex;
    sGeoCurrentObjectModelId = savedObjectModelId;
    sGeoCurrentObjectBehavior = savedObjectBehavior;
    sGeoCurrentObjectBehParams = savedObjectBehParams;
    sGeoCurrentObjectBehParam2 = savedObjectBehParam2;
    sGeoCurrentObjectAnimId = savedObjectAnimId;
    sGeoCurrentObjectProbeRenderer = savedObjectProbeRenderer;
}

/**
 * Process an object parent node. Temporarily assigns itself as the parent of
 * the subtree rooted at 'sharedChild' and processes the subtree, after which the
 * actual children are be processed. (in practice they are null though)
 */
static void geo_process_object_parent(struct GraphNodeObjectParent *node) {
    if (node->sharedChild != NULL) {
        node->sharedChild->parent = (struct GraphNode *) node;
        geo_process_node_and_siblings(node->sharedChild);
        node->sharedChild->parent = NULL;
    }
    if (node->node.children != NULL) {
        geo_process_node_and_siblings(node->node.children);
    }
}

/**
 * Process a held object node.
 */
void geo_process_held_object(struct GraphNodeHeldObject *node) {
    Mat4 mat;
    Vec3f translation;
    Mtx *mtx = alloc_display_list(sizeof(*mtx));

#ifdef F3DEX_GBI_2
    gSPLookAt(gDisplayListHead++, &lookAt);
#endif

    if (node->fnNode.func != NULL) {
        node->fnNode.func(GEO_CONTEXT_RENDER, &node->fnNode.node, gMatStack[gMatStackIndex]);
    }
    if (node->objNode != NULL && node->objNode->header.gfx.sharedChild != NULL) {
        s32 hasAnimation = (node->objNode->header.gfx.node.flags & GRAPH_RENDER_HAS_ANIMATION) != 0;

        translation[0] = node->translation[0] / 4.0f;
        translation[1] = node->translation[1] / 4.0f;
        translation[2] = node->translation[2] / 4.0f;

        mtxf_translate(mat, translation);
        mtxf_copy(gMatStack[gMatStackIndex + 1], *gCurGraphNodeObject->throwMatrix);
        gMatStack[gMatStackIndex + 1][3][0] = gMatStack[gMatStackIndex][3][0];
        gMatStack[gMatStackIndex + 1][3][1] = gMatStack[gMatStackIndex][3][1];
        gMatStack[gMatStackIndex + 1][3][2] = gMatStack[gMatStackIndex][3][2];
        mtxf_mul(gMatStack[gMatStackIndex + 1], mat, gMatStack[gMatStackIndex + 1]);
        mtxf_scale_vec3f(gMatStack[gMatStackIndex + 1], gMatStack[gMatStackIndex + 1],
                         node->objNode->header.gfx.scale);
        if (node->fnNode.func != NULL) {
            node->fnNode.func(GEO_CONTEXT_HELD_OBJ, &node->fnNode.node,
                              (struct AllocOnlyPool *) gMatStack[gMatStackIndex + 1]);
        }
        gMatStackIndex++;
        mtxf_to_mtx(mtx, gMatStack[gMatStackIndex]);
        gMatStackFixed[gMatStackIndex] = mtx;
        gGeoTempState.type = gCurAnimType;
        gGeoTempState.enabled = gCurAnimEnabled;
        gGeoTempState.frame = gCurrAnimFrame;
        gGeoTempState.translationMultiplier = gCurAnimTranslationMultiplier;
        gGeoTempState.attribute = gCurrAnimAttribute;
        gGeoTempState.data = gCurAnimData;
        gCurAnimType = 0;
        gCurGraphNodeHeldObject = (void *) node;
        if (node->objNode->header.gfx.unk38.curAnim != NULL) {
            geo_set_animation_globals(&node->objNode->header.gfx.unk38, hasAnimation);
        }

        geo_process_node_and_siblings(node->objNode->header.gfx.sharedChild);
        gCurGraphNodeHeldObject = NULL;
        gCurAnimType = gGeoTempState.type;
        gCurAnimEnabled = gGeoTempState.enabled;
        gCurrAnimFrame = gGeoTempState.frame;
        gCurAnimTranslationMultiplier = gGeoTempState.translationMultiplier;
        gCurrAnimAttribute = gGeoTempState.attribute;
        gCurAnimData = gGeoTempState.data;
        gMatStackIndex--;
    }

    if (node->fnNode.node.children != NULL) {
        geo_process_node_and_siblings(node->fnNode.node.children);
    }
}

/**
 * Processes the children of the given GraphNode if it has any
 */
void geo_try_process_children(struct GraphNode *node) {
    if (node->children != NULL) {
        geo_process_node_and_siblings(node->children);
    }
}

/**
 * Process a generic geo node and its siblings.
 * The first argument is the start node, and all its siblings will
 * be iterated over.
 */
void geo_process_node_and_siblings(struct GraphNode *firstNode) {
    s16 iterateChildren = TRUE;
    struct GraphNode *curGraphNode = firstNode;
    struct GraphNode *parent = curGraphNode->parent;

    // In the case of a switch node, exactly one of the children of the node is
    // processed instead of all children like usual
    if (parent != NULL) {
        iterateChildren = (parent->type != GRAPH_NODE_TYPE_SWITCH_CASE);
    }

    do {
        if (curGraphNode->flags & GRAPH_RENDER_ACTIVE) {
            if (curGraphNode->flags & GRAPH_RENDER_CHILDREN_FIRST) {
                geo_try_process_children(curGraphNode);
            } else {
                switch (curGraphNode->type) {
                    case GRAPH_NODE_TYPE_ORTHO_PROJECTION:
                        geo_process_ortho_projection((struct GraphNodeOrthoProjection *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_PERSPECTIVE:
                        geo_process_perspective((struct GraphNodePerspective *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_MASTER_LIST:
                        geo_process_master_list((struct GraphNodeMasterList *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_LEVEL_OF_DETAIL:
                        geo_process_level_of_detail((struct GraphNodeLevelOfDetail *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_SWITCH_CASE:
                        geo_process_switch((struct GraphNodeSwitchCase *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_CAMERA:
                        geo_process_camera((struct GraphNodeCamera *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_TRANSLATION_ROTATION:
                        geo_process_translation_rotation(
                            (struct GraphNodeTranslationRotation *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_TRANSLATION:
                        geo_process_translation((struct GraphNodeTranslation *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_ROTATION:
                        geo_process_rotation((struct GraphNodeRotation *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_OBJECT:
                        geo_process_object((struct Object *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_ANIMATED_PART:
                        geo_process_animated_part((struct GraphNodeAnimatedPart *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_BILLBOARD:
                        geo_process_billboard((struct GraphNodeBillboard *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_DISPLAY_LIST:
                        geo_process_display_list((struct GraphNodeDisplayList *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_SCALE:
                        geo_process_scale((struct GraphNodeScale *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_SHADOW:
                        geo_process_shadow((struct GraphNodeShadow *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_OBJECT_PARENT:
                        geo_process_object_parent((struct GraphNodeObjectParent *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_GENERATED_LIST:
                        geo_process_generated_list((struct GraphNodeGenerated *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_BACKGROUND:
                        geo_process_background((struct GraphNodeBackground *) curGraphNode);
                        break;
                    case GRAPH_NODE_TYPE_HELD_OBJ:
                        geo_process_held_object((struct GraphNodeHeldObject *) curGraphNode);
                        break;
                    default:
                        geo_try_process_children((struct GraphNode *) curGraphNode);
                        break;
                }
            }
        } else {
            if (curGraphNode->type == GRAPH_NODE_TYPE_OBJECT) {
                ((struct GraphNodeObject *) curGraphNode)->throwMatrix = NULL;
            }
        }
    } while (iterateChildren && (curGraphNode = curGraphNode->next) != firstNode);
}

/**
 * Process a root node. This is the entry point for processing the scene graph.
 * The root node itself sets up the viewport, then all its children are processed
 * to set up the projection and draw display lists.
 */
void geo_process_root(struct GraphNodeRoot *node, Vp *b, Vp *c, s32 clearColor) {
    UNUSED s32 unused;

    if (node->node.flags & GRAPH_RENDER_ACTIVE) {
        Mtx *initialMatrix;
        Vp *viewport = alloc_display_list(sizeof(*viewport));

        gDisplayListHeap = alloc_only_pool_init(main_pool_available() - sizeof(struct AllocOnlyPool),
                                                MEMORY_POOL_LEFT);
        initialMatrix = alloc_display_list(sizeof(*initialMatrix));
        gMatStackIndex = 0;
        gCurAnimType = 0;
        vec3s_set(viewport->vp.vtrans, node->x * 4, node->y * 4, 511);
        vec3s_set(viewport->vp.vscale, node->width * 4, node->height * 4, 511);
        if (b != NULL) {
            clear_frame_buffer(clearColor);
            make_viewport_clip_rect(b);
            *viewport = *b;
        }

        else if (c != NULL) {
            clear_frame_buffer(clearColor);
            make_viewport_clip_rect(c);
        }

        mtxf_identity(gMatStack[gMatStackIndex]);
        mtxf_to_mtx(initialMatrix, gMatStack[gMatStackIndex]);
        gMatStackFixed[gMatStackIndex] = initialMatrix;
        gSPViewport(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(viewport));
        gSPMatrix(gDisplayListHead++, VIRTUAL_TO_PHYSICAL(gMatStackFixed[gMatStackIndex]),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gCurGraphNodeRoot = node;
        if (node->node.children != NULL) {
            geo_process_node_and_siblings(node->node.children);
        }
        gCurGraphNodeRoot = NULL;
        if (gShowDebugText) {
            print_text_fmt_int(180, 36, "MEM %d",
                               gDisplayListHeap->totalSpace - gDisplayListHeap->usedSpace);
        }
        main_pool_free(gDisplayListHeap);
    }
}
