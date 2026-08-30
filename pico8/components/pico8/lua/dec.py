# Parser adapted to 5.2 from https://openpunk.com/pages/lua-bytecode-parser/
# while looking at reference from https://blog.tst.sh/lua-5-2-5-3-bytecode-reference-incomplete/

'''
    Luac.py

    A Lua5.1 cross-platform bytecode deserializer. This module pulls int and size_t sizes from the
    chunk header, meaning it should be able to deserialize lua bytecode dumps from most platforms,
    regardless of the host machine.

    For details on the Lua5.1 bytecode format, I read [this PDF](https://archive.org/download/a-no-frills-intro-to-lua-5.1-vm-instructions/a-no-frills-intro-to-lua-5.1-vm-instructions_archive.torrent)
    as well as read the lundump.c source file from the Lua5.1 source.
'''

import struct
import array
from typing import Tuple
from enum import IntEnum, Enum, auto
from dataclasses import dataclass

LUA_SIGNATURE = bytearray([0x1B, 0x4C, 0x75, 0x61])
LUA_MAGIC = bytearray([0x19, 0x93, 0x0D, 0x0A, 0x1A, 0x0A])

GLOBALS_TABLE = "_ENV"
SLOW_BUILTINS = [
        "spr",
        "sspr",
        "cls",
        "palt",
        "pal",
        "print",
        "rectfill",
        "rect",
        "line",
        "circ",
        "circfill",
        "btn",
        "btnp",
        "map",
        "rnd",
        "pset",
        "pget",
        "fget",
        "mset",
        "mget",
        "sget",
        "t",
        "time",
        "sfx",
        "printh",
        "cartdata",
        "dget",
        "dset",
        "menuitem",
        "music",
        "camera",
        "stat",
        "clip",
        "color",
        # math builtins to lua
        "max",
        "min",
        "mid",
        "atan2",
        "band",
        "bor",
        "bxor",
        "shl",
        "lshr",
        "rotl",
        "rotr",
        "tostr",
        "tonum",
        "chr",
        "ord",
        "split",
        "foreach",
        # stdlib
        "all",
        "sub",
        "add",
        "del",
        "count",
        "assert",
        "yield",
        "cocreate",
        "coresume",
        "costatus",
]

FAST_BUILTINS = [
        "ceil",
        "flr",
        "cos",
        "sin",
        "sqrt",
        "abs",
        "sgn",
        "bnot",
        "shr",
        ]

BUILTINS = FAST_BUILTINS + SLOW_BUILTINS

@dataclass(frozen=True)
class OptimizableInstruction:
    const: 'Constant'
    chunk: 'Chunk'
    inst: 'Instruction'

class InstructionType(Enum):
    ABC = auto(),
    ABx = auto(),
    AsBx = auto()
    Ax = auto()

class ConstType(IntEnum):
    NIL     = 0,
    BOOL    = 1,
    NUMBER  = 3,
    STRING  = 4,


def _get_tabup_ref(chunk: 'Chunk', i: 'Instruction') -> Tuple['Upvalue', 'Constant']:
        # GETTABUP => a = b[c];
        if i.name == "GETTABUP":
            _c = chunk.constants[-i.C-1]
            _u = chunk.upvalues[i.B]
            return _u, _c

        # SETTABUP => a[b] = c;
        if i.name == "SETTABUP":
            _c = chunk.constants[-i.B-1]
            _u = chunk.upvalues[i.A]
            return _u, _c

        raise ValueError(f"Called get tabup ref with {i}")

