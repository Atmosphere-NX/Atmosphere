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
#include <stratosphere.hpp>

namespace ams {

    namespace fssrv::impl {

        const char *GetExecutionDirectoryPath();

    }

    namespace {

        void GetPath(char *dst, size_t dst_size, const char *src) {
            if (fs::IsPathAbsolute(src)) {
                util::SNPrintf(dst, dst_size, "%s", src);
            } else {
                util::SNPrintf(dst, dst_size, "%s%s", fssrv::impl::GetExecutionDirectoryPath(), src);
            }
        }

        #define TEST_R_EXPECT(__EXPR__, __EXPECTED__)                                                                                                                        \
        ({                                                                                                                                                                   \
            const Result __test_result = (__EXPR__);                                                                                                                         \
            if (!(__EXPECTED__ ::Includes(__test_result))) {                                                                                                                 \
                printf("Unexpected result: %s gave 0x%08x (2%03d-%04d)\n", # __EXPR__, __test_result.GetValue(), __test_result.GetModule(), __test_result.GetDescription()); \
                return;                                                                                                                                                      \
            }                                                                                                                                                                \
            __test_result;                                                                                                                                                   \
        })

        #define TEST_R_TRY(__EXPR__)                                                                                                                                         \
        ({                                                                                                                                                                   \
            const Result __test_result = (__EXPR__);                                                                                                                         \
            if (R_FAILED(__test_result)) {                                                                                                                                   \
                printf("Unexpected result: %s gave 0x%08x (2%03d-%04d)\n", # __EXPR__, __test_result.GetValue(), __test_result.GetModule(), __test_result.GetDescription()); \
                return;                                                                                                                                                      \
            }                                                                                                                                                                \
            __test_result;                                                                                                                                                   \
        })

        u8 g_buffer[64_KB];

        void DoFsTests() {
            /* Declare buffer to hold any work paths we have. */
            char path_buf[fs::EntryNameLengthMax + 1];
            char path_buf2[fs::EntryNameLengthMax + 1];
            #define FORMAT_PATH(S)  ({ GetPath(path_buf, sizeof(path_buf), S); path_buf; })
            #define FORMAT_PATH2(S) ({ GetPath(path_buf2, sizeof(path_buf2), S); path_buf2; })
            AMS_UNUSED(path_buf);
            AMS_UNUSED(path_buf2);

            /* Clear anything from a previous test run, no obligation for this to succeed. */
            fs::DeleteDirectoryRecursively(FORMAT_PATH("./test_dir/"));

            /* Verify that the test directory does not exist. */
            fs::DirectoryEntryType entry_type;
            TEST_R_EXPECT(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/")), fs::ResultPathNotFound);

            /* Create the subdirectory. */
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/")));

            /* Verify the test directory exists and is a directory. */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_Directory);

            /* ==================================================================================================================== */
            /* Create File                                                                                                          */
            /* ==================================================================================================================== */

            /* Create a file. */
            TEST_R_EXPECT(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/test_rand.bin")), fs::ResultPathNotFound);
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/test_rand.bin"), sizeof(g_buffer)));

            /* Check the file has correct entry type. */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/test_rand.bin")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_File);

            /* Create already existing file -> fs::ResultPathAlreadyExists(). */
            TEST_R_EXPECT(fs::CreateFile(FORMAT_PATH("./test_dir/test_rand.bin"), sizeof(g_buffer)), fs::ResultPathAlreadyExists);

            /* Create already existing dir  -> fs::ResultPathAlreadyExists(). */
            TEST_R_EXPECT(fs::CreateFile(FORMAT_PATH("./test_dir/"), sizeof(g_buffer)), fs::ResultPathAlreadyExists);

            /* Create file without parent existing -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::CreateFile(FORMAT_PATH("./test_dir/aaa/bbb.bin"), sizeof(g_buffer)), fs::ResultPathNotFound);

            /* ==================================================================================================================== */
            /* Create Directory                                                                                                     */
            /* ==================================================================================================================== */

            /* Create the subdirectory. */
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/test_subdir/")));

            /* Verify the test directory exists and is a directory. */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/test_subdir/")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_Directory);

            /* Create already existing file -> fs::ResultPathAlreadyExists(). */
            TEST_R_EXPECT(fs::CreateDirectory(FORMAT_PATH("./test_dir/test_rand.bin")), fs::ResultPathAlreadyExists);

            /* Create already existing dir -> fs::ResultPathAlreadyExists(). */
            TEST_R_EXPECT(fs::CreateDirectory(FORMAT_PATH("./test_dir/")), fs::ResultPathAlreadyExists);

            /* Create dir without parent existing -> fs::ResultPathAlreadyExists(). */
            TEST_R_EXPECT(fs::CreateDirectory(FORMAT_PATH("./test_dir/aaa/bbb/")), fs::ResultPathNotFound);

            /* ==================================================================================================================== */
            /* Delete File                                                                                                          */
            /* ==================================================================================================================== */

            /* Delete file succeeds. */
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/tmp_for_delete.bin"), sizeof(g_buffer)));
            TEST_R_TRY(fs::DeleteFile(FORMAT_PATH("./test_dir/tmp_for_delete.bin")));

            /* Delete on invalid path -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::DeleteFile(FORMAT_PATH("./test_dir/invalid")), fs::ResultPathNotFound);

            /* Delete on directory -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::DeleteFile(FORMAT_PATH("./test_dir/test_subdir/")), fs::ResultPathNotFound);

            /* ==================================================================================================================== */
            /* Delete Directory                                                                                                     */
            /* ==================================================================================================================== */

            /* Delete dir succeeds. */
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/tmp_for_delete/")));
            TEST_R_TRY(fs::DeleteDirectory(FORMAT_PATH("./test_dir/tmp_for_delete/")));

            /* Delete on invalid path -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::DeleteDirectory(FORMAT_PATH("./test_dir/invalid/")), fs::ResultPathNotFound);

            /* Delete on file -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::DeleteDirectory(FORMAT_PATH("./test_dir/test_rand.bin")), fs::ResultPathNotFound);

            /* Delete on non-empty directory -> fs::ResultDirectoryNotEmpty(). */
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/tmp_for_delete/")));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/tmp_for_delete/tmp_for_delete.bin"), sizeof(g_buffer)));
            TEST_R_EXPECT(fs::DeleteDirectory(FORMAT_PATH("./test_dir/tmp_for_delete/")), fs::ResultDirectoryNotEmpty);
            TEST_R_TRY(fs::DeleteFile(FORMAT_PATH("./test_dir/tmp_for_delete/tmp_for_delete.bin")));
            TEST_R_TRY(fs::DeleteDirectory(FORMAT_PATH("./test_dir/tmp_for_delete/")));

            /* ==================================================================================================================== */
            /* Delete Directory Recursively                                                                                         */
            /* ==================================================================================================================== */
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/0/0/")));

            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/0/y.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/0/z.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/0/0/x.bin"), 0));

            TEST_R_TRY(fs::DeleteDirectoryRecursively(FORMAT_PATH("./test_dir/0/")));

            /* Verify the test directory still exists and is a directory. */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_Directory);

