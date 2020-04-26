# fatal
This module is a reimplementation of the Horizon OS's `fatal` system module, which is responsible for managing fatal reports.

Atmosphère's reimplementation prevents error report creation and draws a custom error screen, showing registers and a backtrace. It also attempts to gather debugging info for any and all crashes and tries to save reports to the SD card under the folder `/atmosphere/fatal_reports/`.
