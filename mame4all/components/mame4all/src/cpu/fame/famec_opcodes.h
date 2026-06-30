#ifdef __cplusplus
extern "C" {
#endif
#ifdef FAME_CHANGE_PC
// proto
void fame_change_pc(unsigned pc);
#endif

/* ORI */
OPCODE(0x0000)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    res = DREGu8((Opcode >> 0) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ORI */
OPCODE(0x0010)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ORI */
OPCODE(0x0018)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ORI */
OPCODE(0x0020)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ORI */
OPCODE(0x0028)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ORI */
OPCODE(0x0030)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* ORI */
OPCODE(0x0038)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ORI */
OPCODE(0x0039)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* ORI */
OPCODE(0x001F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ORI */
OPCODE(0x0027)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ORI */
OPCODE(0x0040)
{
    uint32_t res;
    uint32_t src;

    FETCH_WORD(src);
    res = DREGu16((Opcode >> 0) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ORI */
OPCODE(0x0050)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ORI */
OPCODE(0x0058)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ORI */
OPCODE(0x0060)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ORI */
OPCODE(0x0068)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ORI */
OPCODE(0x0070)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* ORI */
OPCODE(0x0078)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ORI */
OPCODE(0x0079)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* ORI */
OPCODE(0x005F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ORI */
OPCODE(0x0067)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ORI */
OPCODE(0x0080)
{
    uint32_t res;
    uint32_t src;

    FETCH_LONG(src);
    res = DREGu32((Opcode >> 0) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(16)
}

/* ORI */
OPCODE(0x0090)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ORI */
OPCODE(0x0098)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ORI */
OPCODE(0x00A0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ORI */
OPCODE(0x00A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* ORI */
OPCODE(0x00B0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(34)
}

/* ORI */
OPCODE(0x00B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* ORI */
OPCODE(0x00B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(36)
}

/* ORI */
OPCODE(0x009F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ORI */
OPCODE(0x00A7)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ORICCR */
OPCODE(0x003C)
{
    uint32_t res;

    FETCH_BYTE(res);
    res &= M68K_CCR_MASK;
    res |= GET_CCR;
    SET_CCR(res)
    RET(20)
}

/* ORISR */
OPCODE(0x007C)
{

    if (flag_S)
    {
        uint32_t res;
        FETCH_WORD(res);
        res &= M68K_SR_MASK;
        res |= GET_SR;
        SET_SR(res)
        CHECK_INT_TO_JUMP(20)
    }
    else
    {
        uint32_t newPC = UNBASED_PC;
        SET_PC(newPC-2);
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(20)
}

/* ANDI */
OPCODE(0x0200)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    res = DREGu8((Opcode >> 0) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ANDI */
OPCODE(0x0210)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ANDI */
OPCODE(0x0218)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ANDI */
OPCODE(0x0220)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ANDI */
OPCODE(0x0228)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ANDI */
OPCODE(0x0230)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* ANDI */
OPCODE(0x0238)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ANDI */
OPCODE(0x0239)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* ANDI */
OPCODE(0x021F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ANDI */
OPCODE(0x0227)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ANDI */
OPCODE(0x0240)
{
    uint32_t res;
    uint32_t src;

    FETCH_WORD(src);
    res = DREGu16((Opcode >> 0) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ANDI */
OPCODE(0x0250)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ANDI */
OPCODE(0x0258)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ANDI */
OPCODE(0x0260)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ANDI */
OPCODE(0x0268)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ANDI */
OPCODE(0x0270)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* ANDI */
OPCODE(0x0278)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ANDI */
OPCODE(0x0279)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* ANDI */
OPCODE(0x025F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ANDI */
OPCODE(0x0267)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ANDI */
OPCODE(0x0280)
{
    uint32_t res;
    uint32_t src;

    FETCH_LONG(src);
    res = DREGu32((Opcode >> 0) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(14)
}

/* ANDI */
OPCODE(0x0290)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ANDI */
OPCODE(0x0298)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ANDI */
OPCODE(0x02A0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ANDI */
OPCODE(0x02A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* ANDI */
OPCODE(0x02B0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(34)
}

/* ANDI */
OPCODE(0x02B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* ANDI */
OPCODE(0x02B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(36)
}

/* ANDI */
OPCODE(0x029F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ANDI */
OPCODE(0x02A7)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ANDICCR */
OPCODE(0x023C)
{
    uint32_t res;

    FETCH_BYTE(res);
    res &= M68K_CCR_MASK;
    res &= GET_CCR;
    SET_CCR(res)
    RET(20)
}

/* ANDISR */
OPCODE(0x027C)
{
    uint32_t res;

    if (flag_S)
    {
        FETCH_WORD(res);
        res &= M68K_SR_MASK;
        res &= GET_SR;
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(20)
    }
    else
    {
        uint32_t newPC = UNBASED_PC;
        SET_PC(newPC-2);
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(20)
}

/* EORI */
OPCODE(0x0A00)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    res = DREGu8((Opcode >> 0) & 7);
    res ^= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 0) & 7) = res;
    RET(8)
}

/* EORI */
OPCODE(0x0A10)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* EORI */
OPCODE(0x0A18)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* EORI */
OPCODE(0x0A20)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* EORI */
OPCODE(0x0A28)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* EORI */
OPCODE(0x0A30)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* EORI */
OPCODE(0x0A38)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* EORI */
OPCODE(0x0A39)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* EORI */
OPCODE(0x0A1F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* EORI */
OPCODE(0x0A27)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* EORI */
OPCODE(0x0A40)
{
    uint32_t res;
    uint32_t src;

    FETCH_WORD(src);
    res = DREGu16((Opcode >> 0) & 7);
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(8)
}

/* EORI */
OPCODE(0x0A50)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* EORI */
OPCODE(0x0A58)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* EORI */
OPCODE(0x0A60)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* EORI */
OPCODE(0x0A68)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* EORI */
OPCODE(0x0A70)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* EORI */
OPCODE(0x0A78)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* EORI */
OPCODE(0x0A79)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* EORI */
OPCODE(0x0A5F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* EORI */
OPCODE(0x0A67)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_WORD(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* EORI */
OPCODE(0x0A80)
{
    uint32_t res;
    uint32_t src;

    FETCH_LONG(src);
    res = DREGu32((Opcode >> 0) & 7);
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(16)
}

/* EORI */
OPCODE(0x0A90)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* EORI */
OPCODE(0x0A98)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* EORI */
OPCODE(0x0AA0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* EORI */
OPCODE(0x0AA8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* EORI */
OPCODE(0x0AB0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(34)
}

/* EORI */
OPCODE(0x0AB8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* EORI */
OPCODE(0x0AB9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(36)
}

/* EORI */
OPCODE(0x0A9F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* EORI */
OPCODE(0x0AA7)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(src);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* EORICCR */
OPCODE(0x0A3C)
{
    uint32_t res;

    FETCH_BYTE(res);
    res &= M68K_CCR_MASK;
    res ^= GET_CCR;
    SET_CCR(res)
    RET(20)
}

/* EORISR */
OPCODE(0x0A7C)
{
    uint32_t res;

    if (flag_S)
    {
        FETCH_WORD(res);
        res &= M68K_SR_MASK;
        res ^= GET_SR;
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(20)
    }
    else
    {
        uint32_t newPC = UNBASED_PC;
        SET_PC(newPC-2);
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(20)
}

/* SUBI */
OPCODE(0x0400)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    dst = DREGu8((Opcode >> 0) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(8)
}

/* SUBI */
OPCODE(0x0410)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* SUBI */
OPCODE(0x0418)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* SUBI */
OPCODE(0x0420)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SUBI */
OPCODE(0x0428)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* SUBI */
OPCODE(0x0430)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* SUBI */
OPCODE(0x0438)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* SUBI */
OPCODE(0x0439)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* SUBI */
OPCODE(0x041F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* SUBI */
OPCODE(0x0427)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SUBI */
OPCODE(0x0440)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_WORD(src);
    dst = DREGu16((Opcode >> 0) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(8)
}

/* SUBI */
OPCODE(0x0450)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* SUBI */
OPCODE(0x0458)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* SUBI */
OPCODE(0x0460)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* SUBI */
OPCODE(0x0468)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* SUBI */
OPCODE(0x0470)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* SUBI */
OPCODE(0x0478)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* SUBI */
OPCODE(0x0479)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_LONG(adr);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* SUBI */
OPCODE(0x045F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* SUBI */
OPCODE(0x0467)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* SUBI */
OPCODE(0x0480)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = DREGu32((Opcode >> 0) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(16)
}

/* SUBI */
OPCODE(0x0490)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* SUBI */
OPCODE(0x0498)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* SUBI */
OPCODE(0x04A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* SUBI */
OPCODE(0x04A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* SUBI */
OPCODE(0x04B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(34)
}

/* SUBI */
OPCODE(0x04B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* SUBI */
OPCODE(0x04B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_LONG(adr);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(36)
}

/* SUBI */
OPCODE(0x049F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* SUBI */
OPCODE(0x04A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ADDI */
OPCODE(0x0600)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    dst = DREGu8((Opcode >> 0) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ADDI */
OPCODE(0x0610)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ADDI */
OPCODE(0x0618)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ADDI */
OPCODE(0x0620)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ADDI */
OPCODE(0x0628)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ADDI */
OPCODE(0x0630)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* ADDI */
OPCODE(0x0638)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ADDI */
OPCODE(0x0639)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* ADDI */
OPCODE(0x061F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ADDI */
OPCODE(0x0627)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ADDI */
OPCODE(0x0640)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_WORD(src);
    dst = DREGu16((Opcode >> 0) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ADDI */
OPCODE(0x0650)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ADDI */
OPCODE(0x0658)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ADDI */
OPCODE(0x0660)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ADDI */
OPCODE(0x0668)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ADDI */
OPCODE(0x0670)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* ADDI */
OPCODE(0x0678)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ADDI */
OPCODE(0x0679)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_LONG(adr);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* ADDI */
OPCODE(0x065F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ADDI */
OPCODE(0x0667)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ADDI */
OPCODE(0x0680)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = DREGu32((Opcode >> 0) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(16)
}

/* ADDI */
OPCODE(0x0690)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ADDI */
OPCODE(0x0698)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ADDI */
OPCODE(0x06A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ADDI */
OPCODE(0x06A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* ADDI */
OPCODE(0x06B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(34)
}

/* ADDI */
OPCODE(0x06B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* ADDI */
OPCODE(0x06B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_LONG(adr);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(36)
}

/* ADDI */
OPCODE(0x069F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ADDI */
OPCODE(0x06A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* CMPI */
OPCODE(0x0C00)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    dst = DREGu8((Opcode >> 0) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(8)
}

/* CMPI */
OPCODE(0x0C10)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMPI */
OPCODE(0x0C18)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMPI */
OPCODE(0x0C20)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(14)
}

/* CMPI */
OPCODE(0x0C28)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(16)
}

/* CMPI */
OPCODE(0x0C30)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(18)
}

/* CMPI */
OPCODE(0x0C38)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(16)
}

/* CMPI */
OPCODE(0x0C39)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(20)
}

/* CMPI */
OPCODE(0x0C1F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMPI */
OPCODE(0x0C27)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(14)
}

/* CMPI */
OPCODE(0x0C40)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_WORD(src);
    dst = DREGu16((Opcode >> 0) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(8)
}

/* CMPI */
OPCODE(0x0C50)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMPI */
OPCODE(0x0C58)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMPI */
OPCODE(0x0C60)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(14)
}

/* CMPI */
OPCODE(0x0C68)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(16)
}

/* CMPI */
OPCODE(0x0C70)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(18)
}

/* CMPI */
OPCODE(0x0C78)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(16)
}

/* CMPI */
OPCODE(0x0C79)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    FETCH_LONG(adr);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(20)
}

/* CMPI */
OPCODE(0x0C5F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMPI */
OPCODE(0x0C67)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_WORD(src);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(14)
}

/* CMPI */
OPCODE(0x0C80)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = DREGu32((Opcode >> 0) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMPI */
OPCODE(0x0C90)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMPI */
OPCODE(0x0C98)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMPI */
OPCODE(0x0CA0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(22)
}

/* CMPI */
OPCODE(0x0CA8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(24)
}

/* CMPI */
OPCODE(0x0CB0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(26)
}

/* CMPI */
OPCODE(0x0CB8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(24)
}

/* CMPI */
OPCODE(0x0CB9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    FETCH_LONG(adr);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(28)
}

/* CMPI */
OPCODE(0x0C9F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMPI */
OPCODE(0x0CA7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(src);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(22)
}

/* BTSTn */
OPCODE(0x0800)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 31);
    res = DREGu32((Opcode >> 0) & 7);
    flag_NotZ = res & src;
    RET(10)
}

/* BTSTn */
OPCODE(0x0810)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(12)
}

/* BTSTn */
OPCODE(0x0818)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(12)
}

/* BTSTn */
OPCODE(0x0820)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(14)
}

/* BTSTn */
OPCODE(0x0828)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(16)
}

/* BTSTn */
OPCODE(0x0830)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(18)
}

/* BTSTn */
OPCODE(0x0838)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(16)
}

/* BTSTn */
OPCODE(0x0839)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(20)
}

/* BTSTn */
OPCODE(0x083A)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(16)
}

/* BTSTn */
OPCODE(0x083B)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(18)
}

/* BTSTn */
OPCODE(0x081F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(12)
}

/* BTSTn */
OPCODE(0x0827)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(14)
}

/* BCHGn */
OPCODE(0x0840)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 31);
    res = DREGu32((Opcode >> 0) & 7);
    flag_NotZ = res & src;
    res ^= src;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(12)
}

/* BCHGn */
OPCODE(0x0850)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCHGn */
OPCODE(0x0858)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCHGn */
OPCODE(0x0860)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BCHGn */
OPCODE(0x0868)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BCHGn */
OPCODE(0x0870)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* BCHGn */
OPCODE(0x0878)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BCHGn */
OPCODE(0x0879)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* BCHGn */
OPCODE(0x085F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCHGn */
OPCODE(0x0867)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BCLRn */
OPCODE(0x0880)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 31);
    res = DREGu32((Opcode >> 0) & 7);
    flag_NotZ = res & src;
    res &= ~src;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(14)
}

/* BCLRn */
OPCODE(0x0890)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCLRn */
OPCODE(0x0898)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCLRn */
OPCODE(0x08A0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BCLRn */
OPCODE(0x08A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BCLRn */
OPCODE(0x08B0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* BCLRn */
OPCODE(0x08B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BCLRn */
OPCODE(0x08B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* BCLRn */
OPCODE(0x089F)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCLRn */
OPCODE(0x08A7)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BSETn */
OPCODE(0x08C0)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 31);
    res = DREGu32((Opcode >> 0) & 7);
    flag_NotZ = res & src;
    res |= src;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(12)
}

/* BSETn */
OPCODE(0x08D0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BSETn */
OPCODE(0x08D8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BSETn */
OPCODE(0x08E0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BSETn */
OPCODE(0x08E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BSETn */
OPCODE(0x08F0)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* BSETn */
OPCODE(0x08F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BSETn */
OPCODE(0x08F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* BSETn */
OPCODE(0x08DF)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BSETn */
OPCODE(0x08E7)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_BYTE(src);
    src = 1 << (src & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BTST */
OPCODE(0x0100)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    src = 1 << (src & 31);
    res = DREGu32((Opcode >> 0) & 7);
    flag_NotZ = res & src;
    RET(6)
}

/* BTST */
OPCODE(0x0110)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(8)
}

/* BTST */
OPCODE(0x0118)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(8)
}

/* BTST */
OPCODE(0x0120)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(10)
}

/* BTST */
OPCODE(0x0128)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(12)
}

/* BTST */
OPCODE(0x0130)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(14)
}

/* BTST */
OPCODE(0x0138)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(12)
}

/* BTST */
OPCODE(0x0139)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(16)
}

/* BTST */
OPCODE(0x013A)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(12)
}

/* BTST */
OPCODE(0x013B)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(14)
}

/* BTST */
OPCODE(0x013C)
{
    uint32_t res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_BYTE(res);
    flag_NotZ = res & src;
    RET(8)
}

/* BTST */
OPCODE(0x011F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(8)
}

/* BTST */
OPCODE(0x0127)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    RET(10)
}

/* BCHG */
OPCODE(0x0140)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    src = 1 << (src & 31);
    res = DREGu32((Opcode >> 0) & 7);
    flag_NotZ = res & src;
    res ^= src;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* BCHG */
OPCODE(0x0150)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BCHG */
OPCODE(0x0158)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BCHG */
OPCODE(0x0160)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* BCHG */
OPCODE(0x0168)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCHG */
OPCODE(0x0170)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BCHG */
OPCODE(0x0178)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCHG */
OPCODE(0x0179)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BCHG */
OPCODE(0x015F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BCHG */
OPCODE(0x0167)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res ^= src;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* BCLR */
OPCODE(0x0180)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    src = 1 << (src & 31);
    res = DREGu32((Opcode >> 0) & 7);
    flag_NotZ = res & src;
    res &= ~src;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(10)
}

/* BCLR */
OPCODE(0x0190)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BCLR */
OPCODE(0x0198)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BCLR */
OPCODE(0x01A0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* BCLR */
OPCODE(0x01A8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCLR */
OPCODE(0x01B0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BCLR */
OPCODE(0x01B8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BCLR */
OPCODE(0x01B9)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BCLR */
OPCODE(0x019F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BCLR */
OPCODE(0x01A7)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res &= ~src;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* BSET */
OPCODE(0x01C0)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    src = 1 << (src & 31);
    res = DREGu32((Opcode >> 0) & 7);
    flag_NotZ = res & src;
    res |= src;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* BSET */
OPCODE(0x01D0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BSET */
OPCODE(0x01D8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BSET */
OPCODE(0x01E0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* BSET */
OPCODE(0x01E8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BSET */
OPCODE(0x01F0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* BSET */
OPCODE(0x01F8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* BSET */
OPCODE(0x01F9)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* BSET */
OPCODE(0x01DF)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* BSET */
OPCODE(0x01E7)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    src = 1 << (src & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    flag_NotZ = res & src;
    res |= src;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEPWaD */
OPCODE(0x0108)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr + 0, res)
    READ_BYTE_F(adr + 2, src)
    DREGu16((Opcode >> 9) & 7) = (res << 8) | src;
    RET(16)
}

/* MOVEPLaD */
OPCODE(0x0148)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res <<= 24;
    adr += 2;
    READ_BYTE_F(adr, src)
    res |= src << 16;
    adr += 2;
    READ_BYTE_F(adr, src)
    res |= src << 8;
    adr += 2;
    READ_BYTE_F(adr, src)
    DREG((Opcode >> 9) & 7) = res | src;
    RET(24)
}

/* MOVEPWDa */
OPCODE(0x0188)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    WRITE_BYTE_F(adr + 0, res >> 8)
    WRITE_BYTE_F(adr + 2, res >> 0)
    RET(16)
}

/* MOVEPLDa */
OPCODE(0x01C8)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    WRITE_BYTE_F(adr, res >> 24)
    adr += 2;
    WRITE_BYTE_F(adr, res >> 16)
    adr += 2;
    WRITE_BYTE_F(adr, res >> 8)
    adr += 2;
    WRITE_BYTE_F(adr, res >> 0)
    RET(24)
}

/* MOVEB */
OPCODE(0x1000)
{
    uint32_t res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEB */
OPCODE(0x1080)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(8)
}

/* MOVEB */
OPCODE(0x10C0)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(8)
}

/* MOVEB */
OPCODE(0x1100)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(8)
}

/* MOVEB */
OPCODE(0x1140)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1180)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x11C0)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x13C0)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1EC0)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(8)
}

/* MOVEB */
OPCODE(0x1F00)
{
    uint32_t adr, res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(8)
}

/* MOVEB */
OPCODE(0x1008)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    */
#endif
    RET(4)
}

/* MOVEB */
OPCODE(0x1088)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(8)
}

/* MOVEB */
OPCODE(0x10C8)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(8)
}

/* MOVEB */
OPCODE(0x1108)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(8)
}

/* MOVEB */
OPCODE(0x1148)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(12)
}

/* MOVEB */
OPCODE(0x1188)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(14)
}

/* MOVEB */
OPCODE(0x11C8)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(12)
}

/* MOVEB */
OPCODE(0x13C8)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(16)
}

/* MOVEB */
OPCODE(0x1EC8)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(8)
}

/* MOVEB */
OPCODE(0x1F08)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    */
#endif
    RET(8)
}

/* MOVEB */
OPCODE(0x1010)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEB */
OPCODE(0x1090)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x10D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1110)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1150)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1190)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x11D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x13D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x1ED0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1F10)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1018)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEB */
OPCODE(0x1098)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x10D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1118)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1158)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1198)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x11D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x13D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x1ED8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1F18)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1020)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* MOVEB */
OPCODE(0x10A0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x10E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x1120)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x1160)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x11A0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x11E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x13E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x1EE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x1F20)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x1028)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEB */
OPCODE(0x10A8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x10E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1128)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1168)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x11A8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x11E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x13E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* MOVEB */
OPCODE(0x1EE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1F28)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1030)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEB */
OPCODE(0x10B0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x10F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x1130)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x1170)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x11B0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* MOVEB */
OPCODE(0x11F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x13F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(26)
}

