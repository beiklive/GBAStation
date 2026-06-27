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

struct NooView: View {
    var core: CoreWrap
    let space = CGColorSpaceCreateDeviceRGB()
    let info = CGBitmapInfo(alpha: .noneSkipLast, component: .integer, byteOrder: .orderDefault)
    let provider: CGDataProvider

    init(path: String) {
        // Initialize the core and framebuffer data provider
        core = CoreWrap(path)
        var callbacks = CGDataProviderSequentialCallbacks(version: 0, getBytes: bytesCb, skipForward: forwardCb, rewind: rewindCb, releaseInfo: nil)
        provider = CGDataProvider(sequentialInfo: nil, callbacks: &callbacks)!
    }

    var body: some View {
        // Redraw the framebuffer on a canvas at 60 FPS
        TimelineView(.periodic(from: Date(), by: 1.0 / 60)) { timeCtx in
            let buffer = CGImage(width: 256, height: 384, bitsPerComponent: 8, bitsPerPixel: 32, bytesPerRow: 256 * 4,
                space: space, bitmapInfo: info, provider: provider, decode: nil, shouldInterpolate: false, intent: .defaultIntent)!
            Canvas { context, size in
                let image = Image(decorative: buffer, scale: 1.0, orientation: .up)
                context.draw(image, in: CGRect(x: 0, y: 0, width: size.width, height: 384 * size.width / 256), style: FillStyle())
            }
            Text(String("\(core.getFps()) FPS")).foregroundColor(.white)
        }
        .background(.black)
    }
}

func bytesCb(info: UnsafeMutableRawPointer?, buffer: UnsafeMutableRawPointer, count: Int) -> Int {
    // Forward the bytes callback to the C++ bridge
    return Int(bytesBridge(info, buffer, Int32(count)))
}

func forwardCb(info: UnsafeMutableRawPointer?, count: off_t) -> off_t {
    // Forward the forward callback to the C++ bridge
    return off_t(forwardBridge(info, Int32(count)))
}

func rewindCb(info: UnsafeMutableRawPointer?) -> Void {
    // Forward the rewind callback to the C++ bridge
    return rewindBridge(info)
}
