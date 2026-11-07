#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "t386_fill.h"
enum { T386_STORE_UNKNOWN,T386_STORE_RAM,T386_STORE_VGA,T386_STORE_MMIO,T386_STORE_UNMAPPED };
/* First observed destination of the slowest completed REP STOS, not its entire
 * range. Physical address comes from an existing successful write translation. */
typedef struct {
    unsigned max_us,count,cs,ip,width,address_bits,reverse,es,offset,linear;
    unsigned physical,physical_valid,kind,value;
    unsigned fill_calls,fill_bytes;
    T386FillReject reject;
} T386StoreProfile;
/* First translated operand only. Later pages may be noncontiguous, alias,
 * wrap or change routing; these endpoints do not certify a bulk-copy range. */
typedef struct {
    unsigned segment,offset,linear,physical,physical_valid,split,kind;
} T386CopyEndpoint;
typedef struct {
    unsigned max_us,count,cs,ip,width,address_bits,reverse,paging;
    unsigned string_calls,string_bytes,ram_calls,ram_bytes;
    T386CopyEndpoint source,destination;
} T386CopyProfile;
typedef struct {
    uint64_t us;
    unsigned calls,slow,max_us,opcode,count,cs,ip;
    unsigned fill_calls,fill_bytes;
    unsigned ram_calls,ram_bytes;
    T386StoreProfile store;
    T386CopyProfile copy;
} T386CPUProfile;
/* Single-VM diagnostic snapshot, reset before each CPU batch. */
void t386_cpu_profile_reset(void);
const T386CPUProfile *t386_cpu_profile_get(void);
typedef struct {
    uint64_t us;
    unsigned calls,slow,max_us,opcode,cs,ip,faults;
} T386InstructionProfile;
const T386InstructionProfile *t386_instruction_profile_get(void);
typedef struct {
    uint64_t us;
    unsigned calls,slow,max_us,port,value,cs,ip;
} T386PortProfile;
const T386PortProfile *t386_port_profile_get(void);

#define T386_CPU_HOT_PROFILE 2
enum {T386_HOT_OPCODES=257};
enum {T386_HOT_PLAIN,T386_HOT_RAM,T386_HOT_VGA,T386_HOT_MMIO,
      T386_HOT_PORT,T386_HOT_MIXED,T386_HOT_UNMAPPED,T386_HOT_ROUTES};
typedef struct {uint64_t us;unsigned calls,max_us;} T386HotBucket;
/* Whole-batch Bernoulli sample, independent of the deadline budget. Timings
 * are inclusive instruction wall time, not separate memory/decoder timings.
 * Opcode prefixes are removed; 0F groups two-byte opcodes; 100 = fetch fault.
 * Scalar data-access counts exclude REP bulk helpers and MMU page-table loads. */
typedef struct {
    T386HotBucket *opcode; /* Caller-owned cold histogram, normally in PSRAM. */
    T386HotBucket route[T386_HOT_ROUTES];
    uint64_t us;
    unsigned batches,small_batches,large_batches,calls,faults;
    unsigned fetch_bytes,ram_reads,ram_writes,ram_read_bytes,ram_write_bytes;
    unsigned mmio_reads,mmio_writes,unmapped,split;
    unsigned translations,paged,tlb_hits,tlb_misses,effective_addresses;
} T386CPUHotProfile;
/* Single VM, reset only between batches at a reporting/menu boundary. */
const T386CPUHotProfile *t386_cpu_hot_profile_get(void);
void t386_cpu_hot_profile_reset(void);
/* Attach storage only between batches; NULL/0 disables aggregate sampling.
 * Caller retains ownership and must detach before freeing it. */
bool t386_cpu_hot_profile_init(T386HotBucket *storage,unsigned count);
