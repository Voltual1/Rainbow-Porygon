#include <setjmp.h>
#include "global.h"
#include "script.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "mystery_gift.h"
#include "random.h"
#include "task.h"
#include "trainer_see.h"
#include "util.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/map_scripts.h"
#include "constants/script_commands.h"
#include "field_message_box.h"

#include "dexnav.h"

#define RAM_SCRIPT_MAGIC 51

enum {
    SCRIPT_MODE_STOPPED,
    SCRIPT_MODE_BYTECODE,
    SCRIPT_MODE_NATIVE,
};

enum {
    CONTEXT_RUNNING,
    CONTEXT_WAITING,
    CONTEXT_SHUTDOWN,
};

extern const u8 *gRamScriptRetAddr;

static u8 sGlobalScriptContextStatus;
static struct ScriptContext sGlobalScriptContext;
static struct ScriptContext sImmediateScriptContext;
static bool8 sLockFieldControls;
EWRAM_DATA u8 gMsgIsSignPost = FALSE;
EWRAM_DATA u8 gMsgBoxIsCancelable = FALSE;

extern ScrCmdFunc gScriptCmdTable[];
extern ScrCmdFunc gScriptCmdTableEnd[];

void InitScriptContext(struct ScriptContext *ctx, void *cmdTable, void *cmdTableEnd)
{
    s32 i;

    ctx->mode = SCRIPT_MODE_STOPPED;
    ctx->scriptPtr = NULL;
    ctx->stackDepth = 0;
    ctx->nativePtr = NULL;
    ctx->cmdTable = cmdTable;
    ctx->cmdTableEnd = cmdTableEnd;

    for (i = 0; i < (int)ARRAY_COUNT(ctx->data); i++)
        ctx->data[i] = 0;

    for (i = 0; i < (int)ARRAY_COUNT(ctx->stack); i++)
        ctx->stack[i] = NULL;

    ctx->breakOnTrainerBattle = FALSE;
}

u8 SetupBytecodeScript(struct ScriptContext *ctx, const u8 *ptr)
{
    ctx->scriptPtr = ptr;
    ctx->mode = SCRIPT_MODE_BYTECODE;
    return 1;
}

void SetupNativeScript(struct ScriptContext *ctx, bool8 (*ptr)(void))
{
    ctx->mode = SCRIPT_MODE_NATIVE;
    ctx->nativePtr = ptr;
}

void StopScript(struct ScriptContext *ctx)
{
    assertf(!FuncIsActiveTask(Task_WarpAndLoadMap), "Leaving script while a warp is in progress: try adding a waitstate");
    ctx->mode = SCRIPT_MODE_STOPPED;
    ctx->scriptPtr = NULL;
}

bool8 RunScriptCommand(struct ScriptContext *ctx)
{
    switch (ctx->mode)
    {
    case SCRIPT_MODE_STOPPED:
        return FALSE;
    case SCRIPT_MODE_NATIVE:
        if (ctx->nativePtr)
        {
            bool8 (*nativeFunc)(void) = (bool8 (*)(void))STRIP_DOMIRROR_TAG(ctx->nativePtr);
            if (nativeFunc() == TRUE)
                ctx->mode = SCRIPT_MODE_BYTECODE;
            return TRUE;
        }
        ctx->mode = SCRIPT_MODE_BYTECODE;
        // fallthrough
    case SCRIPT_MODE_BYTECODE:
        while (1)
        {
            u8 cmdCode;
            ScrCmdFunc *func;

            if (ctx->scriptPtr == NULL)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }

            cmdCode = *(ctx->scriptPtr);
            ctx->scriptPtr++;
            func = &ctx->cmdTable[cmdCode];

            if (func >= ctx->cmdTableEnd)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }

            ScrCmdFunc cmdFunc = (ScrCmdFunc)STRIP_DOMIRROR_TAG(*func);
            if (cmdFunc(ctx) == TRUE)
            {
                return TRUE;
            }
        }
    }

    return TRUE;
}

static bool8 ScriptPush(struct ScriptContext *ctx, const u8 *ptr)
{
    if (ctx->stackDepth + 1 >= (int)ARRAY_COUNT(ctx->stack))
    {
        return TRUE;
    }
    else
    {
        ctx->stack[ctx->stackDepth] = ptr;
        ctx->stackDepth++;
        return FALSE;
    }
}

static const u8 *ScriptPop(struct ScriptContext *ctx)
{
    if (ctx->stackDepth == 0)
        return NULL;

    ctx->stackDepth--;
    return ctx->stack[ctx->stackDepth];
}

void ScriptJump(struct ScriptContext *ctx, const u8 *ptr)
{
    assertf(ptr != NULL, "goto to NULL");
    ctx->scriptPtr = ptr;
}

