#include "LedStrip.hpp"
#include "Meteor.hpp"
#include "Util.hpp"
#include "WifiGateway.hpp"

#include <vector>

enum class State {
    WHITE,
    ROTATED_RAINBOW,
    BUBBLE_SORT,
    METEOR,
    SLEEP,
    STATE_COUNT
};

const int kStateCount = static_cast<int>(State::STATE_COUNT);
const int32_t kPixelCount = 850;
const uint8_t kPin = 4;

State current_state = State::METEOR;
bool state_initialized = false;

std::unique_ptr<LedStrip> led_strip;
std::unique_ptr<WifiGateway> wifi_gateway;
std::unique_ptr<Meteor> meteor;

void IncrementState() {
    current_state =
        static_cast<State>((static_cast<int>(current_state) + 1) % kStateCount);
    state_initialized = false;
}

void setup() {
    Serial.begin(115200);
    led_strip = make_unique<LedStrip>(kPixelCount, kPin);
    wifi_gateway = make_unique<WifiGateway>(IncrementState);
    meteor = make_unique<Meteor>(led_strip.get());
}

int32_t ColorToId(RgbColor color) {
    return -255 + color.R + 2 * color.G + 3 * color.B;
}

RgbColor IdToColor(int32_t id) {
    if (id < 255) {
        return RgbColor{static_cast<uint8_t>(255 - id),
                        static_cast<uint8_t>(id), 0};
    }
    id -= 255;
    return RgbColor{0, static_cast<uint8_t>(255 - id),
                    static_cast<uint8_t>(id)};
}

void RandomLeds(int32_t group_count) {
    for (int32_t group = 0; group < group_count; ++group) {
        RgbColor color = IdToColor(RandomInt(0, 255 * 2));
        int32_t first_pixel = kPixelCount * group / group_count;
        int32_t last_pixel = kPixelCount * (group + 1) / group_count - 1;
        led_strip->SetColors(first_pixel, last_pixel, color);
    }
}

void UpdateRotatedRainbow() {
    if (!state_initialized) {
        RgbColor red{255, 0, 0};
        RgbColor green{0, 255, 0};
        RgbColor blue{0, 0, 255};
        int32_t s1 = kPixelCount / 3;
        int32_t s2 = 2 * kPixelCount / 3;
        led_strip->Gradient(0, s1 - 1, red, green);
        led_strip->Gradient(s1, s2 - 1, green, blue);
        led_strip->Gradient(s2, kPixelCount - 1, blue, red);
    } else {
        led_strip->Rotate(4);
    }
}

void DisplayColorIds(const std::vector<int32_t> &ids) {
    int32_t right_pixel = 0;
    for (int32_t group = 0; group < ids.size(); ++group) {
        int32_t left_pixel = right_pixel;
        right_pixel =
            kPixelCount * (group + 1) / static_cast<int32_t>(ids.size());
        led_strip->SetColors(left_pixel, right_pixel - 1,
                             IdToColor(ids[group]));
    }
}

void BubbleSortStep(std::vector<int32_t> &ids, bool initialized) {
    static int32_t current_group;
    static int32_t max_group;
    static bool changed;
    if (!initialized) {
        current_group = 0;
        max_group = static_cast<int32_t>(ids.size()) - 1;
        changed = false;
    }
    if (ids[current_group] > ids[current_group + 1]) {
        std::swap(ids[current_group], ids[current_group + 1]);
        changed = true;
    }
    ++current_group;
    if (current_group == max_group) {
        if (!changed) {
            current_state = State::SLEEP;
        } else {
            current_group = 0;
            --max_group;
            changed = false;
        }
    }
}

void UpdateBubbleSort() {
    const int32_t group_size = 15;
    const int32_t group_count = kPixelCount / group_size;
    static std::vector<int32_t> ids(group_count);
    if (!state_initialized) {
        for (auto &id : ids) {
            id = RandomInt(0, 512);
        }
        BubbleSortStep(ids, false);
    } else {
        BubbleSortStep(ids, true);
    }
    DisplayColorIds(ids);
}

void UpdateState() {
    switch (current_state) {
    case State::WHITE:
        led_strip->SetColors(RgbColor{10, 10, 10});
        break;
    case State::ROTATED_RAINBOW:
        UpdateRotatedRainbow();
        break;
    case State::BUBBLE_SORT:
        UpdateBubbleSort();
        break;
    case State::METEOR:
        meteor->Update();
        break;
    }
    state_initialized = true;
    led_strip->Update();
}

void loop() {
    wifi_gateway->Update();
    UpdateState();
    delay(20);
}
