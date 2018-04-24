#pragma once
#include <switch.h>
#include <cstdio>

#define MAGIC_NSO0 0x304F534E
#define NSO_NUM_MAX 13

class NsoUtils {
    public:
        struct NsoSegment {
            u32 file_offset;
            u32 dst_offset;
            u32 decomp_size;
            u32 align_or_total_size;
        };
        
        struct NsoHeader {
            u32 magic;
            u32 _0x4;
            u32 _0x8;
            u32 flags;
            NsoSegment segments[3];
            u8 build_id[0x20];
            u32 compressed_sizes[3];
            u8 _0x6C[0x24];
            u64 dynstr_extents;
            u64 dynsym_extents;
            u8 section_hashes[3][0x20];
        };
        
        
        static_assert(sizeof(NsoHeader) == 0x100, "Incorrectly defined NsoHeader!");
        
        static const char *GetNsoFileName(unsigned int index) {
            switch (index) {
                case 0:
                    return "rtld";
                case 1:
                    return "main";
                case 2:
                    return "subsdk0";
                case 3:
                    return "subsdk1";
                case 4:
                    return "subsdk2";
                case 5:
                    return "subsdk3";
                case 6:
                    return "subsdk4";
                case 7:
                    return "subsdk5";
                case 8:
                    return "subsdk6";
                case 9:
                    return "subsdk7";
                case 10:
                    return "subsdk8";
                case 11:
                    return "subsdk9";
                case 12:
                    return "sdk";
                default:
                    /* TODO: Panic. */
                    return "?";
            }
        }
        
        static FILE *OpenNsoFromExeFS(unsigned int index);
        static FILE *OpenNsoFromSdCard(unsigned int index, u64 title_id);
        static FILE *OpenNso(unsigned int index, u64 title_id);
        
        static bool IsNsoPresent(unsigned int index);
        static Result LoadNsoHeaders(u64 title_id);
        static Result ValidateNsoLoadSet();
};