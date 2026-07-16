#pragma once

#include <string>

struct AudioSourceDefinition
{
    std::string type = "oscillator";
    std::string wave = "square";
    float duty = 0.5f;
};

struct AudioMovementDefinition
{
    std::string type = "none";
    float amount = 0.0f;
};

struct AudioMaterialDefinition
{
    float brightness = 0.5f;
    float roughness = 0.0f;
    float noise = 0.0f;
    float resonance = 0.0f;
    float metal = 0.0f;
};

struct AudioEnvelopeDefinition
{
    float attack = 0.0f;
    float decay = 0.05f;
    float sustain = 0.0f;
    float release = 0.0f;
};

struct AudioSpaceDefinition
{
    std::string mode = "mono";
    float width = 0.0f;
    float echo = 0.0f;
};

struct AudioToneDefinition
{
    AudioMaterialDefinition material;
    AudioEnvelopeDefinition envelope;
    AudioSpaceDefinition space;
};

struct SoundKindDefinition
{
    AudioSourceDefinition source;
    float noteFrequency = 440.0f;
    float slide = 0.0f;
    AudioMovementDefinition movement;
};

struct SoundDefinition
{
    SoundKindDefinition kind;
    AudioToneDefinition tone;
    float duration = 0.1f;
    float volume = 0.4f;
};
