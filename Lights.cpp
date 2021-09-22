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
#include <fstream>

// TODO: Recurse all backlight files!
static const std::string BACKLIGHT_DIR = "/sys/class/backlight/backlight/";
static const std::string BACKLIGHT_MAX_BRIGHTNESS = BACKLIGHT_DIR + "max_brightness";
static const std::string BACKLIGHT_BRIGHTNESS = BACKLIGHT_DIR + "brightness";

static int32_t rgbToBrightness(int32_t color)
{
    auto r = (color >> 16) & 0xff;
    auto g = (color >> 8) & 0xff;
    auto b = color & 0xff;
    return (77 * r + 150 * g + 29 * b) >> 8;
}

ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState &state)
{
    LOG(INFO) << "Lights setting state for id=" << id << " to color " << std::hex << state.color;
    switch (id) {
    case 0: {
        auto brightness = rgbToBrightness(state.color);
        // Adding half of the max (255/2=127) provides proper rounding while staying in integer mode:
        brightness = (brightness * backlightMaxBrightness + 127) / 255;
        LOG(INFO) << "Changing backlight to level " << brightness << "/" << backlightMaxBrightness;
        std::ofstream stream(BACKLIGHT_BRIGHTNESS);
        if (!stream) {
            LOG(ERROR) << "Failed to open backlight file for writing";
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        }
        stream << brightness;
        return ndk::ScopedAStatus::fromExceptionCode(EX_NONE);
    }
    default:
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight> *lights)
{
    LOG(INFO) << "Lights reporting supported lights";
    lights->emplace_back(HwLight {
        .id = 0,
        .ordinal = 0,
        .type = LightType::BACKLIGHT });
    return ndk::ScopedAStatus::ok();
}

Lights::Lights()
{
    std::ifstream stream(BACKLIGHT_MAX_BRIGHTNESS);
    stream >> backlightMaxBrightness;
}
