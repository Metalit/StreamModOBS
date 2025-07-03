#pragma once

static constexpr auto PropWidth = "width";
static constexpr auto PropHeight = "height";
static constexpr auto PropBitrate = "bitrate";
static constexpr auto PropFPS = "fps";
static constexpr auto PropFOV = "fov";
static constexpr auto PropSmoothness = "smoothness";
static constexpr auto PropMicrophone = "mic";
static constexpr auto PropGameVolume = "gameVolume";
static constexpr auto PropMicVolume = "micVolume";
static constexpr auto PropMicThreshold = "micThreshold";
static constexpr auto PropMicMix = "micMix";

static constexpr auto PropAddress = "address";
static constexpr auto PropResolution = "resolution";

#include <array>

static std::array<char const*, 3> inline const Resolutions = {"720p", "1080p", "1440p"};
// not sure what's wrong with array for this one
static std::pair<int, int> inline const ResolutionValues[] = {{1280, 720}, {1920, 1080}, {2560, 1440}};

static std::array<char const*, 3> inline const MicMixes = {"Combine", "Duck", "Add"};
