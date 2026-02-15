#pragma once
#include <JuceHeader.h>
#include <array>
#include <random>
#include <cmath>

class VibratoEngine
{
public:
    struct Params
    {
        bool  triggered  = false;
        float onsetMs    = 200.0f;   // 10 - 2000
        float rateHz     = 5.5f;     // 0.5 - 15
        float pitchCents = 50.0f;    // 0 - 200
        float amplitude  = 0.0f;     // 0 - 100  (%)
        float formant    = 0.0f;     // 0 - 100  (%)
        float variation  = 0.0f;     // 0 - 100  (%)
    };

    void prepare (double sampleRate, int maxBlockSize);
    void process (juce::AudioBuffer<float>& buffer, const Params& params);
    void reset();

private:
    //==========================================================================
    // Topology-preserving SVF – safe for per-sample modulation
    struct SVFilter
    {
        float s1 = 0.f, s2 = 0.f;
        float a1 = 0.f, a2 = 0.f, a3 = 0.f;

        void setParams (float cutoffHz, float Q, double sampleRate)
        {
            float maxFreq = static_cast<float> (sampleRate) * 0.48f;
            float fc = juce::jlimit (80.0f, maxFreq, cutoffHz);
            float g  = std::tan (juce::MathConstants<float>::pi * fc
                                 / static_cast<float> (sampleRate));
            float k  = 1.0f / Q;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        float processBandpass (float x)
        {
            float v3 = x - s2;
            float v1 = a1 * s1 + a2 * v3;
            float v2 = s2  + a2 * s1 + a3 * v3;
            s1 = 2.0f * v1 - s1;
            s2 = 2.0f * v2 - s2;
            return v1;
        }

        void resetState() { s1 = s2 = 0.0f; }
    };

    //==========================================================================
    double sr = 44100.0;

    // Delay line ---------------------------------------------------------------
    static constexpr int MAX_CHANNELS   = 2;
    static constexpr int DELAY_BUF_SIZE = 4096;          // must be power-of-2
    static constexpr float BASE_DELAY   = 1024.0f;       // ~21 ms @ 48 kHz
    float delayBuf[MAX_CHANNELS][DELAY_BUF_SIZE] = {};
    int   writePos = 0;

    // LFO ----------------------------------------------------------------------
    float lfoPhase = 0.0f;

    // Envelope -----------------------------------------------------------------
    float envelope = 0.0f;

    // Variation ----------------------------------------------------------------
    std::mt19937 rng { 42 };
    std::uniform_real_distribution<float> dist { -1.0f, 1.0f };
    float variationSmoothed = 0.0f;
    float variationTarget   = 0.0f;
    int   variationCountdown = 0;

    // Formant filters ----------------------------------------------------------
    static constexpr int NUM_FORMANTS = 3;
    SVFilter formantFilters[MAX_CHANNELS][NUM_FORMANTS];
    float formantBaseFreqs[NUM_FORMANTS] = { 600.0f, 1500.0f, 2800.0f };
    int   formantUpdateCounter = 0;

    // Helpers ------------------------------------------------------------------
    float readDelay (int channel, float delaySamples) const;
};
