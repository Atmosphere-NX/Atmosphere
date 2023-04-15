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
#include <haze.hpp>
#include <haze/console_main_loop.hpp>

int main(int argc, char **argv) {
    haze::PtpObjectHeap ptp_object_heap;

    haze::EventReactor event_reactor;
    haze::PtpResponder ptp_responder;
    haze::ConsoleMainLoop console_main_loop;

    consoleInit(NULL);
    appletSetAutoSleepDisabled(true);

    ptp_responder.Initialize(std::addressof(event_reactor), std::addressof(ptp_object_heap));
    console_main_loop.Initialize(std::addressof(event_reactor), std::addressof(ptp_object_heap));

    while (!event_reactor.GetStopRequested()) {
        ptp_responder.HandleRequest();
    }

    console_main_loop.Finalize();
    ptp_responder.Finalize();

    appletSetAutoSleepDisabled(false);
    consoleExit(NULL);
    return 0;
}
