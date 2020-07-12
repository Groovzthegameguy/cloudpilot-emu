#include "Miscellaneous.h"

#include "Byteswapping.h"
#include "EmBankMapped.h"
#include "EmLowMem.h"
#include "EmMemory.h"
#include "EmPalmFunction.h"
#include "EmPalmStructs.h"
#include "EmPatchModuleHtal.h"
#include "Logging.h"
#include "ROMStubs.h"
#include "UAE.h"

#define LOGGING 1
#ifdef LOGGING
    #define PRINTF log::printf
#else
    #define PRINTF(...) ;
#endif

uint32 NextPowerOf2(uint32 n) {
    // Smear down the upper 1 bit to all bits lower than it.

    uint32 n2 = n;

    n2 |= n2 >> 1;
    n2 |= n2 >> 2;
    n2 |= n2 >> 4;
    n2 |= n2 >> 8;
    n2 |= n2 >> 16;

    // Now use itself to clear all the lower bits.

    n2 &= ~(n2 >> 1);

    // If n2 ends up being the same as what we started with, keep it.
    // Otherwise, we need to bump it by a factor of two (round up).

    if (n2 != n) n2 <<= 1;

    return n2;
}

/***********************************************************************
 *
 * FUNCTION:	GetSystemCallContext
 *
 * DESCRIPTION: .
 *
 * PARAMETERS:	None.
 *
 * RETURNED:	Nothing.
 *
 ***********************************************************************/

Bool GetSystemCallContext(emuptr pc, SystemCallContext& context) {
    context.fPC = pc;

    // Determine how the system function is being called.  There are two ways:
    //
    //		* Via SYS_TRAP macro:
    //
    //			TRAP	$F
    //			DC.W	$Axxx
    //
    //		* Via SYS_TRAP_FAST macro:
    //
    //			MOVE.L	struct(LowMemType.fixed.globals.sysDispatchTableP), A1
    //			MOVE.L	((trapNum-sysTrapBase)*4)(A1), A1
    //			JSR 	(A1)	; opcode == 0x4e91
    //
    // The PC is current pointing to either the TRAP $F or the JSR (A1),
    // so we can look at the opcode to determine how we got here.

    uint8* realMem = EmMemGetRealAddress(pc);
    uint16 opcode = EmMemDoGet16(realMem);

    context.fViaTrap = opcode == (m68kTrapInstr + sysDispatchTrapNum);
    context.fViaJsrA1 = opcode == (0x4e91);

    if (context.fViaTrap) {
        // Not all development systems generate the correct dispatch
        // numbers; some leave off the preceding "A".  Make sure it's
        // set so that we can recognize it as a trap dispatch number.
        // (This code is here specifically so that the profiling routines
        //	will work, which check for trap numbers masquerading as function
        //	addresses by checking to see if they are in the sysTrapBase range.)

        context.fTrapWord = EmMemGet16(pc + 2) | sysTrapBase;
        context.fNextPC = pc + 4;
    } else if (context.fViaJsrA1) {
        context.fTrapWord = (EmMemGet16(pc - 2) / 4) | sysTrapBase;
        context.fNextPC = pc + 2;
    } else {
        EmAssert(false);
        return false;
    }

    if (::IsSystemTrap(context.fTrapWord)) {
        context.fTrapIndex = SysTrapIndex(context.fTrapWord);
        context.fExtra = m68k_dreg(regs, 2);
    } else {
        context.fTrapIndex = LibTrapIndex(context.fTrapWord);
        context.fExtra = EmMemGet16(m68k_areg(regs, 7));
    }

    EmAssert((context.fTrapWord >= sysTrapBase) && (context.fTrapWord < sysTrapBase + 0x1000));

    return true;
}

/***********************************************************************
 *
 * FUNCTION:    SeparateList
 *
 * DESCRIPTION: Break up a comma-delimited list of items, returning the
 *				pieces in a StringList.
 *
 * PARAMETERS:  stringList - the StringList to receive the broken-up
 *					pieces of the comma-delimited list.  This collection
 *					is *not* first cleared out, so it's possible to add
 *					to the collection with this function.
 *
 *				str - the string containing the comma-delimited items.
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

void SeparateList(StringList& stringList, string str, char delimiter) {
    string::size_type offset;

    while ((offset = str.find(delimiter)) != string::npos) {
        string nextElement = str.substr(0, offset);
        str = str.substr(offset + 1);
        stringList.push_back(nextElement);
    }

    stringList.push_back(str);
}

/***********************************************************************
 *
 * FUNCTION:	EndsWith
 *
 * DESCRIPTION:	Determine if a string end with the given pattern.
 *
 * PARAMETERS:	s - string to test.
 *
 *				p - pattern to test with.
 *
 * RETURNED:	True if "s" ends with "p".
 *
 ***********************************************************************/