void ScriptCall(struct ScriptContext *ctx, const u8 *ptr)
{
    assertf(ptr != NULL, "call to NULL")
    {
        return;
    }

    bool32 failed = ScriptPush(ctx, ctx->scriptPtr);
    assertf(!failed, "could not push %p", ptr)
    {
        return;
    }

    ctx->scriptPtr = ptr;
}

void ScriptReturn(struct ScriptContext *ctx)
{
    ctx->scriptPtr = ScriptPop(ctx);
}

u16 ScriptReadHalfword(struct ScriptContext *ctx)
{
    u16 value = *(ctx->scriptPtr++);
    value |= *(ctx->scriptPtr++) << 8;
    return value;
}

u16 ScriptPeekHalfword(struct ScriptContext *ctx)
{
    u16 value = *(ctx->scriptPtr);
    value |= *(ctx->scriptPtr + 1) << 8;
    return value;
}

u32 ScriptReadWord(struct ScriptContext *ctx)
{
    u32 value0 = *(ctx->scriptPtr++);
    u32 value1 = *(ctx->scriptPtr++);
    u32 value2 = *(ctx->scriptPtr++);
    u32 value3 = *(ctx->scriptPtr++);
    return (((((value3 << 8) + value2) << 8) + value1) << 8) + value0;
}

u32 ScriptPeekWord(struct ScriptContext *ctx)
{
    u32 value0 = *(ctx->scriptPtr);
    u32 value1 = *(ctx->scriptPtr + 1);
    u32 value2 = *(ctx->scriptPtr + 2);
    u32 value3 = *(ctx->scriptPtr + 3);
    return (((((value3 << 8) + value2) << 8) + value1) << 8) + value0;
}

void LockPlayerFieldControls(void)
{
    sLockFieldControls = TRUE;
    EndDexNavSearch();
}

void UnlockPlayerFieldControls(void)
{
    sLockFieldControls = FALSE;
}

bool8 ArePlayerFieldControlsLocked(void)
{
    return sLockFieldControls;
}

bool8 ScriptContext_IsEnabled(void)
{
    if (sGlobalScriptContextStatus == CONTEXT_RUNNING)
        return TRUE;
    else
        return FALSE;
}

void ScriptContext_Init(void)
{
    InitScriptContext(&sGlobalScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    sGlobalScriptContextStatus = CONTEXT_SHUTDOWN;
}

bool8 ScriptContext_RunScript(void)
{
    if (sGlobalScriptContextStatus == CONTEXT_SHUTDOWN)
        return FALSE;

    if (sGlobalScriptContextStatus == CONTEXT_WAITING)
        return FALSE;

    LockPlayerFieldControls();

    if (!RunScriptCommand(&sGlobalScriptContext))
    {
        sGlobalScriptContextStatus = CONTEXT_SHUTDOWN;
        UnlockPlayerFieldControls();
        return FALSE;
    }

    return TRUE;
}

void ScriptContext_SetupScript(const u8 *ptr)
{
    InitScriptContext(&sGlobalScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    SetupBytecodeScript(&sGlobalScriptContext, ptr);
    LockPlayerFieldControls();
    if (OW_FOLLOWERS_SCRIPT_MOVEMENT)
        FlagSet(FLAG_SAFE_FOLLOWER_MOVEMENT);
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
}

void ScriptContext_ContinueScript(struct ScriptContext *ctx)
{
    sGlobalScriptContext = *ctx;
    LockPlayerFieldControls();
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
}

void ScriptContext_Stop(void)
{
    sGlobalScriptContextStatus = CONTEXT_WAITING;
}

void ScriptContext_Enable(void)
{
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
    LockPlayerFieldControls();
}

void RunScriptImmediately(const u8 *ptr)
{
    InitScriptContext(&sImmediateScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    SetupBytecodeScript(&sImmediateScriptContext, ptr);

    int loopCount = 0;
    while (RunScriptCommand(&sImmediateScriptContext) == TRUE)
    {
        if (++loopCount > 5000)
            break;   // 防止死循环卡死创建新游戏等流程
    }
}

const u8 *MapHeaderGetScriptTable(u8 tag)
{
    const u8 *mapScripts = gMapHeader.mapScripts;

    if (!mapScripts)
        return NULL;

    while (1)
    {
        if (!*mapScripts)
            return NULL;
        if (*mapScripts == tag)
        {
            mapScripts++;
            return T2_READ_PTR(mapScripts);
        }
        mapScripts += 5;
    }
}

void MapHeaderRunScriptType(u8 tag)
{
    const u8 *ptr = MapHeaderGetScriptTable(tag);
    if (ptr)
        RunScriptImmediately(ptr);
}

const u8 *MapHeaderCheckScriptTable(u8 tag)
{
    const u8 *ptr = MapHeaderGetScriptTable(tag);

    if (!ptr)
        return NULL;

    while (1)
    {
        u16 varIndex1;
        u16 varIndex2;

        varIndex1 = T1_READ_16(ptr);
        if (!varIndex1)
            return NULL;
        ptr += 2;

        varIndex2 = T1_READ_16(ptr);
        ptr += 2;

        if (VarGet(varIndex1) == VarGet(varIndex2))
        {
            const u8 *mapScript = T2_READ_PTR(ptr);
            if (!Script_HasNoEffect(mapScript))
                return mapScript;
        }

        ptr += 4;
    }
}

void RunOnLoadMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_LOAD);
}

void RunOnTransitionMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_TRANSITION);
}

void RunOnResumeMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_RESUME);
}

void RunOnReturnToFieldMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_RETURN_TO_FIELD);
}

void RunOnDiveWarpMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_DIVE_WARP);
}

bool8 TryRunOnFrameMapScript(void)
{
    const u8 *ptr = MapHeaderCheckScriptTable(MAP_SCRIPT_ON_FRAME_TABLE);

    if (!ptr)
        return FALSE;

    ScriptContext_SetupScript(ptr);
    return TRUE;
}

void TryRunOnWarpIntoMapScript(void)
{
    const u8 *ptr = MapHeaderCheckScriptTable(MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE);
    if (ptr)
        RunScriptImmediately(ptr);
}

u32 CalculateRamScriptChecksum(void)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    return CalcCRC16WithTable((u8 *)(&gSaveBlock1Ptr->ramScript.data), sizeof(gSaveBlock1Ptr->ramScript.data));
#else
    return 0;
#endif
}

void ClearRamScript(void)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    CpuFill32(0, &gSaveBlock1Ptr->ramScript, sizeof(struct RamScript));
#endif
}

bool8 InitRamScript(const u8 *script, u16 scriptSize, u8 mapGroup, u8 mapNum, u8 localId)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;

    ClearRamScript();

    if (scriptSize > sizeof(scriptData->script))
        return FALSE;

    scriptData->magic = RAM_SCRIPT_MAGIC;
    scriptData->mapGroup = mapGroup;
    scriptData->mapNum = mapNum;
    scriptData->localId = localId;
    memcpy(scriptData->script, script, scriptSize);
    gSaveBlock1Ptr->ramScript.checksum = CalculateRamScriptChecksum();
    return TRUE;
#else
    return FALSE;
#endif
}

const u8 *GetRamScript(u8 localId, const u8 *script)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    gRamScriptRetAddr = NULL;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return script;
    if (scriptData->mapGroup != gSaveBlock1Ptr->location.mapGroup)
        return script;
    if (scriptData->mapNum != gSaveBlock1Ptr->location.mapNum)
        return script;
    if (scriptData->localId != localId)
        return script;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
    {
        ClearRamScript();
        return script;
    }
    else
    {
        gRamScriptRetAddr = script;
        return scriptData->script;
    }
#else
    return script;
#endif
}

#define NO_OBJECT LOCALID_PLAYER

bool32 ValidateSavedRamScript(void)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return FALSE;
    if (scriptData->mapGroup != MAP_GROUP(MAP_UNDEFINED))
        return FALSE;
    if (scriptData->mapNum != MAP_NUM(MAP_UNDEFINED))
        return FALSE;
    if (scriptData->localId != NO_OBJECT)
        return FALSE;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
        return FALSE;
    return TRUE;
#else
    return FALSE;
#endif
}

u8 *GetSavedRamScriptIfValid(void)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    if (!ValidateSavedWonderCard())
        return NULL;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return NULL;
    if (scriptData->mapGroup != MAP_GROUP(MAP_UNDEFINED))
        return NULL;
    if (scriptData->mapNum != MAP_NUM(MAP_UNDEFINED))
        return NULL;
    if (scriptData->localId != NO_OBJECT)
        return NULL;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
    {
        ClearRamScript();
        return NULL;
    }
    else
    {
        return scriptData->script;
    }
#else
    return NULL;
#endif
}

void InitRamScript_NoObjectEvent(u8 *script, u16 scriptSize)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    if (scriptSize > sizeof(gSaveBlock1Ptr->ramScript.data.script))
        scriptSize = sizeof(gSaveBlock1Ptr->ramScript.data.script);
    InitRamScript(script, scriptSize, MAP_GROUP(MAP_UNDEFINED), MAP_NUM(MAP_UNDEFINED), NO_OBJECT);
#endif
}

