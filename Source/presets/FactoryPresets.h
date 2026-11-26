#pragma once
#include <vector>
#include <utility>

// A factory preset only lists the parameters that differ from the defaults
// (values are in real units: Hz, seconds, choice index, 0/1 for switches).
struct FactoryPreset
{
    const char* name;
    const char* category;
    const char* blurb;     // one-liner shown in the character's speech bubble
    std::vector<std::pair<const char*, float>> values;
};

namespace presetcat
{
    inline const char* const all[] = { "ICONIC LEADS", "WEIRD PLUCKS", "DIGITAL KEYS", "METALLIC / FM",
                                       "DARK TEXTURES", "AMBIENT", "BASS", "EXPERIMENTAL" };
    constexpr int count = 8;
}

const std::vector<FactoryPreset>& getFactoryPresets();
