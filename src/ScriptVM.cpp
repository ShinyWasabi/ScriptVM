#include "ScriptVM.hpp"
#include "ScriptBreakpoint.hpp"
#include "rage/Joaat.hpp"
#include "rage/scrThread.hpp"
#include "rage/scrProgram.hpp"
#include "rage/scrNativeCallContext.hpp"
#include "rage/Opcode.hpp"
#include "rage/tlsContext.hpp"

#define GET_BYTE (*++opcode)
#define GET_WORD ((opcode += 2), *reinterpret_cast<std::uint16_t*>(opcode - 1))
#define GET_SWORD ((opcode += 2), *reinterpret_cast<std::int16_t*>(opcode - 1))
#define GET_24BIT ((opcode += 3), *reinterpret_cast<std::uint32_t*>(opcode - 3) >> 8)
#define GET_DWORD ((opcode += 4), *reinterpret_cast<std::uint32_t*>(opcode - 3))

#define SCRIPT_FAILURE(str)                                                                   \
    do                                                                                        \
    {                                                                                         \
        if (auto thread = rage::tlsContext::Get()->m_CurrentScriptThread)                     \
        {                                                                                     \
            thread->m_Context.m_ProgramCounter = static_cast<std::int32_t>(opcode - basePtr); \
        }                                                                                     \
                                                                                              \
        MessageBoxA(nullptr, str, "ScriptVM", MB_OK | MB_ICONERROR);                          \
                                                                                              \
        return context->m_State = rage::ThreadState::KILLED;                                  \
    } while (0)

#define JUMP(_offset)                                              \
    do                                                             \
    {                                                              \
        std::int64_t offset = _offset;                             \
        opcode = bytecode[offset >> 14U] + (offset & 0x3FFFU) - 1; \
        basePtr = &opcode[-offset];                                \
    }                                                              \
    while (0)

void AssignString(char* dest, std::uint32_t size, const char* src)
{
    if (!src || !size)
        return;

    const char* s = src;
    char* d = dest;
    std::uint32_t remaining = size;

    for (;;)
    {
        if (--remaining == 0 || !(*d++ = *s++))
            break;
    }

    *d = '\0';
}

void AppendString(char* dest, std::uint32_t size, const char* src)
{
    if (!size)
        return;

    char* d = dest;
    std::uint32_t remaining = size;

    while (remaining && *d)
    {
        ++d;
        --remaining;
    }

    if (remaining)
    {
        const char* s = src;
        while (--remaining && (*d++ = *s++));
        *d = '\0';
    }
}

void Itoa(char* dest, std::int32_t value)
{
    char* d = dest;

    std::uint32_t uval;
    if (value < 0)
    {
        *d++ = '-';
        uval = static_cast<std::uint32_t>(-value);
    }
    else
        uval = static_cast<std::uint32_t>(value);

    std::uint32_t tmp = uval;
    std::int32_t digits = 0;
    do
    {
        ++digits;
        tmp /= 10;
    } while (tmp);

    d[digits] = '\0';

    while (digits--)
    {
        d[digits] = '0' + (uval % 10);
        uval /= 10;
    }
}