class Instruction:
    def __init__(self, type: InstructionType, name: str, opcode: int = 0, idx: int = 0) -> None:
        self.type = type
        self.name = name
        self.opcode = opcode
        self.A: int = None
        self.B: int = None
        self.C: int = None
        self.line: int = -1
        self.idx: int = idx

    @property
    def branchy(self):
        return self.name in ["FORPREP", "FORLOOP", "JMP", "TFORCALL", "TFORLOOP"]

    def adjust_to_ignore(self, idx):
        assert self.name in ["FORPREP", "FORLOOP", "JMP"], f"Can't adjust {self.name} yet"
        if self.name == "FORPREP":
            if idx < self.idx:
                return
            if idx > self.idx + self.B:
                return
            self.B -= 1
        elif self.name == "FORLOOP":
            if idx > self.idx:
                return
            if idx < self.idx + self.B:
                return
            self.B += 1
        elif self.name == "JMP":
            # B is signed delta to apply to PC; PC already points to next instruction
            # so if B were 0, this would be a no-op
            _target_within_fwd_jump = self.idx < idx < (self.idx + self.B + 1)
            _target_within_back_jump = self.idx > idx > (self.idx + self.B + 1)
            if _target_within_fwd_jump or _target_within_back_jump:
                self.B -= 1


    def toString(self, chunk: 'Chunk'):
        _s = str(self)
        # GETTABUP => a = b[c];
        if self.name == "GETTABUP":
            _u, _c = _get_tabup_ref(chunk, self)
            _s += f'; {_u.name} {_c}'

        # SETTABUP => a[b] = c;
        if self.name == "SETTABUP":
            _u, _c = _get_tabup_ref(chunk, self)
            _s += f'; {_u.name} {_c}'

        # LOADK => a = bx;
        if self.name == "LOADK":
            const = chunk.constants[self.B]
            _s += f'; {const}'

        # BINARY OP => R(A) := RK(B) op RK(C)
        if self.name in ["MUL"]:
            op = "*"
            if self.A >= len(chunk.locals):
                a = f'(T)R({self.A})'
            else:
                a = f'(L)R({self.A})'
            b = ''
            c = ''
            if self.B < 0:
                b = f'K({abs(self.B)})'
            else:
                b = f'R({self.B})'

            if self.C < 0:
                c = f'K({abs(self.C)})'
            else:
                c = f'R({self.C})'
            _s += f'; {a} := {b} {op} {c}'
        return _s

    def __repr__(self):
        return self.name

    def __str__(self):
        instr = "%10s" % self.name
        regs = ""

        if self.type == InstructionType.ABC:
            regs = "%d %d %d" % (self.A, self.B, self.C)
        elif self.type == InstructionType.ABx or self.type == InstructionType.AsBx:
            regs = "%d %d" % (self.A, self.B)

        return "[%03d] [%02d] %s : %s" % (self.idx, self.line, instr, regs)

    @property
    def source_registers(self):
        if self.name in ["ADD", "SUB", "MUL", "MOD", "POW", "DIV", "IDIV", "BAND", "BOR", "BXOR", "SHL", "SHR", "MOVE", "LOADK", "GETTABUP", "FORPREP", "FORLOOP", "CONCAT"]:
            return [self.B, self.C]
        if self.name in ["CALL"]:  #TODO
            return [self.A]
        if self.name in ["JMP"]:
            return [self.A]
        if self.name in ["TEST"]:
            return [self.A, self.C]
        if self.name in ["TESTSET"]:
            return [self.B, self.C]
        if self.name in ["EQ", "LT", "LE"]:
            return [self.A, self.B, self.C]

        print(f"Didn't know the source register of {self.name}")

    @property
    def target_register(self):
        if self.name in ["ADD", "SUB", "MUL", "MOD", "POW", "DIV", "IDIV", "BAND", "BOR", "BXOR", "SHL", "SHR", "MOVE", "LOADK", "GETTABUP", "FORPREP", "FORLOOP", "CONCAT", "TESTSET"]:
            return self.A
        if self.name in ["SETTABUP", "RETURN", "CALL", "TEST", "JMP", "LT", "EQ", "LE"]:
            return None
        print(f"Didn't know the target register of {self.name}")

    @target_register.setter
    def target_register(self, value):
        if self.name in ["ADD", "SUB", "MUL", "MOD", "POW", "DIV", "IDIV", "BAND", "BOR", "BXOR", "SHL", "SHR", "MOVE", "LOADK", "GETTABUP", "FORPREP", "FORLOOP", "CONCAT", "TESTSET"]:
            self.A = value
            return
        if self.name in ["SETTABUP", "RETURN", "CALL", "TEST", "JMP", "LT", "EQ", "LE"]:
            raise ValueError(f"Tried to set target on {self.name}")
        print(f"Don't know how to re-target register of {self.name}")

    def replace_source_register(self, old_register, new_register):
        if self.name in ["ADD", "SUB", "MUL", "MOD", "POW", "DIV", "IDIV", "BAND", "BOR", "BXOR", "SHL", "SHR", "MOVE", "LOADK", "GETTABUP", "FORPREP", "FORLOOP", "CONCAT"]:
            if self.B == old_register:
                self.B = new_register
            if self.C == old_register:
                self.C = new_register
            return
        if self.name in ["CALL"]:  #TODO
            if self.A == old_register:
                self.A = new_register
            return
        print(f"Don't know how to replace source register of {self.name}")


    @staticmethod
    def from_bytes(data: bytes) -> 'Instruction':
        opcode = _get_bits(data, 0, 6)
        template = instr_lookup_tbl[opcode]
        instruction = Instruction(template.type, template.name, opcode)

        # i read the lopcodes.h file to get these bit position and sizes.
        instruction.A = _get_bits(data, 6, 8) # starts after POS_OP + SIZE_OP (6), with a size of 8

        if instruction.type == InstructionType.ABC:
            instruction.B = _get_bits(data, 23, 9) # starts after POS_C + SIZE_C (23), with a size of 9
            instruction.C = _get_bits(data, 14, 9) # starts after POS_A + SIZE_A (14), with a size of 9
            if instruction.B > 255:
                instruction.B = 255 - instruction.B
            if instruction.C > 255:
                instruction.C = 255 - instruction.C

        elif instruction.type == InstructionType.ABx:
            instruction.B = _get_bits(data, 14, 18) # starts after POS_A + SIZE_A (14), with a size of 18
        elif instruction.type == InstructionType.AsBx:
            instruction.B = _get_bits(data, 14, 18) - 131071 # Bx is now signed, so just sub half of the MAX_UINT for 18 bits
        return instruction

    def dump(self) -> bytes:
        i = 0
        i |= self.opcode & 0b111111  # lower 6 bits
        i |= (self.A & 0xff) << 6 # lower 8 bits, displaced 6 bits
        if self.type == InstructionType.ABC:
            _c = self.C
            _b = self.B
            if _c < 0:
                _c = 255 - _c
            if _b < 0:
                _b = 255 - _b

            i |= (_c & 0x1ff) << 14  # lower _9_ bits, displaced 14
            i |= (_b & 0x1ff) << 23  # lower _9_ bits, displaced 23
        elif self.type == InstructionType.ABx:
            i |= (self.B & 0x3ffff) << 14  # lower _18_ bits, displaced 14
        elif self.type == InstructionType.AsBx:
            i |= ((abs(self.B + 131071) & 0x3ffff) << 14) # lower _18_ bits, displaced 14; add MAX_UINT (131071) to make signed
        return _to_u32(i)

