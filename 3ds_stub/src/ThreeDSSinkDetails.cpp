#include "audio_core/sink_details.h"

#include <array>
#include <algorithm>

#include "three_ds_stub/ThreeDSAudio.hpp"

namespace AudioCore {
namespace {

const std::array sink_details = {
    SinkDetails{SinkType::Auto, "Nintendo Switch audout",
                &beiklive::three_ds_stub::CreateThreeDSSwitchAudioSink,
                &beiklive::three_ds_stub::ListThreeDSSwitchAudioDevices},
};

} // namespace

std::vector<SinkDetails> ListSinks() {
    return {sink_details.begin(), sink_details.end()};
}

const SinkDetails& GetSinkDetails(SinkType) {
    return sink_details.front();
}

} // namespace AudioCore
