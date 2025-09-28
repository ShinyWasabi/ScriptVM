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
        return context->m_State = rage::scrThreadState::KILLED;                               \
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

rage::scrThreadState RunScriptThread(rage::scrValue* stack, rage::scrValue** globals, rage::scrProgram* program, rage::scrThreadContext* context)
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
    if (ScriptBreakpoint::OnBreakpoint(script, pc, context))
        return context->m_State = rage::scrThreadState::PAUSED;

    switch (GET_BYTE)
    {
    case rage::scrOpcode::NOP:
    {
        JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::scrOpcode::IADD:
    {
        --stackPtr;
        stackPtr[0].Int += stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::ISUB:
    {
        --stackPtr;
        stackPtr[0].Int -= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::IMUL:
    {
        --stackPtr;
        stackPtr[0].Int *= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::IDIV:
    {
        --stackPtr;
        if (stackPtr[1].Int)
            stackPtr[0].Int /= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::IMOD:
    {
        --stackPtr;
        if (stackPtr[1].Int)
            stackPtr[0].Int %= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::INOT:
    {
        stackPtr[0].Int = !stackPtr[0].Int;
        goto NEXT;
    }
    case rage::scrOpcode::INEG:
    {
        stackPtr[0].Int = -stackPtr[0].Int;
        goto NEXT;
    }
    case rage::scrOpcode::IEQ:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int == stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::INE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int != stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::IGT:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int > stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::IGE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int >= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::ILT:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int < stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::ILE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Int <= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::FADD:
    {
        --stackPtr;
        stackPtr[0].Float += stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FSUB:
    {
        --stackPtr;
        stackPtr[0].Float -= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FMUL:
    {
        --stackPtr;
        stackPtr[0].Float *= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FDIV:
    {
        --stackPtr;
        if (stackPtr[1].Int)
            stackPtr[0].Float /= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FMOD:
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
    case rage::scrOpcode::FNEG:
    {
        stackPtr[0].Uns ^= 0x80000000;
        goto NEXT;
    }
    case rage::scrOpcode::FEQ:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float == stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FNE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float != stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FGT:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float > stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FGE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float >= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FLT:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float < stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::FLE:
    {
        --stackPtr;
        stackPtr[0].Int = stackPtr[0].Float <= stackPtr[1].Float;
        goto NEXT;
    }
    case rage::scrOpcode::VADD:
    {
        stackPtr -= 3;
        stackPtr[-2].Float += stackPtr[1].Float;
        stackPtr[-1].Float += stackPtr[2].Float;
        stackPtr[0].Float += stackPtr[3].Float;
        goto NEXT;
    }
    case rage::scrOpcode::VSUB:
    {
        stackPtr -= 3;
        stackPtr[-2].Float -= stackPtr[1].Float;
        stackPtr[-1].Float -= stackPtr[2].Float;
        stackPtr[0].Float -= stackPtr[3].Float;
        goto NEXT;
    }
    case rage::scrOpcode::VMUL:
    {
        stackPtr -= 3;
        stackPtr[-2].Float *= stackPtr[1].Float;
        stackPtr[-1].Float *= stackPtr[2].Float;
        stackPtr[0].Float *= stackPtr[3].Float;
        goto NEXT;
    }
    case rage::scrOpcode::VDIV:
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
    case rage::scrOpcode::VNEG:
    {
        stackPtr[-2].Uns ^= 0x80000000;
        stackPtr[-1].Uns ^= 0x80000000;
        stackPtr[0].Uns ^= 0x80000000;
        goto NEXT;
    }
    case rage::scrOpcode::IAND:
    {
        --stackPtr;
        stackPtr[0].Int &= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::IOR:
    {
        --stackPtr;
        stackPtr[0].Int |= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::IXOR:
    {
        --stackPtr;
        stackPtr[0].Int ^= stackPtr[1].Int;
        goto NEXT;
    }
    case rage::scrOpcode::I2F:
    {
        stackPtr[0].Float = static_cast<float>(stackPtr[0].Int);
        goto NEXT;
    }
    case rage::scrOpcode::F2I:
    {
        stackPtr[0].Int = static_cast<std::int32_t>(stackPtr[0].Float);
        goto NEXT;
    }
    case rage::scrOpcode::F2V:
    {
        stackPtr += 2;
        stackPtr[-1].Int = stackPtr[0].Int = stackPtr[-2].Int;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_U8:
    {
        ++stackPtr;
        stackPtr[0].Int = GET_BYTE;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_U8_U8:
    {
        stackPtr += 2;
        stackPtr[-1].Int = GET_BYTE;
        stackPtr[0].Int = GET_BYTE;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_U8_U8_U8:
    {
        stackPtr += 3;
        stackPtr[-2].Int = GET_BYTE;
        stackPtr[-1].Int = GET_BYTE;
        stackPtr[0].Int = GET_BYTE;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_U32:
    case rage::scrOpcode::PUSH_CONST_F:
    {
        ++stackPtr;
        stackPtr[0].Uns = GET_DWORD;
        goto NEXT;
    }
    case rage::scrOpcode::DUP:
    {
        ++stackPtr;
        stackPtr[0].Any = stackPtr[-1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::DROP:
    {
        --stackPtr;
        goto NEXT;
    }
    case rage::scrOpcode::NATIVE:
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

        // In case WAIT, TERMINATE_THIS_THREAD, or TERMINATE_THREAD is called
        if (context->m_State != rage::scrThreadState::RUNNING)
            return context->m_State;

        ctx.FixVectors();

        stackPtr -= argCount - retCount;
        goto NEXT;
    }
    case rage::scrOpcode::ENTER:
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
    case rage::scrOpcode::LEAVE:
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
            return context->m_State = rage::scrThreadState::KILLED;

        goto NEXT;
    }
    case rage::scrOpcode::LOAD:
    {
        stackPtr[0].Any = stackPtr[0].Reference->Any;
        goto NEXT;
    }
    case rage::scrOpcode::STORE:
    {
        stackPtr -= 2;
        stackPtr[2].Reference->Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::STORE_REV:
    {
        --stackPtr;
        stackPtr[0].Reference->Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::LOAD_N:
    {
        rage::scrValue* data = (stackPtr--)->Reference;
        std::uint32_t itemCount = (stackPtr--)->Int;
        for (std::uint32_t i = 0; i < itemCount; i++)
            (++stackPtr)->Any = data[i].Any;
        goto NEXT;
    }
    case rage::scrOpcode::STORE_N:
    {
        rage::scrValue* data = (stackPtr--)->Reference;
        std::uint32_t itemCount = (stackPtr--)->Int;
        for (std::uint32_t i = 0; i < itemCount; i++)
            data[itemCount - 1 - i].Any = (stackPtr--)->Any;
        goto NEXT;
    }
    case rage::scrOpcode::ARRAY_U8:
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
    case rage::scrOpcode::ARRAY_U8_LOAD:
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
    case rage::scrOpcode::ARRAY_U8_STORE:
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
    case rage::scrOpcode::LOCAL_U8:
    {
        ++stackPtr;
        stackPtr[0].Reference = framePtr + GET_BYTE;
        goto NEXT;
    }
    case rage::scrOpcode::LOCAL_U8_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = framePtr[GET_BYTE].Any;
        goto NEXT;
    }
    case rage::scrOpcode::LOCAL_U8_STORE:
    {
        --stackPtr;
        framePtr[GET_BYTE].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U8:
    {
        ++stackPtr;
        stackPtr[0].Reference = stack + GET_BYTE;
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U8_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = stack[GET_BYTE].Any;
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U8_STORE:
    {
        --stackPtr;
        stack[GET_BYTE].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::IADD_U8:
    {
        stackPtr[0].Int += GET_BYTE;
        goto NEXT;
    }
    case rage::scrOpcode::IMUL_U8:
    {
        stackPtr[0].Int *= GET_BYTE;
        goto NEXT;
    }
    case rage::scrOpcode::IOFFSET:
    {
        --stackPtr;
        stackPtr[0].Any += stackPtr[1].Int * sizeof(rage::scrValue);
        goto NEXT;
    }
    case rage::scrOpcode::IOFFSET_U8:
    {
        stackPtr[0].Any += GET_BYTE * sizeof(rage::scrValue);
        goto NEXT;
    }
    case rage::scrOpcode::IOFFSET_U8_LOAD:
    {
        stackPtr[0].Any = stackPtr[0].Reference[GET_BYTE].Any;
        goto NEXT;
    }
    case rage::scrOpcode::IOFFSET_U8_STORE:
    {
        stackPtr -= 2;
        stackPtr[2].Reference[GET_BYTE].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_S16:
    {
        ++stackPtr;
        stackPtr[0].Int = GET_SWORD;
        goto NEXT;
    }
    case rage::scrOpcode::IADD_S16:
    {
        stackPtr[0].Int += GET_SWORD;
        goto NEXT;
    }
    case rage::scrOpcode::IMUL_S16:
    {
        stackPtr[0].Int *= GET_SWORD;
        goto NEXT;
    }
    case rage::scrOpcode::IOFFSET_S16:
    {
        stackPtr[0].Any += GET_SWORD * sizeof(rage::scrValue);
        goto NEXT;
    }
    case rage::scrOpcode::IOFFSET_S16_LOAD:
    {
        stackPtr[0].Any = stackPtr[0].Reference[GET_SWORD].Any;
        goto NEXT;
    }
    case rage::scrOpcode::IOFFSET_S16_STORE:
    {
        stackPtr -= 2;
        stackPtr[2].Reference[GET_SWORD].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::ARRAY_U16:
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
    case rage::scrOpcode::ARRAY_U16_LOAD:
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
    case rage::scrOpcode::ARRAY_U16_STORE:
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
    case rage::scrOpcode::LOCAL_U16:
    {
        ++stackPtr;
        stackPtr[0].Reference = framePtr + GET_WORD;
        goto NEXT;
    }
    case rage::scrOpcode::LOCAL_U16_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = framePtr[GET_WORD].Any;
        goto NEXT;
    }
    case rage::scrOpcode::LOCAL_U16_STORE:
    {
        --stackPtr;
        framePtr[GET_WORD].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U16:
    {
        ++stackPtr;
        stackPtr[0].Reference = stack + GET_WORD;
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U16_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = stack[GET_WORD].Any;
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U16_STORE:
    {
        --stackPtr;
        stack[GET_WORD].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::GLOBAL_U16:
    {
        ++stackPtr;
        stackPtr[0].Reference = globals[0] + GET_WORD;
        goto NEXT;
    }
    case rage::scrOpcode::GLOBAL_U16_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = globals[0][GET_WORD].Any;
        goto NEXT;
    }
    case rage::scrOpcode::GLOBAL_U16_STORE:
    {
        --stackPtr;
        globals[0][GET_WORD].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::J:
    {
        std::int32_t ofs = GET_SWORD;
        JUMP(opcode - basePtr + ofs);
        goto NEXT;
    }
    case rage::scrOpcode::JZ:
    {
        std::int32_t ofs = GET_SWORD;
        --stackPtr;
        if (stackPtr[1].Int == 0)
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::scrOpcode::IEQ_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int == stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::scrOpcode::INE_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int != stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::scrOpcode::IGT_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int > stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::scrOpcode::IGE_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int >= stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::scrOpcode::ILT_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int < stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::scrOpcode::ILE_JZ:
    {
        std::int32_t ofs = GET_SWORD;
        stackPtr -= 2;
        if (!(stackPtr[1].Int <= stackPtr[2].Int))
            JUMP(opcode - basePtr + ofs);
        else
            JUMP(opcode - basePtr);
        goto NEXT;
    }
    case rage::scrOpcode::CALL:
    {
        std::uint32_t ofs = GET_24BIT;
        ++stackPtr;
        stackPtr[0].Uns = static_cast<std::int32_t>(opcode - basePtr);
        JUMP(ofs);
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U24:
    {
        ++stackPtr;
        stackPtr[0].Reference = stack + GET_24BIT;
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U24_LOAD:
    {
        ++stackPtr;
        stackPtr[0].Any = stack[GET_24BIT].Any;
        goto NEXT;
    }
    case rage::scrOpcode::STATIC_U24_STORE:
    {
        --stackPtr;
        stack[GET_24BIT].Any = stackPtr[1].Any;
        goto NEXT;
    }
    case rage::scrOpcode::GLOBAL_U24:
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
    case rage::scrOpcode::GLOBAL_U24_LOAD:
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
    case rage::scrOpcode::GLOBAL_U24_STORE:
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
    case rage::scrOpcode::PUSH_CONST_U24:
    {
        ++stackPtr;
        stackPtr[0].Int = GET_24BIT;
        goto NEXT;
    }
    case rage::scrOpcode::SWITCH:
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
    case rage::scrOpcode::STRING:
    {
        std::uint32_t ofs = stackPtr[0].Uns;
        stackPtr[0].String = program->m_Strings[ofs >> 14U] + (ofs & 0x3FFFU);
        goto NEXT;
    }
    case rage::scrOpcode::STRINGHASH:
    {
        stackPtr[0].Uns = rage::Joaat(stackPtr[0].String);
        goto NEXT;
    }
    case rage::scrOpcode::TEXT_LABEL_ASSIGN_STRING:
    {
        stackPtr -= 2;
        char* dest = const_cast<char*>(stackPtr[2].String);
        const char* src = stackPtr[1].String;
        AssignString(dest, GET_BYTE, src);
        goto NEXT;
    }
    case rage::scrOpcode::TEXT_LABEL_ASSIGN_INT:
    {
        stackPtr -= 2;
        char* dest = const_cast<char*>(stackPtr[2].String);
        std::int32_t value = stackPtr[1].Int;
        Itoa(buffer, value);
        AssignString(dest, GET_BYTE, buffer);
        goto NEXT;
    }
    case rage::scrOpcode::TEXT_LABEL_APPEND_STRING:
    {
        stackPtr -= 2;
        char* dest = const_cast<char*>(stackPtr[2].String);
        const char* src = stackPtr[1].String;
        AppendString(dest, GET_BYTE, src);
        goto NEXT;
    }
    case rage::scrOpcode::TEXT_LABEL_APPEND_INT:
    {
        stackPtr -= 2;
        char* dest = const_cast<char*>(stackPtr[2].String);
        std::int32_t value = stackPtr[1].Int;
        Itoa(buffer, value);
        AppendString(dest, GET_BYTE, buffer);
        goto NEXT;
    }
    case rage::scrOpcode::TEXT_LABEL_COPY:
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
    case rage::scrOpcode::CATCH:
    {
        context->m_CatchProgramCounter = static_cast<std::int32_t>(opcode - basePtr);
        context->m_CatchFramePointer = static_cast<std::int32_t>(framePtr - stack);
        context->m_CatchStackPointer = static_cast<std::int32_t>(stackPtr - stack + 1);
        ++stackPtr;
        stackPtr[0].Int = -1;
        goto NEXT;
    }
    case rage::scrOpcode::THROW:
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
    case rage::scrOpcode::CALLINDIRECT:
    {
        std::uint32_t ofs = stackPtr[0].Uns;
        if (!ofs)
            SCRIPT_FAILURE("Function pointer is NULL!");
        stackPtr[0].Uns = static_cast<std::int32_t>(opcode - basePtr);
        JUMP(ofs);
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_M1:
    {
        ++stackPtr;
        stackPtr[0].Int = -1;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_0:
    {
        ++stackPtr;
        stackPtr[0].Any = 0;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_1:
    {
        ++stackPtr;
        stackPtr[0].Int = 1;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_2:
    {
        ++stackPtr;
        stackPtr[0].Int = 2;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_3:
    {
        ++stackPtr;
        stackPtr[0].Int = 3;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_4:
    {
        ++stackPtr;
        stackPtr[0].Int = 4;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_5:
    {
        ++stackPtr;
        stackPtr[0].Int = 5;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_6:
    {
        ++stackPtr;
        stackPtr[0].Int = 6;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_7:
    {
        ++stackPtr;
        stackPtr[0].Int = 7;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_FM1:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0xBF800000;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_F0:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x00000000;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_F1:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x3F800000;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_F2:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40000000;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_F3:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40400000;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_F4:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40800000;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_F5:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40A00000;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_F6:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40C00000;
        goto NEXT;
    }
    case rage::scrOpcode::PUSH_CONST_F7:
    {
        ++stackPtr;
        stackPtr[0].Uns = 0x40E00000;
        goto NEXT;
    }
    case rage::scrOpcode::IS_BIT_SET:
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
    return rage::scrThreadState::KILLED;
}