/* MOVEB */
OPCODE(0x1EF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x1F30)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x1038)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEB */
OPCODE(0x10B8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x10F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1138)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1178)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x11B8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x11F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x13F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* MOVEB */
OPCODE(0x1EF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1F38)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1039)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEB */
OPCODE(0x10B9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x10F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x1139)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x1179)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* MOVEB */
OPCODE(0x11B9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(26)
}

/* MOVEB */
OPCODE(0x11F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* MOVEB */
OPCODE(0x13F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(28)
}

/* MOVEB */
OPCODE(0x1EF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x1F39)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x103A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEB */
OPCODE(0x10BA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x10FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x113A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x117A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x11BA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x11FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x13FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* MOVEB */
OPCODE(0x1EFA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x1F3A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x103B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEB */
OPCODE(0x10BB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x10FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x113B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x117B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x11BB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(24)
}

/* MOVEB */
OPCODE(0x11FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x13FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(26)
}

/* MOVEB */
OPCODE(0x1EFB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x1F3B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x103C)
{
    uint32_t res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEB */
OPCODE(0x10BC)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x10FC)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x113C)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x117C)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x11BC)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x11FC)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x13FC)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x1EFC)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1F3C)
{
    uint32_t adr, res;

    FETCH_BYTE(res);
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x101F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEB */
OPCODE(0x109F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x10DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x111F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x115F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x119F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x11DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* MOVEB */
OPCODE(0x13DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x1EDF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1F1F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* MOVEB */
OPCODE(0x1027)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* MOVEB */
OPCODE(0x10A7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x10E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x1127)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x1167)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x11A7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* MOVEB */
OPCODE(0x11E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_SWORD(adr);
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MOVEB */
OPCODE(0x13E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    FETCH_LONG(adr);
    WRITE_BYTE_F(adr, res)
    RET(22)
}

/* MOVEB */
OPCODE(0x1EE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEB */
OPCODE(0x1F27)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* MOVEL */
OPCODE(0x2000)
{
    uint32_t res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEL */
OPCODE(0x2080)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x20C0)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x2100)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x2140)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(16)
}

/* MOVEL */
OPCODE(0x2180)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(18)
}

/* MOVEL */
OPCODE(0x21C0)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(16)
}

/* MOVEL */
OPCODE(0x23C0)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2EC0)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x2F00)
{
    uint32_t adr, res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x2008)
{
    uint32_t res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEL */
OPCODE(0x2088)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x20C8)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x2108)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x2148)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(16)
}

/* MOVEL */
OPCODE(0x2188)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(18)
}

/* MOVEL */
OPCODE(0x21C8)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(16)
}

/* MOVEL */
OPCODE(0x23C8)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2EC8)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x2F08)
{
    uint32_t adr, res;

    res = AREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(12)
}

/* MOVEL */
OPCODE(0x2010)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEL */
OPCODE(0x2090)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x20D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2110)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2150)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2190)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x21D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x23D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x2ED0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2F10)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2018)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEL */
OPCODE(0x2098)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x20D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2118)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2158)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2198)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x21D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x23D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x2ED8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2F18)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2020)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEL */
OPCODE(0x20A0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x20E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x2120)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x2160)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x21A0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x21E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x23E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x2EE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x2F20)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x2028)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEL */
OPCODE(0x20A8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x20E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2128)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2168)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x21A8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x21E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x23E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* MOVEL */
OPCODE(0x2EE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2F28)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2030)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* MOVEL */
OPCODE(0x20B0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x20F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x2130)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x2170)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x21B0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* MOVEL */
OPCODE(0x21F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x23F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(34)
}

/* MOVEL */
OPCODE(0x2EF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x2F30)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x2038)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEL */
OPCODE(0x20B8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x20F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2138)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2178)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x21B8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x21F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x23F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* MOVEL */
OPCODE(0x2EF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2F38)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2039)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* MOVEL */
OPCODE(0x20B9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x20F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x2139)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x2179)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* MOVEL */
OPCODE(0x21B9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(34)
}

/* MOVEL */
OPCODE(0x21F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* MOVEL */
OPCODE(0x23F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(36)
}

/* MOVEL */
OPCODE(0x2EF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x2F39)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x203A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEL */
OPCODE(0x20BA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x20FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x213A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x217A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x21BA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x21FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x23FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* MOVEL */
OPCODE(0x2EFA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x2F3A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x203B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* MOVEL */
OPCODE(0x20BB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x20FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x213B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x217B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x21BB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(32)
}

/* MOVEL */
OPCODE(0x21FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x23FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(34)
}

/* MOVEL */
OPCODE(0x2EFB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x2F3B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x203C)
{
    uint32_t res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEL */
OPCODE(0x20BC)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x20FC)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x213C)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x217C)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x21BC)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x21FC)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x23FC)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x2EFC)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2F3C)
{
    uint32_t adr, res;

    FETCH_LONG(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x201F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEL */
OPCODE(0x209F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x20DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x211F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x215F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x219F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x21DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* MOVEL */
OPCODE(0x23DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x2EDF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2F1F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(20)
}

/* MOVEL */
OPCODE(0x2027)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEL */
OPCODE(0x20A7)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x20E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x2127)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x2167)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x21A7)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* MOVEL */
OPCODE(0x21E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_SWORD(adr);
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* MOVEL */
OPCODE(0x23E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    FETCH_LONG(adr);
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* MOVEL */
OPCODE(0x2EE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7);
    AREG(7) += 4;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* MOVEL */
OPCODE(0x2F27)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    WRITE_LONG_DEC_F(adr, res)
    RET(22)
}

/* MOVEAL */
OPCODE(0x2040)
{
    uint32_t res;

    res = (int32_t)DREGs32((Opcode >> 0) & 7);
    AREG((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEAL */
OPCODE(0x2048)
{
    uint32_t res;

    res = (int32_t)AREGs32((Opcode >> 0) & 7);
    AREG((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEAL */
OPCODE(0x2050)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEAL */
OPCODE(0x2058)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEAL */
OPCODE(0x2060)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEAL */
OPCODE(0x2068)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEAL */
OPCODE(0x2070)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* MOVEAL */
OPCODE(0x2078)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEAL */
OPCODE(0x2079)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(20)
}

/* MOVEAL */
OPCODE(0x207A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEAL */
OPCODE(0x207B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* MOVEAL */
OPCODE(0x207C)
{
    uint32_t res;

    FETCH_LONG(res);
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEAL */
OPCODE(0x205F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEAL */
OPCODE(0x2067)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READSX_LONG_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEW */
OPCODE(0x3000)
{
    uint32_t res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEW */
OPCODE(0x3080)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x30C0)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x3100)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x3140)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3180)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x31C0)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x33C0)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3EC0)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x3F00)
{
    uint32_t adr, res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x3008)
{
    uint32_t res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEW */
OPCODE(0x3088)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x30C8)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x3108)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x3148)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3188)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x31C8)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x33C8)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3EC8)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x3F08)
{
    uint32_t adr, res;

    res = AREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(8)
}

/* MOVEW */
OPCODE(0x3010)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEW */
OPCODE(0x3090)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x30D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3110)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3150)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3190)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x31D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x33D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x3ED0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3F10)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3018)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEW */
OPCODE(0x3098)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x30D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3118)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3158)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3198)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x31D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x33D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x3ED8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3F18)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3020)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* MOVEW */
OPCODE(0x30A0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x30E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x3120)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x3160)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x31A0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x31E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x33E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x3EE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x3F20)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x3028)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEW */
OPCODE(0x30A8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x30E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3128)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3168)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x31A8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x31E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x33E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* MOVEW */
OPCODE(0x3EE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3F28)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3030)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEW */
OPCODE(0x30B0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x30F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x3130)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x3170)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x31B0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* MOVEW */
OPCODE(0x31F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x33F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(26)
}

/* MOVEW */
OPCODE(0x3EF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x3F30)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x3038)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEW */
OPCODE(0x30B8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x30F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3138)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3178)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x31B8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x31F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x33F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* MOVEW */
OPCODE(0x3EF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3F38)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3039)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEW */
OPCODE(0x30B9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x30F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x3139)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x3179)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* MOVEW */
OPCODE(0x31B9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(26)
}