bool8 LoadTrainerObjectScript(void)
{
    sGlobalScriptContext.scriptPtr = gApproachingTrainers[gNoOfApproachingTrainers - 1].trainerScriptPtr;
    return TRUE;
}

struct ScriptEffectContext {
    u32 breakOn;
    jmp_buf breakTo;
    const u8 *nextCmd;
};

struct ScriptEffectContext *gScriptEffectContext = NULL;

static bool32 Script_IsEffectInstrumentedCommand(ScrCmdFunc func)
{
    return IsEffectInstrumented((void*)func);
}

static bool32 RunScriptImmediatelyUntilEffect_InternalLoop(struct ScriptContext *ctx)
{
    if (setjmp(gScriptEffectContext->breakTo) == 0)
    {
        while (TRUE)
        {
            u32 cmdCode;
            ScrCmdFunc *func;

            gScriptEffectContext->nextCmd = ctx->scriptPtr;

            if (!ctx->scriptPtr)
                return FALSE;

            cmdCode = *ctx->scriptPtr;
            ctx->scriptPtr++;
            func = &ctx->cmdTable[cmdCode];

            if (func >= ctx->cmdTableEnd)
                return TRUE;

            if (!Script_IsEffectInstrumentedCommand(*func))
                return TRUE;

            ScrCmdFunc cmdFunc = (ScrCmdFunc)STRIP_DOMIRROR_TAG(*func);
            if (cmdFunc(ctx))
            {
                gScriptEffectContext->nextCmd = ctx->scriptPtr;
                return TRUE;
            }
        }
    }
    else
    {
        return TRUE;
    }
}

void Script_GotoBreak_Internal(void)
{
    longjmp(gScriptEffectContext->breakTo, 1);
}

bool32 RunScriptImmediatelyUntilEffect_Internal(u32 effects, const u8 *ptr, struct ScriptContext *ctx)
{
    bool32 result;
    struct ScriptEffectContext seCtx;
    seCtx.breakOn = effects & 0x7FFFFFFF;

    if (ctx == NULL)
        ctx = &sImmediateScriptContext;

    InitScriptContext(ctx, gScriptCmdTable, gScriptCmdTableEnd);
    if (effects & SCREFF_TRAINERBATTLE)
        ctx->breakOnTrainerBattle = TRUE;
    SetupBytecodeScript(ctx, ptr);

    rng_value_t rngValue = gRngValue;
    gScriptEffectContext = &seCtx;
    result = RunScriptImmediatelyUntilEffect_InternalLoop(ctx);
    gScriptEffectContext = NULL;
    gRngValue = rngValue;

    if (result)
        ctx->scriptPtr = seCtx.nextCmd;

    return result;
}

bool32 Script_HasNoEffect(const u8 *ptr)
{
    return !RunScriptImmediatelyUntilEffect(SCREFF_V1 | SCREFF_SAVE | SCREFF_HARDWARE, ptr, NULL);
}

void Script_RequestEffects_Internal(u32 effects)
{
    if (gScriptEffectContext->breakOn & effects)
        longjmp(gScriptEffectContext->breakTo, 1);
}

void Script_RequestWriteVar_Internal(u32 varId)
{
    if (varId == 0)
        return;
    if (SPECIAL_VARS_START <= varId && varId <= SPECIAL_VARS_END)
        return;
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
}

bool32 Script_MatchesCallNative(const u8 *script, void *funcPtr, bool32 requestEffects)
{
    if (script[0] != SCR_OP_CALLNATIVE)
        return FALSE;
    u32 callnativeFunc = (((((script[4] << 8) + script[3]) << 8) + script[2]) << 8) + script[1];
    uintptr_t targetFunc = (uintptr_t)funcPtr;
    if (requestEffects) {
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
        targetFunc += 0x02000000;
#else
        targetFunc |= 0xA000000;
#endif
    }
    if ((uintptr_t)callnativeFunc == targetFunc)
        return TRUE;
    return FALSE;
}

bool32 Script_MatchesSpecial(const u8 *script, void *funcPtr)
{
    if (script[0] != SCR_OP_SPECIAL)
        return FALSE;
    typedef u16 (*SpecialFunc)(void);
    extern const SpecialFunc gSpecials[];
    SpecialFunc specialFunc = gSpecials[(script[2] << 8) + script[1]];
    if ((uintptr_t)specialFunc == ((uintptr_t)funcPtr))
        return TRUE;
    return FALSE;
}

void DisableMsgBoxWalkaway(void)
{
    // sMsgBoxWalkawayDisabled = TRUE;
}

void SetWalkingIntoSignVars(void)
{
    // gWalkAwayFromSignInhibitTimer = 6;
    // sMsgBoxIsCancelable = TRUE;
}
