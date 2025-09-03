/*
 * Copyright (c) Adubbz
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
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <dirent.h>
#include "ui.hpp"
#include "ui_util.hpp"
#include "assert.hpp"

namespace dbk {

    namespace {

        static constexpr u32 ExosphereApiVersionConfigItem = 65000;
        static constexpr u32 ExosphereHasRcmBugPatch       = 65004;
        static constexpr u32 ExosphereEmummcType           = 65007;
        static constexpr u32 ExosphereSupportedHosVersion  = 65011;

        /* Insets of content within windows. */
        static constexpr float HorizontalInset       = 20.0f;
        static constexpr float BottomInset           = 20.0f;

        /* Insets of content within text areas. */
        static constexpr float TextHorizontalInset   = 8.0f;
        static constexpr float TextVerticalInset     = 8.0f;

        static constexpr float ButtonHeight          = 60.0f;
        static constexpr float ButtonHorizontalGap   = 10.0f;

        static constexpr float VerticalGap           = 10.0f;

        u32 g_screen_width;
        u32 g_screen_height;

        constinit u32 g_supported_version = std::numeric_limits<u32>::max();

        std::shared_ptr<Menu> g_current_menu;
        bool g_initialized = false;
        bool g_exit_requested = false;

        PadState g_pad;

        u32 g_prev_touch_count = -1;
        HidTouchScreenState g_start_touch;
        bool g_started_touching = false;
        bool g_tapping = false;
        bool g_touches_moving = false;
        bool g_finished_touching = false;

        /* Update install state. */
        char g_update_path[FS_MAX_PATH];
        bool g_reset_to_factory = false;
        bool g_exfat_supported = false;
        bool g_use_exfat = false;

        constexpr u32 MaxTapMovement = 20;

        void UpdateInput() {
            /* Scan for input and update touch state. */
            padUpdate(&g_pad);
            HidTouchScreenState current_touch;
            hidGetTouchScreenStates(&current_touch, 1);
            const u32 touch_count = current_touch.count;

            if (g_prev_touch_count == 0 && touch_count > 0) {
                hidGetTouchScreenStates(&g_start_touch, 1);
                g_started_touching = true;
                g_tapping = true;
            } else {
                g_started_touching = false;
            }

            if (g_prev_touch_count > 0 && touch_count == 0) {
                g_finished_touching = true;
                g_tapping = false;
            } else {
                g_finished_touching = false;
            }

            /* Check if currently moving. */
            if (g_prev_touch_count > 0 && touch_count > 0) {
                if ((abs(current_touch.touches[0].x - g_start_touch.touches[0].x) > MaxTapMovement || abs(current_touch.touches[0].y - g_start_touch.touches[0].y) > MaxTapMovement)) {
                    g_touches_moving = true;
                    g_tapping = false;
                } else {
                    g_touches_moving = false;
                }
            } else {
                g_touches_moving = false;
            }

            /* Update the previous touch count. */
            g_prev_touch_count = current_touch.count;
        }

        void ChangeMenu(std::shared_ptr<Menu> menu) {
            g_current_menu = menu;
        }

        void ReturnToPreviousMenu() {
            /* Go to the previous menu if there is one. */
            if (g_current_menu->GetPrevMenu() != nullptr) {
                g_current_menu = g_current_menu->GetPrevMenu();
            }
        }

        Result IsPathBottomLevel(const char *path, bool *out) {
            Result rc = 0;
            FsFileSystem *fs;
            char translated_path[FS_MAX_PATH] = {};
            DBK_ABORT_UNLESS(fsdevTranslatePath(path, &fs, translated_path) != -1);

            FsDir dir;
            if (R_FAILED(rc = fsFsOpenDirectory(fs, translated_path, FsDirOpenMode_ReadDirs, &dir))) {
                return rc;
            }

            s64 entry_count;
            if (R_FAILED(rc = fsDirGetEntryCount(&dir, &entry_count))) {
                return rc;
            }

            *out = entry_count == 0;
            fsDirClose(&dir);
            return rc;
        }

        u32 EncodeVersion(u32 major, u32 minor, u32 micro, u32 relstep = 0) {
            return ((major & 0xFF) << 24) | ((minor & 0xFF) << 16) | ((micro & 0xFF) << 8) | ((relstep & 0xFF) << 8);
        }

    }

    void Menu::AddButton(u32 id, const char *text, float x, float y, float w, float h) {
        DBK_ABORT_UNLESS(id < MaxButtons);
        Button button = {
            .id = id,
            .selected = false,
            .enabled = true,
            .x = x,
            .y = y,
            .w = w,
            .h = h,
        };

        strncpy(button.text, text, sizeof(button.text)-1);
        m_buttons[id] = button;
    }

    void Menu::SetButtonSelected(u32 id, bool selected) {
        DBK_ABORT_UNLESS(id < MaxButtons);
        auto &button = m_buttons[id];

        if (button) {
            button->selected = selected;
        }
    }

    void Menu::DeselectAllButtons() {
        for (auto &button : m_buttons) {
            /* Ensure button is present. */
            if (!button) {
                continue;
            }
            button->selected = false;
        }
    }

    void Menu::SetButtonEnabled(u32 id, bool enabled) {
        DBK_ABORT_UNLESS(id < MaxButtons);
        auto &button = m_buttons[id];
        button->enabled = enabled;
    }

    Button *Menu::GetButton(u32 id) {
        DBK_ABORT_UNLESS(id < MaxButtons);
        return !m_buttons[id] ? nullptr : &(*m_buttons[id]);
    }

    Button *Menu::GetSelectedButton() {
        for (auto &button : m_buttons) {
            if (button && button->enabled && button->selected) {
                return &(*button);
            }
        }

        return nullptr;
    }

    Button *Menu::GetClosestButtonToSelection(Direction direction) {
        const Button *selected_button = this->GetSelectedButton();

        if (selected_button == nullptr || direction == Direction::Invalid) {
            return nullptr;
        }

        Button *closest_button = nullptr;
        float closest_distance = 0.0f;

        for (auto &button : m_buttons) {
            /* Skip absent button. */
            if (!button || !button->enabled) {
                continue;
            }

            /* Skip buttons that are in the wrong direction. */
            if ((direction == Direction::Down && button->y <= selected_button->y)  ||
                (direction == Direction::Up && button->y >= selected_button->y)    ||
                (direction == Direction::Right && button->x <= selected_button->x) ||
                (direction == Direction::Left && button->x >= selected_button->x)) {
                continue;
            }

            const float x_dist = button->x - selected_button->x;
            const float y_dist = button->y - selected_button->y;
            const float sq_dist = x_dist * x_dist + y_dist * y_dist;

            /* If we don't already have a closest button, set it. */
            if (closest_button == nullptr) {
                closest_button = &(*button);
                closest_distance = sq_dist;
                continue;
            }

            /* Update the closest button if this one is closer. */
            if (sq_dist < closest_distance) {
                closest_button = &(*button);
                closest_distance = sq_dist;
            }
        }

        return closest_button;
    }

    Button *Menu::GetTouchedButton() {
        HidTouchScreenState current_touch;
        hidGetTouchScreenStates(&current_touch, 1);
        const u32 touch_count = current_touch.count;

        for (u32 i = 0; i < touch_count && g_started_touching; i++) {
            for (auto &button : m_buttons) {
                if (button && button->enabled && button->IsPositionInBounds(current_touch.touches[i].x, current_touch.touches[i].y)) {
                    return &(*button);
                }
            }
        }

        return nullptr;
    }

    Button *Menu::GetActivatedButton() {
        Button *selected_button = this->GetSelectedButton();

        if (selected_button == nullptr) {
            return nullptr;
        }

        const u64 k_down = padGetButtonsDown(&g_pad);

        if (k_down & HidNpadButton_A || this->GetTouchedButton() == selected_button) {
            return selected_button;
        }

        return nullptr;
    }

    void Menu::UpdateButtons() {
        const u64 k_down = padGetButtonsDown(&g_pad);
        Direction direction = Direction::Invalid;

        if (k_down & HidNpadButton_AnyDown) {
            direction = Direction::Down;
        } else if (k_down & HidNpadButton_AnyUp) {
            direction = Direction::Up;
        } else if (k_down & HidNpadButton_AnyLeft) {
            direction = Direction::Left;
        } else if (k_down & HidNpadButton_AnyRight) {
            direction = Direction::Right;
        }

        /* Select the closest button. */
        if (const Button *closest_button = this->GetClosestButtonToSelection(direction); closest_button != nullptr) {
            this->DeselectAllButtons();
            this->SetButtonSelected(closest_button->id, true);
        }

        /* Select the touched button. */
        if (const Button *touched_button = this->GetTouchedButton(); touched_button != nullptr) {
            this->DeselectAllButtons();
            this->SetButtonSelected(touched_button->id, true);
        }
    }

    void Menu::DrawButtons(NVGcontext *vg, u64 ns) {
        for (auto &button : m_buttons) {
            /* Ensure button is present. */
            if (!button) {
                continue;
            }

            /* Set the button style. */
            auto style = ButtonStyle::StandardDisabled;
            if (button->enabled) {
                style = button->selected ? ButtonStyle::StandardSelected : ButtonStyle::Standard;
            }

            DrawButton(vg, button->text, button->x, button->y, button->w, button->h, style, ns);
        }
    }

    void Menu::LogText(const char *format, ...) {
        /* Create a temporary string. */
        char tmp[0x100];
        va_list args;
        va_start(args, format);
        vsnprintf(tmp, sizeof(tmp), format, args);
        va_end(args);

        /* Append the text to the log buffer. */
        strncat(m_log_buffer, tmp, sizeof(m_log_buffer)-1);
    }

    std::shared_ptr<Menu> Menu::GetPrevMenu() {
        return m_prev_menu;
    }

    AlertMenu::AlertMenu(std::shared_ptr<Menu> prev_menu, const char *text, const char *subtext, Result rc) : Menu(prev_menu), m_text{}, m_subtext{}, m_result_text{}, m_rc(rc){
        /* Copy the input text. */
        strncpy(m_text, text, sizeof(m_text)-1);
        strncpy(m_subtext, subtext, sizeof(m_subtext)-1);

        /* Copy result text if there is a result. */
        if (R_FAILED(rc)) {
            snprintf(m_result_text, sizeof(m_result_text), "Result: 0x%08x", rc);
        }
    }

    void AlertMenu::Draw(NVGcontext *vg, u64 ns) {
        const float window_height = WindowHeight + (R_FAILED(m_rc) ? SubTextHeight : 0.0f);
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - window_height / 2.0f;

        DrawWindow(vg, m_text, x, y, WindowWidth, window_height);
        DrawText(vg, x + HorizontalInset, y + TitleGap, WindowWidth - HorizontalInset * 2.0f, m_subtext);

        /* Draw the result if there is one. */
        if (R_FAILED(m_rc)) {
            DrawText(vg, x + HorizontalInset, y + TitleGap + SubTextHeight, WindowWidth - HorizontalInset * 2.0f, m_result_text);
        }

        this->DrawButtons(vg, ns);
    }

    ErrorMenu::ErrorMenu(const char *text, const char *subtext, Result rc) : AlertMenu(nullptr, text, subtext, rc)  {
        const float window_height = WindowHeight + (R_FAILED(m_rc) ? SubTextHeight : 0.0f);
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - window_height / 2.0f;
        const float button_y = y + TitleGap + SubTextHeight + VerticalGap * 2.0f + (R_FAILED(m_rc) ? SubTextHeight : 0.0f);
        const float button_width = WindowWidth - HorizontalInset * 2.0f;

        /* Add buttons. */
        this->AddButton(ExitButtonId, "Exit", x + HorizontalInset, button_y, button_width, ButtonHeight);
        this->SetButtonSelected(ExitButtonId, true);
    }

    void ErrorMenu::Update(u64 ns) {
        u64 k_down = padGetButtonsDown(&g_pad);

        /* Go back if B is pressed. */
        if (k_down & HidNpadButton_B) {
            g_exit_requested = true;
            return;
        }

        /* Take action if a button has been activated. */
        if (const Button *activated_button = this->GetActivatedButton(); activated_button != nullptr) {
            switch (activated_button->id) {
                case ExitButtonId:
                    g_exit_requested = true;
                    break;
            }
        }

        this->UpdateButtons();

        /* Fallback on selecting the exfat button. */
        if (const Button *selected_button = this->GetSelectedButton(); k_down && selected_button == nullptr) {
            this->SetButtonSelected(ExitButtonId, true);
        }
    }

    WarningMenu::WarningMenu(std::shared_ptr<Menu> prev_menu, std::shared_ptr<Menu> next_menu, const char *text, const char *subtext, Result rc) : AlertMenu(prev_menu, text, subtext, rc), m_next_menu(next_menu) {
        const float window_height = WindowHeight + (R_FAILED(m_rc) ? SubTextHeight : 0.0f);
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - window_height / 2.0f;

        const float button_y = y + TitleGap + SubTextHeight + VerticalGap * 2.0f + (R_FAILED(m_rc) ? SubTextHeight : 0.0f);
        const float button_width = (WindowWidth - HorizontalInset * 2.0f) / 2.0f - ButtonHorizontalGap;
        this->AddButton(BackButtonId, "Back", x + HorizontalInset, button_y, button_width, ButtonHeight);
        this->AddButton(ContinueButtonId, "Continue", x + HorizontalInset + button_width + ButtonHorizontalGap, button_y, button_width, ButtonHeight);
        this->SetButtonSelected(ContinueButtonId, true);
    }

    void WarningMenu::Update(u64 ns) {
        u64 k_down = padGetButtonsDown(&g_pad);

        /* Go back if B is pressed. */
        if (k_down & HidNpadButton_B) {
            ReturnToPreviousMenu();
            return;
        }

        /* Take action if a button has been activated. */
        if (const Button *activated_button = this->GetActivatedButton(); activated_button != nullptr) {
            switch (activated_button->id) {
                case BackButtonId:
                    ReturnToPreviousMenu();
                    return;
                case ContinueButtonId:
                    ChangeMenu(m_next_menu);
                    return;
            }
        }

        this->UpdateButtons();

        /* Fallback on selecting the exfat button. */
        if (const Button *selected_button = this->GetSelectedButton(); k_down && selected_button == nullptr) {
            this->SetButtonSelected(ContinueButtonId, true);
        }
    }

    MainMenu::MainMenu() : Menu(nullptr) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;

        this->AddButton(InstallButtonId, "Install", x + HorizontalInset, y + TitleGap, WindowWidth - HorizontalInset * 2, ButtonHeight);
        this->AddButton(ExitButtonId, "Exit", x + HorizontalInset, y + TitleGap + ButtonHeight + VerticalGap, WindowWidth - HorizontalInset * 2, ButtonHeight);
        this->SetButtonSelected(InstallButtonId, true);
    }

    void MainMenu::Update(u64 ns) {
        u64 k_down = padGetButtonsDown(&g_pad);

        if (k_down & HidNpadButton_B) {
            g_exit_requested = true;
        }

        /* Take action if a button has been activated. */
        if (const Button *activated_button = this->GetActivatedButton(); activated_button != nullptr) {
            switch (activated_button->id) {
                case InstallButtonId:
                {
                    const auto file_menu = std::make_shared<FileMenu>(g_current_menu, "/");

                    Result rc = 0;
                    u64 hardware_type;
                    u64 has_rcm_bug_patch;
                    u64 is_emummc;

                    if (R_FAILED(rc = splGetConfig(SplConfigItem_HardwareType, &hardware_type))) {
                        ChangeMenu(std::make_shared<ErrorMenu>("An error has occurred", "Failed to get hardware type.", rc));
                        return;
                    }

                    if (R_FAILED(rc = splGetConfig(static_cast<SplConfigItem>(ExosphereHasRcmBugPatch), &has_rcm_bug_patch))) {
                        ChangeMenu(std::make_shared<ErrorMenu>("An error has occurred", "Failed to check RCM bug status.", rc));
                        return;
                    }

                    if (R_FAILED(rc = splGetConfig(static_cast<SplConfigItem>(ExosphereEmummcType), &is_emummc))) {
                        ChangeMenu(std::make_shared<ErrorMenu>("An error has occurred", "Failed to check emuMMC status.", rc));
                        return;
                    }

                    /* Warn if we're working with a patched unit. */
                    const bool is_erista = hardware_type == 0 || hardware_type == 1;
                    if (is_erista && has_rcm_bug_patch && !is_emummc) {
                        ChangeMenu(std::make_shared<WarningMenu>(g_current_menu, file_menu, "Warning: Patched unit detected", "You may burn fuses or render your switch inoperable."));
                    } else {
                        ChangeMenu(file_menu);
                    }

                    return;
                }
                case ExitButtonId:
                    g_exit_requested = true;
                    return;
            }
        }

        this->UpdateButtons();

        /* Fallback on selecting the install button. */
        if (const Button *selected_button = this->GetSelectedButton(); k_down && selected_button == nullptr) {
            this->SetButtonSelected(InstallButtonId, true);
        }
    }

    void MainMenu::Draw(NVGcontext *vg, u64 ns) {
        DrawWindow(vg, "Daybreak", g_screen_width / 2.0f - WindowWidth / 2.0f, g_screen_height / 2.0f - WindowHeight / 2.0f, WindowWidth, WindowHeight);
        this->DrawButtons(vg, ns);
    }

    FileMenu::FileMenu(std::shared_ptr<Menu> prev_menu, const char *root) : Menu(prev_menu), m_current_index(0), m_scroll_offset(0), m_touch_start_scroll_offset(0), m_touch_finalize_selection(false) {
        Result rc = 0;

        strncpy(m_root, root, sizeof(m_root)-1);

        if (R_FAILED(rc = this->PopulateFileEntries())) {
            fatalThrow(rc);
        }
    }

    Result FileMenu::PopulateFileEntries() {
        /* Open the directory. */
        DIR *dir = opendir(m_root);
        if (dir == nullptr) {
            return fsdevGetLastResult();
        }

        /* Add file entries to the list. */
        struct dirent *ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_DIR) {
                FileEntry file_entry = {};
                strncpy(file_entry.name, ent->d_name, sizeof(file_entry.name));
                m_file_entries.push_back(file_entry);
            }
        }

        /* Close the directory. */
        closedir(dir);

        /* Sort the file entries. */
        std::sort(m_file_entries.begin(), m_file_entries.end(), [](const FileEntry &a, const FileEntry &b) {
            return strncmp(a.name, b.name, sizeof(a.name)) < 0;
        });

        return 0;
    }

    bool FileMenu::IsSelectionVisible() {
        const float visible_start = m_scroll_offset;
        const float visible_end = visible_start + FileListHeight;
        const float entry_start = static_cast<float>(m_current_index) * (FileRowHeight + FileRowGap);
        const float entry_end = entry_start + (FileRowHeight + FileRowGap);
        return entry_start >= visible_start && entry_end <= visible_end;
    }

    void FileMenu::ScrollToSelection() {
        const float visible_start = m_scroll_offset;
        const float visible_end = visible_start + FileListHeight;
        const float entry_start = static_cast<float>(m_current_index) * (FileRowHeight + FileRowGap);
        const float entry_end = entry_start + (FileRowHeight + FileRowGap);

        if (entry_end > visible_end) {
            m_scroll_offset += entry_end - visible_end;
        } else if (entry_end < visible_end) {
            m_scroll_offset = entry_start;
        }
    }

    bool FileMenu::IsEntryTouched(u32 i) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;

        HidTouchScreenState current_touch;
        hidGetTouchScreenStates(&current_touch, 1);

        /* Check if the tap is within the x bounds. */
        if (current_touch.touches[0].x >= x + TextBackgroundOffset + FileRowHorizontalInset && current_touch.touches[0].x <= WindowWidth - (TextBackgroundOffset + FileRowHorizontalInset) * 2.0f) {
            const float y_min = y + TitleGap + FileRowGap + i * (FileRowHeight + FileRowGap) - m_scroll_offset;
            const float y_max = y_min + FileRowHeight;

            /* Check if the tap is within the y bounds. */
            if (current_touch.touches[0].y >= y_min && current_touch.touches[0].y <= y_max) {
                return true;
            }
        }

        return false;
    }

    void FileMenu::UpdateTouches() {
        /* Setup values on initial touch. */
        if (g_started_touching) {
            m_touch_start_scroll_offset = m_scroll_offset;

            /* We may potentially finalize the selection later if we start off touching it. */
            if (this->IsEntryTouched(m_current_index)) {
                m_touch_finalize_selection = true;
            }
        }

        /* Scroll based on touch movement. */
        if (g_touches_moving) {
            HidTouchScreenState current_touch;
            hidGetTouchScreenStates(&current_touch, 1);

            const int dist_y = current_touch.touches[0].y - g_start_touch.touches[0].y;
            float new_scroll_offset = m_touch_start_scroll_offset - static_cast<float>(dist_y);
            float max_scroll = (FileRowHeight + FileRowGap) * static_cast<float>(m_file_entries.size()) - FileListHeight;

            /* Don't allow scrolling if there is not enough elements. */
            if (max_scroll < 0.0f) {
                max_scroll = 0.0f;
            }

            /* Don't allow scrolling before the first element. */
            if (new_scroll_offset < 0.0f) {
                new_scroll_offset = 0.0f;
            }

            /* Don't allow scrolling past the last element. */
            if (new_scroll_offset > max_scroll) {
                new_scroll_offset = max_scroll;
            }

            m_scroll_offset = new_scroll_offset;
        }

        /* Select any tapped entries. */
        if (g_tapping) {
            for (u32 i = 0; i < m_file_entries.size(); i++) {
                if (this->IsEntryTouched(i)) {
                    /* The current index is checked later. */
                    if (i == m_current_index) {
                        continue;
                    }

                    m_current_index = i;

                    /* Don't finalize selection if we touch something else. */
                    m_touch_finalize_selection = false;
                    break;
                }
            }
        }

        /* Don't finalize selection if we aren't finished and we've either stopped tapping or are no longer touching the selection. */
        if (!g_finished_touching && (!g_tapping || !this->IsEntryTouched(m_current_index))) {
            m_touch_finalize_selection = false;
        }

        /* Finalize selection if the currently selected entry is touched for the second time. */
        if (g_finished_touching && m_touch_finalize_selection) {
            this->FinalizeSelection();
            m_touch_finalize_selection = false;
        }
    }

    void FileMenu::FinalizeSelection() {
        DBK_ABORT_UNLESS(m_current_index < m_file_entries.size());
        FileEntry &entry = m_file_entries[m_current_index];

        /* Determine the selected path. */
        char current_path[FS_MAX_PATH] = {};
        const int path_len = snprintf(current_path, sizeof(current_path), "%s%s/", m_root, entry.name);
        DBK_ABORT_UNLESS(path_len >= 0 && path_len < static_cast<int>(sizeof(current_path)));

        /* Determine if the chosen path is the bottom level. */
        Result rc = 0;
        bool bottom_level;
        if (R_FAILED(rc = IsPathBottomLevel(current_path, &bottom_level))) {
            fatalThrow(rc);
        }

        /* Show exfat settings or the next file menu. */
        if (bottom_level) {
            /* Set the update path. */
            snprintf(g_update_path, sizeof(g_update_path), "%s", current_path);

            /* Change the menu. */
            ChangeMenu(std::make_shared<ValidateUpdateMenu>(g_current_menu));
        } else {
            ChangeMenu(std::make_shared<FileMenu>(g_current_menu, current_path));
        }
    }

    void FileMenu::Update(u64 ns) {
        u64 k_down = padGetButtonsDown(&g_pad);

        /* Go back if B is pressed. */
        if (k_down & HidNpadButton_B) {
            ReturnToPreviousMenu();
            return;
        }

        /* Finalize selection on pressing A. */
        if (k_down & HidNpadButton_A) {
            this->FinalizeSelection();
        }

        /* Update touch input. */
        this->UpdateTouches();

        const u32 prev_index = m_current_index;

        if (k_down & HidNpadButton_AnyDown) {
            /* Scroll down. */
            if (m_current_index >= (m_file_entries.size() - 1)) {
                m_current_index = 0;
            } else {
                m_current_index++;
            }
        } else if (k_down & HidNpadButton_AnyUp) {
            /* Scroll up. */
            if (m_current_index == 0) {
                m_current_index = m_file_entries.size() - 1;
            } else {
                m_current_index--;
            }
        }

        /* Scroll to the selection if it isn't visible. */
        if (prev_index != m_current_index && !this->IsSelectionVisible()) {
            this->ScrollToSelection();
        }
    }

    void FileMenu::Draw(NVGcontext *vg, u64 ns) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;

        DrawWindow(vg, "Select an update directory", x, y, WindowWidth, WindowHeight);
        DrawTextBackground(vg, x + TextBackgroundOffset, y + TitleGap, WindowWidth - TextBackgroundOffset * 2.0f, (FileRowHeight + FileRowGap) * MaxFileRows + FileRowGap);

        nvgSave(vg);
        nvgScissor(vg, x + TextBackgroundOffset, y + TitleGap, WindowWidth - TextBackgroundOffset * 2.0f, (FileRowHeight + FileRowGap) * MaxFileRows + FileRowGap);

        for (u32 i = 0; i < m_file_entries.size(); i++) {
            FileEntry &entry = m_file_entries[i];
            auto style = ButtonStyle::FileSelect;

            if (i == m_current_index) {
                style = ButtonStyle::FileSelectSelected;
            }

            DrawButton(vg, entry.name, x + TextBackgroundOffset + FileRowHorizontalInset, y + TitleGap + FileRowGap + i * (FileRowHeight + FileRowGap) - m_scroll_offset, WindowWidth - (TextBackgroundOffset + FileRowHorizontalInset) * 2.0f, FileRowHeight, style, ns);
        }

        nvgRestore(vg);
    }

    ValidateUpdateMenu::ValidateUpdateMenu(std::shared_ptr<Menu> prev_menu) : Menu(prev_menu), m_has_drawn(false), m_has_info(false), m_has_validated(false) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;
        const float button_width = (WindowWidth - HorizontalInset * 2.0f) / 2.0f - ButtonHorizontalGap;

        /* Add buttons. */
        this->AddButton(BackButtonId, "Back", x + HorizontalInset, y + WindowHeight - BottomInset - ButtonHeight, button_width, ButtonHeight);
        this->AddButton(ContinueButtonId, "Continue", x + HorizontalInset + button_width + ButtonHorizontalGap, y + WindowHeight - BottomInset - ButtonHeight, button_width, ButtonHeight);
        this->SetButtonEnabled(BackButtonId, false);
        this->SetButtonEnabled(ContinueButtonId, false);

        /* Obtain update information. */
        if (R_FAILED(this->GetUpdateInformation())) {
            this->SetButtonEnabled(BackButtonId, true);
            this->SetButtonSelected(BackButtonId, true);
        } else {
            /* Log this early so it is printed out before validation causes stalling. */
            this->LogText("Validating update, this may take a moment...\n");
        }
    }

    Result ValidateUpdateMenu::GetUpdateInformation() {
        Result rc = 0;
        this->LogText("Directory %s\n", g_update_path);

        /* Attempt to get the update information. */
        if (R_FAILED(rc = amssuGetUpdateInformation(&m_update_info, g_update_path))) {
            if (rc == 0x1a405) {
                this->LogText("No update found in folder.\nEnsure your ncas are named correctly!\nResult: 0x%08x\n", rc);
            } else {
                this->LogText("Failed to get update information.\nResult: 0x%08x\n", rc);
            }
            return rc;
        }

        /* Print update information. */
        this->LogText("- Version: %d.%d.%d\n", (m_update_info.version >> 26) & 0x1f, (m_update_info.version >> 20) & 0x1f, (m_update_info.version >> 16) & 0xf);
        if (m_update_info.exfat_supported) {
            this->LogText("- exFAT: Supported\n");
        } else {
            this->LogText("- exFAT: Unsupported\n");
        }
        this->LogText("- Firmware variations: %d\n", m_update_info.num_firmware_variations);

        /* Mark as having obtained update info. */
        m_has_info = true;
        return rc;
    }

    void ValidateUpdateMenu::ValidateUpdate() {
        Result rc = 0;

        /* Validate the update. */
        if (R_FAILED(rc = amssuValidateUpdate(&m_validation_info, g_update_path))) {
            this->LogText("Failed to validate update.\nResult: 0x%08x\n", rc);
            return;
        }

        /* Check the result. */
        if (R_SUCCEEDED(m_validation_info.result)) {
            this->LogText("Update is valid!\n");

            if (R_FAILED(m_validation_info.exfat_result)) {
                const u32 version = m_validation_info.invalid_key.version;
                this->LogText("exFAT Validation failed with result: 0x%08x\n", m_validation_info.exfat_result);
                this->LogText("Missing content:\n- Program id: %016lx\n- Version: %d.%d.%d\n", m_validation_info.invalid_key.id, (version >> 26) & 0x1f, (version >> 20) & 0x1f, (version >> 16) & 0xf);

                /* Log the missing content id. */
                this->LogText("- Content id: ");
                for (size_t i = 0; i < sizeof(NcmContentId); i++) {
                    this->LogText("%02x", m_validation_info.invalid_content_id.c[i]);
                }
                this->LogText("\n");
            }

            /* Enable the back and continue buttons and select the continue button. */
            this->SetButtonEnabled(BackButtonId, true);
            this->SetButtonEnabled(ContinueButtonId, true);
            this->SetButtonSelected(ContinueButtonId, true);
        } else {
            /* Log the missing content info. */
            const u32 version = m_validation_info.invalid_key.version;
            this->LogText("Validation failed with result: 0x%08x\n", m_validation_info.result);
            this->LogText("Missing content:\n- Program id: %016lx\n- Version: %d.%d.%d\n", m_validation_info.invalid_key.id, (version >> 26) & 0x1f, (version >> 20) & 0x1f, (version >> 16) & 0xf);

            /* Log the missing content id. */
            this->LogText("- Content id: ");
            for (size_t i = 0; i < sizeof(NcmContentId); i++) {
                this->LogText("%02x", m_validation_info.invalid_content_id.c[i]);
            }
            this->LogText("\n");

            /* Enable the back button and select it. */
            this->SetButtonEnabled(BackButtonId, true);
            this->SetButtonSelected(BackButtonId, true);
        }

        /* Mark validation as being complete. */
        m_has_validated = true;
    }

    void ValidateUpdateMenu::Update(u64 ns) {
        /* Perform validation if it hasn't been done already. */
        if (m_has_info && m_has_drawn && !m_has_validated) {
            this->ValidateUpdate();
        }

        u64 k_down = padGetButtonsDown(&g_pad);

        /* Go back if B is pressed. */
        if (k_down & HidNpadButton_B) {
            ReturnToPreviousMenu();
            return;
        }

        /* Take action if a button has been activated. */
        if (const Button *activated_button = this->GetActivatedButton(); activated_button != nullptr) {
            switch (activated_button->id) {
                case BackButtonId:
                    ReturnToPreviousMenu();
                    return;
                case ContinueButtonId:
                    /* Don't continue if validation hasn't been done or has failed. */
                    if (!m_has_validated || R_FAILED(m_validation_info.result)) {
                        break;
                    }

                    /* Check if exfat is supported. */
                    g_exfat_supported = m_update_info.exfat_supported && R_SUCCEEDED(m_validation_info.exfat_result);
                    if (!g_exfat_supported) {
                        g_use_exfat = false;
                    }

                    /* Create the next menu. */
                    std::shared_ptr<Menu> next_menu = std::make_shared<ChooseResetMenu>(g_current_menu);

                    /* Warn the user if they're updating with exFAT supposed to be supported but not present/corrupted. */
                    if (m_update_info.exfat_supported && R_FAILED(m_validation_info.exfat_result)) {
                        next_menu = std::make_shared<WarningMenu>(g_current_menu, next_menu, "Warning: exFAT firmware is missing or corrupt", "Are you sure you want to proceed?");
                    }

                    /* Warn the user if they're updating to a version higher than supported. */
                    const u32 version = m_validation_info.invalid_key.version;
                    if (EncodeVersion((version >> 26) & 0x1f, (version >> 20) & 0x1f, (version >> 16) & 0xf) > g_supported_version) {
                        next_menu = std::make_shared<WarningMenu>(g_current_menu, next_menu, "Warning: firmware is too new and not known to be supported", "Are you sure you want to proceed?");
                    }

                    /* Change to the next menu. */
                    ChangeMenu(next_menu);
                    return;
            }
        }

        this->UpdateButtons();
    }

    void ValidateUpdateMenu::Draw(NVGcontext *vg, u64 ns) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;

        DrawWindow(vg, "Update information", x, y, WindowWidth, WindowHeight);
        DrawTextBackground(vg, x + HorizontalInset, y + TitleGap, WindowWidth - HorizontalInset * 2.0f, TextAreaHeight);
        DrawTextBlock(vg, m_log_buffer, x + HorizontalInset + TextHorizontalInset, y + TitleGap + TextVerticalInset, WindowWidth - (HorizontalInset + TextHorizontalInset) * 2.0f, TextAreaHeight - TextVerticalInset * 2.0f);

        this->DrawButtons(vg, ns);
        m_has_drawn = true;
    }

    ChooseResetMenu::ChooseResetMenu(std::shared_ptr<Menu> prev_menu) : Menu(prev_menu) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;
        const float button_width = (WindowWidth - HorizontalInset * 2.0f) / 2.0f - ButtonHorizontalGap;

        /* Add buttons. */
        this->AddButton(ResetToFactorySettingsButtonId, "Reset to factory settings", x + HorizontalInset, y + TitleGap, button_width, ButtonHeight);
        this->AddButton(PreserveSettingsButtonId, "Preserve settings", x + HorizontalInset + button_width + ButtonHorizontalGap, y + TitleGap, button_width, ButtonHeight);
        this->SetButtonSelected(PreserveSettingsButtonId, true);
    }

    void ChooseResetMenu::Update(u64 ns) {
        u64 k_down = padGetButtonsDown(&g_pad);

        /* Go back if B is pressed. */
        if (k_down & HidNpadButton_B) {
            ReturnToPreviousMenu();
            return;
        }

        /* Take action if a button has been activated. */
        if (const Button *activated_button = this->GetActivatedButton(); activated_button != nullptr) {
            switch (activated_button->id) {
                case ResetToFactorySettingsButtonId:
                    g_reset_to_factory = true;
                    break;
                case PreserveSettingsButtonId:
                    g_reset_to_factory = false;
                    break;
            }

            std::shared_ptr<Menu> next_menu;

            if (g_exfat_supported) {
                next_menu = std::make_shared<ChooseExfatMenu>(g_current_menu);
            } else {
                next_menu = std::make_shared<WarningMenu>(g_current_menu, std::make_shared<InstallUpdateMenu>(g_current_menu), "Ready to begin update installation", "Are you sure you want to proceed?");
            }

            if (g_reset_to_factory) {
                ChangeMenu(std::make_shared<WarningMenu>(g_current_menu, next_menu, "Warning: Factory reset selected", "Saves and installed games will be permanently deleted."));
            } else {
                ChangeMenu(next_menu);
            }
        }

        this->UpdateButtons();

        /* Fallback on selecting the exfat button. */
        if (const Button *selected_button = this->GetSelectedButton(); k_down && selected_button == nullptr) {
            this->SetButtonSelected(PreserveSettingsButtonId, true);
        }
    }

    void ChooseResetMenu::Draw(NVGcontext *vg, u64 ns) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;

        DrawWindow(vg, "Select settings mode", x, y, WindowWidth, WindowHeight);
        this->DrawButtons(vg, ns);
    }

    ChooseExfatMenu::ChooseExfatMenu(std::shared_ptr<Menu> prev_menu) : Menu(prev_menu) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;
        const float button_width = (WindowWidth - HorizontalInset * 2.0f) / 2.0f - ButtonHorizontalGap;

        /* Add buttons. */
        this->AddButton(Fat32ButtonId, "Install (FAT32)", x + HorizontalInset, y + TitleGap, button_width, ButtonHeight);
        this->AddButton(ExFatButtonId, "Install (FAT32 + exFAT)", x + HorizontalInset + button_width + ButtonHorizontalGap, y + TitleGap, button_width, ButtonHeight);

        /* Set the default selected button based on the user's current install. We aren't particularly concerned if fsIsExFatSupported fails. */
        bool exfat_supported = false;
        fsIsExFatSupported(&exfat_supported);

        if (exfat_supported) {
            this->SetButtonSelected(ExFatButtonId, true);
        } else {
            this->SetButtonSelected(Fat32ButtonId, true);
        }
    }

    void ChooseExfatMenu::Update(u64 ns) {
        u64 k_down = padGetButtonsDown(&g_pad);

        /* Go back if B is pressed. */
        if (k_down & HidNpadButton_B) {
            ReturnToPreviousMenu();
            return;
        }

        /* Take action if a button has been activated. */
        if (const Button *activated_button = this->GetActivatedButton(); activated_button != nullptr) {
            switch (activated_button->id) {
                case Fat32ButtonId:
                    g_use_exfat = false;
                    break;
                case ExFatButtonId:
                    g_use_exfat = true;
                    break;
            }

            ChangeMenu(std::make_shared<WarningMenu>(g_current_menu, std::make_shared<InstallUpdateMenu>(g_current_menu), "Ready to begin update installation", "Are you sure you want to proceed?"));
        }

        this->UpdateButtons();

        /* Fallback on selecting the exfat button. */
        if (const Button *selected_button = this->GetSelectedButton(); k_down && selected_button == nullptr) {
            this->SetButtonSelected(ExFatButtonId, true);
        }
    }

    void ChooseExfatMenu::Draw(NVGcontext *vg, u64 ns) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;

        DrawWindow(vg, "Select driver variant", x, y, WindowWidth, WindowHeight);
        this->DrawButtons(vg, ns);
    }

    InstallUpdateMenu::InstallUpdateMenu(std::shared_ptr<Menu> prev_menu) : Menu(prev_menu), m_install_state(InstallState::NeedsDraw), m_progress_percent(0.0f) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;
        const float button_width = (WindowWidth - HorizontalInset * 2.0f) / 2.0f - ButtonHorizontalGap;

        /* Add buttons. */
        this->AddButton(ShutdownButtonId, "Shutdown", x + HorizontalInset, y + WindowHeight - BottomInset - ButtonHeight, button_width, ButtonHeight);
        this->AddButton(RebootButtonId, "Reboot", x + HorizontalInset + button_width + ButtonHorizontalGap, y + WindowHeight - BottomInset - ButtonHeight, button_width, ButtonHeight);
        this->SetButtonEnabled(ShutdownButtonId, false);
        this->SetButtonEnabled(RebootButtonId, false);

        /* Prevent the home button from being pressed during installation. */
        hiddbgDeactivateHomeButton();
    }

    void InstallUpdateMenu::MarkForReboot() {
        this->SetButtonEnabled(ShutdownButtonId, true);
        this->SetButtonEnabled(RebootButtonId, true);
        this->SetButtonSelected(RebootButtonId, true);
        m_install_state = InstallState::AwaitingReboot;
    }

    Result InstallUpdateMenu::TransitionUpdateState() {
        Result rc = 0;
        if (m_install_state == InstallState::NeedsSetup) {
            /* Setup the update. */
            if (R_FAILED(rc = amssuSetupUpdate(nullptr, UpdateTaskBufferSize, g_update_path, g_use_exfat))) {
                this->LogText("Failed to setup update.\nResult: 0x%08x\n", rc);
                this->MarkForReboot();
                return rc;
            }

            /* Log setup completion. */
            this->LogText("Update setup complete.\n");
            m_install_state = InstallState::NeedsPrepare;
        } else if (m_install_state == InstallState::NeedsPrepare) {
            /* Request update preparation. */
            if (R_FAILED(rc = amssuRequestPrepareUpdate(&m_prepare_result))) {
                this->LogText("Failed to request update preparation.\nResult: 0x%08x\n", rc);
                this->MarkForReboot();
                return rc;
            }

            /* Log awaiting prepare. */
            this->LogText("Preparing update...\n");
            m_install_state = InstallState::AwaitingPrepare;
        } else if (m_install_state == InstallState::AwaitingPrepare) {
            /* Check if preparation has a result. */
            if (R_FAILED(rc = asyncResultWait(&m_prepare_result, 0)) && rc != 0xea01) {
                this->LogText("Failed to check update preparation result.\nResult: 0x%08x\n", rc);
                this->MarkForReboot();
                return rc;
            } else if (R_SUCCEEDED(rc)) {
                if (R_FAILED(rc = asyncResultGet(&m_prepare_result))) {
                    this->LogText("Failed to prepare update.\nResult: 0x%08x\n", rc);
                    this->MarkForReboot();
                    return rc;
                }
            }

            /* Check if the update has been prepared. */
            bool prepared;
            if (R_FAILED(rc = amssuHasPreparedUpdate(&prepared))) {
                this->LogText("Failed to check if update has been prepared.\nResult: 0x%08x\n", rc);
                this->MarkForReboot();
                return rc;
            }

            /* Mark for application if preparation complete. */
            if (prepared) {
                this->LogText("Update preparation complete.\nApplying update...\n");
                m_install_state = InstallState::NeedsApply;
                return rc;
            }

            /* Check update progress. */
            NsSystemUpdateProgress update_progress = {};
            if (R_FAILED(rc = amssuGetPrepareUpdateProgress(&update_progress))) {
                this->LogText("Failed to check update progress.\nResult: 0x%08x\n", rc);
                this->MarkForReboot();
                return rc;
            }

            /* Update progress percent. */
            if (update_progress.total_size > 0.0f) {
                m_progress_percent = static_cast<float>(update_progress.current_size) / static_cast<float>(update_progress.total_size);
            } else {
                m_progress_percent = 0.0f;
            }
        } else if (m_install_state == InstallState::NeedsApply) {
            /* Apply the prepared update. */
            if (R_FAILED(rc = amssuApplyPreparedUpdate())) {
                this->LogText("Failed to apply update.\nResult: 0x%08x\n", rc);
            } else {
                /* Log success. */
                this->LogText("Update applied successfully.\n");

                if (g_reset_to_factory) {
                    if (R_FAILED(rc = nsResetToFactorySettingsForRefurbishment())) {
                        /* Fallback on ResetToFactorySettings. */
                        if (rc == MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer)) {
                            if (R_FAILED(rc = nsResetToFactorySettings())) {
                                this->LogText("Failed to reset to factory settings.\nResult: 0x%08x\n", rc);
                                this->MarkForReboot();
                                return rc;
                            }
                        } else {
                            this->LogText("Failed to reset to factory settings for refurbishment.\nResult: 0x%08x\n", rc);
                            this->MarkForReboot();
                            return rc;
                        }
                    }

                    this->LogText("Successfully reset to factory settings.\n", rc);
                }
            }

            this->MarkForReboot();
            return rc;
        }

        return rc;
    }

    void InstallUpdateMenu::Update(u64 ns) {
        /* Transition to the next update state. */
        if (m_install_state != InstallState::NeedsDraw && m_install_state != InstallState::AwaitingReboot) {
            this->TransitionUpdateState();
        }

        /* Take action if a button has been activated. */
        if (const Button *activated_button = this->GetActivatedButton(); activated_button != nullptr) {
            switch (activated_button->id) {
                case ShutdownButtonId:
                    if (R_FAILED(appletRequestToShutdown())) {
                        spsmShutdown(false);
                    }
                    break;
                case RebootButtonId:
                    if (R_FAILED(appletRequestToReboot())) {
                        spsmShutdown(true);
                    }
                    break;
            }
        }

        this->UpdateButtons();
    }

    void InstallUpdateMenu::Draw(NVGcontext *vg, u64 ns) {
        const float x = g_screen_width / 2.0f - WindowWidth / 2.0f;
        const float y = g_screen_height / 2.0f - WindowHeight / 2.0f;

        DrawWindow(vg, "Installing update", x, y, WindowWidth, WindowHeight);
        DrawProgressText(vg, x + HorizontalInset, y + TitleGap, m_progress_percent);
        DrawProgressBar(vg, x + HorizontalInset, y + TitleGap + ProgressTextHeight, WindowWidth - HorizontalInset * 2.0f, ProgressBarHeight, m_progress_percent);
        DrawTextBackground(vg, x + HorizontalInset, y + TitleGap + ProgressTextHeight + ProgressBarHeight + VerticalGap, WindowWidth - HorizontalInset * 2.0f, TextAreaHeight);
        DrawTextBlock(vg, m_log_buffer, x + HorizontalInset + TextHorizontalInset, y + TitleGap + ProgressTextHeight + ProgressBarHeight + VerticalGap + TextVerticalInset, WindowWidth - (HorizontalInset + TextHorizontalInset) * 2.0f, TextAreaHeight - TextVerticalInset * 2.0f);

        this->DrawButtons(vg, ns);

        /* We have drawn now, allow setup to occur. */
        if (m_install_state == InstallState::NeedsDraw) {
            this->LogText("Beginning update setup...\n");
            m_install_state = InstallState::NeedsSetup;
        }
    }

    bool InitializeMenu(u32 screen_width, u32 screen_height) {
        Result rc = 0;

        /* Configure and initialize the gamepad. */
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&g_pad);

        /* Initialize the touch screen. */
        hidInitializeTouchScreen();

        /* Set the screen width and height. */
        g_screen_width = screen_width;
        g_screen_height = screen_height;

        /* Mark as initialized. */
        g_initialized = true;

        /* Attempt to get the exosphere version. */
        u64 version;
        if (R_FAILED(rc = splGetConfig(static_cast<SplConfigItem>(ExosphereApiVersionConfigItem), &version))) {
            ChangeMenu(std::make_shared<ErrorMenu>("Atmosphere not found", "Daybreak requires Atmosphere to be installed.", rc));
            return false;
        }

        const u32 version_micro = (version >> 40) & 0xff;
        const u32 version_minor = (version >> 48) & 0xff;
        const u32 version_major = (version >> 56) & 0xff;

        /* Validate the exosphere version. */
        const bool ams_supports_sysupdate_api = EncodeVersion(version_major, version_minor, version_micro) >= EncodeVersion(0, 14, 0);
        if (!ams_supports_sysupdate_api) {
            ChangeMenu(std::make_shared<ErrorMenu>("Outdated Atmosphere version", "Daybreak requires Atmosphere 0.14.0 or later.", rc));
            return false;
        }

        /* Ensure DayBreak is ran as a NRO. */
        if (envIsNso()) {
            ChangeMenu(std::make_shared<ErrorMenu>("Unsupported Environment", "Please launch Daybreak via the Homebrew menu.", rc));
            return false;
        }

        /* Attempt to get the supported version. */
        if (R_SUCCEEDED(rc = splGetConfig(static_cast<SplConfigItem>(ExosphereSupportedHosVersion), &version))) {
            g_supported_version = static_cast<u32>(version);
        }

        /* Initialize ams:su. */
        if (R_FAILED(rc = amssuInitialize())) {
            fatalThrow(rc);
        }

        /* Change the current menu to the main menu. */
        g_current_menu = std::make_shared<MainMenu>();

        return true;
    }

    bool InitializeMenu(u32 screen_width, u32 screen_height, const char *update_path) {
        if (InitializeMenu(screen_width, screen_height)) {

            /* Set the update path. */
            strncpy(g_update_path, update_path, sizeof(g_update_path)-1);

            /* Change the menu. */
            ChangeMenu(std::make_shared<ValidateUpdateMenu>(g_current_menu));

            return true;
        }

        return false;
    }

    void UpdateMenu(u64 ns) {
        DBK_ABORT_UNLESS(g_initialized);
        DBK_ABORT_UNLESS(g_current_menu != nullptr);
        UpdateInput();
        g_current_menu->Update(ns);
    }

    void RenderMenu(NVGcontext *vg, u64 ns) {
        DBK_ABORT_UNLESS(g_initialized);
        DBK_ABORT_UNLESS(g_current_menu != nullptr);

        /* Draw background. */
        DrawBackground(vg, g_screen_width, g_screen_height);

        /* Draw stars. */
        DrawStar(vg, 40.0f, 64.0f, 3.0f);
        DrawStar(vg, 110.0f, 300.0f, 3.0f);
        DrawStar(vg, 200.0f, 150.0f, 4.0f);
        DrawStar(vg, 370.0f, 280.0f, 3.0f);
        DrawStar(vg, 450.0f, 40.0f, 3.5f);
        DrawStar(vg, 710.0f, 90.0f, 3.0f);
        DrawStar(vg, 900.0f, 240.0f, 3.0f);
        DrawStar(vg, 970.0f, 64.0f, 4.0f);
        DrawStar(vg, 1160.0f, 160.0f, 3.5f);
        DrawStar(vg, 1210.0f, 350.0f, 3.0f);

        g_current_menu->Draw(vg, ns);
    }

    bool IsExitRequested() {
        return g_exit_requested;
    }

}
