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

struct FileRow: View {
    let name: String
    let folder: Bool

    var body: some View {
        // List a file/folder with an icon and name
        HStack {
            Image(systemName: folder ? "folder.fill" : "document.fill")
                .font(.largeTitle)
            Text(name)
            Spacer()
        }
    }
}

struct FileBrowser: View {
    @Binding var running: Bool
    @Binding var path: String
    @State var path2: String
    @State var contents: [String]
    let manager = FileManager.default

    init(running: Binding<Bool>, path: Binding<String>) {
        // Initialize values for the base directory
        let docsUrl = manager.urls(for: .documentDirectory, in: .userDomainMask).first!
        _running = running
        _path = path;
        path2 = docsUrl.path
        contents = try! manager.contentsOfDirectory(atPath: docsUrl.path).sorted()
    }

    var body: some View {
        // Show a list of folders and ROMs at the current directory
        List(contents, id: \.self) { content in
            var isDir = false as ObjCBool
            let folder = manager.fileExists(atPath: path2 + "/" + content, isDirectory: &isDir) && isDir.boolValue
            if (folder || content.hasSuffix(".nds") || content.hasSuffix(".gba")) {
                FileRow(name: content, folder: folder).onTapGesture {
                    // Navigate to a selected folder or run a selected ROM
                    path2 += "/" + content
                    if folder {
                        contents = try! manager.contentsOfDirectory(atPath: path2).sorted()
                    }
                    else {
                        running = true
                        path = path2
                    }
                }
            }
        }
    }
}
