/*
 * Copyright (c) 2018 Atmosphère-NX
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

enum class Boot2KnownTitleId : u64 {
    fs = 0x0100000000000000UL,
    loader = 0x0100000000000001UL,
    ncm = 0x0100000000000002UL,
    pm = 0x0100000000000003UL,
    sm = 0x0100000000000004UL,
    boot = 0x0100000000000005UL,
    usb = 0x0100000000000006UL,
    tma = 0x0100000000000007UL,
    boot2 = 0x0100000000000008UL,
    settings = 0x0100000000000009UL,
    bus = 0x010000000000000AUL,
    bluetooth = 0x010000000000000BUL,
    bcat = 0x010000000000000CUL,
    dmnt = 0x010000000000000DUL,
    friends = 0x010000000000000EUL,
    nifm = 0x010000000000000FUL,
    ptm = 0x0100000000000010UL,
    shell = 0x0100000000000011UL,
    bsdsockets = 0x0100000000000012UL,
    hid = 0x0100000000000013UL,
    audio = 0x0100000000000014UL,
    lm = 0x0100000000000015UL,
    wlan = 0x0100000000000016UL,
    cs = 0x0100000000000017UL,
    ldn = 0x0100000000000018UL,
    nvservices = 0x0100000000000019UL,
    pcv = 0x010000000000001AUL,
    ppc = 0x010000000000001BUL,
    nvnflinger = 0x010000000000001CUL,
    pcie = 0x010000000000001DUL,
    account = 0x010000000000001EUL,
    ns = 0x010000000000001FUL,
    nfc = 0x0100000000000020UL,
    psc = 0x0100000000000021UL,
    capsrv = 0x0100000000000022UL,
    am = 0x0100000000000023UL,
    ssl = 0x0100000000000024UL,
    nim = 0x0100000000000025UL,
    spl = 0x0100000000000028UL,
    lbl = 0x0100000000000029UL,
    btm = 0x010000000000002AUL,
    erpt = 0x010000000000002BUL,
    vi = 0x010000000000002DUL,
    pctl = 0x010000000000002EUL,
    npns = 0x010000000000002FUL,
    eupld = 0x0100000000000030UL,
    glue = 0x0100000000000031UL,
    eclct = 0x0100000000000032UL,
    es = 0x0100000000000033UL,
    fatal = 0x0100000000000034UL,
    grc = 0x0100000000000035UL,
    creport = 0x0100000000000036UL,
    ro = 0x0100000000000037UL,
    profiler = 0x0100000000000038UL,
    sdb = 0x0100000000000039UL,
    migration = 0x010000000000003AUL,
    jit = 0x010000000000003BUL,
    jpegdec = 0x010000000000003CUL,
    safemode = 0x010000000000003DUL,
    olsc = 0x010000000000003EUL,
};

class EmbeddedBoot2 {
    public:
        static void Main();
};