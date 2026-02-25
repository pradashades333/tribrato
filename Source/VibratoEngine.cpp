#include "VibratoEngine.h"

//==============================================================================
void VibratoEngine::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = sampleRate;
    reset();
}

//==============================================================================
void VibratoEngine::reset()
{
    std::memset (delayBuf, 0, sizeof (delayBuf));
    writePos = 0;
    lfoPhase = 0.0f;
    envelope = 0.0f;
    variationSmoothed  = 0.0f;
    variationTarget    = 0.0f;
    variationCountdown = 0;
    formantUpdateCounter = 0;
    pitchEnvelope      = 0.0f;
    smoothedDelay      = BASE_DELAY;
    smoothedPitchCents = 0.0f;
    smoothedAmpDepth   = 0.0f;

    for (int ch = 0; ch < MAX_CHANNELS; ++ch)
    {
        formantInPower[ch]  = 1e-6f;
        formantOutPower[ch] = 1e-6f;
        for (int f = 0; f < NUM_FORMANTS; ++f)
            formantFilters[ch][f].resetState();
    }
}

//==============================================================================
void VibratoEngine::process (juce::AudioBuffer<float>& buffer, const Params& p)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), MAX_CHANNELS);

    // Envelope rates -----------------------------------------------------------
    const float attackRate      = 1.0f / juce::jmax (1.0f,
                                    (p.onsetMs / 1000.0f) * static_cast<float> (sr));
    // 80 ms release for amplitude/formant — tight enough to feel responsive
    const float releaseRate     = 1.0f / juce::jmax (1.0f,
                                    0.080f * static_cast<float> (sr));
    // 300 ms release for pitch only — delay position returns slowly, no Doppler snap
    const float pitchReleaseRate = 1.0f / juce::jmax (1.0f,
                                    0.300f * static_cast<float> (sr));
    const float envTarget        = p.triggered ? 1.0f : 0.0f;

    // Smoothing coefficients (one-pole, per-sample) ----------------------------
    // 30 ms pitch smoother — prevents snap clicks when the knob is moved
    const float pitchSmoothCoeff = 1.0f - std::exp (
                                    -1.0f / (0.030f * static_cast<float> (sr)));
    // 30 ms amp-depth smoother — prevents snap when amplitude knob is turned
    const float ampSmoothCoeff   = 1.0f - std::exp (
                                    -1.0f / (0.030f * static_cast<float> (sr)));
    // 5 ms delay-position smoother — eliminates residual micro-clicks
    const float delaySmoothCoeff = 1.0f - std::exp (
                                    -1.0f / (0.005f * static_cast<float> (sr)));

    // Normalised depths --------------------------------------------------------
    const float ampDepth = p.amplitude / 100.0f;
    const float fmtDepth = p.formant   / 100.0f;
    const float varAmt   = p.variation  / 100.0f;

    // 30 ms RMS tracker for dynamic formant gain compensation
    const float fmtPowerCoeff = 1.0f - std::exp (
                                    -1.0f / (0.030f * static_cast<float> (sr)));

    // Per-sample loop ----------------------------------------------------------
    for (int i = 0; i < numSamples; ++i)
    {
        // --- Main envelope (amplitude + formant, 80 ms release) ---------------
        if (envelope < envTarget)
        {
            envelope += attackRate;
            if (envelope > envTarget) envelope = envTarget;
        }
        else if (envelope > envTarget)
        {
            envelope -= releaseRate;
            if (envelope < envTarget) envelope = envTarget;
        }

        // --- Pitch envelope (300 ms release — prevents Doppler snap on release)
        if (pitchEnvelope < envTarget)
        {
            pitchEnvelope += attackRate;
            if (pitchEnvelope > envTarget) pitchEnvelope = envTarget;
        }
        else if (pitchEnvelope > envTarget)
        {
            pitchEnvelope -= pitchReleaseRate;
            if (pitchEnvelope < envTarget) pitchEnvelope = envTarget;
        }

        // --- Smooth amplitude depth (prevents snap when knob is moved) --------
        smoothedAmpDepth += ampSmoothCoeff * (ampDepth - smoothedAmpDepth);

        // --- Variation (slowly drifting random value) -------------------------
        if (varAmt > 0.0f)
        {
            if (--variationCountdown <= 0)
            {
                variationTarget    = dist (rng);
                variationCountdown = static_cast<int> (sr * 0.04f); // 40 ms
            }
            variationSmoothed += (variationTarget - variationSmoothed) * 0.002f;
        }
        else
        {
            variationSmoothed = 0.0f;
        }

        // --- LFO --------------------------------------------------------------
        float effectiveRate = p.rateHz
                            * (1.0f + variationSmoothed * varAmt * 0.25f);
        effectiveRate = juce::jmax (1.0f, effectiveRate);

        lfoPhase += effectiveRate / static_cast<float> (sr);
        while (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

        float lfoValue = std::sin (2.0f * juce::MathConstants<float>::pi
                                   * lfoPhase);

        // Variation applied to waveshape
        float lfo = juce::jlimit (-1.0f, 1.0f,
                        lfoValue + variationSmoothed * varAmt * 0.15f);

        // --- Smooth pitch parameter (prevents snap when knob moves) -----------
        smoothedPitchCents += pitchSmoothCoeff * (p.pitchCents - smoothedPitchCents);

        // --- Delay modulation (vibrato / pitch) --------------------------------
        float delayMod = 0.0f;
        if (smoothedPitchCents > 0.0f && effectiveRate > 0.0f)
        {
            float effPitch = smoothedPitchCents
                           * (1.0f + variationSmoothed * varAmt * 0.15f);
            effPitch = juce::jmax (0.0f, effPitch);

            float modAmp = (std::pow (2.0f, effPitch / 1200.0f) - 1.0f)
                         * static_cast<float> (sr)
                         / (2.0f * juce::MathConstants<float>::pi * effectiveRate);
            delayMod = lfo * modAmp * pitchEnvelope;   // slow-release pitch envelope
        }

        float totalDelay = BASE_DELAY + delayMod;
        totalDelay = juce::jlimit (2.0f,
                        static_cast<float> (DELAY_BUF_SIZE - 4), totalDelay);

        // Smooth the delay read-position — eliminates clicks on trigger/release
        smoothedDelay += delaySmoothCoeff * (totalDelay - smoothedDelay);

        // --- Amplitude modulation (tremolo) ------------------------------------
        // Uses smoothedAmpDepth so knob changes never snap.
        // Makeup gain restores mean level so perceived loudness stays constant.
        float ampMod    = 1.0f - smoothedAmpDepth * envelope * (1.0f - lfo) * 0.5f;
        float ampMakeup = juce::jmin (3.0f,
                            1.0f / juce::jmax (0.1f, 1.0f - smoothedAmpDepth * envelope * 0.5f));

        // --- Update formant filter coeffs every 32 samples --------------------
        if (fmtDepth > 0.0f && envelope > 0.001f)
        {
            if (++formantUpdateCounter >= 32)
            {
                formantUpdateCounter = 0;
                float depth    = fmtDepth * envelope;
                float freqMult = 1.0f + lfo * depth * 0.4f;   // +/- 40 %
                freqMult = juce::jmax (0.3f, freqMult);

                for (int f = 0; f < NUM_FORMANTS; ++f)
                {
                    float freq = formantBaseFreqs[f] * freqMult;
                    for (int ch = 0; ch < numChannels; ++ch)
                        formantFilters[ch][f].setParams (freq, 2.0f, sr);
                }
            }
        }

        // --- Per-channel processing -------------------------------------------
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample (ch, i);

            // Write into delay line
            delayBuf[ch][writePos] = input;

            // Read from delay line using smoothed position (click-free)
            float delayed = readDelay (ch, smoothedDelay);

            // Always track input power (keeps tracker warm even when formant is off)
            formantInPower[ch] += fmtPowerCoeff * (delayed * delayed - formantInPower[ch]);

            // Formant colouring
            float processed = delayed;
            if (fmtDepth > 0.0f && envelope > 0.001f)
            {
                float fGain = fmtDepth * envelope * 0.5f;
                float fSum  = 0.0f;
                for (int f = 0; f < NUM_FORMANTS; ++f)
                    fSum += formantFilters[ch][f].processBandpass (delayed);
                processed = delayed + fGain * fSum;
            }

            // Dynamic gain compensation: continuously track output/input power
            // ratio and apply the exact inverse — works for any signal content
            formantOutPower[ch] += fmtPowerCoeff * (processed * processed - formantOutPower[ch]);
            const float formantGainComp = (formantOutPower[ch] > 1e-12f)
                ? std::sqrt (formantInPower[ch] / formantOutPower[ch])
                : 1.0f;
            // Never boost (capped at 1.0); allow up to 6 dB of reduction max
            processed *= juce::jlimit (0.5f, 1.0f, formantGainComp);

            // Tremolo with level compensation
            processed *= ampMod * ampMakeup;

            buffer.setSample (ch, i, processed);
        }

        writePos = (writePos + 1) & (DELAY_BUF_SIZE - 1);
    }
}

//==============================================================================
float VibratoEngine::readDelay (int channel, float delaySamples) const
{
    float readPos = static_cast<float> (writePos) - delaySamples;
    while (readPos < 0.0f) readPos += static_cast<float> (DELAY_BUF_SIZE);

    int   idx  = static_cast<int> (readPos);
    float frac = readPos - static_cast<float> (idx);

    // Hermite cubic interpolation
    int im1 = (idx - 1 + DELAY_BUF_SIZE) & (DELAY_BUF_SIZE - 1);
    int i0  =  idx                        & (DELAY_BUF_SIZE - 1);
    int i1  = (idx + 1)                   & (DELAY_BUF_SIZE - 1);
    int i2  = (idx + 2)                   & (DELAY_BUF_SIZE - 1);

    float ym1 = delayBuf[channel][im1];
    float y0  = delayBuf[channel][i0];
    float y1  = delayBuf[channel][i1];
    float y2  = delayBuf[channel][i2];

    float c0 = y0;
    float c1 = 0.5f  * (y1 - ym1);
    float c2 = ym1   - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    float c3 = 0.5f  * (y2 - ym1) + 1.5f * (y0 - y1);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