class Constant:
    def __init__(self, type: ConstType, data) -> None:
        self.type = type
        self.data = data

    def __str__(self):
        printable_data = self.data
        if self.type == ConstType.NUMBER:
            #printable_data = float((self.data & 0xFFFF0000) >> 16) + ((self.data & 0xFFFF)/0xFFFF)
            _int = (self.data & 0xFFFF0000) >> 16
            _dec = (self.data & 0x0000FFFF)/0xFFFF
            printable_data = _int + _dec

        if self.type == ConstType.STRING:
            return f'"{printable_data}"'
        return str(printable_data)

    def __repr__(self):
        return str(self)

    def toString(self):
        return str(self)

    @staticmethod
    def from_bytes(b: bytes) -> Tuple['Constant', int]:
        type = b[0]
        read = 1
        constant = None

        if type == 0: #nil
            constant = Constant(ConstType.NIL, None)
        elif type == 1: # bool
            constant = Constant(ConstType.BOOL, (b[1] != 0))
            read += 1
        elif type == 3: # number
            data = int.from_bytes(b[1:5], byteorder='little', signed=False)
            constant = Constant(ConstType.NUMBER, data)
            read += 4
        elif type == 4: # string
            # FIXME size_t
            _strlen = int.from_bytes(b[1:9], byteorder='little', signed=False)
            _str = b[9:9+_strlen-1].decode()
            constant = Constant(ConstType.STRING, _str)
            # 8 = size_t
            # 1 = null byte
            read += len(constant.data) + 1 + 8
        else:
            raise Exception("Unknown Datatype! [%d]" % type)

        return (constant, read)

    def dump(self) -> bytes:
        b = _to_u8(self.type)

        if self.type == ConstType.NIL:
            pass
        elif self.type == ConstType.BOOL:
            b.extend(_to_u8(int(self.data)))
        elif self.type == ConstType.NUMBER:
            b.extend(_to_u32(self.data))
        elif self.type == ConstType.STRING:
            b.extend(_to_str(self.data))
        else:
            raise Exception("Unknown Datatype! [%d]" % type)
        return b


