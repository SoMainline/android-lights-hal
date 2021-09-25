/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Lights.h"

#include <android-base/logging.h>
#include <dirent.h>
#include <fstream>

static const std::string BACKLIGHT_DIR = "/sys/class/backlight";

Light::Light(HwLight hwLight, std::string path)
    : hwLight(hwLight)
    , path(path)
{
}

Backlight::Backlight(HwLight hwLight, std::string path, uint32_t maxBrightness)
    : Light(hwLight, path)
    , maxBrightness(maxBrightness)
{
}

Backlight *Backlight::createBacklight(HwLight hwLight, std::string path)
{
    uint32_t maxBrightness;
    std::ifstream stream(path + "/max_brightness");
    if (auto stream = std::ifstream(path + "/max_brightness")) {
        stream >> maxBrightness;
    } else {
        LOG(ERROR) << "Failed to read `max_brightness` for " << path;
        return nullptr;
    }

    LOG(INFO) << "Creating backlight " << path << " with max brightness " << maxBrightness;

    return new Backlight(hwLight, path, maxBrightness);
}

static int32_t rgbToBrightness(int32_t color)
{
    auto r = (color >> 16) & 0xff;
    auto g = (color >> 8) & 0xff;
    auto b = color & 0xff;
    return (77 * r + 150 * g + 29 * b) >> 8;
}

ndk::ScopedAStatus Backlight::setLightState(const HwLightState &state) const
{
    auto brightness = rgbToBrightness(state.color);
    // Adding half of the max (255/2=127) provides proper rounding while staying in integer mode:
    brightness = (brightness * maxBrightness + 127) / 255;
    if (state.brightnessMode == BrightnessMode::LOW_PERSISTENCE)
        LOG(ERROR) << "TODO: Implement Low Persistence brightness mode";
    LOG(DEBUG) << "Changing backlight to level " << brightness << "/" << maxBrightness;
    if (auto stream = std::ofstream(path + "/brightness")) {
        stream << brightness;
        return ndk::ScopedAStatus::ok();
    } else {
        LOG(ERROR) << "Failed to write `brightness` to " << path;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState &state)
{
    LOG(DEBUG) << "Lights setting state for id=" << id << " to color " << std::hex << state.color;

    if (id >= lights.size())
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);

    const auto &light = lights[id];
    return light->setLightState(state);
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight> *hwLights)
{
    for (const auto &light : lights)
        hwLights->emplace_back(light->hwLight);
    return ndk::ScopedAStatus::ok();
}

Lights::Lights()
{
    int id = 0;
    int ordinal = 0;
    // Cannot use std::filesystem from libc++fs which is not available for vendor modules.
    // Maybe with Android S?
    // for (const auto &backlight : std::filesystem::directory_iterator("/sys/class/backlight"))
    //     if (backlight.is_directory() || backlight.is_symlink())
    //         lights.emplace_back(..);

    if (auto backlights = opendir(BACKLIGHT_DIR.c_str())) {
        while (dirent *ent = readdir(backlights)) {
            if ((ent->d_type == DT_DIR && ent->d_name[0] != '.') || ent->d_type == DT_LNK) {
                std::string backlightPath = BACKLIGHT_DIR + "/" + ent->d_name;
                if (auto backlight = Backlight::createBacklight(
                        HwLight { .id = id++, .ordinal = ordinal++, .type = LightType::BACKLIGHT },
                        backlightPath))
                    lights.emplace_back(backlight);
            }
        }
        closedir(backlights);
    } else {
        LOG(ERROR) << "Failed to open " << BACKLIGHT_DIR;
    }

    LOG(INFO) << "Found " << ordinal << " backlights";
}
