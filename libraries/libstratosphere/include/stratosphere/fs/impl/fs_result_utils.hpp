/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

namespace ams::fs::impl {

    bool IsAbortNeeded(Result result);
    void LogErrorMessage(Result result, const char *function);

}

#define AMS_FS_R_CHECK_ABORT_IMPL(__RESULT__, __FORCE__)                             \
    ({                                                                               \
        if (::ams::fs::impl::IsAbortNeeded(__RESULT__) || (__FORCE__)) {             \
            ::ams::fs::impl::LogErrorMessage(__RESULT__, AMS_CURRENT_FUNCTION_NAME); \
            R_ABORT_UNLESS(__RESULT__);                                              \
        }                                                                            \
    })

#define AMS_FS_R_TRY(__RESULT__)                            \
    ({                                                      \
        const ::ams::Result __tmp_fs_result = (__RESULT__); \
        AMS_FS_R_CHECK_ABORT_IMPL(__tmp_fs_result, false);  \
        R_TRY(__tmp_fs_result);                             \
    })

#define AMS_FS_R_ABORT_UNLESS(__RESULT__)                   \
    ({                                                      \
        const ::ams::Result __tmp_fs_result = (__RESULT__); \
        AMS_FS_R_CHECK_ABORT_IMPL(__tmp_fs_result, true);   \
    })

#define AMS_FS_ABORT_UNLESS_WITH_RESULT(__EXPR__, __RESULT__) \
    ({                                                        \
        if (!(__EXPR__)) {                                    \
            AMS_FS_R_ABORT_UNLESS((__RESULT__));              \
        }                                                     \
    })

#define AMS_FS_R_THROW(__RESULT__)                          \
    ({                                                      \
        const ::ams::Result __tmp_fs_result = (__RESULT__); \
        AMS_FS_R_CHECK_ABORT_IMPL(__tmp_fs_result, false);  \
        return __tmp_fs_result;                             \
    })

#define AMS_FS_R_UNLESS(__EXPR__, __RESULT__)               \
    ({                                                      \
        if (!(__EXPR__)) {                                  \
            AMS_FS_R_THROW((__RESULT__));                   \
        }                                                   \
    })

#define AMS_FS_R_TRY_CATCH(__EXPR__) R_TRY_CATCH(__EXPR__)

#define AMS_FS_R_CATCH(...) R_CATCH(__VA_ARGS__)

#define AMS_FS_R_END_TRY_CATCH                     \
            else if (R_FAILED(R_CURRENT_RESULT)) { \
                AMS_FS_R_THROW(R_CURRENT_RESULT);  \
            }                                      \
        }                                          \
    })

#define AMS_FS_R_END_TRY_CATCH_WITH_ABORT_UNLESS         \
            else {                                       \
                AMS_FS_R_ABORT_UNLESS(R_CURRENT_RESULT); \
            }                                            \
        }                                                \
    })