class Local:
    def __init__(self, name: str, start: int, end: int):
        self.name = name
        self.start = start
        self.end = end

    def __str__(self):
        return f'{self.name}\t{self.start}\t{self.end}'

    def dump(self) -> bytearray:
        b = bytearray()
        b.extend(_to_str(self.name))
        b.extend(_to_u32(self.start))
        b.extend(_to_u32(self.end))
        return b


class Upvalue:
    def __init__(self, idx: int, stack: int, register: int, name: str = '??'):
        self.idx = idx
        self.stack = stack
        self.register = register
        self.name = name

    def __str__(self):
        return f'{self.idx} {self.name} {self.stack} {self.register}'

    def dump(self) -> bytearray:
        b = bytearray()
        b.extend(_to_u8(self.stack))
        b.extend(_to_u8(self.register))
        return b

class Chunk:
    def __init__(self) -> None:
        self.constants: list[Constant] = []
        self.instructions: list[Instruction] = []
        self.protos: list[Chunk] = []

        self.source: str = "??"
        self.frst_line: int = 0
        self.last_line: int = 0
        self.numUpvals: int = 0
        self.numParams: int = 0
        self.isVarg: bool = False
        self.maxStack: int = 0

        self.upvalues: list[Upvalue] = []
        self.locals: list[Local] = []

    @property
    def name(self):
        if self.frst_line == 0:
            _name = "main"
        else:
            _name = "function"
        return f"{_name} <{self.source[1:]}:{self.frst_line},{self.last_line}>"


    def appendInstruction(self, instr: Instruction):
        self.instructions.append(instr)

    def appendConstant(self, const: Constant):
        self.constants.append(const)

    def appendProto(self, proto):
        self.protos.append(proto)

    def print(self):
        print(f'{self.name} ({len(self.instructions)} instructions)')
        for i in range(len(self.instructions)):
            #print("\t[%3d] %s" % (i, self.instructions[i].toString(self)))
            print("\t%s" % (self.instructions[i].toString(self)))

        print(f'constants ({len(self.constants)})')
        for z in range(len(self.constants)):
            i = self.constants[z]
            print('\t' + str(z+1) + ": " + i.toString())

        print(f'locals ({len(self.locals)})')
        for i, l in enumerate(self.locals):
            print(f'\t{i}\t{l}')

        print(f'upvalues ({len(self.upvalues)})')
        for u in self.upvalues:
            print('\t' + str(u))

        # print("==== [[" + str(self.name) + "'s protos]] ====")
        for z in self.protos:
            z.print()

    def __repr__(self):
        return self.name

    def __str__(self):
        return self.name

    def dump(self) -> bytearray:
        buf = bytearray()

        buf.extend(_to_u32(self.frst_line))
        buf.extend(_to_u32(self.last_line))
        buf.extend(_to_u8(self.numParams))
        buf.extend(_to_u8(int(self.isVarg)))
        buf.extend(_to_u8(int(self.numUpvals)))

        buf.extend(_to_u32(len(self.instructions)))
        for i in self.instructions:
            buf.extend(i.dump())

        buf.extend(_to_u32(len(self.constants)))
        for c in self.constants:
            buf.extend(c.dump())

        buf.extend(_to_u32(len(self.protos)))
        for p in self.protos:
            buf.extend(p.dump())

        buf.extend(_to_u32(len(self.upvalues)))
        for u in self.upvalues:
            buf.extend(u.dump())

        buf.extend(_to_str(self.source))

        buf.extend(_to_u32(len(self.instructions)))
        for i in self.instructions:
            buf.extend(_to_u32(i.line))

        buf.extend(_to_u32(len(self.locals)))
        for l in self.locals:
            buf.extend(l.dump())

        buf.extend(_to_u32(len(self.upvalues)))
        for u in self.upvalues:
            buf.extend(_to_str(u.name))

        return buf

    def shadow_temp_regs(self) -> None:
        last_valid_loc_idx = len(self.locals)
        last_seen_reg = -1
        for i in self.instructions:
            if i.target_register:
                last_seen_reg = max(last_seen_reg, i.target_register)

        print(f'should shadow til {last_valid_loc_idx}')
        for k in range(last_valid_loc_idx, last_seen_reg+1):
            self.add_local(f"shadow_reg_{k}")

    def add_local(self, _id: str) -> int:
        self.locals.append(Local(f'__tmpLocal_{_id}', self.frst_line, self.last_line))
        return len(self.locals)-1

    def pop_i_by_idx(self, idx: int):
        inst = self.instructions[idx]
        for i in self.instructions:
            if i.branchy:
                i.adjust_to_ignore(idx)
            if i.idx > idx:
                i.idx -= 1
        self.instructions.remove(inst)

    def pop_i(self, inst: Instruction):
        idx = self.instructions.index(inst)
        self.pop_i_by_idx(idx)
    

    def retarget_reads_until_write(self, idx: int, old_register: int, new_register: int):
        # FIXME: this needs to take branching into account
        for i in self.instructions[idx:]:
            print(f"Looking at {i}, {i.branchy}")
            if i.target_register == old_register:
                # writing to `source_register`; further reads are for other values
                return
            if old_register in i.source_registers:
                i.replace_source_register(old_register, new_register)

    def retarget_write_before(self, idx: int, target_register: int):
        print(f'retargeting write before {idx} with target {target_register}')
        for i in self.instructions[idx:0:-1]:
            print(i)
            if i.target_register:
                print('retargetted')
                i.target_register = target_register
                return


