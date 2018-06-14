#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "fsmitm_romstorage.hpp"
#include "fsmitm_romfsbuild.hpp"
#include "fsmitm_utils.hpp"


/* Represents a merged RomFS. */
class LayeredRomFS : public IROStorage {
    private:
        /* Data Sources. */
        std::shared_ptr<RomInterfaceStorage> storage_romfs;
        std::shared_ptr<RomFileStorage> file_romfs;
        /* Information about the merged RomFS. */
        u64 title_id;
        std::shared_ptr<std::vector<RomFSSourceInfo>> p_source_infos;

        LayeredRomFS *Clone() override {
            return new LayeredRomFS(*this);
        };
                
    public:
        LayeredRomFS(std::shared_ptr<RomInterfaceStorage> s_r, std::shared_ptr<RomFileStorage> f_r, u64 tid);
        virtual ~LayeredRomFS();
        
        Result Read(void *buffer, size_t size, u64 offset) override;
        Result GetSize(u64 *out_size) override;
        Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override;
};
