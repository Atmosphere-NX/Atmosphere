/*
 * Copyright (c) 2020 Adubbz
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

#include <array>
#include <memory>
#include <vector>
#include <optional>
#include <nanovg.h>
#include <switch.h>
#include "ams_su.h"

namespace dbk {

    struct Button {
        static constexpr u32 InvalidButtonId = -1;

        u32 id;
        bool selected;
        bool enabled;
        char text[256];
        float x;
        float y;
        float w;
        float h;

        inline bool IsPositionInBounds(float x, float y) {
            return x >= this->x && y >= this->y && x < (this->x + this->w) && y < (this->y + this->h);
        }
    };

    enum class Direction {
        Up,
        Down,
        Left,
        Right,
        Invalid,
    };

    class Menu {
        protected:
            static constexpr size_t MaxButtons = 32;
            static constexpr size_t LogBufferSize = 0x1000;
        protected:
            std::array<std::optional<Button>, MaxButtons> m_buttons;
            const std::shared_ptr<Menu> m_prev_menu;
            char m_log_buffer[LogBufferSize];
        protected:
            void AddButton(u32 id, const char *text, float x, float y, float w, float h);
            void SetButtonSelected(u32 id, bool selected);
            void DeselectAllButtons();
            void SetButtonEnabled(u32 id, bool enabled);

            Button *GetButton(u32 id);
            Button *GetSelectedButton();
            Button *GetClosestButtonToSelection(Direction direction);
            Button *GetTouchedButton();
            Button *GetActivatedButton();

            void UpdateButtons();
            void DrawButtons(NVGcontext *vg, u64 ns);

            void LogText(const char *format, ...);
        public:
            Menu(std::shared_ptr<Menu> prev_menu) : m_buttons({}), m_prev_menu(prev_menu), m_log_buffer{} { /* ... */ }

            std::shared_ptr<Menu> GetPrevMenu();
            virtual void Update(u64 ns) = 0;
            virtual void Draw(NVGcontext *vg, u64 ns) = 0;
    };

    class AlertMenu : public Menu {
        protected:
            static constexpr float WindowWidth           = 600.0f;
            static constexpr float WindowHeight          = 214.0f;
            static constexpr float TitleGap              = 90.0f;
            static constexpr float SubTextHeight         = 24.0f;
        protected:
            char m_text[0x100];
            char m_subtext[0x100];
            char m_result_text[0x20];
            Result m_rc;
        public:
            AlertMenu(std::shared_ptr<Menu> prev_menu, const char *text, const char *subtext, Result rc = 0);

            virtual void Draw(NVGcontext *vg, u64 ns) override;
    };

    class ErrorMenu : public AlertMenu {
        private:
            static constexpr u32 ExitButtonId = 0;
        public:
            ErrorMenu(const char *text, const char *subtext, Result rc = 0);

            virtual void Update(u64 ns) override;
    };

    class WarningMenu : public AlertMenu {
        private:
            static constexpr u32 BackButtonId     = 0;
            static constexpr u32 ContinueButtonId = 1;
        private:
            const std::shared_ptr<Menu> m_next_menu;
        public:
            WarningMenu(std::shared_ptr<Menu> prev_menu, std::shared_ptr<Menu> next_menu, const char *text, const char *subtext, Result rc = 0);

            virtual void Update(u64 ns) override;
    };

    class MainMenu : public Menu {
        private:
            static constexpr u32 InstallButtonId = 0;
            static constexpr u32 ExitButtonId    = 1;

            static constexpr float WindowWidth             = 400.0f;
            static constexpr float WindowHeight            = 240.0f;
            static constexpr float TitleGap                = 90.0f;
        public:
            MainMenu();

            virtual void Update(u64 ns) override;
            virtual void Draw(NVGcontext *vg, u64 ns) override;
    };

    class FileMenu : public Menu {
        private:
            struct FileEntry {
                char name[FS_MAX_PATH];
            };
        private:
            static constexpr size_t MaxFileRows = 11;

            static constexpr float WindowWidth            = 1200.0f;
            static constexpr float WindowHeight           = 680.0f;
            static constexpr float TitleGap               = 90.0f;
            static constexpr float TextBackgroundOffset   = 20.0f;
            static constexpr float FileRowHeight          = 40.0f;
            static constexpr float FileRowGap             = 10.0f;
            static constexpr float FileRowHorizontalInset = 10.0f;
            static constexpr float FileListHeight         = MaxFileRows * (FileRowHeight + FileRowGap);
        private:
            char m_root[FS_MAX_PATH];
            std::vector<FileEntry> m_file_entries;
            u32 m_current_index;
            float m_scroll_offset;
            float m_touch_start_scroll_offset;
            bool m_touch_finalize_selection;

            Result PopulateFileEntries();
            bool IsSelectionVisible();
            void ScrollToSelection();
            bool IsEntryTouched(u32 i);
            void UpdateTouches();
            void FinalizeSelection();
        public:
            FileMenu(std::shared_ptr<Menu> prev_menu, const char *root);

            virtual void Update(u64 ns) override;
            virtual void Draw(NVGcontext *vg, u64 ns) override;
    };

    class ValidateUpdateMenu : public Menu {
        private:
            static constexpr u32 BackButtonId     = 0;
            static constexpr u32 ContinueButtonId = 1;

            static constexpr float WindowWidth           = 600.0f;
            static constexpr float WindowHeight          = 600.0f;
            static constexpr float TitleGap              = 90.0f;
            static constexpr float TextAreaHeight        = 410.0f;
        private:
            AmsSuUpdateInformation m_update_info;
            AmsSuUpdateValidationInfo m_validation_info;
            bool m_has_drawn;
            bool m_has_info;
            bool m_has_validated;

            Result GetUpdateInformation();
            void ValidateUpdate();
        public:
            ValidateUpdateMenu(std::shared_ptr<Menu> prev_menu);

            virtual void Update(u64 ns) override;
            virtual void Draw(NVGcontext *vg, u64 ns) override;
    };

    class ChooseResetMenu : public Menu {
        private:
            static constexpr u32 ResetToFactorySettingsButtonId = 0;
            static constexpr u32 PreserveSettingsButtonId = 1;

            static constexpr float WindowWidth           = 600.0f;
            static constexpr float WindowHeight          = 170.0f;
            static constexpr float TitleGap              = 90.0f;
        public:
            ChooseResetMenu(std::shared_ptr<Menu> prev_menu);

            virtual void Update(u64 ns) override;
            virtual void Draw(NVGcontext *vg, u64 ns) override;
    };

    class ChooseExfatMenu : public Menu {
        private:
            static constexpr u32 Fat32ButtonId = 0;
            static constexpr u32 ExFatButtonId = 1;

            static constexpr float WindowWidth           = 600.0f;
            static constexpr float WindowHeight          = 170.0f;
            static constexpr float TitleGap              = 90.0f;
        public:
            ChooseExfatMenu(std::shared_ptr<Menu> prev_menu);

            virtual void Update(u64 ns) override;
            virtual void Draw(NVGcontext *vg, u64 ns) override;
    };

    class InstallUpdateMenu : public Menu {
        private:
            enum class InstallState {
                NeedsDraw,
                NeedsSetup,
                NeedsPrepare,
                AwaitingPrepare,
                NeedsApply,
                AwaitingReboot,
            };
        private:
            static constexpr u32 ShutdownButtonId = 0;
            static constexpr u32 RebootButtonId   = 1;

            static constexpr float WindowWidth           = 600.0f;
            static constexpr float WindowHeight          = 600.0f;
            static constexpr float TitleGap              = 120.0f;
            static constexpr float ProgressTextHeight    = 20.0f;
            static constexpr float ProgressBarHeight     = 30.0f;
            static constexpr float TextAreaHeight        = 320.0f;

            static constexpr size_t UpdateTaskBufferSize = 0x100000;
        private:
            InstallState m_install_state;
            AsyncResult m_prepare_result;
            float m_progress_percent;

            void MarkForReboot();
            Result TransitionUpdateState();
        public:
            InstallUpdateMenu(std::shared_ptr<Menu> prev_menu);

            virtual void Update(u64 ns) override;
            virtual void Draw(NVGcontext *vg, u64 ns) override;
    };

    void InitializeMenu(u32 screen_width, u32 screen_height);
    void UpdateMenu(u64 ns);
    void RenderMenu(NVGcontext *vg, u64 ns);
    bool IsExitRequested();

}