instr_lookup_tbl = [
        Instruction(InstructionType.ABC, "MOVE"),
        Instruction(InstructionType.ABx, "LOADK"),
        Instruction(InstructionType.ABx, "LOADKX"),
        Instruction(InstructionType.ABC, "LOADBOOL"),
        Instruction(InstructionType.ABC, "LOADNIL"),
        Instruction(InstructionType.ABC, "GETUPVAL"),
        Instruction(InstructionType.ABC, "GETTABUP"),
        Instruction(InstructionType.ABC, "GETTABLE"),
        Instruction(InstructionType.ABC, "SETTABUP"),
        Instruction(InstructionType.ABC, "SETUPVAL"),
        Instruction(InstructionType.ABC, "SETTABLE"),
        Instruction(InstructionType.ABC, "NEWTABLE"),
        Instruction(InstructionType.ABC, "SELF"),
        Instruction(InstructionType.ABC, "ADD"),
        Instruction(InstructionType.ABC, "SUB"),
        Instruction(InstructionType.ABC, "MUL"),
        Instruction(InstructionType.ABC, "DIV"),
        Instruction(InstructionType.ABC, "MOD"),
        Instruction(InstructionType.ABC, "POW"),

        Instruction(InstructionType.ABC, "IDIV"),
        Instruction(InstructionType.ABC, "BAND"),
        Instruction(InstructionType.ABC, "BOR"),
        Instruction(InstructionType.ABC, "BXOR"),
        Instruction(InstructionType.ABC, "SHL"),
        Instruction(InstructionType.ABC, "SHR"),
        Instruction(InstructionType.ABC, "LSHR"),
        Instruction(InstructionType.ABC, "ROTL"),
        Instruction(InstructionType.ABC, "ROTR"),

        Instruction(InstructionType.ABC, "UNM"),
        Instruction(InstructionType.ABC, "BNOT"),
        Instruction(InstructionType.ABC, "NOT"),
        Instruction(InstructionType.ABC, "PEEK"),
        Instruction(InstructionType.ABC, "PEEK2"),
        Instruction(InstructionType.ABC, "PEEK4"),

        Instruction(InstructionType.ABC, "LEN"),
        Instruction(InstructionType.ABC, "CONCAT"),
        Instruction(InstructionType.AsBx, "JMP"),
        Instruction(InstructionType.ABC, "EQ"),
        Instruction(InstructionType.ABC, "LT"),
        Instruction(InstructionType.ABC, "LE"),
        Instruction(InstructionType.ABC, "TEST"),
        Instruction(InstructionType.ABC, "TESTSET"),
        Instruction(InstructionType.ABC, "CALL"),
        Instruction(InstructionType.ABC, "TAILCALL"),
        Instruction(InstructionType.ABC, "RETURN"),
        Instruction(InstructionType.AsBx, "FORLOOP"),
        Instruction(InstructionType.AsBx, "FORPREP"),
        Instruction(InstructionType.ABC, "TFORCALL"),
        Instruction(InstructionType.ABC, "TFORLOOP"),
        Instruction(InstructionType.ABC, "SETLIST"),
        Instruction(InstructionType.ABx, "CLOSURE"),
        Instruction(InstructionType.ABC, "VARARG"),
        Instruction(InstructionType.Ax, "EXTRAARG"),
]