/* MOVEW */
OPCODE(0x31F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* MOVEW */
OPCODE(0x33F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(28)
}

/* MOVEW */
OPCODE(0x3EF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x3F39)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x303A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEW */
OPCODE(0x30BA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x30FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x313A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x317A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x31BA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x31FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x33FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* MOVEW */
OPCODE(0x3EFA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x3F3A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x303B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEW */
OPCODE(0x30BB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x30FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x313B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x317B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x31BB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(24)
}

/* MOVEW */
OPCODE(0x31FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x33FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(26)
}

/* MOVEW */
OPCODE(0x3EFB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x3F3B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x303C)
{
    uint32_t res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEW */
OPCODE(0x30BC)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x30FC)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x313C)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x317C)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x31BC)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x31FC)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x33FC)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x3EFC)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3F3C)
{
    uint32_t adr, res;

    FETCH_WORD(res);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x301F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEW */
OPCODE(0x309F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x30DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x311F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x315F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x319F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x31DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVEW */
OPCODE(0x33DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x3EDF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3F1F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVEW */
OPCODE(0x3027)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* MOVEW */
OPCODE(0x30A7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x30E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x3127)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x3167)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 9) & 7);
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x31A7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG((Opcode >> 9) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVEW */
OPCODE(0x31E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVEW */
OPCODE(0x33E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(22)
}

/* MOVEW */
OPCODE(0x3EE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEW */
OPCODE(0x3F27)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEAW */
OPCODE(0x3040)
{
    uint32_t res;

    res = (int32_t)DREGs16((Opcode >> 0) & 7);
    AREG((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEAW */
OPCODE(0x3048)
{
    uint32_t res;

    res = (int32_t)AREGs16((Opcode >> 0) & 7);
    AREG((Opcode >> 9) & 7) = res;
    RET(4)
}

/* MOVEAW */
OPCODE(0x3050)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEAW */
OPCODE(0x3058)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEAW */
OPCODE(0x3060)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(10)
}

/* MOVEAW */
OPCODE(0x3068)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEAW */
OPCODE(0x3070)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEAW */
OPCODE(0x3078)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEAW */
OPCODE(0x3079)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* MOVEAW */
OPCODE(0x307A)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* MOVEAW */
OPCODE(0x307B)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* MOVEAW */
OPCODE(0x307C)
{
    uint32_t res;

    FETCH_SWORD(res);
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEAW */
OPCODE(0x305F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* MOVEAW */
OPCODE(0x3067)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READSX_WORD_F(adr, res)
    AREG((Opcode >> 9) & 7) = res;
    RET(10)
}

/* NEGX */
OPCODE(0x4000)
{
    uint32_t res;
    uint32_t src;

    src = DREGu8((Opcode >> 0) & 7);
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* NEGX */
OPCODE(0x4010)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NEGX */
OPCODE(0x4018)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NEGX */
OPCODE(0x4020)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* NEGX */
OPCODE(0x4028)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* NEGX */
OPCODE(0x4030)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* NEGX */
OPCODE(0x4038)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* NEGX */
OPCODE(0x4039)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* NEGX */
OPCODE(0x401F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NEGX */
OPCODE(0x4027)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* NEGX */
OPCODE(0x4040)
{
    uint32_t res;
    uint32_t src;

    src = DREGu16((Opcode >> 0) & 7);
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(4)
}

/* NEGX */
OPCODE(0x4050)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NEGX */
OPCODE(0x4058)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NEGX */
OPCODE(0x4060)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* NEGX */
OPCODE(0x4068)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* NEGX */
OPCODE(0x4070)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* NEGX */
OPCODE(0x4078)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* NEGX */
OPCODE(0x4079)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* NEGX */
OPCODE(0x405F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NEGX */
OPCODE(0x4067)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* NEGX */
OPCODE(0x4080)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 0) & 7);
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(6)
}

/* NEGX */
OPCODE(0x4090)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) | (src >> 1) | (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NEGX */
OPCODE(0x4098)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NEGX */
OPCODE(0x40A0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* NEGX */
OPCODE(0x40A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* NEGX */
OPCODE(0x40B0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* NEGX */
OPCODE(0x40B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* NEGX */
OPCODE(0x40B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* NEGX */
OPCODE(0x409F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NEGX */
OPCODE(0x40A7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    res = -src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = (res?1:0)<<8;
    /*	flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23; */
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* CLR */
OPCODE(0x4200)
{
    uint32_t res;

    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* CLR */
OPCODE(0x4210)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x4218)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x4220)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* CLR */
OPCODE(0x4228)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* CLR */
OPCODE(0x4230)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* CLR */
OPCODE(0x4238)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* CLR */
OPCODE(0x4239)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* CLR */
OPCODE(0x421F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x4227)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* CLR */
OPCODE(0x4240)
{
    uint32_t res;

    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(4)
}

/* CLR */
OPCODE(0x4250)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x4258)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x4260)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* CLR */
OPCODE(0x4268)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* CLR */
OPCODE(0x4270)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* CLR */
OPCODE(0x4278)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* CLR */
OPCODE(0x4279)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* CLR */
OPCODE(0x425F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x4267)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* CLR */
OPCODE(0x4280)
{
    uint32_t res;

    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(6)
}

/* CLR */
OPCODE(0x4290)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x4298)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x42A0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x42A8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x42B0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x42B8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x42B9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x429F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* CLR */
OPCODE(0x42A7)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    res = 0;
    flag_N = flag_NotZ = flag_V = flag_C = 0;
    WRITE_LONG_F(adr, res)
    RET(12)
}

/* NEG */
OPCODE(0x4400)
{
    uint32_t res;
    uint32_t src;

    src = DREGu8((Opcode >> 0) & 7);
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* NEG */
OPCODE(0x4410)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NEG */
OPCODE(0x4418)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NEG */
OPCODE(0x4420)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* NEG */
OPCODE(0x4428)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* NEG */
OPCODE(0x4430)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* NEG */
OPCODE(0x4438)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* NEG */
OPCODE(0x4439)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* NEG */
OPCODE(0x441F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NEG */
OPCODE(0x4427)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    res = -src;
    flag_V = res & src;
    flag_N = flag_X = flag_C = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* NEG */
OPCODE(0x4440)
{
    uint32_t res;
    uint32_t src;

    src = DREGu16((Opcode >> 0) & 7);
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(4)
}

/* NEG */
OPCODE(0x4450)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NEG */
OPCODE(0x4458)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NEG */
OPCODE(0x4460)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* NEG */
OPCODE(0x4468)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* NEG */
OPCODE(0x4470)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* NEG */
OPCODE(0x4478)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* NEG */
OPCODE(0x4479)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* NEG */
OPCODE(0x445F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NEG */
OPCODE(0x4467)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    res = -src;
    flag_V = (res & src) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* NEG */
OPCODE(0x4480)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 0) & 7);
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(6)
}

/* NEG */
OPCODE(0x4490)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NEG */
OPCODE(0x4498)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NEG */
OPCODE(0x44A0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* NEG */
OPCODE(0x44A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* NEG */
OPCODE(0x44B0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* NEG */
OPCODE(0x44B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* NEG */
OPCODE(0x44B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* NEG */
OPCODE(0x449F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NEG */
OPCODE(0x44A7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    res = -src;
    flag_NotZ = res;
    flag_V = (res & src) >> 24;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* NOT */
OPCODE(0x4600)
{
    uint32_t res;
    uint32_t src;

    src = DREGu8((Opcode >> 0) & 7);
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* NOT */
OPCODE(0x4610)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NOT */
OPCODE(0x4618)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NOT */
OPCODE(0x4620)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* NOT */
OPCODE(0x4628)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* NOT */
OPCODE(0x4630)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* NOT */
OPCODE(0x4638)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* NOT */
OPCODE(0x4639)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* NOT */
OPCODE(0x461F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* NOT */
OPCODE(0x4627)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_N = res;
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* NOT */
OPCODE(0x4640)
{
    uint32_t res;
    uint32_t src;

    src = DREGu16((Opcode >> 0) & 7);
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(4)
}

/* NOT */
OPCODE(0x4650)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NOT */
OPCODE(0x4658)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NOT */
OPCODE(0x4660)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* NOT */
OPCODE(0x4668)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* NOT */
OPCODE(0x4670)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* NOT */
OPCODE(0x4678)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* NOT */
OPCODE(0x4679)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* NOT */
OPCODE(0x465F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* NOT */
OPCODE(0x4667)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    res = ~src;
    flag_C = 0;
    flag_V = 0;
    flag_NotZ = res & 0xFFFF;
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* NOT */
OPCODE(0x4680)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 0) & 7);
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(6)
}

/* NOT */
OPCODE(0x4690)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NOT */
OPCODE(0x4698)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NOT */
OPCODE(0x46A0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* NOT */
OPCODE(0x46A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* NOT */
OPCODE(0x46B0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* NOT */
OPCODE(0x46B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* NOT */
OPCODE(0x46B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* NOT */
OPCODE(0x469F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* NOT */
OPCODE(0x46A7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    res = ~src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* MOVESRa */
OPCODE(0x40C0)
{
    uint32_t res;

    res = GET_SR;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(6)
}

/* MOVESRa */
OPCODE(0x40D0)
{
    uint32_t adr, res;

    res = GET_SR;
    adr = AREG((Opcode >> 0) & 7);
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVESRa */
OPCODE(0x40D8)
{
    uint32_t adr, res;

    res = GET_SR;
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVESRa */
OPCODE(0x40E0)
{
    uint32_t adr, res;

    res = GET_SR;
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVESRa */
OPCODE(0x40E8)
{
    uint32_t adr, res;

    res = GET_SR;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVESRa */
OPCODE(0x40F0)
{
    uint32_t adr, res;

    res = GET_SR;
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* MOVESRa */
OPCODE(0x40F8)
{
    uint32_t adr, res;

    res = GET_SR;
    FETCH_SWORD(adr);
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* MOVESRa */
OPCODE(0x40F9)
{
    uint32_t adr, res;

    res = GET_SR;
    FETCH_LONG(adr);
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* MOVESRa */
OPCODE(0x40DF)
{
    uint32_t adr, res;

    res = GET_SR;
    adr = AREG(7);
    AREG(7) += 2;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* MOVESRa */
OPCODE(0x40E7)
{
    uint32_t adr, res;

    res = GET_SR;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* MOVEaCCR */
OPCODE(0x44C0)
{
    uint32_t res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_CCR(res)
    RET(12)
}

/* MOVEaCCR */
OPCODE(0x44D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(16)
}

/* MOVEaCCR */
OPCODE(0x44D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(16)
}

/* MOVEaCCR */
OPCODE(0x44E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(18)
}

/* MOVEaCCR */
OPCODE(0x44E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(20)
}

/* MOVEaCCR */
OPCODE(0x44F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(22)
}

/* MOVEaCCR */
OPCODE(0x44F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(20)
}

/* MOVEaCCR */
OPCODE(0x44F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(24)
}

/* MOVEaCCR */
OPCODE(0x44FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(20)
}

/* MOVEaCCR */
OPCODE(0x44FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(22)
}

/* MOVEaCCR */
OPCODE(0x44FC)
{
    uint32_t res;

    FETCH_WORD(res);
    SET_CCR(res)
    RET(16)
}

/* MOVEaCCR */
OPCODE(0x44DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(16)
}

/* MOVEaCCR */
OPCODE(0x44E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_CCR(res)
    RET(18)
}

/* MOVEaSR */
OPCODE(0x46C0)
{
    uint32_t res;

    if (flag_S)
    {
        res = DREGu16((Opcode >> 0) & 7);
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(12)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(12)
}

/* MOVEaSR */
OPCODE(0x46D0)
{
    uint32_t adr, res;

    if (flag_S)
    {
        adr = AREG((Opcode >> 0) & 7);
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(16)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(16)
}

/* MOVEaSR */
OPCODE(0x46D8)
{
    uint32_t adr, res;

    if (flag_S)
    {
        adr = AREG((Opcode >> 0) & 7);
        AREG((Opcode >> 0) & 7) += 2;
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(16)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(16)
}

/* MOVEaSR */
OPCODE(0x46E0)
{
    uint32_t adr, res;

    if (flag_S)
    {
        adr = AREG((Opcode >> 0) & 7) - 2;
        AREG((Opcode >> 0) & 7) = adr;
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(18)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(18)
}

/* MOVEaSR */
OPCODE(0x46E8)
{
    uint32_t adr, res;

    if (flag_S)
    {
        FETCH_SWORD(adr);
        adr += AREG((Opcode >> 0) & 7);
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(20)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(20)
}

/* MOVEaSR */
OPCODE(0x46F0)
{
    uint32_t adr, res;

    if (flag_S)
    {
        adr = AREG((Opcode >> 0) & 7);
        DECODE_EXT_WORD
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(22)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(22)
}


/* MOVEaSR */
OPCODE(0x46F8)
{
    uint32_t adr, res;

    if (flag_S)
    {
        FETCH_SWORD(adr);
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(20)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(20)
}

/* MOVEaSR */
OPCODE(0x46F9)
{
    uint32_t adr, res;

    if (flag_S)
    {
        FETCH_LONG(adr);
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(24)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(24)
}

/* MOVEaSR */
OPCODE(0x46FA)
{
    uint32_t adr, res;

    if (flag_S)
    {
        adr = GET_SWORD + UNBASED_PC;
        INC_PC(2);
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(24)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(20)
}

/* MOVEaSR */
OPCODE(0x46FB)
{
    uint32_t adr, res;

    if (flag_S)
    {
        adr = UNBASED_PC;
        DECODE_EXT_WORD
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(22)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(22)
}

/* MOVEaSR */
OPCODE(0x46FC)
{
    uint32_t res;

    if (flag_S)
    {
        FETCH_WORD(res);
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(16)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(16)
}

/* MOVEaSR */
OPCODE(0x46DF)
{
    uint32_t adr, res;

    if (flag_S)
    {
        adr = AREG(7);
        AREG(7) += 2;
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(16)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(16)
}

/* MOVEaSR */
OPCODE(0x46E7)
{
    uint32_t adr, res;

    if (flag_S)
    {
        adr = AREG(7) - 2;
        AREG(7) = adr;
        READ_WORD_F(adr, res)
        SET_SR(res)
        if (!flag_S)
        {
            res = AREG(7);
            AREG(7) = ASP;
            ASP = res;
        }
        CHECK_INT_TO_JUMP(18)
    }
    else
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    RET(18)
}

/* NBCD */
OPCODE(0x4800)
{
    uint32_t res;

    res = DREGu8((Opcode >> 0) & 7);
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(6)
}

/* NBCD */
OPCODE(0x4810)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(12)
}

/* NBCD */
OPCODE(0x4818)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(12)
}

/* NBCD */
OPCODE(0x4820)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(14)
}

/* NBCD */
OPCODE(0x4828)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(16)
}

/* NBCD */
OPCODE(0x4830)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(18)
}

/* NBCD */
OPCODE(0x4838)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(16)
}

/* NBCD */
OPCODE(0x4839)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(20)
}

/* NBCD */
OPCODE(0x481F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(12)
}

/* NBCD */
OPCODE(0x4827)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    res = 0x9a - res - ((flag_X >> M68K_SR_X_SFT) & 1);

    if (res != 0x9a)
    {
        if ((res & 0x0f) == 0xa) res = (res & 0xf0) + 0x10;
        res &= 0xFF;
        WRITE_BYTE_F(adr, res)
        flag_NotZ |= res;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_N = res;
    RET(14)
}

/* PEA */
OPCODE(0x4850)
{
    uint32_t adr;

    adr = AREG((Opcode >> 0) & 7);
    PUSH_32_F(adr)
    RET(12)
}

/* PEA */
OPCODE(0x4868)
{
    uint32_t adr;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    PUSH_32_F(adr)
    RET(16)
}

/* PEA */
OPCODE(0x4870)
{
    uint32_t adr;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    PUSH_32_F(adr)
    RET(20)
}

/* PEA */
OPCODE(0x4878)
{
    uint32_t adr;

    FETCH_SWORD(adr);
    PUSH_32_F(adr)
    RET(16)
}

/* PEA */
OPCODE(0x4879)
{
    uint32_t adr;

    FETCH_LONG(adr);
    PUSH_32_F(adr)
    RET(20)
}

/* PEA */
OPCODE(0x487A)
{
    uint32_t adr;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    PUSH_32_F(adr)
    RET(16)
}

/* PEA */
OPCODE(0x487B)
{
    uint32_t adr;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    PUSH_32_F(adr)
    RET(20)
}

/* SWAP */
OPCODE(0x4840)
{
    uint32_t res;

    res = DREGu32((Opcode >> 0) & 7);
    res = (res >> 16) | (res << 16);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(4)
}

/* MOVEMRa */
OPCODE(0x4890)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(8)
}

/* MOVEMRa */
OPCODE(0x48A0)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    psrc = &AREGu32(7);
    dst = adr;
    do
    {
        if (res & 1)
        {
            adr -= 2;
            WRITE_WORD_F(adr, *psrc)
        }
        psrc--;
    }
    while (res >>= 1);
    AREG((Opcode >> 0) & 7) = adr;
    io_cycle_counter -= (dst - adr) * 2;
    RET(8)
}

/* MOVEMRa */
OPCODE(0x48A8)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMRa */
OPCODE(0x48B0)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(14)
}

/* MOVEMRa */
OPCODE(0x48B8)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_SWORD(adr);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMRa */
OPCODE(0x48B9)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_LONG(adr);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(16)
}

/* MOVEMRa */
OPCODE(0x48A7)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG(7);
    psrc = &AREGu32(7);
    dst = adr;
    do
    {
        if (res & 1)
        {
            adr -= 2;
            WRITE_WORD_F(adr, *psrc)
        }
        psrc--;
    }
    while (res >>= 1);
    AREG(7) = adr;
    io_cycle_counter -= (dst - adr) * 2;
    RET(8)
}

/* MOVEMRa */
OPCODE(0x48D0)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(8)
}

/* MOVEMRa */
OPCODE(0x48E0)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    psrc = &AREGu32(7);
    dst = adr;
    do
    {
        if (res & 1)
        {
            adr -= 4;
            WRITE_LONG_DEC_F(adr, *psrc)
        }
        psrc--;
    }
    while (res >>= 1);
    AREG((Opcode >> 0) & 7) = adr;
    io_cycle_counter -= (dst - adr) * 2;
    RET(8)
}

/* MOVEMRa */
OPCODE(0x48E8)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMRa */
OPCODE(0x48F0)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(14)
}

/* MOVEMRa */
OPCODE(0x48F8)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_SWORD(adr);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMRa */
OPCODE(0x48F9)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_LONG(adr);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            WRITE_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(16)
}

/* MOVEMRa */
OPCODE(0x48E7)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG(7);
    psrc = &AREGu32(7);
    dst = adr;
    do
    {
        if (res & 1)
        {
            adr -= 4;
            WRITE_LONG_DEC_F(adr, *psrc)
        }
        psrc--;
    }
    while (res >>= 1);
    AREG(7) = adr;
    io_cycle_counter -= (dst - adr) * 2;
    RET(8)
}

/* EXT */
OPCODE(0x4880)
{
    uint32_t res;

    res = (int32_t)DREGs8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    DREGu16((Opcode >> 0) & 7) = res;
    RET(4)
}

/* EXT */
OPCODE(0x48C0)
{
    uint32_t res;

    res = (int32_t)DREGs16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(4)
}

/* TST */
OPCODE(0x4A00)
{
    uint32_t res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    RET(4)
}

/* TST */
OPCODE(0x4A10)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(8)
}

/* TST */
OPCODE(0x4A18)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(8)
}

/* TST */
OPCODE(0x4A20)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(10)
}

/* TST */
OPCODE(0x4A28)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(12)
}

/* TST */
OPCODE(0x4A30)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(14)
}

/* TST */
OPCODE(0x4A38)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(12)
}

/* TST */
OPCODE(0x4A39)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(16)
}

/* TST */
OPCODE(0x4A1F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(8)
}

/* TST */
OPCODE(0x4A27)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
    RET(10)
}

/* TST */
OPCODE(0x4A40)
{
    uint32_t res;

    res = DREGu16((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(4)
}

/* TST */
OPCODE(0x4A50)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(8)
}

/* TST */
OPCODE(0x4A58)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(8)
}

/* TST */
OPCODE(0x4A60)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(10)
}

/* TST */
OPCODE(0x4A68)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(12)
}

/* TST */
OPCODE(0x4A70)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(14)
}

/* TST */
OPCODE(0x4A78)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(12)
}

/* TST */
OPCODE(0x4A79)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(16)
}

/* TST */
OPCODE(0x4A5F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(8)
}

/* TST */
OPCODE(0x4A67)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    RET(10)
}

/* TST */
OPCODE(0x4A80)
{
    uint32_t res;

    res = DREGu32((Opcode >> 0) & 7);
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(4)
}

/* TST */
OPCODE(0x4A90)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(12)
}

/* TST */
OPCODE(0x4A98)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(12)
}

/* TST */
OPCODE(0x4AA0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(14)
}

/* TST */
OPCODE(0x4AA8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(16)
}

/* TST */
OPCODE(0x4AB0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(18)
}

/* TST */
OPCODE(0x4AB8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(16)
}

/* TST */
OPCODE(0x4AB9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(20)
}

/* TST */
OPCODE(0x4A9F)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(12)
}

/* TST */
OPCODE(0x4AA7)
{
    uint32_t adr, res;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    RET(14)
}

/* TAS */
OPCODE(0x4AC0)
{
    uint32_t res;

    res = DREGu8((Opcode >> 0) & 7);
    SET_FLAGS_NZ_VC0
    res |= 0x80;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* TAS */
OPCODE(0x4AD0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(18)
}

/* TAS */
OPCODE(0x4AD8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(18)
}

/* TAS */
OPCODE(0x4AE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(20)
}

/* TAS */
OPCODE(0x4AE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(22)
}

/* TAS */
OPCODE(0x4AF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(24)
}

/* TAS */
OPCODE(0x4AF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(22)
}

/* TAS */
OPCODE(0x4AF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(26)
}

/* TAS */
OPCODE(0x4ADF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(18)
}

/* TAS */
OPCODE(0x4AE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    SET_FLAGS_NZ_VC0
#ifndef FAME_BYPASS_TAS_WRITEBACK
    WRITE_BYTE_F(adr, res|0x80)
#endif
    RET(20)
}

/* ILLEGAL */
OPCODE(0x4AFC)
{
    uint32_t oldPC=UNBASED_PC;
    SET_PC(oldPC-2)
    execute_exception(M68K_ILLEGAL_INSTRUCTION_EX);
    RET(0)
}

/* ILLEGAL A000-AFFF */
OPCODE(0xA000)
{
    uint32_t oldPC=UNBASED_PC;
    SET_PC(oldPC-2)
    execute_exception(M68K_1010_EX);
    RET(0)
}

/* ILLEGAL F000-FFFF */
OPCODE(0xF000)
{
    uint32_t oldPC=UNBASED_PC;
    SET_PC(oldPC-2)
    execute_exception(M68K_1111_EX);
    RET(0)
}

/* MOVEMaR */
OPCODE(0x4C90)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMaR */
OPCODE(0x4C98)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    AREG((Opcode >> 0) & 7) = adr;
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMaR */
OPCODE(0x4CA8)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(16)
}

/* MOVEMaR */
OPCODE(0x4CB0)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(18)
}

/* MOVEMaR */
OPCODE(0x4CB8)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    FETCH_SWORD(adr);
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(16)
}

/* MOVEMaR */
OPCODE(0x4CB9)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    FETCH_LONG(adr);
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(20)
}

/* MOVEMaR */
OPCODE(0x4CBA)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(16)
}

/* MOVEMaR */
OPCODE(0x4CBB)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    adr = UNBASED_PC;
    DECODE_EXT_WORD
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(18)
}

/* MOVEMaR */
OPCODE(0x4C9F)
{
    uint32_t adr, res;
    uint32_t dst;

    int32_t *psrc;

    FETCH_WORD(res);
    adr = AREG(7);
    psrc = &DREGs32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READSX_WORD_F(adr, *psrc)
            adr += 2;
        }
        psrc++;
    }
    while (res >>= 1);
    AREG(7) = adr;
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMaR */
OPCODE(0x4CD0)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMaR */
OPCODE(0x4CD8)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    AREG((Opcode >> 0) & 7) = adr;
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* MOVEMaR */
OPCODE(0x4CE8)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(16)
}

/* MOVEMaR */
OPCODE(0x4CF0)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(18)
}

/* MOVEMaR */
OPCODE(0x4CF8)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_SWORD(adr);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(16)
}

/* MOVEMaR */
OPCODE(0x4CF9)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    FETCH_LONG(adr);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(20)
}

/* MOVEMaR */
OPCODE(0x4CFA)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(16)
}

/* MOVEMaR */
OPCODE(0x4CFB)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = UNBASED_PC;
    DECODE_EXT_WORD
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    io_cycle_counter -= (adr - dst) * 2;
    RET(18)
}

/* MOVEMaR */
OPCODE(0x4CDF)
{
    uint32_t adr, res;
    uint32_t dst;

    uint32_t *psrc;

    FETCH_WORD(res);
    adr = AREG(7);
    psrc = &DREGu32(0);
    dst = adr;
    do
    {
        if (res & 1)
        {
            READ_LONG_F(adr, *psrc)
            adr += 4;
        }
        psrc++;
    }
    while (res >>= 1);
    AREG(7) = adr;
    io_cycle_counter -= (adr - dst) * 2;
    RET(12)
}

