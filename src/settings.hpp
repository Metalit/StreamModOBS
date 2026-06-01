#pragma once

inline constexpr auto PropAddress = "address";
inline constexpr auto PropBufferMs = "buffer";

inline constexpr auto PropResolution = "resolution";
inline constexpr auto PropWidth = "width";
inline constexpr auto PropHeight = "height";
inline constexpr auto PropBitrate = "bitrate";

inline constexpr auto PropGameVolume = "gameVolume";
inline constexpr auto PropMicrophone = "mic";
inline constexpr auto PropMicVolume = "micVolume";
inline constexpr auto PropMicThreshold = "micThreshold";
inline constexpr auto PropMicMix = "micMix";

#include <array>
#include <utility>

inline constexpr std::array ResolutionValues = {std::make_pair(1280, 720), std::make_pair(1920, 1080), std::make_pair(2560, 1440)};
inline constexpr std::array Resolutions = {"res.720", "res.1080", "res.1440"};
inline constexpr std::array MicMixes = {"mic_mix.combine", "mic_mix.duck", "mic_mix.add"};