# at [p]osition, with [s]ize of bits
def _get_bits(num, p, s):
    num = num >> p
    num = num & ((2**s)-1)
    return num

def _set_bits(num, p, s):

    return num

class LuaUndump:
    def __init__(self):
        self.rootChunk: Chunk = None
        self.index = 0

    @staticmethod
    def dis_chunk(chunk: Chunk):
        chunk.print()

    def _current_buf(self) -> bytearray:
        return bytearray(self.bytecode[self.index:])

    def loadBlock(self, sz) -> bytearray:
        if self.index + sz > len(self.bytecode):
            raise Exception("Malformed bytecode!")

        temp = bytearray(self.bytecode[self.index:self.index+sz])
        # print(f"bytecode range for block of size {sz} is {['{:02x}'.format(x) for x in temp]}")
        self.index = self.index + sz
        return temp

    def get_byte(self) -> int:
        return self.loadBlock(1)[0]

    def get_int16(self) -> int:
        if (self.big_endian):
            return int.from_bytes(self.loadBlock(2), byteorder='big', signed=False)
        else:
            return int.from_bytes(self.loadBlock(2), byteorder='little', signed=False)

    def get_int32(self) -> int:
        if (self.big_endian):
            return int.from_bytes(self.loadBlock(4), byteorder='big', signed=False)
        else:
            return int.from_bytes(self.loadBlock(4), byteorder='little', signed=False)

    def get_int(self) -> int:
        if (self.big_endian):
            return int.from_bytes(self.loadBlock(self.int_size), byteorder='big', signed=False)
        else:
            return int.from_bytes(self.loadBlock(self.int_size), byteorder='little', signed=False)

    def get_size_t(self) -> int:
        if (self.big_endian):
            return int.from_bytes(self.loadBlock(self.size_t), byteorder='big', signed=False)
        else:
            return int.from_bytes(self.loadBlock(self.size_t), byteorder='little', signed=False)

    def get_string(self, size) -> str:
        if (size == None):
            size = self.get_size_t()
            if (size == 0):
                return ""

        return "".join(chr(x) for x in self.loadBlock(size))

    def decode_chunk(self):
        chunk = Chunk()

        chunk.frst_line = self.get_int32()
        chunk.last_line = self.get_int32()

        chunk.numParams = self.get_byte()
        chunk.isVarg = (self.get_byte() != 0)
        chunk.numUpvals = self.get_byte()

        # parse instructions
        num = self.get_int()
        for i in range(num):
            data   = self.get_int32()
            inst = Instruction.from_bytes(data)
            inst.idx = i
            _b = inst.dump()
            my_inst = Instruction.from_bytes(int.from_bytes(_b, byteorder='little', signed=False))
            my_inst.idx = i
            assert inst.type == my_inst.type
            assert inst.name == my_inst.name
            assert inst.opcode == my_inst.opcode
            assert inst.A == my_inst.A
            assert inst.B == my_inst.B
            assert inst.C == my_inst.C, f"{inst.C=} != {my_inst.C=}"
            assert inst.line == my_inst.line
            chunk.appendInstruction(inst)

        # get constants
        num = self.get_int()
        for i in range(num):
            constant, bytes_read = Constant.from_bytes(self._current_buf())
            self.index += bytes_read

            _b = constant.dump()
            my_const, _ = Constant.from_bytes(_b)
            assert constant.data == my_const.data, f'{constant.data=} {my_const.data=}'
            chunk.appendConstant(constant)

        # parse protos / "primitives"
        num = self.get_int()
        for i in range(num):
            chunk.appendProto(self.decode_chunk())

        # upvalues
        num = self.get_int()
