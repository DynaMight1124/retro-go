#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

enum { GP_FS_P4, GP_FS_S3, GP_FS_INTERPRETER };
typedef struct { int max_level, initial_level; unsigned raise_gain, drop_gain; } gp_fs_profile_t;
typedef struct { uint64_t total, normal; uint32_t count, normal_count; } gp_fs_sample_t;
typedef struct {
    gp_fs_profile_t profile;
    int mode, level, good, hold, since_drop, since_raise;
    gp_fs_sample_t sample[2]; /* skipped, drawn */
    uint32_t measured[6], drawn_ref[6], peak;
    uint32_t last_drawn, last_skipped, last_average;
    bool filtered;
} gp_fs_t;

static inline gp_fs_profile_t gp_fs_profile(int backend)
{
    if (backend == GP_FS_P4) return (gp_fs_profile_t){2,0,6,4};
    if (backend == GP_FS_S3) return (gp_fs_profile_t){2,0,8,6};
    return (gp_fs_profile_t){5,1,0,0};
}

static inline void gp_fs_clear_window(gp_fs_t *s)
{
    memset(s->sample, 0, sizeof(s->sample));
    s->peak = 0;
    s->good = 0;
}

static inline void gp_fs_init(gp_fs_t *s, gp_fs_profile_t profile, int mode)
{
    memset(s, 0, sizeof(*s));
    s->profile = profile;
    s->mode = mode < -1 || mode > profile.max_level ? -1 : mode;
    s->level = s->mode < 0 ? profile.initial_level : s->mode;
    s->hold = 3;
    s->since_drop = s->since_raise = 30;
}

static inline void gp_fs_record(gp_fs_t *s, uint32_t us, bool drawn)
{
    if (s->mode >= 0) return;
    gp_fs_sample_t *p = &s->sample[drawn ? 1 : 0];
    p->total += us; p->count++;
    if (us < 100000) { p->normal += us; p->normal_count++; }
    if (us > s->peak) s->peak = us;
}

static inline uint32_t gp_fs_cost(uint32_t drawn, uint32_t skipped, int level)
{
    return (uint32_t)(((uint64_t)drawn + (uint64_t)level * skipped) / (unsigned)(level + 1));
}

static inline unsigned gp_fs_gain(uint32_t drawn, uint32_t skipped, int from)
{
    uint32_t before = gp_fs_cost(drawn, skipped, from);
    uint32_t after = gp_fs_cost(drawn, skipped, from + 1);
    return before && before > after ? (unsigned)((uint64_t)(before - after) * 100 / before) : 0;
}

/* S3-derived once-per-active-second decision. Audio pacing never enters samples.
 * Returns true only when the level changed; fixed modes never adapt. */
static inline bool gp_fs_update(gp_fs_t *s, uint32_t budget)
{
    if (s->mode >= 0) return false;
    if (budget < 1000) budget = 1000;
    gp_fs_sample_t d = s->sample[1], k = s->sample[0];
    uint32_t n = d.count + k.count, normal_n = d.normal_count + k.normal_count;
    if (!n) return false;
    bool clean_classes = (!d.count || d.normal_count) && (!k.count || k.normal_count);
    s->filtered = normal_n >= 8 && (n - normal_n) <= n / 4 && clean_classes && normal_n != n;
    uint32_t dn = s->filtered ? d.normal_count : d.count;
    uint32_t kn = s->filtered ? k.normal_count : k.count;
    uint64_t du = s->filtered ? d.normal : d.total, ku = s->filtered ? k.normal : k.total;
    uint32_t drawn = dn ? (uint32_t)(du / dn) : 0, skipped = kn ? (uint32_t)(ku / kn) : 0;
    uint32_t average = (uint32_t)((du + ku) / (dn + kn));
    s->last_drawn = drawn; s->last_skipped = skipped; s->last_average = average;
    int level = s->level;
    if (s->peak < 100000) {
        s->measured[level] = average;
        if (!s->drawn_ref[level] && drawn) s->drawn_ref[level] = drawn;
    }
    memset(s->sample, 0, sizeof(s->sample)); s->peak = 0;
    if (s->since_drop < 30) s->since_drop++;
    if (s->since_raise < 30) s->since_raise++;
    if (s->since_raise >= 30) s->hold = 3;
    uint32_t raise_at = (uint32_t)((uint64_t)budget * 105 / 100);
    bool useful_raise = level == 0 || !s->profile.raise_gain ||
        (dn && kn && gp_fs_gain(drawn, skipped, level) >= s->profile.raise_gain);
    if (average > raise_at && level < s->profile.max_level && useful_raise) {
        if (s->since_drop <= 3) s->hold = s->hold * 2 > 30 ? 30 : s->hold * 2;
        /* Remember the load that caused the raise. A scene change immediately
         * afterwards must not replace it with the lighter arrival workload. */
        s->level++; s->drawn_ref[s->level] = drawn;
        s->since_raise = 0; s->good = 0;
    } else if (level > 0 && dn) {
        uint32_t predicted = gp_fs_cost(drawn, skipped, level - 1);
        uint32_t below = s->measured[level - 1];
        bool can_drop = predicted <= raise_at &&
            ((!below && predicted < (uint64_t)budget * 78 / 100) ||
             (below && below <= (uint64_t)budget * 97 / 100) ||
             (s->drawn_ref[level] && (uint64_t)drawn + budget * 12 / 100 <= s->drawn_ref[level]));
        bool low_value = level > 1 && kn && s->profile.drop_gain &&
            gp_fs_gain(drawn, skipped, level - 1) < s->profile.drop_gain;
        bool fast = can_drop && drawn <= (uint64_t)budget * 72 / 100;
        if ((can_drop || low_value) && (fast || ++s->good >= s->hold)) {
            s->level--; s->drawn_ref[s->level] = 0;
            s->since_drop = 0; s->good = 0;
        } else if (!can_drop && !low_value) s->good = 0;
    } else s->good = 0;
    return level != s->level;
}

static inline uint32_t gp_fs_next(const gp_fs_t *s, uint32_t remaining)
{
    return remaining ? remaining - 1 : (uint32_t)s->level;
}
