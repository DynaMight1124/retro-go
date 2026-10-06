/* Media Engine bootstrap.  mk64k.prx copies this to 0xbfc00040, the ME's
 * reset entry, and takes the ME out of reset (mek.c me_boot).  The
 * sequence -- bus setup, cache tag initialisation, interrupts off, FPU on,
 * jump to the dispatch loop with its argument -- is the one the pspsdk ME
 * sample and the mrbrown/crazyc melib established; the words at 0xbfc00600
 * and 0xbfc00604 hold the entry point and its argument.  Position
 * independent: only immediates and local branches. */
    .set noreorder
    .set noat
    .text
    .globl me_stub
    .globl me_stub_end
    .ent me_stub
me_stub:
    li      $k0, 0xbc100000         /* system registers: ME bus */
    li      $t0, 7
    sw      $t0, 0x50($k0)
    li      $t0, 2
    sw      $t0, 0x40($k0)

    mtc0    $zero, $28              /* TagLo */
    mtc0    $zero, $29              /* TagHi */
    li      $t1, 8192               /* instruction cache: every index gets a clean tag */
1:  addiu   $t1, $t1, -64
    bne     $t1, $zero, 1b
    cache   0x01, 0($t1)
    li      $t1, 8192               /* data cache likewise */
2:  addiu   $t1, $t1, -64
    bne     $t1, $zero, 2b
    cache   0x11, 0($t1)

    mtc0    $zero, $13              /* Cause */
    li      $t0, 0xbc000000         /* the ME's interrupt controller: everything masked / cleared */
    li      $t1, 0xbc00002c
    li      $t2, -1
3:  sw      $t2, 0($t0)
    bne     $t0, $t1, 3b
    addiu   $t0, $t0, 4

    li      $t0, 0x20000000         /* Status: CU1 (FPU) on; ERL, EXL, IE off */
    mtc0    $t0, $12
    sync

    li      $t0, 0xbfc00000
    lw      $a0, 0x604($t0)         /* argument */
    lw      $k0, 0x600($t0)         /* entry point */
    li      $sp, 0x80200000         /* top of the ME's own 2 MB */
    jr      $k0
    nop
me_stub_end:
    .end me_stub