#        print(f'getting {num} upvalues')
        for i in range(num):
            stack = self.get_byte()
            register = self.get_byte()
            chunk.upvalues.append(Upvalue(i, stack, register))

        source = self.get_string(None)[:-1]
        chunk.source = source

        # line numbers
        num = self.get_int()
        for i in range(num):
            line = self.get_int()
            chunk.instructions[i].line = line

        num = self.get_int()
        for i in range(num):
            localname = self.get_string(None)[:-1]
            startpc = self.get_int()
            endpc = self.get_int()
            chunk.locals.append(Local(localname, startpc, endpc))

        num = self.get_int()
        for i in range(num):
            upvalname = self.get_string(None)[:-1]
            chunk.upvalues[i].name = upvalname


        return chunk

    def decode_rawbytecode(self, rawbytecode):
        # bytecode sanity checks
        if not rawbytecode[0:4] == b'\x1bLua':
            raise Exception("Lua Bytecode expected!")

        bytecode = array.array('b', rawbytecode)
        return self.decode_bytecode(bytecode)

    def decode_bytecode(self, bytecode):
        self.bytecode   = bytecode

        # aligns index, skips header
        self.index = 4

        self.vm_version = self.get_byte()
        self.bytecode_format = self.get_byte()
        self.big_endian = (self.get_byte() == 0)
        assert not self.big_endian
        self.int_size   = self.get_byte()
        self.size_t     = self.get_byte()
        self.instr_size = self.get_byte() # gets size of instructions
        self.l_number_size = self.get_byte() # size of lua_Number
        self.integral_flag = self.get_byte()

        assert self.get_byte() == 0x19
        assert self.get_byte() == 0x93
        assert self.get_byte() == ord('\r')
        assert self.get_byte() == ord('\n')
        assert self.get_byte() == 0x1a  # ?
        assert self.get_byte() == ord('\n')

        self.rootChunk = self.decode_chunk()
        return self.rootChunk

    def dump(self) -> bytes:
        b = bytearray()
        b.extend(LUA_SIGNATURE)
        b.extend(_to_u8(self.vm_version))
        b.extend(_to_u8(self.bytecode_format))
        b.extend(_to_u8(int(not self.big_endian)))
        b.extend(_to_u8(self.int_size))
        b.extend(_to_u8(self.size_t))
        b.extend(_to_u8(self.instr_size))
        b.extend(_to_u8(self.l_number_size))
        b.extend(_to_u8(self.integral_flag))
        b.extend(LUA_MAGIC)
        b.extend(self.rootChunk.dump())
        return b

    def loadFile(self, luaCFile):
        with open(luaCFile, 'rb') as luac_file:
            bytecode = luac_file.read()
            return self.decode_rawbytecode(bytecode)

    def print_dissassembly(self):
        LuaUndump.dis_chunk(self.rootChunk)

    def find_localization_candidates(self):
        print('\n############ Optimizations ############\n')
        # recurse through all rootChunk.protos; find GETTABUP and SETTABUP to
        _known_funcs = []
        # TODO: this shouldn't be needed; the current problem is that a lookup that doesn't 
        # immediately call a function will be optimized
        all_known_functions(self.rootChunk, _known_funcs)
        d = {}
        tabup_access_per_chunk(self.rootChunk, d)

        const_to_locals = {}
        for const, optimizables in d.items():
            if len({o.chunk for o in optimizables}) > 1:
                continue
            if const in _known_funcs:
                continue

            for k in sorted(optimizables, key=lambda o: o.inst.idx):
                inst_idx = k.chunk.instructions.index(k.inst)
                k.chunk.shadow_temp_regs()
                if k.inst.name == 'SETTABUP':
                    local_idx = k.chunk.add_local(k.const.data)
                    const_to_locals[k.const.data] = local_idx
                    k.chunk.pop_i(k.inst)
                    # this was going to set in a table R(A)
                    # should replace that (1 write, going back) with the new local idx
                    # but also, all reads of that register (if any?) until a write or EOF
                    # TODO
                    k.chunk.retarget_write_before(inst_idx-1, local_idx)
                    k.chunk.retarget_reads_until_write(inst_idx, k.inst.target_register, local_idx)
                    #for _followup_inst in k.chunk.instructions[inst_idx:]:
                    #    break
                elif k.inst.name == 'GETTABUP':
                    #k.chunk.pop_i_by_idx(inst_idx-1) # GETTABUP is preceded by LOADK
                    _const = const_to_locals[k.const.data]
                    #k.chunk.pop_i(k.inst)
                    _tpl = [i for i in instr_lookup_tbl if i.name == 'MOVE'][0]
                    k.inst.name = _tpl.name
                    k.inst.opcode = _tpl.opcode
                    k.inst.type = _tpl.type
                    k.inst.B = _const
                    k.inst.C = 0