Bool EndsWith(const char* s, const char* p) {
    if (strlen(s) < strlen(p)) return false;

    const char* buffer = s + strlen(s) - strlen(p);
    return (strcasecmp(buffer, p) == 0);
}

StMemoryMapper::StMemoryMapper(const void* memory, long size) : fMemory(memory) {
    if (fMemory) Memory::MapPhysicalMemory(fMemory, size);
}

StMemoryMapper::~StMemoryMapper(void) {
    if (fMemory) Memory::UnmapPhysicalMemory(fMemory);
}

/***********************************************************************
 *
 * FUNCTION:	GetLibraryName
 *
 * DESCRIPTION:
 *
 * PARAMETERS:	none
 *
 * RETURNED:	The libraries name, or an empty string if the library
 *				could not be found.
 *
 ***********************************************************************/

string GetLibraryName(uint16 refNum) {
    if (refNum == sysInvalidRefNum) return string();

    CEnableFullAccess munge;  // Remove blocks on memory access.

    /*
            The System Library Table (sysLibTableP) is an array of
            sysLibTableEntries entries.  Each entry has the following
            format:

                    Ptr*		dispatchTblP;	// pointer to library dispatch table
                    void*		globalsP;		// Library globals
                    LocalID 	dbID;			// database id of the library
                    MemPtr	 	codeRscH;		// library code resource handle for
       RAM-based libraries

            The latter two fields are present only in Palm OS 2.0 and
            later.	So our first steps are to (a) get the pointer to
            the array, (b) make sure that the index into the array (the
            refNum passed as the first parameter to all library calls)
            is within range, (c) get a pointer to the right entry,
            taking into account the Palm OS version, and (d) getting the
            dispatchTblP field.

            The "library dispatch table" is an array of 16-bit offsets.  The
            values are all relative to the beginning of the table (dispatchTblP).
            The first entry in the array corresponds to the library name.  All
            subsequent entries are offsets to the various library functions,
            starting with the required four: sysLibTrapOpen, sysLibTrapClose,
            sysLibTrapSleep, and sysLibTrapWake.
    */

    emuptr sysLibTableP = EmLowMem_GetGlobal(sysLibTableP);
    UInt16 sysLibTableEntries = EmLowMem_GetGlobal(sysLibTableEntries);

    if (sysLibTableP == EmMemNULL) {
        // !!! No library table!
        EmAssert(false);
        return string();
    }

    if (refNum >= sysLibTableEntries) {
        if (refNum != 0x0666) {
            // !!! RefNum out of range!
            EmAssert(false);
        }

        return string();
    }

    // emuptr libEntry;
    emuptr dispatchTblP;

    EmAliasSysLibTblEntryType<PAS> libEntries(sysLibTableP);
    dispatchTblP = libEntries[refNum].dispatchTblP;

#if 0  // CSTODO
    if (EmSystemState::OSMajorVersion() > 1) {
        libEntry = sysLibTableP + refNum * sizeof(SysLibTblEntryType);
        dispatchTblP = EmMemGet32(libEntry + offsetof(SysLibTblEntryType, dispatchTblP));
    } else {
        libEntry = sysLibTableP + refNum * sizeof(SysLibTblEntryTypeV10);
        dispatchTblP = EmMemGet32(libEntry + offsetof(SysLibTblEntryTypeV10, dispatchTblP));
    }
#endif

    // The first entry in the table is always the offset from the
    // start of the table to the library name.	Use this information
    // get the library name.

    int16 offset = EmMemGet16(dispatchTblP + LibTrapIndex(sysLibTrapName) * 2);
    emuptr libNameP = dispatchTblP + offset;

    char libName[256];
    EmMem_strcpy(libName, libNameP);

    PRINTF("library %u = %s", refNum, libName);

    return string(libName);
}

/***********************************************************************
 *
 * FUNCTION:	DateToDays
 *
 * DESCRIPTION: Convert a year, month, and day into the number of days
 *				since 1/1/1904.
 *
 *				Parameters are not checked for valid dates, so it's
 *				possible to feed in things like March 35, 1958.  This
 *				function also assumes that year is at least 1904, and
 *				will only work up until 2040 or so.
 *
 * PARAMETERS:	year - full year
 *
 *				month - 1..12
 *
 *				day - 1..31
 *
 * RETURNED:	Number of days since 1/1/1904.
 *
 ***********************************************************************/

