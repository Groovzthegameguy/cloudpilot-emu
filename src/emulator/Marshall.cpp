#include "EmPalmStructs.h"
#include "Marshal.h"

void Marshal::GetPointType(emuptr p, PointType& dest) {
    memset(&dest, 0, sizeof(PointType));

    if (p) {
        EmAliasPointType<PAS> src(p);

        dest.x = src.x;
        dest.y = src.y;
    }
}

void Marshal::PutPointType(emuptr p, const PointType& src) {
    if (p) {
        EmAliasPointType<PAS> dest(p);

        dest.x = src.x;
        dest.y = src.y;
    }
}

void Marshal::GetSysKernelInfoType(emuptr p, SysKernelInfoType& dest) {
    memset(&dest, 0, sizeof(SysKernelInfoType));

    if (p) {
        EmAliasSysKernelInfoType<PAS> src(p);

        dest.selector = src.selector;
        dest.reserved = src.reserved;
        dest.id = src.id;

        switch (dest.selector) {
            case sysKernelInfoSelCurTaskInfo:
            case sysKernelInfoSelTaskInfo:
                dest.param.task.id = src.task.id;
                dest.param.task.nextID = src.task.nextID;
                dest.param.task.tag = src.task.tag;
                dest.param.task.status = src.task.status;
                dest.param.task.timer = src.task.timer;
                dest.param.task.timeSlice = src.task.timeSlice;
                dest.param.task.priority = src.task.priority;
                dest.param.task.attributes = src.task.attributes;
                dest.param.task.pendingCalls = src.task.pendingCalls;
                dest.param.task.senderTaskID = src.task.senderTaskID;
                dest.param.task.msgExchangeID = src.task.msgExchangeID;
                dest.param.task.tcbP = src.task.tcbP;
                dest.param.task.stackP = src.task.stackP;
                dest.param.task.stackStart = src.task.stackStart;
                dest.param.task.stackSize = src.task.stackSize;
                break;

            case sysKernelInfoSelSemaphoreInfo:
                dest.param.semaphore.id = src.semaphore.id;
                dest.param.semaphore.nextID = src.semaphore.nextID;
                dest.param.semaphore.tag = src.semaphore.tag;
                dest.param.semaphore.initValue = src.semaphore.initValue;
                dest.param.semaphore.curValue = src.semaphore.curValue;
                dest.param.semaphore.nestLevel = src.semaphore.nestLevel;
                dest.param.semaphore.ownerID = src.semaphore.ownerID;
                break;

            case sysKernelInfoSelTimerInfo:
                dest.param.timer.id = src.timer.id;
                dest.param.timer.nextID = src.timer.nextID;
                dest.param.timer.tag = src.timer.tag;
                dest.param.timer.ticksLeft = src.timer.ticksLeft;
                dest.param.timer.period = src.timer.period;
                dest.param.timer.proc = src.timer.proc;
                break;

            default:
                EmAssert(false);
        }
    }
}

void Marshal::PutSysKernelInfoType(emuptr p, const SysKernelInfoType& src) {
    if (p) {
        EmAliasSysKernelInfoType<PAS> dest(p);

        dest.selector = src.selector;
        dest.reserved = src.reserved;
        dest.id = src.id;

        switch (dest.selector) {
            case sysKernelInfoSelCurTaskInfo:
            case sysKernelInfoSelTaskInfo:
                dest.task.id = src.param.task.id;
                dest.task.nextID = src.param.task.nextID;
                dest.task.tag = src.param.task.tag;
                dest.task.status = src.param.task.status;
                dest.task.timer = src.param.task.timer;
                dest.task.timeSlice = src.param.task.timeSlice;
                dest.task.priority = src.param.task.priority;
                dest.task.attributes = src.param.task.attributes;
                dest.task.pendingCalls = src.param.task.pendingCalls;
                dest.task.senderTaskID = src.param.task.senderTaskID;
                dest.task.msgExchangeID = src.param.task.msgExchangeID;
                dest.task.tcbP = src.param.task.tcbP;
                dest.task.stackP = src.param.task.stackP;
                dest.task.stackStart = src.param.task.stackStart;
                dest.task.stackSize = src.param.task.stackSize;
                break;

            case sysKernelInfoSelSemaphoreInfo:
                dest.semaphore.id = src.param.semaphore.id;
                dest.semaphore.nextID = src.param.semaphore.nextID;
                dest.semaphore.tag = src.param.semaphore.tag;
                dest.semaphore.initValue = src.param.semaphore.initValue;
                dest.semaphore.curValue = src.param.semaphore.curValue;
                dest.semaphore.nestLevel = src.param.semaphore.nestLevel;
                dest.semaphore.ownerID = src.param.semaphore.ownerID;
                break;

            case sysKernelInfoSelTimerInfo:
                dest.timer.id = src.param.timer.id;
                dest.timer.nextID = src.param.timer.nextID;
                dest.timer.tag = src.param.timer.tag;
                dest.timer.ticksLeft = src.param.timer.ticksLeft;
                dest.timer.period = src.param.timer.period;
                dest.timer.proc = src.param.timer.proc;
                break;

            default:
                EmAssert(false);
        }
    }
}