rage::ThreadState RunScriptThread(rage::scrValue* stack, rage::scrValue** globals, rage::scrProgram* program, rage::scrThreadContext* context)
{
    char buffer[16];
    std::uint8_t* opcode;
    std::uint8_t* basePtr;

    std::uint8_t** bytecode = program->m_CodeBlocks;
    rage::scrValue* stackPtr = stack + context->m_StackPointer - 1;
    rage::scrValue* framePtr = stack + context->m_FramePointer;

    JUMP(context->m_ProgramCounter);

NEXT:

    const auto script = static_cast<std::uint32_t>(context->m_ScriptHash);
    const auto pc = static_cast<std::uint32_t>(opcode - basePtr);
    if (ScriptBreakpoint::Has(script, pc))
    {
        ScriptBreakpoint::IncrementHitCount(script, pc);
        if (!ScriptBreakpoint::ShouldSkip(script, pc))
        {
            if (ScriptBreakpoint::ShouldPause(script, pc))
            {
                context->m_ProgramCounter = pc;
                ScriptBreakpoint::Skip(script, pc);
                return context->m_State = rage::ThreadState::PAUSED;
            }
        }
    }

    switch (GET_BYTE)
    {
    case rage::Opcodes::NOP:
    {
        JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::IADD:
    {
        --stackPtr;
        stackPtr[0].Int += stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::ISUB:
    {
        --stackPtr;
        stackPtr[0].Int -= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::IMUL:
    {
        --stackPtr;
        stackPtr[0].Int *= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::IDIV:
    {
        --stackPtr;
        if (stackPtr[1].Int)
            stackPtr[0].Int /= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::IMOD:
    {
        --stackPtr;
        if (stackPtr[1].Int)
            stackPtr[0].Int %= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::INOT:
    {
        stackPtr[0].Int = !stackPtr[0].Int;
        goto NEXT;
    }
    case rage::Opcodes::INEG:
    {
        stackPtr[0].Int = -stackPtr[0].Int;
        goto NEXT;
    }
    case rage::Opcodes::IEQ:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int == stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::INE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int != stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::IGT:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int > stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::IGE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int >= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::ILT:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int < stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::ILE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int <= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::FADD:
    {
        --stackPtr;
        stackPtr[0].Float += stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FSUB:
    {
        --stackPtr;
        stackPtr[0].Float -= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FMUL:
    {
        --stackPtr;
        stackPtr[0].Float *= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FDIV:
    {
        --stackPtr;
        if (stackPtr[1].Int)
            stackPtr[0].Float /= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FMOD:
    {
        --stackPtr;
        if (stackPtr[1].Int)
        {
            float x = stackPtr[0].Float;
            float y = stackPtr[1].Float;
            stackPtr[0].Float = y ? x - (static_cast<std::int32_t>(x / y) * y) : 0.0f;
        }
        goto NEXT;
    }
    case rage::Opcodes::FNEG:
    {
        stackPtr[0].Uns ^= 0x80000000;
        goto NEXT;
    }
    case rage::Opcodes::FEQ:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float == stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FNE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float != stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FGT:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float > stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FGE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float >= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FLT:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float < stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::FLE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float <= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::Opcodes::VADD:
    {
        stackPtr -= 3;
        stackPtr[-2].Float += stackPtr[1].Float;
        stackPtr[-1].Float += stackPtr[2].Float;
        stackPtr[0].Float += stackPtr[3].Float;
        goto NEXT;
    }
    case rage::Opcodes::VSUB:
    {
        stackPtr -= 3;
        stackPtr[-2].Float -= stackPtr[1].Float;
        stackPtr[-1].Float -= stackPtr[2].Float;
        stackPtr[0].Float -= stackPtr[3].Float;
        goto NEXT;
    }
    case rage::Opcodes::VMUL:
    {
        stackPtr -= 3;
        stackPtr[-2].Float *= stackPtr[1].Float;
        stackPtr[-1].Float *= stackPtr[2].Float;
        stackPtr[0].Float *= stackPtr[3].Float;
        goto NEXT;
    }
    case rage::Opcodes::VDIV:
    {
        stackPtr -= 3;
        if (stackPtr[1].Int)
            stackPtr[-2].Float /= stackPtr[1].Float;
        if (stackPtr[2].Int)
            stackPtr[-1].Float /= stackPtr[2].Float;
        if (stackPtr[3].Int)
            stackPtr[0].Float /= stackPtr[3].Float;
        goto NEXT;
    }
    case rage::Opcodes::VNEG:
    {
        stackPtr[-2].Uns ^= 0x80000000;
        stackPtr[-1].Uns ^= 0x80000000;
        stackPtr[0].Uns ^= 0x80000000;
        goto NEXT;
    }
    case rage::Opcodes::IAND:
    {
        --stackPtr;
        stackPtr[0].Int &= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::IOR:
    {
        --stackPtr;
        stackPtr[0].Int |= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::IXOR:
    {
        --stackPtr;
        stackPtr[0].Int ^= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::Opcodes::I2F:
    {
        stackPtr[0].Float = static_cast<float>(stackPtr[0].Int);
        goto NEXT;
    }
    case rage::Opcodes::F2I:
    {
        stackPtr[0].Int = static_cast<std::int32_t>(stackPtr[0].Float);
        goto NEXT;
    }
    case rage::Opcodes::F2V:
    {
        stackPtr += 2;
        stackPtr[-1].Int = stackPtr[0].Int = stackPtr[-2].Int;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_U8:
    {
        ++stackPtr;
        stackPtr[0].Int = GET_BYTE;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_U8_U8:
    {
        stackPtr += 2;
        stackPtr[-1].Int = GET_BYTE;
        stackPtr[0].Int = GET_BYTE;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_U8_U8_U8:
    {
        stackPtr += 3;
        stackPtr[-2].Int = GET_BYTE;
        stackPtr[-1].Int = GET_BYTE;
        stackPtr[0].Int = GET_BYTE;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_U32:
    case rage::Opcodes::PUSH_CONST_F:
    {
        ++stackPtr;
        stackPtr[0].Uns = GET_DWORD;
        goto NEXT;
    }
    case rage::Opcodes::DUP:
    {
        ++stackPtr;
        stackPtr[0].Any = stackPtr[-1].Any;
        goto NEXT;
    }
    case rage::Opcodes::DROP:
    {
        --stackPtr;
        goto NEXT;
    }
    case rage::Opcodes::NATIVE:
    {
        std::uint8_t native = GET_BYTE;
        std::int32_t argCount = (native >> 2) & 0x3F;
        std::int32_t retCount = native & 3;

        rage::scrNativeHandler handler = reinterpret_cast<rage::scrNativeHandler>(program->m_Natives[(GET_BYTE << 8) | GET_BYTE]);

        context->m_ProgramCounter = static_cast<std::int32_t>(opcode - basePtr - 4);
        context->m_StackPointer = static_cast<std::int32_t>(stackPtr - stack + 1);
        context->m_FramePointer = static_cast<std::int32_t>(framePtr - stack);

        rage::scrNativeCallContext ctx(retCount ? &stack[context->m_StackPointer - argCount] : 0, argCount, &stack[context->m_StackPointer - argCount]);
        (*handler)(&ctx);

        // In case TERMINATE_THIS_THREAD or TERMINATE_THREAD is called
        if (context->m_State != rage::ThreadState::RUNNING)
            return context->m_State;

        ctx.FixVectors();

        stackPtr -= argCount - retCount;
        goto NEXT;
    }
    case rage::Opcodes::ENTER:
    {
        std::uint32_t argCount = GET_BYTE;
        std::uint32_t localCount = GET_WORD;
        std::uint32_t nameCount = GET_BYTE;

        if (context->m_CallDepth < 16)
            context->m_CallStack[context->m_CallDepth] = static_cast<std::int32_t>(opcode - basePtr + 2);
        ++context->m_CallDepth;

        opcode += nameCount;

        if (stackPtr - stack >= static_cast<std::int32_t>((context->m_StackSize - localCount)))
            SCRIPT_FAILURE("Stack overflow!");
        (++stackPtr)->Int = static_cast<std::int32_t>(framePtr - stack);

        framePtr = stackPtr - argCount - 1;

        while (localCount--)
            (++stackPtr)->Any = 0;

        stackPtr -= argCount;
        goto NEXT;
    }
    case rage::Opcodes::LEAVE:
    {
        --context->m_CallDepth;

        std::uint32_t argCount = GET_BYTE;
        std::uint32_t retCount = GET_BYTE;

        rage::scrValue* ret = stackPtr - retCount;
        stackPtr = framePtr + argCount - 1;
        framePtr = stack + stackPtr[2].Uns;

        std::uint32_t caller = stackPtr[1].Uns;
        JUMP(caller);
        stackPtr -= argCount;

        while (retCount--)
            *++stackPtr = *++ret;

        // Script reached end of code
        if (!caller)
            return context->m_State = rage::ThreadState::KILLED;

        goto NEXT;
    }
    case rage::Opcodes::LOAD:
    {
        stackPtr[0].Any = stackPtr[0].Reference->Any;
        goto NEXT;
    }
    case rage::Opcodes::STORE:
    {
        stackPtr -= 2;
        stackPtr[2].Reference->Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::STORE_REV:
    {
        --stackPtr;
        stackPtr[0].Reference->Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::LOAD_N:
    {
        rage::scrValue* data = (stackPtr--)->Reference;
        std::uint32_t itemCount = (stackPtr--)->Int;
        for (std::uint32_t i = 0; i < itemCount; i++)
            (++stackPtr)->Any = data[i].Any;
        goto NEXT;
    }
    case rage::Opcodes::STORE_N:
    {
        rage::scrValue* data = (stackPtr--)->Reference;
        std::uint32_t itemCount = (stackPtr--)->Int;
        for (std::uint32_t i = 0; i < itemCount; i++)
            data[itemCount - 1 - i].Any = (stackPtr--)->Any;
        goto NEXT;
    }
    case rage::Opcodes::ARRAY_U8:
    {
        --stackPtr;
        rage::scrValue* ref = stackPtr[1].Reference;
        std::uint32_t index = stackPtr[0].Uns;
        if (index >= ref->Uns)
            SCRIPT_FAILURE("Array access out of bounds!");
        ref += 1U + index * GET_BYTE;
        stackPtr[0].Reference = ref;
        goto NEXT;
    }
    case rage::Opcodes::ARRAY_U8_LOAD:
    {
        --stackPtr;
        rage::scrValue* ref = stackPtr[1].Reference;
        std::uint32_t index = stackPtr[0].Uns;
        if (index >= ref->Uns)
            SCRIPT_FAILURE("Array access out of bounds!");
        ref += 1U + index * GET_BYTE;
        stackPtr[0].Any = ref->Any;
        goto NEXT;
    }
    case rage::Opcodes::ARRAY_U8_STORE:
    {
        stackPtr -= 3;
        rage::scrValue* ref = stackPtr[3].Reference;
        std::uint32_t index = stackPtr[2].Uns;
        if (index >= ref->Uns)
            SCRIPT_FAILURE("Array access out of bounds!");
        ref += 1U + index * GET_BYTE;
        ref->Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::LOCAL_U8:
    {
        ++stackPtr;
        stackPtr[0].Reference = framePtr + GET_BYTE;
        goto NEXT;
    }
    case rage::Opcodes::LOCAL_U8_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = framePtr[GET_BYTE].Any;
        goto NEXT;
    }
    case rage::Opcodes::LOCAL_U8_STORE:
    {
        --stackPtr;
        framePtr[GET_BYTE].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U8:
    {
        ++stackPtr;
        stackPtr[0].Reference = stack + GET_BYTE;
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U8_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = stack[GET_BYTE].Any;
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U8_STORE:
    {
        --stackPtr;
        stack[GET_BYTE].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::IADD_U8:
    {
        stackPtr[0].Int += GET_BYTE;
        goto NEXT;
    }
    case rage::Opcodes::IMUL_U8:
    {
        stackPtr[0].Int *= GET_BYTE;
        goto NEXT;
    }
    case rage::Opcodes::IOFFSET:
    {
        --stackPtr;
        stackPtr[0].Any += stackPtr[1].Int * sizeof(rage::scrValue);
        goto NEXT;
    }
    case rage::Opcodes::IOFFSET_U8:
    {
        stackPtr[0].Any += GET_BYTE * sizeof(rage::scrValue);
        goto NEXT;
    }
    case rage::Opcodes::IOFFSET_U8_LOAD:
    {
        stackPtr[0].Any = stackPtr[0].Reference[GET_BYTE].Any;
        goto NEXT;
    }
    case rage::Opcodes::IOFFSET_U8_STORE:
    {
        stackPtr -= 2;
        stackPtr[2].Reference[GET_BYTE].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_S16:
    {
        ++stackPtr;
        stackPtr[0].Int = GET_SWORD;
        goto NEXT;
    }
    case rage::Opcodes::IADD_S16:
    {
        stackPtr[0].Int += GET_SWORD;
        goto NEXT;
    }
    case rage::Opcodes::IMUL_S16:
    {
        stackPtr[0].Int *= GET_SWORD;
        goto NEXT;
    }
    case rage::Opcodes::IOFFSET_S16:
    {
        stackPtr[0].Any += GET_SWORD * sizeof(rage::scrValue);
        goto NEXT;
    }
    case rage::Opcodes::IOFFSET_S16_LOAD:
    {
        stackPtr[0].Any = stackPtr[0].Reference[GET_SWORD].Any;
        goto NEXT;
    }
    case rage::Opcodes::IOFFSET_S16_STORE:
    {
        stackPtr -= 2;
        stackPtr[2].Reference[GET_SWORD].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::ARRAY_U16:
    {
        --stackPtr;
        rage::scrValue* ref = stackPtr[1].Reference;
        std::uint32_t index = stackPtr[0].Uns;
        if (index >= ref->Uns)
            SCRIPT_FAILURE("Array access out of bounds!");
        ref += 1U + index * GET_WORD;
        stackPtr[0].Reference = ref;
        goto NEXT;
    }
    case rage::Opcodes::ARRAY_U16_LOAD:
    {
        --stackPtr;
        rage::scrValue* ref = stackPtr[1].Reference;
        std::uint32_t index = stackPtr[0].Uns;
        if (index >= ref->Uns)
            SCRIPT_FAILURE("Array access out of bounds!");
        ref += 1U + index * GET_WORD;
        stackPtr[0].Any = ref->Any;
        goto NEXT;
    }
    case rage::Opcodes::ARRAY_U16_STORE:
    {
        stackPtr -= 3;
        rage::scrValue* ref = stackPtr[3].Reference;
        std::uint32_t index = stackPtr[2].Uns;
        if (index >= ref->Uns)
            SCRIPT_FAILURE("Array access out of bounds!");
        ref += 1U + index * GET_WORD;
        ref->Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::LOCAL_U16:
    {
        ++stackPtr;
        stackPtr[0].Reference = framePtr + GET_WORD;
        goto NEXT;
    }
    case rage::Opcodes::LOCAL_U16_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = framePtr[GET_WORD].Any;
        goto NEXT;
    }
    case rage::Opcodes::LOCAL_U16_STORE:
    {
        --stackPtr;
        framePtr[GET_WORD].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U16:
    {
        ++stackPtr;
        stackPtr[0].Reference = stack + GET_WORD;
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U16_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = stack[GET_WORD].Any;
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U16_STORE:
    {
        --stackPtr;
        stack[GET_WORD].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::GLOBAL_U16:
    {
        ++stackPtr;
        stackPtr[0].Reference = globals[0] + GET_WORD;
        goto NEXT;
    }
    case rage::Opcodes::GLOBAL_U16_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = globals[0][GET_WORD].Any;
        goto NEXT;
    }
    case rage::Opcodes::GLOBAL_U16_STORE:
    {
        --stackPtr;
        globals[0][GET_WORD].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::J:
    {
        std::int32_t ofs = GET_SWORD;
        JUMP(opcode - basePtr + ofs);
        goto NEXT;
    }
    case rage::Opcodes::JZ:
    {
        std::int32_t ofs = GET_SWORD;
        --stackPtr;
        if (stackPtr[1].Int == 0)
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::IEQ_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int == stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::INE_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int != stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::IGT_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int > stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::IGE_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int >= stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::ILT_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int < stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::ILE_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int <= stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::CALL:
    {
        std::uint32_t ofs = GET_24BIT;
        ++stackPtr;
        stackPtr[0].Uns = static_cast<std::int32_t>(opcode - basePtr);
        JUMP(ofs);
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U24:
    {
        ++stackPtr;
        stackPtr[0].Reference = stack + GET_24BIT;
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U24_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = stack[GET_24BIT].Any;
        goto NEXT;
    }
    case rage::Opcodes::STATIC_U24_STORE:
    {
        --stackPtr;
        stack[GET_24BIT].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::GLOBAL_U24:
    {
        std::uint32_t global = GET_24BIT;
        std::uint32_t block = global >> 0x12U;
        std::uint32_t index = global & 0x3FFFFU;
        ++stackPtr;
        if (!globals[block])
            SCRIPT_FAILURE("Global block is not loaded!");
        else
            stackPtr[0].Reference = &globals[block][index];
        goto NEXT;
    }
    case rage::Opcodes::GLOBAL_U24_LOAD:
    {
        std::uint32_t global = GET_24BIT;
        std::uint32_t block = global >> 0x12U;
        std::uint32_t index = global & 0x3FFFFU;
        ++stackPtr;
        if (!globals[block])
            SCRIPT_FAILURE("Global block is not loaded!");
        else
            stackPtr[0].Any = globals[block][index].Any;
        goto NEXT;
    }
    case rage::Opcodes::GLOBAL_U24_STORE:
    {
        std::uint32_t global = GET_24BIT;
        std::uint32_t block = global >> 0x12U;
        std::uint32_t index = global & 0x3FFFFU;
        --stackPtr;
        if (!globals[block])
            SCRIPT_FAILURE("Global block is not loaded!");
        else
            globals[block][index].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_U24:
    {
        ++stackPtr;
        stackPtr[0].Int = GET_24BIT;
        goto NEXT;
    }
    case rage::Opcodes::SWITCH:
    {
        --stackPtr;
        std::uint32_t switchVal = stackPtr[1].Uns;
        std::uint32_t caseCount = GET_BYTE;
        JUMP(opcode - basePtr);
        for (std::uint32_t i = 0; i < caseCount; i++)
        {
            std::uint32_t caseVal = GET_DWORD;
            std::uint32_t ofs = GET_WORD;
            if (switchVal == caseVal)
            {
                JUMP(opcode - basePtr + ofs);
                break;
            }
        }
        JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::Opcodes::STRING:
    {
        std::uint32_t ofs = stackPtr[0].Uns;
        stackPtr[0].String = program->m_Strings[ofs >> 14U] + (ofs & 0x3FFFU);
        goto NEXT;
    }
    case rage::Opcodes::STRINGHASH:
    {
        stackPtr[0].Uns = rage::Joaat(stackPtr[0].String);
        goto NEXT;
    }
    case rage::Opcodes::TEXT_LABEL_ASSIGN_STRING:
    {
        stackPtr -= 2;
        char* dest = const_cast<char*>(stackPtr[2].String);
        const char* src = stackPtr[1].String;
        AssignString(dest, GET_BYTE, src);
        goto NEXT;
    }
    case rage::Opcodes::TEXT_LABEL_ASSIGN_INT:
    {
        stackPtr -= 2;
        char* dest = const_cast<char*>(stackPtr[2].String);
        std::int32_t value = stackPtr[1].Int;
        Itoa(buffer, value);
        AssignString(dest, GET_BYTE, buffer);
        goto NEXT;
    }
    case rage::Opcodes::TEXT_LABEL_APPEND_STRING:
    {
        stackPtr -= 2;
        char* dest = const_cast<char*>(stackPtr[2].String);
        const char* src = stackPtr[1].String;
        AppendString(dest, GET_BYTE, src);
        goto NEXT;
    }
    case rage::Opcodes::TEXT_LABEL_APPEND_INT:
    {
        stackPtr -= 2;
        char* dest = const_cast<char*>(stackPtr[2].String);
        std::int32_t value = stackPtr[1].Int;
        Itoa(buffer, value);
        AppendString(dest, GET_BYTE, buffer);
        goto NEXT;
    }
    case rage::Opcodes::TEXT_LABEL_COPY:
    {
        stackPtr -= 3;
        rage::scrValue* dest = stackPtr[3].Reference;
        std::int32_t destSize = stackPtr[2].Int;
        std::int32_t srcSize = stackPtr[1].Int;
        if (srcSize > destSize)
        {
            std::int32_t excess = srcSize - destSize;
            stackPtr -= excess;
            srcSize = destSize;
        }
        rage::scrValue* destPtr = dest + srcSize - 1;
        for (std::int32_t i = 0; i < srcSize; i++)
            *destPtr-- = *stackPtr--;
        reinterpret_cast<char*>(dest)[srcSize * sizeof(rage::scrValue) - 1] = '\0';
        goto NEXT;
    }
    case rage::Opcodes::CATCH:
    {
        context->m_CatchProgramCounter = static_cast<std::int32_t>(opcode - basePtr);
        context->m_CatchFramePointer = static_cast<std::int32_t>(framePtr - stack);
        context->m_CatchStackPointer = static_cast<std::int32_t>(stackPtr - stack + 1);
        ++stackPtr;
        stackPtr[0].Int = -1;
        goto NEXT;
    }
    case rage::Opcodes::THROW:
    {
        std::int32_t ofs = stackPtr[0].Int;
        if (!context->m_CatchProgramCounter)
            SCRIPT_FAILURE("Catch PC is NULL!");
        else
        {
            JUMP(context->m_CatchProgramCounter);
            framePtr = stack + context->m_CatchFramePointer;
            stackPtr = stack + context->m_CatchStackPointer;
            stackPtr[0].Int = ofs;
        }
        goto NEXT;
    }
    case rage::Opcodes::CALLINDIRECT:
    {
        std::uint32_t ofs = stackPtr[0].Uns;
        if (!ofs)
            SCRIPT_FAILURE("Function pointer is NULL!");
        stackPtr[0].Uns = static_cast<std::int32_t>(opcode - basePtr);
        JUMP(ofs);
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_M1:
    {
        ++stackPtr;
        stackPtr[0].Int = -1;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_0:
    {
        ++stackPtr;
        stackPtr[0].Any = 0;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_1:
    {
        ++stackPtr;
        stackPtr[0].Int = 1;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_2:
    {
        ++stackPtr;
        stackPtr[0].Int = 2;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_3:
    {
        ++stackPtr;
        stackPtr[0].Int = 3;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_4:
    {
        ++stackPtr;
        stackPtr[0].Int = 4;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_5:
    {
        ++stackPtr;
        stackPtr[0].Int = 5;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_6:
    {
        ++stackPtr;
        stackPtr[0].Int = 6;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_7:
    {
        ++stackPtr;
        stackPtr[0].Int = 7;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_FM1:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0xBF800000;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_F0:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x00000000;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_F1:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x3F800000;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_F2:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40000000;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_F3:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40400000;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_F4:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40800000;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_F5:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40A00000;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_F6:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40C00000;
        goto NEXT;
    }
    case rage::Opcodes::PUSH_CONST_F7:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40E00000;
        goto NEXT;
    }
    case rage::Opcodes::IS_BIT_SET:
    {
        --stackPtr;
        stackPtr[0].Int = (stackPtr[0].Int & (1 << stackPtr[1].Int)) != 0;
        goto NEXT;
    }
    default:
    {
        SCRIPT_FAILURE("Unknown opcode!");
    }
    }

    // It should never reach here
    return rage::ThreadState::KILLED;
}