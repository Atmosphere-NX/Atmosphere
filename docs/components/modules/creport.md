# creport
This module is a reimplementation of the Horizon OS's `creport` system module, which is responsible for managing crash reports.

Atmosphère's reimplementation redirects writing of generated crash reports to the SD card under the folder `/atmosphere/crash_reports/`. It also prevents the automatic uploading of said crash reports.