#if 0
void Marshal::GetDlkServerSessionType(emuptr p, DlkServerSessionType& dest) {
    memset(&dest, 0, sizeof(DlkServerSessionType));

    if (p) {
        EmAliasDlkServerSessionType<PAS> src(p);

        dest.htalLibRefNum = src.htalLibRefNum;
        dest.maxHtalXferSize = src.maxHtalXferSize;
        dest.eventProcP = (DlkEventProcPtr)(emuptr)src.eventProcP;
        dest.eventRef = src.eventRef;
        dest.canProcP = (DlkUserCanProcPtr)(emuptr)src.canProcP;
        dest.canRef = src.canRef;
        dest.condFilterH = (MemHandle)(emuptr)src.condFilterH;
        dest.dlkDBID = src.dlkDBID;
        dest.reserved1 = src.reserved1;
        dest.dbR = (DmOpenRef)(emuptr)src.dbR;
        dest.cardNo = src.cardNo;
        dest.dbCreator = src.dbCreator;
        //	dest.dbName				= src.dbName;
        dest.dbOpenMode = src.dbOpenMode;
        dest.created = src.created;
        dest.isResDB = src.isResDB;
        dest.ramBased = src.ramBased;
        dest.readOnly = src.readOnly;
        dest.dbLocalID = src.dbLocalID;
        dest.initialModNum = src.initialModNum;
        dest.curRecIndex = src.curRecIndex;
        //	dest.creatorList		= src.creatorList;
        dest.syncState = src.syncState;
        dest.complete = src.complete;
        dest.conduitOpened = src.conduitOpened;
        dest.logCleared = src.logCleared;
        dest.resetPending = src.resetPending;
        dest.gotCommand = src.gotCommand;
        dest.cmdTID = src.cmdTID;
        dest.reserved2 = src.reserved2;
        dest.cmdLen = src.cmdLen;
        dest.cmdP = (void*)(emuptr)src.cmdP;
        dest.cmdH = (MemHandle)(emuptr)src.cmdH;
        dest.wStateFlags = src.wStateFlags;
        //	dest.dbSearchState		= src.dbSearchState;
    }
}
#endif

void Marshal::PutDlkServerSessionType(emuptr p, const DlkServerSessionType& src) {
    if (p) {
        EmAliasDlkServerSessionType<PAS> dest(p);

        dest.htalLibRefNum = src.htalLibRefNum;
        dest.maxHtalXferSize = src.maxHtalXferSize;
        dest.eventProcP = (emuptr)(long)src.eventProcP;
        dest.eventRef = src.eventRef;
        dest.canProcP = (emuptr)(long)src.canProcP;
        dest.canRef = src.canRef;
        dest.condFilterH = (emuptr)(long)src.condFilterH;
        dest.dlkDBID = src.dlkDBID;
        dest.reserved1 = src.reserved1;
        dest.dbR = (emuptr)(long)src.dbR;
        dest.cardNo = src.cardNo;
        dest.dbCreator = src.dbCreator;
        //	dest.dbName				= src.dbName;
        dest.dbOpenMode = src.dbOpenMode;
        dest.created = src.created;
        dest.isResDB = src.isResDB;
        dest.ramBased = src.ramBased;
        dest.readOnly = src.readOnly;
        dest.dbLocalID = src.dbLocalID;
        dest.initialModNum = src.initialModNum;
        dest.curRecIndex = src.curRecIndex;
        //	dest.creatorList		= src.creatorList;
        dest.syncState = src.syncState;
        dest.complete = src.complete;
        dest.conduitOpened = src.conduitOpened;
        dest.logCleared = src.logCleared;
        dest.resetPending = src.resetPending;
        dest.gotCommand = src.gotCommand;
        dest.cmdTID = src.cmdTID;
        dest.reserved2 = src.reserved2;
        dest.cmdLen = src.cmdLen;
        dest.cmdP = (emuptr)(long)src.cmdP;
        dest.cmdH = (emuptr)(long)src.cmdH;
        dest.wStateFlags = src.wStateFlags;
        //	dest.dbSearchState		= src.dbSearchState;
    }
}
