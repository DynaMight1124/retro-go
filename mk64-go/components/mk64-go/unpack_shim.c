/* Minimal globals needed to reuse the decomp's course display-list unpacker
 * during extraction. Replace these as the full game loop is brought in. */
#include <stdint.h>
#include <stddef.h>
#include "course_segments.h"
#include "segment_bridge.h"

/* The real game sources provide these as they are linked in. Keep fallback
 * storage for the extraction-only build while that integration is staged. */
__attribute__((weak)) uintptr_t gHeapEndPtr;
__attribute__((weak)) uintptr_t gSegmentTable[16];
__attribute__((weak)) int32_t gIsMirrorMode;
static int sActiveCourse = -1;
static bool sLogoActive;
static bool sCeremonyActive;

void mk64_segment_set_course(int course)
{
    sActiveCourse = course >= 0 && course < 20 ? course : -1;
    if (sActiveCourse >= 0) {
        sLogoActive = false;
        sCeremonyActive = false;
    }
}

void mk64_segment_set_logo(bool active)
{
    sLogoActive = active;
    if (active)
        sActiveCourse = -1;
}

void mk64_segment_set_ceremony(bool active)
{
    sCeremonyActive = active;
}

void *port_seg_to_ptr(uintptr_t address)
{
    if (address >= 0x10000000u)
        return (void *)address;
    uint32_t segment = address >> 24;
    uint32_t offset = address & 0x00ffffffu;
    if (segment == 0x0du) {
        const void *mapped = mk64_static_segment_lookup(MK64_STATIC_COMMON, offset);
        if (mapped) return (void *)mapped;
    }
    if (segment == 0x0bu && sCeremonyActive) {
        const void *mapped = mk64_static_segment_lookup(MK64_STATIC_CEREMONY, offset);
        if (mapped) return (void *)mapped;
    }
    if (segment == 6 && sLogoActive) {
        const void *mapped = mk64_static_segment_lookup(MK64_STATIC_LOGO, offset);
        if (mapped) return (void *)mapped;
    }
    if (sActiveCourse >= 0 && (segment == 6 || segment == 7)) {
        const void *mapped = mk64_course_segment_lookup(
            (unsigned)sActiveCourse, segment, offset);
        if (mapped)
            return (void *)mapped;
    }
    if (segment < 16 && gSegmentTable[segment])
        return (void *)(gSegmentTable[segment] + (address & 0x00ffffffu));
    return NULL;
}
