/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <vapours.hpp>

namespace ams::socket {

    enum class Errno : u32 {
        ESuccess        =   0,
        EPerm           =   1,
        ENoEnt          =   2,
        ESrch           =   3,
        EIntr           =   4,
        EIo             =   5,
        ENxIo           =   6,
        E2Big           =   7,
        ENoExec         =   8,
        EBadf           =   9,
        EChild          =  10,
        EAgain          =  11,
        EWouldBlock     =  EAgain,
        ENoMem          =  12,
        EAcces          =  13,
        EFault          =  14,
        ENotBlk         =  15,
        EBusy           =  16,
        EExist          =  17,
        EXDev           =  18,
        ENoDev          =  19,
        ENotDir         =  20,
        EIsDir          =  21,
        EInval          =  22,
        ENFile          =  23,
        EMFile          =  24,
        ENotTy          =  25,
        ETxtBsy         =  26,
        EFBig           =  27,
        ENoSpc          =  28,
        ESPipe          =  29,
        ERofs           =  30,
        EMLink          =  31,
        EPipe           =  32,
        EDom            =  33,
        ERange          =  34,
        EDeadLk         =  35,
        EDeadLock       =  EDeadLk,
        ENameTooLong    =  36,
        ENoLck          =  37,
        ENoSys          =  38,
        ENotEmpty       =  39,
        ELoop           =  40,
        ENoMsg          =  42,
        EIdrm           =  43,
        EChrng          =  44,
        EL2NSync        =  45,
        EL3Hlt          =  46,
        EL3Rst          =  47,
        ELnrng          =  48,
        EUnatch         =  49,
        ENoCsi          =  50,
        EL2Hlt          =  51,
        EBade           =  52,
        EBadr           =  53,
        EXFull          =  54,
        ENoAno          =  55,
        EBadRqc         =  56,
        EBadSsl         =  57,
        EBFont          =  59,
        ENoStr          =  60,
        ENoData         =  61,
        ETime           =  62,
        ENoSr           =  63,
        ENoNet          =  64,
        ENoPkg          =  65,
        ERemote         =  66,
        ENoLink         =  67,
        EAdv            =  68,
        ESrmnt          =  69,
        EComm           =  70,
        EProto          =  71,
        EMultiHop       =  72,
        EDotDot         =  73,
        EBadMsg         =  74,
        EOverflow       =  75,
        ENotUnuq        =  76,
        EBadFd          =  77,
        ERemChg         =  78,
        ELibAcc         =  79,
        ELibBad         =  80,
        ELibScn         =  81,
        ELibMax         =  82,
        ELibExec        =  83,
        EIlSeq          =  84,
        ERestart        =  85,
        EStrPipe        =  86,
        EUsers          =  87,
        ENotSock        =  88,
        EDestAddrReq    =  89,
        EMsgSize        =  90,
        EPrototype      =  91,
        ENoProtoOpt     =  92,
        EProtoNoSupport =  93,
        ESocktNoSupport =  94,
        EOpNotSupp      =  95,
        ENotSup         =  EOpNotSupp,
        EPfNoSupport    =  96,
        EAfNoSupport    =  97,
        EAddrInUse      =  98,
        EAddrNotAvail   =  99,
        ENetDown        = 100,
        ENetUnreach     = 101,
        ENetReset       = 102,
        EConnAborted    = 103,
        EConnReset      = 104,
        ENoBufs         = 105,
        EIsConn         = 106,
        ENotConn        = 107,
        EShutDown       = 108,
        ETooManyRefs    = 109,
        ETimedOut       = 110,
        EConnRefused    = 111,
        EHostDown       = 112,
        EHostUnreach    = 113,
        EAlready        = 114,
        EInProgress     = 115,
        EStale          = 116,
        EUClean         = 117,
        ENotNam         = 118,
        ENAvail         = 119,
        EIsNam          = 120,
        ERemoteIo       = 121,
        EDQuot          = 122,
        ENoMedium       = 123,
        EMediumType     = 124,
        ECanceled       = 125,
        ENoKey          = 126,
        EKeyExpired     = 127,
        EKeyRevoked     = 128,
        EKeyRejected    = 129,
        EOwnerDead      = 130,
        ENotRecoverable = 131,
        ERfKill         = 132,
        EHwPoison       = 133,
        /* ... */
        EProcLim        = 156,
    };

    enum class HErrno : s32 {
        Netdb_Internal = -1,
        Netdb_Success  = 0,
        Host_Not_Found = 1,
        Try_Again      = 2,
        No_Recovery    = 3,
        No_Data        = 4,

        No_Address     = No_Data,
    };

    enum class AiErrno : u32 {
        EAi_Success = 0,
        /* ... */
    };

    constexpr inline bool operator!(Errno e)   { return e == Errno::ESuccess; }
    constexpr inline bool operator!(HErrno e)  { return e == HErrno::Netdb_Success; }
    constexpr inline bool operator!(AiErrno e) { return e == AiErrno::EAi_Success; }

}