/* TRAP */
OPCODE(0x4E40)
{
    execute_exception(M68K_TRAP_BASE_EX + (Opcode & 0xF));
    RET(4)
}

/* LINK */
OPCODE(0x4E50)
{
    uint32_t res;

    res = AREGu32((Opcode >> 0) & 7);
    PUSH_32_F(res)
    res = AREG(7);
    AREG((Opcode >> 0) & 7) = res;
    FETCH_SWORD(res);
    AREG(7) += res;
    RET(16)
}

/* LINKA7 */
OPCODE(0x4E57)
{
    uint32_t res;

    AREG(7) -= 4;
    WRITE_LONG_DEC_F(AREG(7), AREG(7))
    FETCH_SWORD(res);
    AREG(7) += res;
    RET(16)
}

/* ULNK */
OPCODE(0x4E58)
{
    uint32_t res;
    uint32_t src;

    src = AREGu32((Opcode >> 0) & 7);
    AREG(7) = src + 4;
    READ_LONG_F(src, res)
    AREG((Opcode >> 0) & 7) = res;
    RET(12)
}

/* ULNKA7 */
OPCODE(0x4E5F)
{

    READ_LONG_F(AREG(7), AREG(7))
    RET(12)
}

/* MOVEAUSP */
OPCODE(0x4E60)
{
    uint32_t res;

    if (!flag_S)
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    res = AREGu32((Opcode >> 0) & 7);
    ASP = res;
    RET(4)
}

/* MOVEUSPA */
OPCODE(0x4E68)
{
    uint32_t res;

    if (!flag_S)
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    res = ASP;
    AREG((Opcode >> 0) & 7) = res;
    RET(4)
}

/* RESET */
OPCODE(0x4E70)
{

    if (!flag_S)
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    if (FAME_CONTEXT.reset_handler)
        (*FAME_CONTEXT.reset_handler)();
    /*	CPU->Reset_CallBack(); */
    RET(132)
}

/* NOP */
OPCODE(0x4E71)
{

    RET(4)
}

/* STOP */
OPCODE(0x4E72)
{
    uint32_t res;

    if (!flag_S)
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    FETCH_WORD(res);
    res &= M68K_SR_MASK;
    SET_SR(res)
    if (!flag_S)
    {
        res = AREG(7);
        AREG(7) = ASP;
        ASP = res;
    }
    FAME_CONTEXT.execinfo |= M68K_HALTED;
    CHECK_INT_TO_JUMP(4)
    RET_STOP(4)
}

/* RTE */
OPCODE(0x4E73)
{
    uint32_t res;

    if (!flag_S)
    {
        uint32_t oldPC=UNBASED_PC;
        SET_PC(oldPC-2)
        execute_exception(M68K_PRIVILEGE_VIOLATION_EX);
        RET(4)
    }
    POP_16_F(res)
    SET_SR(res)
    POP_32_F(res)
    SET_PC(res)
    if (!flag_S)
    {
        res = AREG(7);
        AREG(7) = ASP;
        ASP = res;
    }
    FAME_CONTEXT.execinfo &= ~(M68K_EMULATE_GROUP_0|M68K_EMULATE_TRACE|M68K_DO_TRACE);
    CHECK_INT_TO_JUMP(20)
    RET(20)
}

/* RTS */
OPCODE(0x4E75)
{
    uint32_t res;

    POP_32_F(res)
    SET_PC(res)
#ifdef FAME_CHANGE_PC
    fame_change_pc(UNBASED_PC);
	//change_pc24bew(UNBASED_PC);
#endif
    CHECK_BRANCH_EXCEPTION(res)
    RET(16)
}

/* TRAPV */
OPCODE(0x4E76)
{
    if (flag_V & 0x80)
        execute_exception(M68K_TRAPV_EX);
    RET(4)
}

/* RTR */
OPCODE(0x4E77)
{
    uint32_t res;

    POP_16_F(res)
    SET_CCR(res)
    POP_32_F(res)
    SET_PC(res)
    CHECK_BRANCH_EXCEPTION(res)
    RET(20)
}

/* JSR */
OPCODE(0x4E90)
{
    uint32_t adr;

    adr = AREG((Opcode >> 0) & 7);
    {
        uint32_t oldPC;

        oldPC = UNBASED_PC;
        PUSH_32_F(oldPC)
    }
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(16)
}

/* JSR */
OPCODE(0x4EA8)
{
    uint32_t adr;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    {
        uint32_t oldPC;

        oldPC = UNBASED_PC;
        PUSH_32_F(oldPC)
    }
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(18)
}

/* JSR */
OPCODE(0x4EB0)
{
    uint32_t adr;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    {
        uint32_t oldPC;

        oldPC = UNBASED_PC;
        PUSH_32_F(oldPC)
    }
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(22)
}

/* JSR */
OPCODE(0x4EB8)
{
    uint32_t adr;

    FETCH_SWORD(adr);
    {
        uint32_t oldPC;

        oldPC = UNBASED_PC;
        PUSH_32_F(oldPC)
    }
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(18)
}

/* JSR */
OPCODE(0x4EB9)
{
    uint32_t adr;

    FETCH_LONG(adr);
    {
        uint32_t oldPC;

        oldPC = UNBASED_PC;
        PUSH_32_F(oldPC)
    }
    SET_PC(adr)
#ifdef FAME_CHANGE_PC
    //m68000_change_pc(UNBASED_PC);
#endif
    CHECK_BRANCH_EXCEPTION(adr)
    RET(20)
}

/* JSR */
OPCODE(0x4EBA)
{
    uint32_t adr;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    {
        uint32_t oldPC;

        oldPC = UNBASED_PC;
        PUSH_32_F(oldPC)
    }
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(18)
}

/* JSR */
OPCODE(0x4EBB)
{
    uint32_t adr;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    {
        uint32_t oldPC;

        oldPC = UNBASED_PC;
        PUSH_32_F(oldPC)
    }
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(22)
}

/* JMP */
OPCODE(0x4ED0)
{
    uint32_t adr;

    adr = AREG((Opcode >> 0) & 7);
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(8)
}

/* JMP */
OPCODE(0x4EE8)
{
    uint32_t adr;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(10)
}

/* JMP */
OPCODE(0x4EF0)
{
    uint32_t adr;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(14)
}

/* JMP */
OPCODE(0x4EF8)
{
    uint32_t adr;

    FETCH_SWORD(adr);
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(10)
}

/* JMP */
OPCODE(0x4EF9)
{
    uint32_t adr;

    FETCH_LONG(adr);
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(12)
}

/* JMP */
OPCODE(0x4EFA)
{
    uint32_t adr;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(10)
}

/* JMP */
OPCODE(0x4EFB)
{
    uint32_t adr;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    SET_PC(adr)
    CHECK_BRANCH_EXCEPTION(adr)
    RET(14)
}

/* CHK */
OPCODE(0x4180)
{
    uint32_t res;
    uint32_t src;

    src = DREGu16((Opcode >> 0) & 7);
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(10)
}

/* CHK */
OPCODE(0x4190)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(14)
}

/* CHK */
OPCODE(0x4198)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(14)
}

/* CHK */
OPCODE(0x41A0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(16)
}

/* CHK */
OPCODE(0x41A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(18)
}

/* CHK */
OPCODE(0x41B0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(20)
}

/* CHK */
OPCODE(0x41B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(18)
}

/* CHK */
OPCODE(0x41B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(22)
}

/* CHK */
OPCODE(0x41BA)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(18)
}

/* CHK */
OPCODE(0x41BB)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(20)
}

/* CHK */
OPCODE(0x41BC)
{
    uint32_t res;
    uint32_t src;

    FETCH_WORD(src);
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(14)
}

/* CHK */
OPCODE(0x419F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(14)
}

/* CHK */
OPCODE(0x41A7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    if (((int32_t)res < 0) || (res > src))
    {
        flag_N = res >> 8;
        execute_exception(M68K_CHK_EX);
    }
    RET(16)
}

/* LEA */
OPCODE(0x41D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    res = adr;
    AREG((Opcode >> 9) & 7) = res;
    RET(4)
}