def all_known_functions(chunk, _list):
    prev_inst = None
    for inst in chunk.instructions:
        if inst.name != 'SETTABUP':
            prev_inst = inst
            continue
        if prev_inst and prev_inst.name != "CLOSURE":
            continue
        u, c = _get_tabup_ref(chunk, inst)
        if u.name != GLOBALS_TABLE:
            continue
        _list.append(c.data)

    for _chunk in chunk.protos:
        all_known_functions(_chunk, _list)

def tabup_access_per_chunk(chunk, _dict):
    prev_inst = None
    for idx, inst in enumerate(chunk.instructions):
        if inst.name not in ['GETTABUP', 'SETTABUP']:
            continue
        if idx > 1:
            prev_inst = chunk.instructions[idx-1]

        if prev_inst and prev_inst.name in ['CLOSURE']:
            # can't make function declarations local.. maybe
            continue
        u, c = _get_tabup_ref(chunk, inst)
        if u.name != GLOBALS_TABLE:
            print("Skipping table on ", u.name)
            continue
        if c.data in BUILTINS:
            print("Skipping var in builtin", c.data)
            continue

        o = OptimizableInstruction(c, chunk, inst)
        _dict.setdefault(c.data, set())
        _dict[c.data].add(o)

    for _chunk in chunk.protos:
        tabup_access_per_chunk(_chunk, _dict)


def _to_str(s: str) -> bytearray:
    b = bytearray()
    # FIXME this is 'size_t'
    b.extend(_to_u64(len(s)+1))  # +1 for null byte
    b.extend(s.encode())
    b.extend(_to_u8(0)) # null byte
    return b

def _to_u64(n: int) -> bytearray:
    assert n < 0xFFFFFFFFFFFFFFFF, f'{n} is too big by {n-0xFFFFFFFFFFFFFFFF}!'
    return bytearray(struct.pack('<Q', n))

def _to_u32(n: int) -> bytearray:
    assert n < 0xFFFFFFFF, f'{n} is too big by {n-0xFFFFFFFF}!'
    return bytearray(struct.pack('<I', n))

def _to_u8(n: int) -> bytearray:
    assert n <= 0xFF
    return bytearray([n])

lu = LuaUndump()
lu.loadFile('luac.out')
# lu.print_dissassembly()

lu.find_localization_candidates()
# "Optimized"
lu.print_dissassembly()

with open('myoutput', 'wb') as fd:
    fd.write(lu.dump())
