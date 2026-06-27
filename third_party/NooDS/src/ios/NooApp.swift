/*
    Copyright 2019-2025 Hydr8gon

    This file is part of NooDS.

    NooDS is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    NooDS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NooDS. If not, see <https://www.gnu.org/licenses/>.
*/

import SwiftUI

@main
struct NooApp: App {
    @State var running = false
    @State var path = String()

    init() {
        // Initialize settings using the app's documents folder
        let docsUrl = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        loadSettings(docsUrl.path + "/noods")
    }

    var body: some Scene {
        // Choose the current window based on run state
        WindowGroup {
            if running {
                NooView(path: path)
            }
            else {
                FileBrowser(running: $running, path: $path)
            }
        }
    }
}