            /* Verify the recursive directory doesn't. */
            TEST_R_EXPECT(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/0/")), fs::ResultPathNotFound);

            /* Delete recursive on invalid path -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::DeleteDirectoryRecursively(FORMAT_PATH("./test_dir/invalid/")), fs::ResultPathNotFound);

            /* Delete recursive on file -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::DeleteDirectoryRecursively(FORMAT_PATH("./test_dir/test_rand.bin")), fs::ResultPathNotFound);

            /* ==================================================================================================================== */
            /* Clean Directory Recursively                                                                                          */
            /* ==================================================================================================================== */
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/0/")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/0/0/")));

            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/0/y.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/0/z.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/0/x.bin"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/0/0/0/0/0/0/0/0/0/1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/000/b/0/0/0/x.bin"), 0));

            TEST_R_TRY(fs::CleanDirectoryRecursively(FORMAT_PATH("./test_dir/0/")));

            /* Verify the recursive directory still exists and is a directory. */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/0/")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_Directory);

            /* Delete the recursive directory. */
            TEST_R_TRY(fs::DeleteDirectory(FORMAT_PATH("./test_dir/0/")));

            /* Clean recursive on invalid path -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::CleanDirectoryRecursively(FORMAT_PATH("./test_dir/invalid/")), fs::ResultPathNotFound);

            /* Clean recursive on file -> fs::ResultPathNotFound(). */
            TEST_R_EXPECT(fs::CleanDirectoryRecursively(FORMAT_PATH("./test_dir/test_rand.bin")), fs::ResultPathNotFound);

            /* ==================================================================================================================== */
            /* Rename File                                                                                                          */
            /* ==================================================================================================================== */

            /* Rename succeeds. */
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/a.bin"), 1_KB));
            TEST_R_TRY(fs::RenameFile(FORMAT_PATH("./test_dir/a.bin"), FORMAT_PATH2("./test_dir/b.bin")));

            TEST_R_EXPECT(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/a.bin")), fs::ResultPathNotFound);
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/b.bin")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_File);

            /* Rename non-existing -> fs::ResultPathNotFound */
            TEST_R_EXPECT(fs::RenameFile(FORMAT_PATH("./test_dir/invalid"), FORMAT_PATH2("./test_dir/invalid2")), fs::ResultPathNotFound);

            /* Rename valid -> already existing gives fs::ResultPathAlreadyExists */
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/a.bin"), 1_KB));
            TEST_R_EXPECT(fs::RenameFile(FORMAT_PATH("./test_dir/a.bin"), FORMAT_PATH2("./test_dir/b.bin")), fs::ResultPathAlreadyExists);

            /* Rename valid -> directory gives fs::ResultPathAlreadyExists */
            TEST_R_EXPECT(fs::RenameFile(FORMAT_PATH("./test_dir/a.bin"), FORMAT_PATH2("./test_dir/test_subdir/")), fs::ResultPathAlreadyExists);

            /* Rename directory -> fs::ResultPathNotFound */
            TEST_R_EXPECT(fs::RenameFile(FORMAT_PATH("./test_dir/test_subdir/"), FORMAT_PATH2("./test_dir/c.bin")), fs::ResultPathNotFound);

            /* Invalid doesn't affect the file/dir. */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/a.bin")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_File);
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/test_subdir/")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_Directory);

            /* ==================================================================================================================== */
            /* Rename Directory                                                                                                     */
            /* ==================================================================================================================== */

            /* Rename succeeds. */
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/dir_a/")));
            TEST_R_TRY(fs::RenameDirectory(FORMAT_PATH("./test_dir/dir_a/"), FORMAT_PATH2("./test_dir/dir_b/")));

            TEST_R_EXPECT(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/dir_a/")), fs::ResultPathNotFound);
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/dir_b/")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_Directory);

            /* Rename non-existing -> fs::ResultPathNotFound */
            TEST_R_EXPECT(fs::RenameDirectory(FORMAT_PATH("./test_dir/invalid"), FORMAT_PATH2("./test_dir/invalid2")), fs::ResultPathNotFound);

            /* Rename valid -> already existing gives fs::ResultPathAlreadyExists */
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/dir_a/")));
            TEST_R_EXPECT(fs::RenameDirectory(FORMAT_PATH("./test_dir/dir_a/"), FORMAT_PATH2("./test_dir/dir_b/")), fs::ResultPathAlreadyExists);

            /* Rename valid -> file gives fs::ResultPathAlreadyExists */
            TEST_R_EXPECT(fs::RenameDirectory(FORMAT_PATH("./test_dir/dir_a/"), FORMAT_PATH2("./test_dir/a.bin")), fs::ResultPathAlreadyExists);

            /* Rename file -> fs::ResultPathNotFound */
            TEST_R_EXPECT(fs::RenameDirectory(FORMAT_PATH("./test_dir/a.bin"), FORMAT_PATH2("./test_dir/dir_c/")), fs::ResultPathNotFound);

            /* Invalid doesn't affect the file/dir. */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/a.bin")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_File);
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/dir_a/")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_Directory);

            /* ==================================================================================================================== */
            /* Get Entry Type                                                                                                       */
            /* ==================================================================================================================== */

            /* File -> file. */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/a.bin")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_File);

            /* Dir -> dir */
            TEST_R_TRY(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/dir_a/")));
            AMS_ABORT_UNLESS(entry_type == fs::DirectoryEntryType_Directory);

            /* Invalid -> fs::ResultPathNotFound */
            TEST_R_EXPECT(fs::GetEntryType(std::addressof(entry_type), FORMAT_PATH("./test_dir/invalid")), fs::ResultPathNotFound);

            /* ==================================================================================================================== */
            /* Get Free Space Size                                                                                                  */
            /* ==================================================================================================================== */

            s64 free_size = 0;
            TEST_R_TRY(fs::GetFreeSpaceSize(std::addressof(free_size), FORMAT_PATH("./test_dir/")));
            AMS_ABORT_UNLESS(free_size > 0);

            /* ==================================================================================================================== */
            /* Get Total Space Size                                                                                                 */
            /* ==================================================================================================================== */

            s64 total_size = 0;
            TEST_R_TRY(fs::GetTotalSpaceSize(std::addressof(total_size), FORMAT_PATH("./test_dir/")));
            AMS_ABORT_UNLESS(total_size >= free_size);

            /* ==================================================================================================================== */
            /* Get File Time Stamp                                                                                                  */
            /* ==================================================================================================================== */

            /* Get timestamp succeeds. */
            fs::FileTimeStamp timestamp;
            TEST_R_TRY(fs::GetFileTimeStamp(std::addressof(timestamp), FORMAT_PATH("./test_dir/a.bin")));
            AMS_ABORT_UNLESS(timestamp.create.value > 0);
            AMS_ABORT_UNLESS(timestamp.access.value > 0);
            AMS_ABORT_UNLESS(timestamp.modify.value > 0);
            AMS_ABORT_UNLESS(!timestamp.is_local_time);

            /* Invalid -> fs::ResultPathNotFound */
            TEST_R_EXPECT(fs::GetFileTimeStamp(std::addressof(timestamp), FORMAT_PATH("./test_dir/invalid")), fs::ResultPathNotFound);

            /* Directory -> fs::ResultPathNotFound */
            TEST_R_EXPECT(fs::GetFileTimeStamp(std::addressof(timestamp), FORMAT_PATH("./test_dir/dir_a/")), fs::ResultPathNotFound);

            /* ==================================================================================================================== */
            /* Query Entry                                                                                                          */
            /* ==================================================================================================================== */

            TEST_R_EXPECT(fs::SetConcatenationFileAttribute(FORMAT_PATH("./test_dir/")), fs::ResultUnsupportedOperation);

            /* ==================================================================================================================== */
            /* Open File                                                                                                            */
            /* ==================================================================================================================== */

            /* Open valid succeeds. */
            fs::FileHandle file;
            TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_ReadWrite | fs::OpenMode_AllowAppend));
            fs::CloseFile(file);

            /* Open invalid -> path not found. */
            TEST_R_EXPECT(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/invalid"), fs::OpenMode_ReadWrite | fs::OpenMode_AllowAppend), fs::ResultPathNotFound);

            /* Open directory -> path not found. */
            TEST_R_EXPECT(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/dir_a/"), fs::OpenMode_ReadWrite | fs::OpenMode_AllowAppend), fs::ResultPathNotFound);

            /* Open with invalid mode -> fs::ResultInvalidOpenMode */
            TEST_R_EXPECT(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), static_cast<fs::OpenMode>(~0u)), fs::ResultInvalidOpenMode);

            /* Read only file is read only. */
            {
                s64 file_size;
                u8 buf[1_KB];

                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* File size matches create. */
                    TEST_R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                    AMS_ABORT_UNLESS(file_size == 1_KB);

                    /* Read succeeds. */
                    TEST_R_TRY(fs::ReadFile(file, 0, buf, sizeof(buf)));

                    /* Completely empty read ok. */
                    TEST_R_TRY(fs::ReadFile(file, 0, nullptr, 0));

                    /* Flush succeeds. */
                    TEST_R_TRY(fs::FlushFile(file));
                }

                /* Incorrect arguments return incorrect results. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::ReadFile(file, -1, buf, sizeof(buf)), fs::ResultOutOfRange);
                }
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::ReadFile(file, 0, buf, -1), fs::ResultOutOfRange);
                }
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::ReadFile(file, 0, nullptr, sizeof(buf)), fs::ResultNullptrArgument);
                }

                /* Write fails. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::WriteFile(file, 0, g_buffer, sizeof(g_buffer), fs::WriteOption::None), fs::ResultWriteNotPermitted);
                }

                /* Set size fails. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::SetFileSize(file, 2_KB), fs::ResultWriteNotPermitted);
                }

                /* File size unchanged by bad set size. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                    AMS_ABORT_UNLESS(file_size == 1_KB);
                }
            }

            /* Write only file is writable but not readable. */
            {
                s64 file_size;
                u8 buf[1_KB];

                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Write succeeds. */
                    std::memset(buf, 0xcc, sizeof(buf));
                    TEST_R_TRY(fs::WriteFile(file, 0, buf, sizeof(buf), fs::WriteOption::None));

                    /* Flush succeeds. */
                    TEST_R_TRY(fs::FlushFile(file));

                    /* Write with flush succeeds. */
                    TEST_R_TRY(fs::WriteFile(file, 0, buf, sizeof(buf), fs::WriteOption::Flush));

                    /* Get size succeeds. */
                    TEST_R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                    AMS_ABORT_UNLESS(file_size == 1_KB);

                    /* Set size succeeds. */
                    TEST_R_TRY(fs::SetFileSize(file, 2_KB));
                    TEST_R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                    AMS_ABORT_UNLESS(file_size == 2_KB);

                    /* Write at updated size works. */
                    TEST_R_TRY(fs::WriteFile(file, 1_KB, buf, sizeof(buf), fs::WriteOption::Flush));

                    /* Truncate down succeeds. */
                    TEST_R_TRY(fs::SetFileSize(file, 1_KB));
                    TEST_R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                    AMS_ABORT_UNLESS(file_size == 1_KB);

                    /* Completely empty write ok. */
                    TEST_R_TRY(fs::WriteFile(file, 0, nullptr, 0, fs::WriteOption::Flush));
                }

                /* Incorrect arguments return incorrect results. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::WriteFile(file, -1, buf, sizeof(buf), fs::WriteOption::None), fs::ResultOutOfRange);
                }
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::WriteFile(file, 0, buf, -1, fs::WriteOption::None), fs::ResultOutOfRange);
                }
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::WriteFile(file, 0, nullptr, sizeof(buf), fs::WriteOption::None), fs::ResultNullptrArgument);
                }
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::WriteFile(file, 1_KB, buf, sizeof(buf), fs::WriteOption::None), fs::ResultFileExtensionWithoutOpenModeAllowAppend);
                }

                /* Read fails. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_EXPECT(fs::ReadFile(file, 0, buf, sizeof(buf)), fs::ResultReadNotPermitted);
                }

                /* Appending works with OpenMode_AllowAppend. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_Write | fs::OpenMode_AllowAppend));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                    AMS_ABORT_UNLESS(file_size == 1_KB);

                    for (size_t i = 0; i < sizeof(buf); ++i) {
                        buf[i] = static_cast<u8>(i);
                    }
                    TEST_R_TRY(fs::WriteFile(file, 1_KB, buf, sizeof(buf), fs::WriteOption::Flush));

                    TEST_R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                    AMS_ABORT_UNLESS(file_size == 2_KB);
                }

                /* Data is persistent. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/a.bin"), fs::OpenMode_ReadWrite));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    TEST_R_TRY(fs::ReadFile(file, 0, buf, sizeof(buf)));

                    for (size_t i = 0; i < 1_KB; ++i) {
                        AMS_ABORT_UNLESS(buf[i] == 0xCC);
                    }

                    TEST_R_TRY(fs::ReadFile(file, 1_KB, buf, sizeof(buf)));

                    for (size_t i = 0; i < 1_KB; ++i) {
                        AMS_ABORT_UNLESS(buf[i] == static_cast<u8>(i));
                    }

                    TEST_R_TRY(fs::WriteFile(file, 0, buf, sizeof(buf), fs::WriteOption::Flush));

                    TEST_R_TRY(fs::SetFileSize(file, 1_KB));

                    TEST_R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                    AMS_ABORT_UNLESS(file_size == 1_KB);

                    TEST_R_TRY(fs::ReadFile(file, 0, buf, sizeof(buf)));

                    for (size_t i = 0; i < 1_KB; ++i) {
                        AMS_ABORT_UNLESS(buf[i] == static_cast<u8>(i));
                    }
                }
            }

            /* More involved file data test using random buffer. */
            {
                u8 buf[1_KB];
                /* Write random data. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/test_rand.bin"), fs::OpenMode_Write));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                    /* Get a bunch of random data. */
                    os::GenerateRandomBytes(g_buffer, sizeof(g_buffer));

                    /* Write it to disk. */
                    TEST_R_TRY(fs::WriteFile(file, 0, g_buffer, sizeof(g_buffer), fs::WriteOption::None));
                    TEST_R_TRY(fs::FlushFile(file));
                }

                /* Read and verify random data. */
                {
                    TEST_R_TRY(fs::OpenFile(std::addressof(file), FORMAT_PATH("./test_dir/test_rand.bin"), fs::OpenMode_Read));
                    ON_SCOPE_EXIT { fs::CloseFile(file); };

                     u32 ofs;
                     for (size_t i = 0; i < 1000; ++i) {
                         os::GenerateRandomBytes(std::addressof(ofs), sizeof(ofs));
                         ofs %= (sizeof(g_buffer) - sizeof(buf));

                         TEST_R_TRY(fs::ReadFile(file, ofs, buf, sizeof(buf)));
                         AMS_ABORT_UNLESS(std::memcmp(buf, g_buffer + ofs, sizeof(buf)) == 0);
                     }
                }
            }

            /* ==================================================================================================================== */
            /* Open Directory                                                                                                       */
            /* ==================================================================================================================== */
            fs::DirectoryHandle dir;
            TEST_R_TRY(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/dir_a/"), fs::OpenDirectoryMode_All | fs::OpenDirectoryMode_NotRequireFileSize));
            fs::CloseDirectory(dir);

            /* Open invalid -> path not found. */
            TEST_R_EXPECT(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/invalid"), fs::OpenDirectoryMode_All | fs::OpenDirectoryMode_NotRequireFileSize), fs::ResultPathNotFound);

            /* Open file -> path not found. */
            TEST_R_EXPECT(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/a.bin"), fs::OpenDirectoryMode_All | fs::OpenDirectoryMode_NotRequireFileSize), fs::ResultPathNotFound);

            /* Open with invalid mode -> fs::ResultInvalidOpenMode */
            TEST_R_EXPECT(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/dir_a/"), static_cast<fs::OpenDirectoryMode>(~0u)), fs::ResultInvalidOpenMode);

            /* Populate test directory with three files and two dirs. */
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/dir_a/f0"), 0));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/dir_a/f1"), 1_KB));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/dir_a/f2"), 0x42069));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/dir_a/d0")));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/dir_a/d1")));
            TEST_R_TRY(fs::CreateFile(FORMAT_PATH("./test_dir/dir_a/d0/file"), 0));
            TEST_R_TRY(fs::CreateDirectory(FORMAT_PATH("./test_dir/dir_a/d0/dir/")));

            /* Directory tests. */
            {
                bool seen_file[3];
                bool seen_dir[2];

                constexpr s64 NumFiles = util::size(seen_file);
                constexpr s64 NumDirs  = util::size(seen_dir);
                constexpr s64 NumAll   = NumFiles + NumDirs;

                fs::DirectoryEntry entries[2 * NumAll];
                s64 entry_count;

                auto ResetSeenFiles = [&] () { for (auto &b : seen_file) { b = false; } };
                auto ResetSeenDirs  = [&] () { for (auto &b : seen_dir) { b = false; } };
                auto ResetSeenAll   = [&] () { ResetSeenFiles(); ResetSeenDirs(); };

                auto CheckSeenFiles = [&] () { for (const auto b : seen_file) { AMS_ABORT_UNLESS(b); } };
                auto CheckSeenDirs  = [&] () { for (const auto b : seen_dir) { AMS_ABORT_UNLESS(b); } };
                auto CheckSeenAll   = [&] () { CheckSeenFiles(); CheckSeenDirs(); };

                auto CheckNotSeenFiles = [&] () { for (const auto b : seen_file) { AMS_ABORT_UNLESS(!b); } };
                auto CheckNotSeenDirs  = [&] () { for (const auto b : seen_dir) { AMS_ABORT_UNLESS(!b); } };

                auto CheckDirectoryEntry = [&] (const fs::DirectoryEntry &entry) {
                    /* Check name. */
                    AMS_ABORT_UNLESS(entry.name[0] == 'f' || entry.name[0] == 'd');
                    AMS_ABORT_UNLESS('0' <= entry.name[1] && entry.name[1] <= '9');
                    AMS_ABORT_UNLESS(entry.name[2] == 0);

                    /* Check type. */
                    if (entry.name[0] == 'f') {
                        AMS_ABORT_UNLESS(entry.type == fs::DirectoryEntryType_File);

                        /* If file, check size. */
                        switch (entry.name[1]) {
                            case '0': AMS_ABORT_UNLESS(entry.file_size == 0); break;
                            case '1': AMS_ABORT_UNLESS(entry.file_size == 1_KB); break;
                            case '2': AMS_ABORT_UNLESS(entry.file_size == 0x42069); break;
                        }

                        AMS_ABORT_UNLESS(!seen_file[(entry.name[1] - '0')]);
                        seen_file[(entry.name[1] - '0')] = true;
                    } else {
                        AMS_ABORT_UNLESS(entry.type == fs::DirectoryEntryType_Directory);

                        AMS_ABORT_UNLESS(!seen_dir[(entry.name[1] - '0')]);
                        seen_dir[(entry.name[1] - '0')] = true;
                    }
                };

                /* Get EntryCount is correct. */
                {
                    /* All returns all entries. */
                    {
                        TEST_R_TRY(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/dir_a/"), fs::OpenDirectoryMode_All));
                        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count), dir));
                        AMS_ABORT_UNLESS(entry_count == NumAll);
                    }

                    /* File returns only files, and does not count things in subdirectories. */
                    {
                        TEST_R_TRY(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/dir_a/"), fs::OpenDirectoryMode_File));
                        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count), dir));
                        AMS_ABORT_UNLESS(entry_count == NumFiles);
                    }

                    /* Dir returns only dirs, and does not count things in subdirectories. */
                    {
                        TEST_R_TRY(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/dir_a/"), fs::OpenDirectoryMode_Directory));
                        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count), dir));
                        AMS_ABORT_UNLESS(entry_count == NumDirs);
                    }
                }

                /* Read is correct, N at a time. */
                for (s64 at_a_time = 1; at_a_time <= 2 * NumAll; ++at_a_time) {
                    /* All returns all entries. */
                    {
                        ResetSeenAll();

                        TEST_R_TRY(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/dir_a/"), fs::OpenDirectoryMode_All));
                        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count), dir));
                        AMS_ABORT_UNLESS(entry_count == NumAll);

                        s64 remaining = entry_count;
                        while (remaining > 0) {
                            s64 cur;
                            TEST_R_TRY(fs::ReadDirectory(std::addressof(cur), entries, dir, at_a_time));
                            AMS_ABORT_UNLESS(cur <= remaining);
                            AMS_ABORT_UNLESS(cur == std::min<s64>(at_a_time, remaining));

                            for (s64 i = 0; i < cur; ++i) {
                                CheckDirectoryEntry(entries[i]);
                            }

                            remaining -= cur;
                        }

                        CheckSeenAll();

                        /* Read succeeds at end of dir. */
                        s64 cur;
                        TEST_R_TRY(fs::ReadDirectory(std::addressof(cur), entries, dir, at_a_time));
                        AMS_ABORT_UNLESS(cur == 0);

                        /* Get entry count still shows correct value. */
                        s64 entry_count2;
                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count2), dir));
                        AMS_ABORT_UNLESS(entry_count2 == entry_count);
                    }

                    /* File returns only files. */
                    {
                        ResetSeenAll();

                        TEST_R_TRY(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/dir_a/"), fs::OpenDirectoryMode_File));
                        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count), dir));
                        AMS_ABORT_UNLESS(entry_count == NumFiles);

                        s64 remaining = entry_count;
                        while (remaining > 0) {
                            s64 cur;
                            TEST_R_TRY(fs::ReadDirectory(std::addressof(cur), entries, dir, at_a_time));
                            AMS_ABORT_UNLESS(cur <= remaining);
                            AMS_ABORT_UNLESS(cur == std::min<s64>(at_a_time, remaining));

                            for (s64 i = 0; i < cur; ++i) {
                                CheckDirectoryEntry(entries[i]);
                            }

                            remaining -= cur;
                        }

                        CheckSeenFiles();
                        CheckNotSeenDirs();

                        /* Read succeeds at end of dir. */
                        s64 cur;
                        TEST_R_TRY(fs::ReadDirectory(std::addressof(cur), entries, dir, at_a_time));
                        AMS_ABORT_UNLESS(cur == 0);

                        /* Get entry count still shows correct value. */
                        s64 entry_count2;
                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count2), dir));
                        AMS_ABORT_UNLESS(entry_count2 == entry_count);
                    }

                    /* Directory returns only dirs. */
                    {
                        ResetSeenAll();

                        TEST_R_TRY(fs::OpenDirectory(std::addressof(dir), FORMAT_PATH("./test_dir/dir_a/"), fs::OpenDirectoryMode_Directory));
                        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count), dir));
                        AMS_ABORT_UNLESS(entry_count == NumDirs);

                        s64 remaining = entry_count;
                        while (remaining > 0) {
                            s64 cur;
                            TEST_R_TRY(fs::ReadDirectory(std::addressof(cur), entries, dir, at_a_time));
                            AMS_ABORT_UNLESS(cur <= remaining);
                            AMS_ABORT_UNLESS(cur == std::min<s64>(at_a_time, remaining));

                            for (s64 i = 0; i < cur; ++i) {
                                CheckDirectoryEntry(entries[i]);
                            }

                            remaining -= cur;
                        }

                        CheckSeenDirs();
                        CheckNotSeenFiles();

                        /* Read succeeds at end of dir. */
                        s64 cur;
                        TEST_R_TRY(fs::ReadDirectory(std::addressof(cur), entries, dir, at_a_time));
                        AMS_ABORT_UNLESS(cur == 0);

                        /* Get entry count still shows correct value. */
                        s64 entry_count2;
                        TEST_R_TRY(fs::GetDirectoryEntryCount(std::addressof(entry_count2), dir));
                        AMS_ABORT_UNLESS(entry_count2 == entry_count);
                    }
                }
            }

            /* ==================================================================================================================== */
            /* Cleanup                                                                                                              */
            /* ==================================================================================================================== */
            TEST_R_TRY(fs::DeleteDirectoryRecursively(FORMAT_PATH("./test_dir/")));
        }

    }


    void Main() {
        fs::SetEnabledAutoAbort(false);

        printf("Doing FS test!\n");
        DoFsTests();
        printf("All tests completed!\n");
    }

}