uint32 DateToDays(uint32 year, uint32 month, uint32 day) {
    static const int month2days[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    // Normalize the values.

    year -= 1904;
    month -= 1;
    day -= 1;

    // Not counting any possible leap-day in the current year, figure out
    // the number of days between now and 1/1/1904.

    const uint32 kNumDaysInLeapCycle = 4 * 365 + 1;

    uint32 days = day + month2days[month] + (year * kNumDaysInLeapCycle + 3) / 4;

    // Now add in this year's leap-day, if there is one.

    if ((month >= 2) && ((year & 3) == 0)) days++;

    return days;
}

void SetHotSyncUserName(const char* userNameP) {
    if (EmLowMem::GetTrapAddress(sysTrapDlkDispatchRequest) == EmMemNULL) return;

    if (!userNameP) return;

    size_t userNameLen = strlen(userNameP) + 1;

    // If the name is too long, just return.  This should really only
    // happen if the user hand-edits the preference file to contain
    // a name that's too long.  The Preferences dialog box handler
    // checks as well, so the name shouldn't get too long from that path.

    if (userNameLen > dlkMaxUserNameLength + 1) return;

    // We need to prepare a command block for the DataLink Manager.
    // Define one large enough to hold all the data we'll pass in.
    //
    // The format of the data block is as follows:
    //
    //		[byte] DlpReqHeaderType.id			: Command request number (==
    // dlpWriteUserInfo) 		[byte] DlpReqHeaderType.argc		: # of arguments for
    // this command
    //(== 1)
    //
    //		[byte] DlpTinyArgWrapperType.bID	: ID of first argument (==
    // dlpWriteUserInfoReqArgID) 		[byte] DlpTinyArgWrapperType.bSize	: Size in
    // bytes of first argument (== whatever)
    //
    //		[long] DlpWriteUserInfoReqHdrType.userID		: Not used here - set to
    // zero 		[long] DlpWriteUserInfoReqHdrType.viewerID		: Not used here -
    // set to zero [long] DlpWriteUserInfoReqHdrType.lastSyncPC	: Not used here - set to zero [8byt]
    // DlpWriteUserInfoReqHdrType.lastSyncDate	: Not used here - set to zero 		[long]
    // DlpWriteUserInfoReqHdrType.modFlags		: Bits saying what values are being set
    //		[byte] DlpWriteUserInfoReqHdrType.userNameLen	: Length of user name + NULL
    //
    //		[str ] userName

    char buffer[sizeof(DlpReqHeaderType) + sizeof(DlpTinyArgWrapperType) +
                sizeof(DlpWriteUserInfoReqHdrType) + dlpMaxUserNameSize];

    // Get handy pointers to all of the above.
    DlpReqHeaderType* reqHdr = (DlpReqHeaderType*)buffer;
    DlpTinyArgWrapperType* reqWrapper =
        (DlpTinyArgWrapperType*)(((char*)reqHdr) + sizeof(DlpReqHeaderType));
    DlpWriteUserInfoReqHdrType* reqArgHdr =
        (DlpWriteUserInfoReqHdrType*)(((char*)reqWrapper) + sizeof(DlpTinyArgWrapperType));
    char* reqName = ((char*)reqArgHdr) + sizeof(DlpWriteUserInfoReqHdrType);

    // Fill in request header
    reqHdr->id = dlpWriteUserInfo;
    reqHdr->argc = 1;

    // Fill in the request arg wrapper
    reqWrapper->bID = (UInt8)dlpWriteUserInfoReqArgID;
    reqWrapper->bSize = (UInt8)(sizeof(*reqArgHdr) + userNameLen);

    // Fill in request arg header
    reqArgHdr->modFlags = dlpUserInfoModName;
    reqArgHdr->userNameLen = userNameLen;

    // Copy in the user name.
    strcpy(reqName, userNameP);

    // Build up a session block to hold the command block.
    DlkServerSessionType session;
    memset(&session, 0, sizeof(session));
    session.htalLibRefNum = EmPatchModuleHtal::kMagicRefNum;  // See comments in HtalLibSendReply.
    session.gotCommand = true;
    session.cmdLen = sizeof(buffer);
    session.cmdP = buffer;

    // For simplicity, byteswap here so that we don't have to reparse all
    // that above data in DlkDispatchRequest.

    Canonical(reqHdr->id);
    Canonical(reqHdr->argc);

    Canonical(reqWrapper->bID);
    Canonical(reqWrapper->bSize);

    Canonical(reqArgHdr->modFlags);
    Canonical(reqArgHdr->userNameLen);

    // Patch up cmdP and map in the buffer it points to.

    StMemoryMapper mapper(session.cmdP, session.cmdLen);
    session.cmdP = (void*)(long)EmBankMapped::GetEmulatedAddress(session.cmdP);

    // Finally, install the name.
    DlkDispatchRequest(&session);
}