/* LEA */
OPCODE(0x41E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    res = adr;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* LEA */
OPCODE(0x41F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    res = adr;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* LEA */
OPCODE(0x41F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    res = adr;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* LEA */
OPCODE(0x41F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    res = adr;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* LEA */
OPCODE(0x41FA)
{
    uint32_t adr, res;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    res = adr;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* LEA */
OPCODE(0x41FB)
{
    uint32_t adr, res;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    res = adr;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* STCC */
OPCODE(0x50C0)
{
    uint32_t res;

    res = 0xFF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(6)
}

/* STCC */
OPCODE(0x51C0)
{
    uint32_t res;

    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x52C0)
{
    uint32_t res;

    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x53C0)
{
    uint32_t res;

    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x54C0)
{
    uint32_t res;

    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x55C0)
{
    uint32_t res;

    if (flag_C & 0x100)
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x56C0)
{
    uint32_t res;

    if (flag_NotZ)
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x57C0)
{
    uint32_t res;

    if (!flag_NotZ)
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x58C0)
{
    uint32_t res;

    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x59C0)
{
    uint32_t res;

    if (flag_V & 0x80)
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x5AC0)
{
    uint32_t res;

    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x5BC0)
{
    uint32_t res;

    if (flag_N & 0x80)
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x5CC0)
{
    uint32_t res;

    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x5DC0)
{
    uint32_t res;

    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x5EC0)
{
    uint32_t res;

    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x5FC0)
{
    uint32_t res;

    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* STCC */
OPCODE(0x50D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x51D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x52D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x53D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x54D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x55D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x56D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x57D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x58D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x59D0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5AD0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5BD0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5CD0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5DD0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5ED0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5FD0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x50D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x51D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x52D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x53D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x54D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x55D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x56D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x57D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x58D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x59D8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5AD8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5BD8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5CD8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5DD8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5ED8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5FD8)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x50E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x51E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x52E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x53E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x54E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x55E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x56E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x57E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x58E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x59E0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5AE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5BE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5CE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5DE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5EE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5FE0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x50E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x51E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x52E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x53E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x54E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x55E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x56E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x57E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x58E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x59E8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5AE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5BE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5CE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5DE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5EE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5FE8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x50F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x51F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x52F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x53F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x54F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x55F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x56F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x57F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x58F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x59F0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x5AF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x5BF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x5CF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x5DF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x5EF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x5FF0)
{
    uint32_t adr, res;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(18)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* STCC */
OPCODE(0x50F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x51F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x52F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x53F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x54F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x55F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x56F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x57F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x58F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x59F8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5AF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5BF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5CF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5DF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5EF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x5FF8)
{
    uint32_t adr, res;

    FETCH_SWORD(adr);
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(16)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* STCC */
OPCODE(0x50F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x51F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x52F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x53F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x54F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x55F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x56F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x57F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x58F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x59F9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x5AF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x5BF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x5CF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x5DF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x5EF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x5FF9)
{
    uint32_t adr, res;

    FETCH_LONG(adr);
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(20)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* STCC */
OPCODE(0x50DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x51DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x52DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x53DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x54DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x55DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x56DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x57DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x58DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x59DF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5ADF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5BDF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5CDF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5DDF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5EDF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x5FDF)
{
    uint32_t adr, res;

    adr = AREG(7);
    AREG(7) += 2;
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(12)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* STCC */
OPCODE(0x50E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    res = 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x51E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x52E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x53E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x54E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (!(flag_C & 0x100))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x55E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (flag_C & 0x100)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x56E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x57E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (!flag_NotZ)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x58E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (!(flag_V & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x59E7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (flag_V & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5AE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (!(flag_N & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5BE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (flag_N & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5CE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5DE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if ((flag_N ^ flag_V) & 0x80)
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5EE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* STCC */
OPCODE(0x5FE7)
{
    uint32_t adr, res;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = 0xFF;
        WRITE_BYTE_F(adr, res)
        RET(14)
    }
    res = 0;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* DBCC */
OPCODE(0x50C8)
{

    INC_PC(2);
    RET(12)
}

/* DBCC */
OPCODE(0x51C8)
{
    uint32_t res;

    res = DREGu16((Opcode >> 0) & 7);
    res--;
    DREGu16((Opcode >> 0) & 7) = res;
    if ((int32_t)res != -1)
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x52C8)
{
    uint32_t res;

    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x53C8)
{
    uint32_t res;

    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x54C8)
{
    uint32_t res;

    if (flag_C & 0x100)
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x55C8)
{
    uint32_t res;

    if (!(flag_C & 0x100))
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x56C8)
{
    uint32_t res;

    if (!flag_NotZ)
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x57C8)
{
    uint32_t res;

    if (flag_NotZ)
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x58C8)
{
    uint32_t res;

    if (flag_V & 0x80)
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x59C8)
{
    uint32_t res;

    if (!(flag_V & 0x80))
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x5AC8)
{
    uint32_t res;

    if (flag_N & 0x80)
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x5BC8)
{
    uint32_t res;

    if (!(flag_N & 0x80))
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x5CC8)
{
    uint32_t res;

    if ((flag_N ^ flag_V) & 0x80)
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x5DC8)
{
    uint32_t res;

    if (!((flag_N ^ flag_V) & 0x80))
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x5EC8)
{
    uint32_t res;

    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* DBCC */
OPCODE(0x5FC8)
{
    uint32_t res;

    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        res = DREGu16((Opcode >> 0) & 7);
        res--;
        DREGu16((Opcode >> 0) & 7) = res;
        if ((int32_t)res != -1)
        {
            uint32_t newPC;

            newPC = UNBASED_PC;
            newPC += GET_SWORD;
            SET_PC(newPC);
            CHECK_BRANCH_EXCEPTION(newPC)
            RET(10)
        }
    }
    else
    {
        INC_PC(2);
        RET(12)
    }
    INC_PC(2);
    RET(14)
}

/* ADDQ */
OPCODE(0x5000)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = DREGu8((Opcode >> 0) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* ADDQ */
OPCODE(0x5010)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ADDQ */
OPCODE(0x5018)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ADDQ */
OPCODE(0x5020)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* ADDQ */
OPCODE(0x5028)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ADDQ */
OPCODE(0x5030)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ADDQ */
OPCODE(0x5038)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ADDQ */
OPCODE(0x5039)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_LONG(adr);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ADDQ */
OPCODE(0x501F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ADDQ */
OPCODE(0x5027)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* ADDQ */
OPCODE(0x5040)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = DREGu16((Opcode >> 0) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(4)
}

/* ADDQ */
OPCODE(0x5048)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = AREGu32((Opcode >> 0) & 7);
    res = dst + src;
    AREG((Opcode >> 0) & 7) = res;
    RET(4)
}

/* ADDQ */
OPCODE(0x5050)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ADDQ */
OPCODE(0x5058)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ADDQ */
OPCODE(0x5060)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ADDQ */
OPCODE(0x5068)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ADDQ */
OPCODE(0x5070)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ADDQ */
OPCODE(0x5078)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ADDQ */
OPCODE(0x5079)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_LONG(adr);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ADDQ */
OPCODE(0x505F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ADDQ */
OPCODE(0x5067)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ADDQ */
OPCODE(0x5080)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = DREGu32((Opcode >> 0) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ADDQ */
OPCODE(0x5088)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = AREGu32((Opcode >> 0) & 7);
    res = dst + src;
    AREG((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ADDQ */
OPCODE(0x5090)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ADDQ */
OPCODE(0x5098)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ADDQ */
OPCODE(0x50A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* ADDQ */
OPCODE(0x50A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* ADDQ */
OPCODE(0x50B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* ADDQ */
OPCODE(0x50B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* ADDQ */
OPCODE(0x50B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_LONG(adr);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ADDQ */
OPCODE(0x509F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ADDQ */
OPCODE(0x50A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* SUBQ */
OPCODE(0x5100)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = DREGu8((Opcode >> 0) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* SUBQ */
OPCODE(0x5110)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* SUBQ */
OPCODE(0x5118)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* SUBQ */
OPCODE(0x5120)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* SUBQ */
OPCODE(0x5128)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* SUBQ */
OPCODE(0x5130)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SUBQ */
OPCODE(0x5138)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* SUBQ */
OPCODE(0x5139)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_LONG(adr);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* SUBQ */
OPCODE(0x511F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* SUBQ */
OPCODE(0x5127)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* SUBQ */
OPCODE(0x5140)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = DREGu16((Opcode >> 0) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(4)
}

/* SUBQ */
OPCODE(0x5148)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = AREGu32((Opcode >> 0) & 7);
    res = dst - src;
    AREG((Opcode >> 0) & 7) = res;
    RET(8)
}

/* SUBQ */
OPCODE(0x5150)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* SUBQ */
OPCODE(0x5158)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* SUBQ */
OPCODE(0x5160)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* SUBQ */
OPCODE(0x5168)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* SUBQ */
OPCODE(0x5170)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* SUBQ */
OPCODE(0x5178)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* SUBQ */
OPCODE(0x5179)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_LONG(adr);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* SUBQ */
OPCODE(0x515F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* SUBQ */
OPCODE(0x5167)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* SUBQ */
OPCODE(0x5180)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = DREGu32((Opcode >> 0) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* SUBQ */
OPCODE(0x5188)
{
    uint32_t res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    dst = AREGu32((Opcode >> 0) & 7);
    res = dst - src;
    AREG((Opcode >> 0) & 7) = res;
    RET(8)
}

/* SUBQ */
OPCODE(0x5190)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* SUBQ */
OPCODE(0x5198)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* SUBQ */
OPCODE(0x51A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* SUBQ */
OPCODE(0x51A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* SUBQ */
OPCODE(0x51B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* SUBQ */
OPCODE(0x51B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_SWORD(adr);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* SUBQ */
OPCODE(0x51B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    FETCH_LONG(adr);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* SUBQ */
OPCODE(0x519F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* SUBQ */
OPCODE(0x51A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = (((Opcode >> 9) - 1) & 7) + 1;
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* BCC */
OPCODE(0x6201)
{

    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6301)
{

    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6401)
{

    if (!(flag_C & 0x100))
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6501)
{

    if (flag_C & 0x100)
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6601)
{

    if (flag_NotZ)
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6701)
{

    if (!flag_NotZ)
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6801)
{

    if (!(flag_V & 0x80))
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6901)
{

    if (flag_V & 0x80)
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6A01)
{

    if (!(flag_N & 0x80))
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6B01)
{

    if (flag_N & 0x80)
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6C01)
{

    if (!((flag_N ^ flag_V) & 0x80))
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6D01)
{

    if ((flag_N ^ flag_V) & 0x80)
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6E01)
{

    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC */
OPCODE(0x6F01)
{

    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        INC_PC((int8_t)(Opcode & 0xFE));
        io_cycle_counter -= 2;
    }
    RET(8)
}

/* BCC16 */
OPCODE(0x6200)
{

    if (flag_NotZ && (!(flag_C & 0x100)))
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6300)
{

    if ((!flag_NotZ) || (flag_C & 0x100))
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6400)
{

    if (!(flag_C & 0x100))
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6500)
{

    if (flag_C & 0x100)
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6600)
{

    if (flag_NotZ)
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6700)
{

    if (!flag_NotZ)
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6800)
{

    if (!(flag_V & 0x80))
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6900)
{

    if (flag_V & 0x80)
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6A00)
{

    if (!(flag_N & 0x80))
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6B00)
{

    if (flag_N & 0x80)
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6C00)
{

    if (!((flag_N ^ flag_V) & 0x80))
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6D00)
{

    if ((flag_N ^ flag_V) & 0x80)
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6E00)
{

    if (flag_NotZ && (!((flag_N ^ flag_V) & 0x80)))
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BCC16 */
OPCODE(0x6F00)
{

    if ((!flag_NotZ) || ((flag_N ^ flag_V) & 0x80))
    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
        RET(10)
    }
    INC_PC(2);
    RET(12)
}

/* BRA */
OPCODE(0x6001)
{
#ifdef FAME_CHECK_BRANCHES
    uint32_t newPC = UNBASED_PC;
    int8_t offs=Opcode;
    newPC += offs;
    SET_PC(newPC);
    CHECK_BRANCH_EXCEPTION(offs)
#else
    INC_PC((int8_t)(Opcode & 0xFE));
#endif
    RET(10)
}

/* BRA16 */
OPCODE(0x6000)
{

    {
        uint32_t newPC;

        newPC = UNBASED_PC;
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
    }
    RET(10)
}

/* BSR */
OPCODE(0x6101)
{
    uint32_t oldPC;


    oldPC = UNBASED_PC;
    PUSH_32_F(oldPC)
#ifdef FAME_CHECK_BRANCHES
	{
		int8_t offs;
		offs = Opcode;
		oldPC += offs;
		SET_PC(oldPC);
		CHECK_BRANCH_EXCEPTION(offs)
	}
#else
    INC_PC((int8_t)(Opcode & 0xFE));
#endif
    RET(18)
}

/* BSR16 */
OPCODE(0x6100)
{

    {
        uint32_t oldPC, newPC;

        newPC = UNBASED_PC;
        oldPC = newPC + 2;
        PUSH_32_F(oldPC)
        newPC += GET_SWORD;
        SET_PC(newPC);
        CHECK_BRANCH_EXCEPTION(newPC)
    }
    RET(18)
}

/* MOVEQ */
OPCODE(0x7000)
{
    uint32_t res;

    res = (int32_t)(int8_t)Opcode;
    flag_C = flag_V = 0;
    flag_N = flag_NotZ = res;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ORaD */
OPCODE(0x8000)
{
    uint32_t res;
    uint32_t src;

    src = DREGu8((Opcode >> 0) & 7);
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ORaD */
OPCODE(0x8010)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x8018)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x8020)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ORaD */
OPCODE(0x8028)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ORaD */
OPCODE(0x8030)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ORaD */
OPCODE(0x8038)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ORaD */
OPCODE(0x8039)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ORaD */
OPCODE(0x803A)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ORaD */
OPCODE(0x803B)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ORaD */
OPCODE(0x803C)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x801F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x8027)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ORaD */
OPCODE(0x8040)
{
    uint32_t res;
    uint32_t src;

    src = DREGu16((Opcode >> 0) & 7);
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ORaD */
OPCODE(0x8050)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x8058)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x8060)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ORaD */
OPCODE(0x8068)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ORaD */
OPCODE(0x8070)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ORaD */
OPCODE(0x8078)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ORaD */
OPCODE(0x8079)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ORaD */
OPCODE(0x807A)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ORaD */
OPCODE(0x807B)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ORaD */
OPCODE(0x807C)
{
    uint32_t res;
    uint32_t src;

    FETCH_WORD(src);
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x805F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x8067)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ORaD */
OPCODE(0x8080)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 0) & 7);
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ORaD */
OPCODE(0x8090)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ORaD */
OPCODE(0x8098)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ORaD */
OPCODE(0x80A0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ORaD */
OPCODE(0x80A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ORaD */
OPCODE(0x80B0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ORaD */
OPCODE(0x80B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ORaD */
OPCODE(0x80B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(22)
}

/* ORaD */
OPCODE(0x80BA)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ORaD */
OPCODE(0x80BB)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ORaD */
OPCODE(0x80BC)
{
    uint32_t res;
    uint32_t src;

    FETCH_LONG(src);
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ORaD */
OPCODE(0x809F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ORaD */
OPCODE(0x80A7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ORDa */
OPCODE(0x8110)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ORDa */
OPCODE(0x8118)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ORDa */
OPCODE(0x8120)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* ORDa */
OPCODE(0x8128)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ORDa */
OPCODE(0x8130)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ORDa */
OPCODE(0x8138)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ORDa */
OPCODE(0x8139)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ORDa */
OPCODE(0x811F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ORDa */
OPCODE(0x8127)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    res |= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* ORDa */
OPCODE(0x8150)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ORDa */
OPCODE(0x8158)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ORDa */
OPCODE(0x8160)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ORDa */
OPCODE(0x8168)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ORDa */
OPCODE(0x8170)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ORDa */
OPCODE(0x8178)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ORDa */
OPCODE(0x8179)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ORDa */
OPCODE(0x815F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ORDa */
OPCODE(0x8167)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ORDa */
OPCODE(0x8190)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ORDa */
OPCODE(0x8198)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ORDa */
OPCODE(0x81A0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* ORDa */
OPCODE(0x81A8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* ORDa */
OPCODE(0x81B0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* ORDa */
OPCODE(0x81B8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* ORDa */
OPCODE(0x81B9)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ORDa */
OPCODE(0x819F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ORDa */
OPCODE(0x81A7)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    res |= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* SBCD */
OPCODE(0x8100)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 0) & 7);
    dst = DREGu8((Opcode >> 9) & 7);
    res = (dst & 0xF) - (src & 0xF) - ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res -= 6;
    res += (dst & 0xF0) - (src & 0xF0);
    if (res > 0x99)
    {
        res += 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(6)
}

/* SBCDM */
OPCODE(0x8108)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = (dst & 0xF) - (src & 0xF) - ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res -= 6;
    res += (dst & 0xF0) - (src & 0xF0);
    if (res > 0x99)
    {
        res += 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SBCD7M */
OPCODE(0x810F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = (dst & 0xF) - (src & 0xF) - ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res -= 6;
    res += (dst & 0xF0) - (src & 0xF0);
    if (res > 0x99)
    {
        res += 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SBCDM7 */
OPCODE(0x8F08)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = (dst & 0xF) - (src & 0xF) - ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res -= 6;
    res += (dst & 0xF0) - (src & 0xF0);
    if (res > 0x99)
    {
        res += 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SBCD7M7 */
OPCODE(0x8F0F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = (dst & 0xF) - (src & 0xF) - ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res -= 6;
    res += (dst & 0xF0) - (src & 0xF0);
    if (res > 0x99)
    {
        res += 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* DIVU */
OPCODE(0x80C0)
{
    uint32_t res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 0;

    src = DREGu16((Opcode >> 0) & 7);
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(106)
#endif
}

/* DIVU */
OPCODE(0x80D0)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 4;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(110)
#endif
}

/* DIVU */
OPCODE(0x80D8)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 4;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(110)
#endif
}

/* DIVU */
OPCODE(0x80E0)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 6;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(112)
#endif
}

/* DIVU */
OPCODE(0x80E8)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 8;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(114)
#endif
}

/* DIVU */
OPCODE(0x80F0)
{
    uint32_t adr, res;
    uint32_t src, dst;
	uint32_t main_ea_cycles = 10;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(116)
#endif
}

/* DIVU */
OPCODE(0x80F8)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 8;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(114)
#endif
}

/* DIVU */
OPCODE(0x80F9)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 12;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(118)
#endif
}

/* DIVU */
OPCODE(0x80FA)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 8;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(114)
#endif
}

/* DIVU */
OPCODE(0x80FB)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 10;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(116)
#endif
}

/* DIVU */
OPCODE(0x80FC)
{
    uint32_t res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 4;

    FETCH_WORD(src);
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(110)
#endif
}

/* DIVU */
OPCODE(0x80DF)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 4;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(110)
#endif
}

/* DIVU */
OPCODE(0x80E7)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 6;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    {
        uint32_t q, r;

        q = dst / src;
        r = dst % src;

        if (q & 0xFFFF0000)
        {
			SET_FLAGS_DIV_OVERFLOW
            RET(main_ea_cycles + 10)
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivu68kCycles(dst, src))
#else
    RET(112)
#endif
}

/* DIVS */
OPCODE(0x81C0)
{
    uint32_t res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 0;

    src = (int32_t)DREGs16((Opcode >> 0) & 7);
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(50)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(139)
#endif
}

/* DIVS */
OPCODE(0x81D0)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 4;

    adr = AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(54)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(143)
#endif
}

/* DIVS */
OPCODE(0x81D8)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 4;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(54)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(143)
#endif
}

/* DIVS */
OPCODE(0x81E0)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 6;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(56)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(145)
#endif
}

/* DIVS */
OPCODE(0x81E8)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 8;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(getDivs68kCycles(dst, src))
#else
        RET(58)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(147)
#endif
}

/* DIVS */
OPCODE(0x81F0)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 10;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(60)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(149)
#endif
}

/* DIVS */
OPCODE(0x81F8)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 8;

    FETCH_SWORD(adr);
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(58)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(147)
#endif
}

/* DIVS */
OPCODE(0x81F9)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 12;

    FETCH_LONG(adr);
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(62)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(151)
#endif
}

/* DIVS */
OPCODE(0x81FA)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 8;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(58)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(147)
#endif
}

/* DIVS */
OPCODE(0x81FB)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 10;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(60)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(149)
#endif
}

/* DIVS */
OPCODE(0x81FC)
{
    uint32_t res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 4;

    FETCH_SWORD(src);
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(54)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(143)
#endif
}

/* DIVS */
OPCODE(0x81DF)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 4;

    adr = AREG(7);
    AREG(7) += 2;
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(54)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(143)
#endif
}

/* DIVS */
OPCODE(0x81E7)
{
    uint32_t adr, res;
    uint32_t src, dst;
	const uint32_t main_ea_cycles = 6;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READSX_WORD_F(adr, src)
    if (src == 0)
    {
        SET_FLAGS_DIV_ZERO
		execute_exception(M68K_ZERO_DIVIDE_EX);
        RET(main_ea_cycles)
    }
    dst = DREGu32((Opcode >> 9) & 7);
    if ((dst == 0x80000000) && (src == (uint32_t)-1))
    {
        flag_NotZ = flag_N = 0;
        flag_V = flag_C = 0;
        res = 0;
        DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
        RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
        RET(56)
#endif
    }
    {
        int32_t q, r;

        q = (int32_t)dst / (int32_t)src;
        r = (int32_t)dst % (int32_t)src;

        if ((q > 0x7FFF) || (q < -0x8000))
        {
			SET_FLAGS_DIV_OVERFLOW
#ifdef FAME_ACCURATE_TIMING
            RET(main_ea_cycles + 16 + ((dst & 0x80000000)?2:0))
#else
			RET(main_ea_cycles + 17)
#endif
        }
        q &= 0x0000FFFF;
        flag_NotZ = q;
        flag_N = q >> 8;
        flag_V = flag_C = 0;
        res = q | (r << 16);
        DREGu32((Opcode >> 9) & 7) = res;
    }
#ifdef FAME_ACCURATE_TIMING
    RET(main_ea_cycles + getDivs68kCycles(dst, src))
#else
    RET(145)
#endif
}

/* SUBaD */
OPCODE(0x9000)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 0) & 7);
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(4)
}

/* SUBaD */
OPCODE(0x9008)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    */
#endif
    RET(4)
}

/* SUBaD */
OPCODE(0x9010)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x9018)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x9020)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* SUBaD */
OPCODE(0x9028)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBaD */
OPCODE(0x9030)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBaD */
OPCODE(0x9038)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBaD */
OPCODE(0x9039)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBaD */
OPCODE(0x903A)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBaD */
OPCODE(0x903B)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBaD */
OPCODE(0x903C)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x901F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x9027)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* SUBaD */
OPCODE(0x9040)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 0) & 7);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* SUBaD */
OPCODE(0x9048)
{
    uint32_t res;
    uint32_t src, dst;

    src = AREGu16((Opcode >> 0) & 7);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* SUBaD */
OPCODE(0x9050)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x9058)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x9060)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* SUBaD */
OPCODE(0x9068)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBaD */
OPCODE(0x9070)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBaD */
OPCODE(0x9078)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBaD */
OPCODE(0x9079)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBaD */
OPCODE(0x907A)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBaD */
OPCODE(0x907B)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBaD */
OPCODE(0x907C)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_WORD(src);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x905F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x9067)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* SUBaD */
OPCODE(0x9080)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 0) & 7);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x9088)
{
    uint32_t res;
    uint32_t src, dst;

    src = AREGu32((Opcode >> 0) & 7);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBaD */
OPCODE(0x9090)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBaD */
OPCODE(0x9098)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBaD */
OPCODE(0x90A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBaD */
OPCODE(0x90A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* SUBaD */
OPCODE(0x90B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* SUBaD */
OPCODE(0x90B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* SUBaD */
OPCODE(0x90B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(22)
}

/* SUBaD */
OPCODE(0x90BA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* SUBaD */
OPCODE(0x90BB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* SUBaD */
OPCODE(0x90BC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBaD */
OPCODE(0x909F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBaD */
OPCODE(0x90A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBDa */
OPCODE(0x9110)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* SUBDa */
OPCODE(0x9118)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* SUBDa */
OPCODE(0x9120)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* SUBDa */
OPCODE(0x9128)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* SUBDa */
OPCODE(0x9130)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SUBDa */
OPCODE(0x9138)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* SUBDa */
OPCODE(0x9139)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* SUBDa */
OPCODE(0x911F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* SUBDa */
OPCODE(0x9127)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* SUBDa */
OPCODE(0x9150)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* SUBDa */
OPCODE(0x9158)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* SUBDa */
OPCODE(0x9160)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* SUBDa */
OPCODE(0x9168)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* SUBDa */
OPCODE(0x9170)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* SUBDa */
OPCODE(0x9178)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* SUBDa */
OPCODE(0x9179)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* SUBDa */
OPCODE(0x915F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* SUBDa */
OPCODE(0x9167)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* SUBDa */
OPCODE(0x9190)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* SUBDa */
OPCODE(0x9198)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* SUBDa */
OPCODE(0x91A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* SUBDa */
OPCODE(0x91A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* SUBDa */
OPCODE(0x91B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* SUBDa */
OPCODE(0x91B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* SUBDa */
OPCODE(0x91B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* SUBDa */
OPCODE(0x919F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* SUBDa */
OPCODE(0x91A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* SUBX */
OPCODE(0x9100)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 0) & 7);
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src - ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ |= res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(4)
}

/* SUBX */
OPCODE(0x9140)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 0) & 7);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src - ((flag_X >> 8) & 1);
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* SUBX */
OPCODE(0x9180)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 0) & 7);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBXM */
OPCODE(0x9108)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SUBXM */
OPCODE(0x9148)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* SUBXM */
OPCODE(0x9188)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* SUBX7M */
OPCODE(0x910F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SUBX7M */
OPCODE(0x914F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* SUBX7M */
OPCODE(0x918F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* SUBXM7 */
OPCODE(0x9F08)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SUBXM7 */
OPCODE(0x9F48)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* SUBXM7 */
OPCODE(0x9F88)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* SUBX7M7 */
OPCODE(0x9F0F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* SUBX7M7 */
OPCODE(0x9F4F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* SUBX7M7 */
OPCODE(0x9F8F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst - src - ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* SUBA */
OPCODE(0x90C0)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)DREGs16((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBA */
OPCODE(0x90C8)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)AREGs16((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBA */
OPCODE(0x90D0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBA */
OPCODE(0x90D8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBA */
OPCODE(0x90E0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBA */
OPCODE(0x90E8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBA */
OPCODE(0x90F0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* SUBA */
OPCODE(0x90F8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBA */
OPCODE(0x90F9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(20)
}

/* SUBA */
OPCODE(0x90FA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBA */
OPCODE(0x90FB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* SUBA */
OPCODE(0x90FC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_SWORD(src);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBA */
OPCODE(0x90DF)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* SUBA */
OPCODE(0x90E7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBA */
OPCODE(0x91C0)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)DREGs32((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBA */
OPCODE(0x91C8)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)AREGs32((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* SUBA */
OPCODE(0x91D0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBA */
OPCODE(0x91D8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBA */
OPCODE(0x91E0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBA */
OPCODE(0x91E8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* SUBA */
OPCODE(0x91F0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(20)
}

/* SUBA */
OPCODE(0x91F8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* SUBA */
OPCODE(0x91F9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(22)
}

/* SUBA */
OPCODE(0x91FA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* SUBA */
OPCODE(0x91FB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(20)
}

/* SUBA */
OPCODE(0x91FC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* SUBA */
OPCODE(0x91DF)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 4;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* SUBA */
OPCODE(0x91E7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* CMP */
OPCODE(0xB000)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 0) & 7);
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(4)
}

/* CMP */
OPCODE(0xB008)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    */
#endif
    RET(4)
}

/* CMP */
OPCODE(0xB010)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(8)
}

/* CMP */
OPCODE(0xB018)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(8)
}

/* CMP */
OPCODE(0xB020)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(10)
}

/* CMP */
OPCODE(0xB028)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMP */
OPCODE(0xB030)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(14)
}

/* CMP */
OPCODE(0xB038)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMP */
OPCODE(0xB039)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(16)
}

/* CMP */
OPCODE(0xB03A)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMP */
OPCODE(0xB03B)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(14)
}

/* CMP */
OPCODE(0xB03C)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(8)
}

/* CMP */
OPCODE(0xB01F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(8)
}

/* CMP */
OPCODE(0xB027)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(10)
}

/* CMP */
OPCODE(0xB040)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 0) & 7);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(4)
}

/* CMP */
OPCODE(0xB048)
{
    uint32_t res;
    uint32_t src, dst;

    src = AREGu16((Opcode >> 0) & 7);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(4)
}

/* CMP */
OPCODE(0xB050)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(8)
}

/* CMP */
OPCODE(0xB058)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(8)
}

/* CMP */
OPCODE(0xB060)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(10)
}

/* CMP */
OPCODE(0xB068)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMP */
OPCODE(0xB070)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(14)
}

/* CMP */
OPCODE(0xB078)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMP */
OPCODE(0xB079)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(16)
}

/* CMP */
OPCODE(0xB07A)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMP */
OPCODE(0xB07B)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(14)
}

/* CMP */
OPCODE(0xB07C)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_WORD(src);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(8)
}

/* CMP */
OPCODE(0xB05F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(8)
}

/* CMP */
OPCODE(0xB067)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(10)
}

/* CMP */
OPCODE(0xB080)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 0) & 7);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(6)
}

/* CMP */
OPCODE(0xB088)
{
    uint32_t res;
    uint32_t src, dst;

    src = AREGu32((Opcode >> 0) & 7);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(6)
}

/* CMP */
OPCODE(0xB090)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMP */
OPCODE(0xB098)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMP */
OPCODE(0xB0A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(16)
}

/* CMP */
OPCODE(0xB0A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(18)
}

/* CMP */
OPCODE(0xB0B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMP */
OPCODE(0xB0B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(18)
}

/* CMP */
OPCODE(0xB0B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(22)
}

/* CMP */
OPCODE(0xB0BA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(18)
}

/* CMP */
OPCODE(0xB0BB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMP */
OPCODE(0xB0BC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMP */
OPCODE(0xB09F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMP */
OPCODE(0xB0A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(16)
}

/* CMPM */
OPCODE(0xB108)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMPM */
OPCODE(0xB148)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMPM */
OPCODE(0xB188)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMP7M */
OPCODE(0xB10F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMP7M */
OPCODE(0xB14F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMP7M */
OPCODE(0xB18F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    adr = AREG((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMPM7 */
OPCODE(0xBF08)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMPM7 */
OPCODE(0xBF48)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMPM7 */
OPCODE(0xBF88)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMP7M7 */
OPCODE(0xBF0F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst - src;
    flag_N = flag_C = res;
    flag_V = (src ^ dst) & (res ^ dst);
    flag_NotZ = res & 0xFF;
    RET(12)
}

/* CMP7M7 */
OPCODE(0xBF4F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst - src;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 8;
    flag_N = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    RET(12)
}

/* CMP7M7 */
OPCODE(0xBF8F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* EORDa */
OPCODE(0xB100)
{
    uint32_t res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    res = DREGu8((Opcode >> 0) & 7);
    res ^= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 0) & 7) = res;
    RET(4)
}

/* EORDa */
OPCODE(0xB110)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* EORDa */
OPCODE(0xB118)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* EORDa */
OPCODE(0xB120)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* EORDa */
OPCODE(0xB128)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* EORDa */
OPCODE(0xB130)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* EORDa */
OPCODE(0xB138)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* EORDa */
OPCODE(0xB139)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* EORDa */
OPCODE(0xB11F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* EORDa */
OPCODE(0xB127)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    res ^= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* EORDa */
OPCODE(0xB140)
{
    uint32_t res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    res = DREGu16((Opcode >> 0) & 7);
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(4)
}

/* EORDa */
OPCODE(0xB150)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* EORDa */
OPCODE(0xB158)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* EORDa */
OPCODE(0xB160)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* EORDa */
OPCODE(0xB168)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* EORDa */
OPCODE(0xB170)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* EORDa */
OPCODE(0xB178)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* EORDa */
OPCODE(0xB179)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* EORDa */
OPCODE(0xB15F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* EORDa */
OPCODE(0xB167)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* EORDa */
OPCODE(0xB180)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    res = DREGu32((Opcode >> 0) & 7);
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* EORDa */
OPCODE(0xB190)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* EORDa */
OPCODE(0xB198)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* EORDa */
OPCODE(0xB1A0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* EORDa */
OPCODE(0xB1A8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* EORDa */
OPCODE(0xB1B0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* EORDa */
OPCODE(0xB1B8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* EORDa */
OPCODE(0xB1B9)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* EORDa */
OPCODE(0xB19F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* EORDa */
OPCODE(0xB1A7)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    res ^= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* CMPA */
OPCODE(0xB0C0)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)DREGs16((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(6)
}

/* CMPA */
OPCODE(0xB0C8)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)AREGs16((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(6)
}

/* CMPA */
OPCODE(0xB0D0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(10)
}

/* CMPA */
OPCODE(0xB0D8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(10)
}

/* CMPA */
OPCODE(0xB0E0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(12)
}

/* CMPA */
OPCODE(0xB0E8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMPA */
OPCODE(0xB0F0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(16)
}

/* CMPA */
OPCODE(0xB0F8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMPA */
OPCODE(0xB0F9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(18)
}

/* CMPA */
OPCODE(0xB0FA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMPA */
OPCODE(0xB0FB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(16)
}

/* CMPA */
OPCODE(0xB0FC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_SWORD(src);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(10)
}

/* CMPA */
OPCODE(0xB0DF)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(10)
}

/* CMPA */
OPCODE(0xB0E7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(12)
}

/* CMPA */
OPCODE(0xB1C0)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)DREGs32((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(6)
}

/* CMPA */
OPCODE(0xB1C8)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)AREGs32((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(6)
}

/* CMPA */
OPCODE(0xB1D0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMPA */
OPCODE(0xB1D8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMPA */
OPCODE(0xB1E0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(16)
}

/* CMPA */
OPCODE(0xB1E8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(18)
}

/* CMPA */
OPCODE(0xB1F0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMPA */
OPCODE(0xB1F8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(18)
}

/* CMPA */
OPCODE(0xB1F9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(22)
}

/* CMPA */
OPCODE(0xB1FA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(18)
}

/* CMPA */
OPCODE(0xB1FB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(20)
}

/* CMPA */
OPCODE(0xB1FC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMPA */
OPCODE(0xB1DF)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 4;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(14)
}

/* CMPA */
OPCODE(0xB1E7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst - src;
    flag_NotZ = res;
    flag_C = ((src & res & 1) + (src >> 1) + (res >> 1)) >> 23;
    flag_V = ((src ^ dst) & (res ^ dst)) >> 24;
    flag_N = res >> 24;
    RET(16)
}

/* ANDaD */
OPCODE(0xC000)
{
    uint32_t res;
    uint32_t src;

    src = DREGu8((Opcode >> 0) & 7);
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ANDaD */
OPCODE(0xC010)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC018)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC020)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ANDaD */
OPCODE(0xC028)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ANDaD */
OPCODE(0xC030)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ANDaD */
OPCODE(0xC038)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ANDaD */
OPCODE(0xC039)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ANDaD */
OPCODE(0xC03A)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ANDaD */
OPCODE(0xC03B)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ANDaD */
OPCODE(0xC03C)
{
    uint32_t res;
    uint32_t src;

    FETCH_BYTE(src);
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC01F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC027)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    res = DREGu8((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_NZ_VC0
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ANDaD */
OPCODE(0xC040)
{
    uint32_t res;
    uint32_t src;

    src = DREGu16((Opcode >> 0) & 7);
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ANDaD */
OPCODE(0xC050)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC058)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC060)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ANDaD */
OPCODE(0xC068)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ANDaD */
OPCODE(0xC070)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ANDaD */
OPCODE(0xC078)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ANDaD */
OPCODE(0xC079)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ANDaD */
OPCODE(0xC07A)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ANDaD */
OPCODE(0xC07B)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ANDaD */
OPCODE(0xC07C)
{
    uint32_t res;
    uint32_t src;

    FETCH_WORD(src);
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC05F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC067)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ANDaD */
OPCODE(0xC080)
{
    uint32_t res;
    uint32_t src;

    src = DREGu32((Opcode >> 0) & 7);
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ANDaD */
OPCODE(0xC090)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ANDaD */
OPCODE(0xC098)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ANDaD */
OPCODE(0xC0A0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ANDaD */
OPCODE(0xC0A8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ANDaD */
OPCODE(0xC0B0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ANDaD */
OPCODE(0xC0B8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ANDaD */
OPCODE(0xC0B9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(22)
}

/* ANDaD */
OPCODE(0xC0BA)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ANDaD */
OPCODE(0xC0BB)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ANDaD */
OPCODE(0xC0BC)
{
    uint32_t res;
    uint32_t src;

    FETCH_LONG(src);
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ANDaD */
OPCODE(0xC09F)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ANDaD */
OPCODE(0xC0A7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    res = DREGu32((Opcode >> 9) & 7);
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ANDDa */
OPCODE(0xC110)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ANDDa */
OPCODE(0xC118)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ANDDa */
OPCODE(0xC120)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* ANDDa */
OPCODE(0xC128)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ANDDa */
OPCODE(0xC130)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ANDDa */
OPCODE(0xC138)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ANDDa */
OPCODE(0xC139)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ANDDa */
OPCODE(0xC11F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ANDDa */
OPCODE(0xC127)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, res)
    res &= src;
    SET_FLAGS_NZ_VC0
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* ANDDa */
OPCODE(0xC150)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ANDDa */
OPCODE(0xC158)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ANDDa */
OPCODE(0xC160)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ANDDa */
OPCODE(0xC168)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ANDDa */
OPCODE(0xC170)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ANDDa */
OPCODE(0xC178)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ANDDa */
OPCODE(0xC179)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ANDDa */
OPCODE(0xC15F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ANDDa */
OPCODE(0xC167)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 8;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ANDDa */
OPCODE(0xC190)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ANDDa */
OPCODE(0xC198)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ANDDa */
OPCODE(0xC1A0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* ANDDa */
OPCODE(0xC1A8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* ANDDa */
OPCODE(0xC1B0)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* ANDDa */
OPCODE(0xC1B8)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* ANDDa */
OPCODE(0xC1B9)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ANDDa */
OPCODE(0xC19F)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ANDDa */
OPCODE(0xC1A7)
{
    uint32_t adr, res;
    uint32_t src;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, res)
    res &= src;
    SET_FLAGS_Z_VC0
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* ABCD */
OPCODE(0xC100)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 0) & 7);
    dst = DREGu8((Opcode >> 9) & 7);
    res = (dst & 0xF) + (src & 0xF) + ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res += 6;
    res += (dst & 0xF0) + (src & 0xF0);
    if (res > 0x99)
    {
        res -= 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(6)
}

/* ABCDM */
OPCODE(0xC108)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = (dst & 0xF) + (src & 0xF) + ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res += 6;
    res += (dst & 0xF0) + (src & 0xF0);
    if (res > 0x99)
    {
        res -= 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ABCD7M */
OPCODE(0xC10F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = (dst & 0xF) + (src & 0xF) + ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res += 6;
    res += (dst & 0xF0) + (src & 0xF0);
    if (res > 0x99)
    {
        res -= 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ABCDM7 */
OPCODE(0xCF08)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = (dst & 0xF) + (src & 0xF) + ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res += 6;
    res += (dst & 0xF0) + (src & 0xF0);
    if (res > 0x99)
    {
        res -= 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ABCD7M7 */
OPCODE(0xCF0F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = (dst & 0xF) + (src & 0xF) + ((flag_X >> M68K_SR_X_SFT) & 1);
    if (res > 9) res += 6;
    res += (dst & 0xF0) + (src & 0xF0);
    if (res > 0x99)
    {
        res -= 0xA0;
        flag_X = flag_C = M68K_SR_C;
    }
    else flag_X = flag_C = 0;
    flag_NotZ |= res & 0xFF;
    flag_N = res;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* MULU */
OPCODE(0xC0C0)
{
    uint32_t res;
    uint32_t src;

    src = DREGu16((Opcode >> 0) & 7);
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(38)
#else
    RET(16 + 38)
#endif
}

/* MULU */
OPCODE(0xC0D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(42)
#else
    RET(16 + 42)
#endif
}

/* MULU */
OPCODE(0xC0D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(42)
#else
    RET(16 + 42)
#endif
}

/* MULU */
OPCODE(0xC0E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(44)
#else
    RET(16 + 44)
#endif
}

/* MULU */
OPCODE(0xC0E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(46)
#else
    RET(16 + 46)
#endif
}

/* MULU */
OPCODE(0xC0F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(48)
#else
    RET(16 + 48)
#endif
}

/* MULU */
OPCODE(0xC0F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(46)
#else
    RET(16 + 46)
#endif
}

/* MULU */
OPCODE(0xC0F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(50)
#else
    RET(16 + 50)
#endif
}

/* MULU */
OPCODE(0xC0FA)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(46)
#else
    RET(16 + 46)
#endif
}

/* MULU */
OPCODE(0xC0FB)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(48)
#else
    RET(16 + 48)
#endif
}

/* MULU */
OPCODE(0xC0FC)
{
    uint32_t res;
    uint32_t src;

    FETCH_WORD(src);
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(42)
#else
    RET(16 + 42)
#endif
}

/* MULU */
OPCODE(0xC0DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(42)
#else
    RET(16 + 42)
#endif
}

/* MULU */
OPCODE(0xC0E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    res = DREGu16((Opcode >> 9) & 7);
    res *= src;
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src & 0xFFFF) * 2;
    RET(44)
#else
    RET(16 + 44)
#endif
}

/* MULS */
OPCODE(0xC1C0)
{
    uint32_t res;
    uint32_t src;

    src = (int32_t)DREGs16((Opcode >> 0) & 7);
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(38)
#else
    RET(8 + 38)
#endif
}

/* MULS */
OPCODE(0xC1D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(42)
#else
    RET(8 + 42)
#endif
}

/* MULS */
OPCODE(0xC1D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(42)
#else
    RET(8 + 42)
#endif
}

/* MULS */
OPCODE(0xC1E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(44)
#else
    RET(8 + 44)
#endif
}

/* MULS */
OPCODE(0xC1E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(46)
#else
    RET(8 + 46)
#endif
}

/* MULS */
OPCODE(0xC1F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(48)
#else
    RET(8 + 48)
#endif
}

/* MULS */
OPCODE(0xC1F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(46)
#else
    RET(8 + 46)
#endif
}

/* MULS */
OPCODE(0xC1F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(50)
#else
    RET(8 + 50)
#endif
}

/* MULS */
OPCODE(0xC1FA)
{
    uint32_t adr, res;
    uint32_t src;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(46)
#else
    RET(8 + 46)
#endif
}

/* MULS */
OPCODE(0xC1FB)
{
    uint32_t adr, res;
    uint32_t src;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(48)
#else
    RET(8 + 48)
#endif
}

/* MULS */
OPCODE(0xC1FC)
{
    uint32_t res;
    uint32_t src;

    FETCH_SWORD(src);
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(42)
#else
    RET(8 + 42)
#endif
}

/* MULS */
OPCODE(0xC1DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(42)
#else
    RET(8 + 42)
#endif
}

/* MULS */
OPCODE(0xC1E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READSX_WORD_F(adr, src)
    res = (int32_t)DREGs16((Opcode >> 9) & 7);
    res = ((int32_t)res) * ((int32_t)src);
    flag_N = res >> 24;
    flag_NotZ = res;
    flag_V = flag_C = 0;
    DREGu32((Opcode >> 9) & 7) = res;
#ifdef FAME_ACCURATE_TIMING
    /* count bits set in the memory operand */
    io_cycle_counter -= bitset_count(src ^ (src << 1)) * 2;
    RET(44)
#else
    RET(8 + 44)
#endif
}

/* EXGDD */
OPCODE(0xC140)
{
    uint32_t res;
    uint32_t src;

    res = DREGu32((Opcode >> 0) & 7);
    src = DREGu32((Opcode >> 9) & 7);
    DREGu32((Opcode >> 9) & 7) = res;
    res = src;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(6)
}

/* EXGAA */
OPCODE(0xC148)
{
    uint32_t res;
    uint32_t src;

    res = AREGu32((Opcode >> 0) & 7);
    src = AREGu32((Opcode >> 9) & 7);
    AREG((Opcode >> 9) & 7) = res;
    res = src;
    AREG((Opcode >> 0) & 7) = res;
    RET(6)
}

/* EXGAD */
OPCODE(0xC188)
{
    uint32_t res;
    uint32_t src;

    res = AREGu32((Opcode >> 0) & 7);
    src = DREGu32((Opcode >> 9) & 7);
    DREGu32((Opcode >> 9) & 7) = res;
    res = src;
    AREG((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ADDaD */
OPCODE(0xD000)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 0) & 7);
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ADDaD */
OPCODE(0xD008)
{
#if 0
    /* can't read byte from Ax registers ! */
    m68kcontext.execinfo |= M68K_FAULTED;
    io_cycle_counter = 0;
    /*
    goto famec_Exec_End;
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    */
#endif
    RET(4)
}

/* ADDaD */
OPCODE(0xD010)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD018)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD020)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ADDaD */
OPCODE(0xD028)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDaD */
OPCODE(0xD030)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDaD */
OPCODE(0xD038)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDaD */
OPCODE(0xD039)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDaD */
OPCODE(0xD03A)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDaD */
OPCODE(0xD03B)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDaD */
OPCODE(0xD03C)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_BYTE(src);
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD01F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD027)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ADDaD */
OPCODE(0xD040)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 0) & 7);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ADDaD */
OPCODE(0xD048)
{
    uint32_t res;
    uint32_t src, dst;

    src = AREGu16((Opcode >> 0) & 7);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ADDaD */
OPCODE(0xD050)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD058)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD060)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ADDaD */
OPCODE(0xD068)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDaD */
OPCODE(0xD070)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDaD */
OPCODE(0xD078)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDaD */
OPCODE(0xD079)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDaD */
OPCODE(0xD07A)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDaD */
OPCODE(0xD07B)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDaD */
OPCODE(0xD07C)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_WORD(src);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD05F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD067)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(10)
}

/* ADDaD */
OPCODE(0xD080)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 0) & 7);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD088)
{
    uint32_t res;
    uint32_t src, dst;

    src = AREGu32((Opcode >> 0) & 7);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDaD */
OPCODE(0xD090)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDaD */
OPCODE(0xD098)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDaD */
OPCODE(0xD0A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDaD */
OPCODE(0xD0A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ADDaD */
OPCODE(0xD0B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ADDaD */
OPCODE(0xD0B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ADDaD */
OPCODE(0xD0B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(22)
}

/* ADDaD */
OPCODE(0xD0BA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ADDaD */
OPCODE(0xD0BB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ADDaD */
OPCODE(0xD0BC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDaD */
OPCODE(0xD09F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDaD */
OPCODE(0xD0A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDDa */
OPCODE(0xD110)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ADDDa */
OPCODE(0xD118)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 1;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ADDDa */
OPCODE(0xD120)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* ADDDa */
OPCODE(0xD128)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ADDDa */
OPCODE(0xD130)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ADDDa */
OPCODE(0xD138)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(16)
}

/* ADDDa */
OPCODE(0xD139)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(20)
}

/* ADDDa */
OPCODE(0xD11F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(12)
}

/* ADDDa */
OPCODE(0xD127)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src;
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ = res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(14)
}

/* ADDDa */
OPCODE(0xD150)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ADDDa */
OPCODE(0xD158)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ADDDa */
OPCODE(0xD160)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ADDDa */
OPCODE(0xD168)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ADDDa */
OPCODE(0xD170)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ADDDa */
OPCODE(0xD178)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ADDDa */
OPCODE(0xD179)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ADDDa */
OPCODE(0xD15F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ADDDa */
OPCODE(0xD167)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 9) & 7);
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src;
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ = res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ADDDa */
OPCODE(0xD190)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ADDDa */
OPCODE(0xD198)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ADDDa */
OPCODE(0xD1A0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* ADDDa */
OPCODE(0xD1A8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* ADDDa */
OPCODE(0xD1B0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(26)
}

/* ADDDa */
OPCODE(0xD1B8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_SWORD(adr);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(24)
}

/* ADDDa */
OPCODE(0xD1B9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    FETCH_LONG(adr);
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(28)
}

/* ADDDa */
OPCODE(0xD19F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7);
    AREG(7) += 4;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(20)
}

/* ADDDa */
OPCODE(0xD1A7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 9) & 7);
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src;
    flag_NotZ = res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(22)
}

/* ADDX */
OPCODE(0xD100)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu8((Opcode >> 0) & 7);
    dst = DREGu8((Opcode >> 9) & 7);
    res = dst + src + ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ |= res & 0xFF;
    DREGu8((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ADDX */
OPCODE(0xD140)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu16((Opcode >> 0) & 7);
    dst = DREGu16((Opcode >> 9) & 7);
    res = dst + src + ((flag_X >> 8) & 1);
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    DREGu16((Opcode >> 9) & 7) = res;
    RET(4)
}

/* ADDX */
OPCODE(0xD180)
{
    uint32_t res;
    uint32_t src, dst;

    src = DREGu32((Opcode >> 0) & 7);
    dst = DREGu32((Opcode >> 9) & 7);
    res = dst + src + ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    DREGu32((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDXM */
OPCODE(0xD108)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ADDXM */
OPCODE(0xD148)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ADDXM */
OPCODE(0xD188)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ADDX7M */
OPCODE(0xD10F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 1;
    AREG((Opcode >> 9) & 7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ADDX7M */
OPCODE(0xD14F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 2;
    AREG((Opcode >> 9) & 7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ADDX7M */
OPCODE(0xD18F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    adr = AREG((Opcode >> 9) & 7) - 4;
    AREG((Opcode >> 9) & 7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ADDXM7 */
OPCODE(0xDF08)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 1;
    AREG((Opcode >> 0) & 7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ADDXM7 */
OPCODE(0xDF48)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ADDXM7 */
OPCODE(0xDF88)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READ_LONG_F(adr, src)
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ADDX7M7 */
OPCODE(0xDF0F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_BYTE_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_N = flag_X = flag_C = res;
    flag_V = (src ^ res) & (dst ^ res);
    flag_NotZ |= res & 0xFF;
    WRITE_BYTE_F(adr, res)
    RET(18)
}

/* ADDX7M7 */
OPCODE(0xDF4F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_V = ((src ^ res) & (dst ^ res)) >> 8;
    flag_N = flag_X = flag_C = res >> 8;
    flag_NotZ |= res & 0xFFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ADDX7M7 */
OPCODE(0xDF8F)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, src)
    adr = AREG(7) - 4;
    AREG(7) = adr;
    READ_LONG_F(adr, dst)
    res = dst + src + ((flag_X >> 8) & 1);
    flag_NotZ |= res;
    flag_X = flag_C = ((src & dst & 1) + (src >> 1) + (dst >> 1)) >> 23;
    flag_V = ((src ^ res) & (dst ^ res)) >> 24;
    flag_N = res >> 24;
    WRITE_LONG_F(adr, res)
    RET(30)
}

/* ADDA */
OPCODE(0xD0C0)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)DREGs16((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDA */
OPCODE(0xD0C8)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)AREGs16((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDA */
OPCODE(0xD0D0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDA */
OPCODE(0xD0D8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDA */
OPCODE(0xD0E0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDA */
OPCODE(0xD0E8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDA */
OPCODE(0xD0F0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ADDA */
OPCODE(0xD0F8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDA */
OPCODE(0xD0F9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ADDA */
OPCODE(0xD0FA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDA */
OPCODE(0xD0FB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ADDA */
OPCODE(0xD0FC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_SWORD(src);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDA */
OPCODE(0xD0DF)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 2;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(12)
}

/* ADDA */
OPCODE(0xD0E7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READSX_WORD_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDA */
OPCODE(0xD1C0)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)DREGs32((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDA */
OPCODE(0xD1C8)
{
    uint32_t res;
    uint32_t src, dst;

    src = (int32_t)AREGs32((Opcode >> 0) & 7);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(8)
}

/* ADDA */
OPCODE(0xD1D0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDA */
OPCODE(0xD1D8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 4;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDA */
OPCODE(0xD1E0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7) - 4;
    AREG((Opcode >> 0) & 7) = adr;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDA */
OPCODE(0xD1E8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ADDA */
OPCODE(0xD1F0)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ADDA */
OPCODE(0xD1F8)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_SWORD(adr);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ADDA */
OPCODE(0xD1F9)
{
    uint32_t adr, res;
    uint32_t src, dst;

    FETCH_LONG(adr);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(22)
}

/* ADDA */
OPCODE(0xD1FA)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = GET_SWORD + UNBASED_PC;
    INC_PC(2);
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(18)
}

/* ADDA */
OPCODE(0xD1FB)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = UNBASED_PC;
    DECODE_EXT_WORD
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(20)
}

/* ADDA */
OPCODE(0xD1FC)
{
    uint32_t res;
    uint32_t src, dst;

    FETCH_LONG(src);
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ADDA */
OPCODE(0xD1DF)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7);
    AREG(7) += 4;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(14)
}

/* ADDA */
OPCODE(0xD1E7)
{
    uint32_t adr, res;
    uint32_t src, dst;

    adr = AREG(7) - 4;
    AREG(7) = adr;
    READSX_LONG_F(adr, src)
    dst = AREGu32((Opcode >> 9) & 7);
    res = dst + src;
    AREG((Opcode >> 9) & 7) = res;
    RET(16)
}

/* ASRk */
OPCODE(0xE000)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = (int32_t)DREGs8((Opcode >> 0) & 7);
    flag_V = 0;
    flag_X = flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = ((int32_t)src) >> sft;
    flag_N = res >> 0;
    flag_NotZ = res;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ASRk */
OPCODE(0xE040)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = (int32_t)DREGs16((Opcode >> 0) & 7);
    flag_V = 0;
    flag_X = flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = ((int32_t)src) >> sft;
    flag_N = res >> 8;
    flag_NotZ = res;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ASRk */
OPCODE(0xE080)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = (int32_t)DREGs32((Opcode >> 0) & 7);
    flag_V = 0;
    flag_X = flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = ((int32_t)src) >> sft;
    flag_N = res >> 24;
    flag_NotZ = res;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* LSRk */
OPCODE(0xE008)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu8((Opcode >> 0) & 7);
    flag_N = flag_V = 0;
    flag_X = flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = src >> sft;
    flag_NotZ = res;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(6)
}

/* LSRk */
OPCODE(0xE048)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu16((Opcode >> 0) & 7);
    flag_N = flag_V = 0;
    flag_X = flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = src >> sft;
    flag_NotZ = res;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(6)
}

/* LSRk */
OPCODE(0xE088)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu32((Opcode >> 0) & 7);
    flag_N = flag_V = 0;
    flag_X = flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = src >> sft;
    flag_NotZ = res;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ROXRk */
OPCODE(0xE010)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu8((Opcode >> 0) & 7);
    src |= (flag_X & M68K_SR_X) << 0;
    res = (src >> sft) | (src << (9 - sft));
    flag_X = flag_C = res >> 0;
    flag_V = 0;
    flag_N = res >> 0;
    flag_NotZ = res & 0x000000FF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ROXRk */
OPCODE(0xE050)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu16((Opcode >> 0) & 7);
    src |= (flag_X & M68K_SR_X) << 8;
    res = (src >> sft) | (src << (17 - sft));
    flag_X = flag_C = res >> 8;
    flag_V = 0;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ROXRk */
OPCODE(0xE090)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu32((Opcode >> 0) & 7);
    flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    if (sft == 1) res = (src >> 1) | ((flag_X & M68K_SR_X) << (32 - (M68K_SR_X_SFT + 1)));
    else res = (src >> sft) | (src << (33 - sft)) | ((flag_X & M68K_SR_X) << (32 - (M68K_SR_X_SFT + sft)));
    flag_X = flag_C;
    flag_V = 0;
    flag_N = res >> 24;
    flag_NotZ = res;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* RORk */
OPCODE(0xE018)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu8((Opcode >> 0) & 7);
    flag_V = 0;
    flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = (src >> sft) | (src << (8 - sft));
    flag_N = res >> 0;
    flag_NotZ = res & 0x000000FF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(6)
}

/* RORk */
OPCODE(0xE058)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu16((Opcode >> 0) & 7);
    flag_V = 0;
    flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = (src >> sft) | (src << (16 - sft));
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(6)
}

/* RORk */
OPCODE(0xE098)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu32((Opcode >> 0) & 7);
    flag_V = 0;
    flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
    res = (src >> sft) | (src << (32 - sft));
    flag_N = res >> 24;
    flag_NotZ = res;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ASLk */
OPCODE(0xE100)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu8((Opcode >> 0) & 7);
    if (sft < 8)
    {
        flag_X = flag_C = src << (0 + sft);
        res = src << sft;
        flag_N = res >> 0;
        flag_NotZ = res & 0x000000FF;
        DREGu8((Opcode >> 0) & 7) = res;
        flag_V = 0;
        if ((sft > 7) && (src)) flag_V = M68K_SR_V;
        else
        {
            uint32_t msk = (((int32_t)0x80000000) >> (sft + 24)) & 0x000000FF;
            src &= msk;
            if ((src) && (src != msk)) flag_V = M68K_SR_V;
        }
        RET(6)
    }

    if (src) flag_V = M68K_SR_V;
    else flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = 0;
    DREGu8((Opcode >> 0) & 7) = res;
    flag_N = 0;
    flag_NotZ = 0;
    RET(6)
}

/* ASLk */
OPCODE(0xE140)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu16((Opcode >> 0) & 7);
    flag_X = flag_C = src >> (8 - sft);
    res = src << sft;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    flag_V = 0;
    {
        uint32_t msk = (((int32_t)0x80000000) >> (sft + 16)) & 0x0000FFFF;
        src &= msk;
        if ((src) && (src != msk)) flag_V = M68K_SR_V;
    }
    RET(6)
}

/* ASLk */
OPCODE(0xE180)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu32((Opcode >> 0) & 7);
    flag_X = flag_C = src >> (24 - sft);
    res = src << sft;
    flag_N = res >> 24;
    flag_NotZ = res & 0xFFFFFFFF;
    DREGu32((Opcode >> 0) & 7) = res;
    flag_V = 0;
    {
        uint32_t msk = (((int32_t)0x80000000) >> (sft + 0)) & 0xFFFFFFFF;
        src &= msk;
        if ((src) && (src != msk)) flag_V = M68K_SR_V;
    }
    RET(8)
}

/* LSLk */
OPCODE(0xE108)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu8((Opcode >> 0) & 7);
    flag_V = 0;
    flag_X = flag_C = src << (0 + sft);
    res = src << sft;
    flag_N = res >> 0;
    flag_NotZ = res & 0x000000FF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(6)
}

/* LSLk */
OPCODE(0xE148)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu16((Opcode >> 0) & 7);
    flag_V = 0;
    flag_X = flag_C = src >> (8 - sft);
    res = src << sft;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(6)
}

/* LSLk */
OPCODE(0xE188)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu32((Opcode >> 0) & 7);
    flag_V = 0;
    flag_X = flag_C = src >> (24 - sft);
    res = src << sft;
    flag_N = res >> 24;
    flag_NotZ = res & 0xFFFFFFFF;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ROXLk */
OPCODE(0xE110)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu8((Opcode >> 0) & 7);
    src |= (flag_X & M68K_SR_X) << 0;
    res = (src << sft) | (src >> (9 - sft));
    flag_X = flag_C = res >> 0;
    flag_V = 0;
    flag_N = res >> 0;
    flag_NotZ = res & 0x000000FF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ROXLk */
OPCODE(0xE150)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu16((Opcode >> 0) & 7);
    src |= (flag_X & M68K_SR_X) << 8;
    res = (src << sft) | (src >> (17 - sft));
    flag_X = flag_C = res >> 8;
    flag_V = 0;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ROXLk */
OPCODE(0xE190)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu32((Opcode >> 0) & 7);
    flag_C = src >> ((32 - M68K_SR_C_SFT) - sft);
    if (sft == 1) res = (src << 1) | ((flag_X & M68K_SR_X) >> ((M68K_SR_X_SFT + 1) - 1));
    else res = (src << sft) | (src >> (33 - sft)) | ((flag_X & M68K_SR_X) >> ((M68K_SR_X_SFT + 1) - sft));
    flag_X = flag_C;
    flag_V = 0;
    flag_N = res >> 24;
    flag_NotZ = res;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ROLk */
OPCODE(0xE118)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu8((Opcode >> 0) & 7);
    flag_V = 0;
    flag_C = src << (0 + sft);
    res = (src << sft) | (src >> (8 - sft));
    flag_N = res >> 0;
    flag_NotZ = res & 0x000000FF;
    DREGu8((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ROLk */
OPCODE(0xE158)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu16((Opcode >> 0) & 7);
    flag_V = 0;
    flag_C = src >> (8 - sft);
    res = (src << sft) | (src >> (16 - sft));
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    DREGu16((Opcode >> 0) & 7) = res;
    RET(6)
}

/* ROLk */
OPCODE(0xE198)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = (((Opcode >> 9) - 1) & 7) + 1;
    io_cycle_counter -= sft * 2;
    src = DREGu32((Opcode >> 0) & 7);
    flag_V = 0;
    flag_C = src >> (24 - sft);
    res = (src << sft) | (src >> (32 - sft));
    flag_N = res >> 24;
    flag_NotZ = res;
    DREGu32((Opcode >> 0) & 7) = res;
    RET(8)
}

/* ASRD */
OPCODE(0xE020)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = (int32_t)DREGs8((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft < 8)
        {
            flag_V = 0;
            flag_X = flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
            res = ((int32_t)src) >> sft;
            flag_N = res >> 0;
            flag_NotZ = res;
            DREGu8((Opcode >> 0) & 7) = res;
            RET(6)
        }

        if (src & (1 << 7))
        {
            flag_N = M68K_SR_N;
            flag_NotZ = 1;
            flag_V = 0;
            flag_C = M68K_SR_C;
            flag_X = M68K_SR_X;
            res = 0x000000FF;
            DREGu8((Opcode >> 0) & 7) = res;
            RET(6)
        }

        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        flag_C = 0;
        flag_X = 0;
        res = 0;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 0;
    flag_NotZ = src;
    RET(6)
}

/* ASRD */
OPCODE(0xE060)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = (int32_t)DREGs16((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft < 16)
        {
            flag_V = 0;
            flag_X = flag_C = (src >> (sft - 1)) << M68K_SR_C_SFT;
            res = ((int32_t)src) >> sft;
            flag_N = res >> 8;
            flag_NotZ = res;
            DREGu16((Opcode >> 0) & 7) = res;
            RET(6)
        }

        if (src & (1 << 15))
        {
            flag_N = M68K_SR_N;
            flag_NotZ = 1;
            flag_V = 0;
            flag_C = M68K_SR_C;
            flag_X = M68K_SR_X;
            res = 0x0000FFFF;
            DREGu16((Opcode >> 0) & 7) = res;
            RET(6)
        }

        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        flag_C = 0;
        flag_X = 0;
        res = 0;
        DREGu16((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 8;
    flag_NotZ = src;
    RET(6)
}

/* ASRD */
OPCODE(0xE0A0)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = (int32_t)DREGs32((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft < 32)
        {
            flag_V = 0;
            flag_X = flag_C = (src >> (sft - 1)) << M68K_SR_C_SFT;
            res = ((int32_t)src) >> sft;
            flag_N = res >> 24;
            flag_NotZ = res;
            DREGu32((Opcode >> 0) & 7) = res;
            RET(8)
        }

        if (src & (1 << 31))
        {
            flag_N = M68K_SR_N;
            flag_NotZ = 1;
            flag_V = 0;
            flag_C = M68K_SR_C;
            flag_X = M68K_SR_X;
            res = 0xFFFFFFFF;
            DREGu32((Opcode >> 0) & 7) = res;
            RET(8)
        }

        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        flag_C = 0;
        flag_X = 0;
        res = 0;
        DREGu32((Opcode >> 0) & 7) = res;
        RET(8)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 24;
    flag_NotZ = src;
    RET(8)
}

/* LSRD */
OPCODE(0xE028)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu8((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft <= 8)
        {
            flag_N = flag_V = 0;
            flag_X = flag_C = src << ((M68K_SR_C_SFT + 1) - sft);
            res = src >> sft;
            flag_NotZ = res;
            DREGu8((Opcode >> 0) & 7) = res;
            RET(6)
        }

        flag_X = flag_C = 0;
        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        res = 0;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 0;
    flag_NotZ = src;
    RET(6)
}

/* LSRD */
OPCODE(0xE068)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu16((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft <= 16)
        {
            flag_N = flag_V = 0;
            flag_X = flag_C = (src >> (sft - 1)) << M68K_SR_C_SFT;
            res = src >> sft;
            flag_NotZ = res;
            DREGu16((Opcode >> 0) & 7) = res;
            RET(6)
        }

        flag_X = flag_C = 0;
        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        res = 0;
        DREGu16((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 8;
    flag_NotZ = src;
    RET(6)
}

/* LSRD */
OPCODE(0xE0A8)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu32((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft < 32)
        {
            flag_N = flag_V = 0;
            flag_X = flag_C = (src >> (sft - 1)) << M68K_SR_C_SFT;
            res = src >> sft;
            flag_NotZ = res;
            DREGu32((Opcode >> 0) & 7) = res;
            RET(8)
        }

        if (sft == 32) flag_C = src >> (31 - M68K_SR_C_SFT);
        else flag_C = 0;
        flag_X = flag_C;
        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        res = 0;
        DREGu32((Opcode >> 0) & 7) = res;
        RET(8)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 24;
    flag_NotZ = src;
    RET(8)
}

/* ROXRD */
OPCODE(0xE030)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu8((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft %= 9;

        src |= (flag_X & M68K_SR_X) << 0;
        res = (src >> sft) | (src << (9 - sft));
        flag_X = flag_C = res >> 0;
        flag_V = 0;
        flag_N = res >> 0;
        flag_NotZ = res & 0x000000FF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = flag_X;
    flag_N = src >> 0;
    flag_NotZ = src;
    RET(6)
}

/* ROXRD */
OPCODE(0xE070)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu16((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft %= 17;

        src |= (flag_X & M68K_SR_X) << 8;
        res = (src >> sft) | (src << (17 - sft));
        flag_X = flag_C = res >> 8;
        flag_V = 0;
        flag_N = res >> 8;
        flag_NotZ = res & 0x0000FFFF;
        DREGu16((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = flag_X;
    flag_N = src >> 8;
    flag_NotZ = src;
    RET(6)
}

/* ROXRD */
OPCODE(0xE0B0)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu32((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft %= 33;

        if (sft != 0)
        {
            if (sft == 1)
				res = (src >> 1) | ((flag_X & M68K_SR_X) << (32 - (M68K_SR_X_SFT + 1)));
            else
				res = (src >> sft) | (src << (33 - sft)) | (((flag_X & M68K_SR_X) << (32 - (M68K_SR_X_SFT + 1))) >> (sft - 1));

			flag_X = ((src >> (sft - 1)) & 1) << M68K_SR_X_SFT;
        }
        else res = src;
        flag_C = flag_X;
        flag_V = 0;
        flag_N = res >> 24;
        flag_NotZ = res;
        DREGu32((Opcode >> 0) & 7) = res;
        RET(8)
    }

    flag_V = 0;
    flag_C = flag_X;
    flag_N = src >> 24;
    flag_NotZ = src;
    RET(8)
}

/* RORD */
OPCODE(0xE038)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu8((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft &= 0x07;

        flag_C = src << (M68K_SR_C_SFT - ((sft - 1) & 7));
        res = (src >> sft) | (src << (8 - sft));
        flag_V = 0;
        flag_N = res >> 0;
        flag_NotZ = res & 0x000000FF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 0;
    flag_NotZ = src;
    RET(6)
}

/* RORD */
OPCODE(0xE078)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu16((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft &= 0x0F;

        flag_C = (src >> ((sft - 1) & 15)) << M68K_SR_C_SFT;
        res = (src >> sft) | (src << (16 - sft));
        flag_V = 0;
        flag_N = res >> 8;
        flag_NotZ = res & 0x0000FFFF;
        DREGu16((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 8;
    flag_NotZ = src;
    RET(6)
}

/* RORD */
OPCODE(0xE0B8)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu32((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft &= 0x1F;

        flag_C = (src >> ((sft - 1) & 31)) << M68K_SR_C_SFT;
        res = (src >> sft) | (src << (32 - sft));
        flag_V = 0;
        flag_N = res >> 24;
        flag_NotZ = res;
        DREGu32((Opcode >> 0) & 7) = res;
        RET(8)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 24;
    flag_NotZ = src;
    RET(8)
}

/* ASLD */
OPCODE(0xE120)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu8((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft < 8)
        {
            flag_X = flag_C = (src << sft) >> 0;
            res = (src << sft) & 0x000000FF;
            flag_N = res >> 0;
            flag_NotZ = res;
            DREGu8((Opcode >> 0) & 7) = res;
            flag_V = 0;
            {
                uint32_t msk = (((int32_t)0x80000000) >> (sft + 24)) & 0x000000FF;
                src &= msk;
                if ((src) && (src != msk)) flag_V = M68K_SR_V;
            }
            RET(6)
        }

        if (sft == 256) flag_C = src << M68K_SR_C_SFT;
        else flag_C = 0;
        flag_X = flag_C;
        if (src) flag_V = M68K_SR_V;
        else flag_V = 0;
        res = 0;
        DREGu8((Opcode >> 0) & 7) = res;
        flag_N = 0;
        flag_NotZ = 0;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 0;
    flag_NotZ = src;
    RET(6)
}

/* ASLD */
OPCODE(0xE160)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu16((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft < 16)
        {
            flag_X = flag_C = (src << sft) >> 8;
            res = (src << sft) & 0x0000FFFF;
            flag_N = res >> 8;
            flag_NotZ = res;
            DREGu16((Opcode >> 0) & 7) = res;
            flag_V = 0;
            {
                uint32_t msk = (((int32_t)0x80000000) >> (sft + 16)) & 0x0000FFFF;
                src &= msk;
                if ((src) && (src != msk)) flag_V = M68K_SR_V;
            }
            RET(6)
        }

        if (sft == 65536) flag_C = src << M68K_SR_C_SFT;
        else flag_C = 0;
        flag_X = flag_C;
        if (src) flag_V = M68K_SR_V;
        else flag_V = 0;
        res = 0;
        DREGu16((Opcode >> 0) & 7) = res;
        flag_N = 0;
        flag_NotZ = 0;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 8;
    flag_NotZ = src;
    RET(6)
}

/* ASLD */
OPCODE(0xE1A0)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu32((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft < 32)
        {
            flag_X = flag_C = (src >> (32 - sft)) << M68K_SR_C_SFT;
            res = src << sft;
            flag_N = res >> 24;
            flag_NotZ = res;
            DREGu32((Opcode >> 0) & 7) = res;
            flag_V = 0;
            {
                uint32_t msk = (((int32_t)0x80000000) >> (sft + 0)) & 0xFFFFFFFF;
                src &= msk;
                if ((src) && (src != msk)) flag_V = M68K_SR_V;
            }
            RET(8)
        }

        if (sft == 0) flag_C = src << M68K_SR_C_SFT;
        else flag_C = 0;
        flag_X = flag_C;
        if (src) flag_V = M68K_SR_V;
        else flag_V = 0;
        res = 0;
        DREGu32((Opcode >> 0) & 7) = res;
        flag_N = 0;
        flag_NotZ = 0;
        RET(8)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 24;
    flag_NotZ = src;
    RET(8)
}

/* LSLD */
OPCODE(0xE128)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu8((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft <= 8)
        {
            flag_X = flag_C = (src << sft) >> 0;
            res = (src << sft) & 0x000000FF;
            flag_V = 0;
            flag_N = res >> 0;
            flag_NotZ = res;
            DREGu8((Opcode >> 0) & 7) = res;
            RET(6)
        }

        flag_X = flag_C = 0;
        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        res = 0;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 0;
    flag_NotZ = src;
    RET(6)
}

/* LSLD */
OPCODE(0xE168)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu16((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft <= 16)
        {
            flag_X = flag_C = (src << sft) >> 8;
            res = (src << sft) & 0x0000FFFF;
            flag_V = 0;
            flag_N = res >> 8;
            flag_NotZ = res;
            DREGu16((Opcode >> 0) & 7) = res;
            RET(6)
        }

        flag_X = flag_C = 0;
        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        res = 0;
        DREGu16((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 8;
    flag_NotZ = src;
    RET(6)
}

/* LSLD */
OPCODE(0xE1A8)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu32((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft < 32)
        {
            flag_X = flag_C = (src >> (32 - sft)) << M68K_SR_C_SFT;
            res = src << sft;
            flag_V = 0;
            flag_N = res >> 24;
            flag_NotZ = res;
            DREGu32((Opcode >> 0) & 7) = res;
            RET(8)
        }

        if (sft == 32) flag_C = src << M68K_SR_C_SFT;
        else flag_C = 0;
        flag_X = flag_C;
        flag_N = 0;
        flag_NotZ = 0;
        flag_V = 0;
        res = 0;
        DREGu32((Opcode >> 0) & 7) = res;
        RET(8)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 24;
    flag_NotZ = src;
    RET(8)
}

/* ROXLD */
OPCODE(0xE130)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu8((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft %= 9;

        src |= (flag_X & M68K_SR_X) << 0;
        res = (src << sft) | (src >> (9 - sft));
        flag_X = flag_C = res >> 0;
        flag_V = 0;
        flag_N = res >> 0;
        flag_NotZ = res & 0x000000FF;
        DREGu8((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = flag_X;
    flag_N = src >> 0;
    flag_NotZ = src;
    RET(6)
}

/* ROXLD */
OPCODE(0xE170)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu16((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft %= 17;

        src |= (flag_X & M68K_SR_X) << 8;
        res = (src << sft) | (src >> (17 - sft));
        flag_X = flag_C = res >> 8;
        flag_V = 0;
        flag_N = res >> 8;
        flag_NotZ = res & 0x0000FFFF;
        DREGu16((Opcode >> 0) & 7) = res;
        RET(6)
    }

    flag_V = 0;
    flag_C = flag_X;
    flag_N = src >> 8;
    flag_NotZ = src;
    RET(6)
}

/* ROXLD */
OPCODE(0xE1B0)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu32((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        sft %= 33;

        if (sft != 0)
        {
            if (sft == 1) res = (src << 1) | ((flag_X >> ((M68K_SR_X_SFT + 1) - 1)) & 1);
            else res = (src << sft) | (src >> (33 - sft)) | (((flag_X >> ((M68K_SR_X_SFT + 1) - 1)) & 1) << (sft - 1));
            flag_X = (src >> (32 - sft)) << M68K_SR_X_SFT;
        }
        else res = src;
        flag_C = flag_X;
        flag_V = 0;
        flag_N = res >> 24;
        flag_NotZ = res;
        DREGu32((Opcode >> 0) & 7) = res;
        RET(8)
    }

    flag_V = 0;
    flag_C = flag_X;
    flag_N = src >> 24;
    flag_NotZ = src;
    RET(8)
}

/* ROLD */
OPCODE(0xE138)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu8((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft &= 0x07)
        {
            flag_C = (src << sft) >> 0;
            res = ((src << sft) | (src >> (8 - sft))) & 0x000000FF;
            flag_V = 0;
            flag_N = res >> 0;
            flag_NotZ = res;
            DREGu8((Opcode >> 0) & 7) = res;
            RET(6)
        }

        flag_V = 0;
        flag_C = src << M68K_SR_C_SFT;
        flag_N = src >> 0;
        flag_NotZ = src;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 0;
    flag_NotZ = src;
    RET(6)
}

/* ROLD */
OPCODE(0xE178)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu16((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft &= 0x0F)
        {
            flag_C = (src << sft) >> 8;
            res = ((src << sft) | (src >> (16 - sft))) & 0x0000FFFF;
            flag_V = 0;
            flag_N = res >> 8;
            flag_NotZ = res;
            DREGu16((Opcode >> 0) & 7) = res;
            RET(6)
        }

        flag_V = 0;
        flag_C = src << M68K_SR_C_SFT;
        flag_N = src >> 8;
        flag_NotZ = src;
        RET(6)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 8;
    flag_NotZ = src;
    RET(6)
}

/* ROLD */
OPCODE(0xE1B8)
{
    uint32_t res;
    uint32_t src;

    uint32_t sft;

    sft = DREG((Opcode >> 9) & 7) & 0x3F;
    src = DREGu32((Opcode >> 0) & 7);
    if (sft)
    {
        io_cycle_counter -= sft * 2;
        if (sft &= 0x1F)
        {
            flag_C = (src >> (32 - sft)) << M68K_SR_C_SFT;
            res = (src << sft) | (src >> (32 - sft));
            flag_V = 0;
            flag_N = res >> 24;
            flag_NotZ = res;
            DREGu32((Opcode >> 0) & 7) = res;
            RET(8)
        }

        flag_V = 0;
        flag_C = src << M68K_SR_C_SFT;
        flag_N = src >> 24;
        flag_NotZ = src;
        RET(8)
    }

    flag_V = 0;
    flag_C = 0;
    flag_N = src >> 24;
    flag_NotZ = src;
    RET(8)
}

/* ASR */
OPCODE(0xE0D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ASR */
OPCODE(0xE0D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ASR */
OPCODE(0xE0E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ASR */
OPCODE(0xE0E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ASR */
OPCODE(0xE0F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ASR */
OPCODE(0xE0F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ASR */
OPCODE(0xE0F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ASR */
OPCODE(0xE0DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ASR */
OPCODE(0xE0E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src & (1 << 15));
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* LSR */
OPCODE(0xE2D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* LSR */
OPCODE(0xE2D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* LSR */
OPCODE(0xE2E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* LSR */
OPCODE(0xE2E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* LSR */
OPCODE(0xE2F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* LSR */
OPCODE(0xE2F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* LSR */
OPCODE(0xE2F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* LSR */
OPCODE(0xE2DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* LSR */
OPCODE(0xE2E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    flag_N = flag_V = 0;
    flag_X = flag_C = src << M68K_SR_C_SFT;
    res = src >> 1;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ROXR */
OPCODE(0xE4D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROXR */
OPCODE(0xE4D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROXR */
OPCODE(0xE4E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ROXR */
OPCODE(0xE4E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ROXR */
OPCODE(0xE4F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ROXR */
OPCODE(0xE4F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ROXR */
OPCODE(0xE4F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ROXR */
OPCODE(0xE4DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROXR */
OPCODE(0xE4E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src >> 1) | ((flag_X & M68K_SR_X) << 7);
    flag_C = flag_X = src << M68K_SR_C_SFT;
    flag_N = res >> 8;
    flag_NotZ = res;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ROR */
OPCODE(0xE6D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROR */
OPCODE(0xE6D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROR */
OPCODE(0xE6E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ROR */
OPCODE(0xE6E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ROR */
OPCODE(0xE6F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ROR */
OPCODE(0xE6F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ROR */
OPCODE(0xE6F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ROR */
OPCODE(0xE6DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROR */
OPCODE(0xE6E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src << M68K_SR_C_SFT;
    res = (src >> 1) | (src << 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ASL */
OPCODE(0xE1D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ASL */
OPCODE(0xE1D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ASL */
OPCODE(0xE1E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ASL */
OPCODE(0xE1E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ASL */
OPCODE(0xE1F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ASL */
OPCODE(0xE1F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ASL */
OPCODE(0xE1F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ASL */
OPCODE(0xE1DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ASL */
OPCODE(0xE1E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_V = (src ^ res) >> 8;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* LSL */
OPCODE(0xE3D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* LSL */
OPCODE(0xE3D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* LSL */
OPCODE(0xE3E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* LSL */
OPCODE(0xE3E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* LSL */
OPCODE(0xE3F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* LSL */
OPCODE(0xE3F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* LSL */
OPCODE(0xE3F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* LSL */
OPCODE(0xE3DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* LSL */
OPCODE(0xE3E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_X = flag_C = src >> 7;
    res = src << 1;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ROXL */
OPCODE(0xE5D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROXL */
OPCODE(0xE5D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROXL */
OPCODE(0xE5E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ROXL */
OPCODE(0xE5E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ROXL */
OPCODE(0xE5F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ROXL */
OPCODE(0xE5F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ROXL */
OPCODE(0xE5F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ROXL */
OPCODE(0xE5DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROXL */
OPCODE(0xE5E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    res = (src << 1) | ((flag_X & M68K_SR_X) >> 8);
    flag_X = flag_C = src >> 7;
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ROL */
OPCODE(0xE7D0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROL */
OPCODE(0xE7D8)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    AREG((Opcode >> 0) & 7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROL */
OPCODE(0xE7E0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7) - 2;
    AREG((Opcode >> 0) & 7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}

/* ROL */
OPCODE(0xE7E8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    adr += AREG((Opcode >> 0) & 7);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ROL */
OPCODE(0xE7F0)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG((Opcode >> 0) & 7);
    DECODE_EXT_WORD
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(18)
}

/* ROL */
OPCODE(0xE7F8)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_SWORD(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(16)
}

/* ROL */
OPCODE(0xE7F9)
{
    uint32_t adr, res;
    uint32_t src;

    FETCH_LONG(adr);
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(20)
}

/* ROL */
OPCODE(0xE7DF)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7);
    AREG(7) += 2;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(12)
}

/* ROL */
OPCODE(0xE7E7)
{
    uint32_t adr, res;
    uint32_t src;

    adr = AREG(7) - 2;
    AREG(7) = adr;
    READ_WORD_F(adr, src)
    flag_V = 0;
    flag_C = src >> 7;
    res = (src << 1) | (src >> 15);
    flag_N = res >> 8;
    flag_NotZ = res & 0x0000FFFF;
    WRITE_WORD_F(adr, res)
    RET(14)
}
#ifdef __cplusplus
}
#endif
