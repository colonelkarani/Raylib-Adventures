// =====================================================================================
//  Audio.cpp -- see Audio.h. All waveforms are generated in-memory as simple decaying
//  sine tones (optionally mixed with a harmonic) so no asset files are required.
// =====================================================================================
#include "Audio.h"
#include "Common.h"
#include <cstring>

Wave AudioManager::MakeToneWave(float freqHz, float durationSec, float amplitude, bool decay, float harmonicMix) {
    int sampleRate = 44100;
    int frameCount = (int)(durationSec * sampleRate);
    std::vector<short> samples(frameCount);
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float envelope = decay ? expf(-t * (1.0f / std::max(0.02f, durationSec)) * 4.5f) : 1.0f;
        float s = sinf(2.0f * PI * freqHz * t);
        if (harmonicMix > 0.0f) s = (1.0f - harmonicMix) * s + harmonicMix * sinf(2.0f * PI * freqHz * 2.0f * t);
        samples[i] = (short)(s * amplitude * envelope * 32000.0f);
    }
    Wave w{};
    w.frameCount = (unsigned int)frameCount;
    w.sampleRate = (unsigned int)sampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = MemAlloc((unsigned int)(frameCount * sizeof(short)));
    memcpy(w.data, samples.data(), frameCount * sizeof(short));
    return w;
}

Sound AudioManager::MakeTone(float freqHz, float durationSec, float amplitude, bool decay, float harmonicMix) {
    Wave w = MakeToneWave(freqHz, durationSec, amplitude, decay, harmonicMix);
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

void AudioManager::Init() {
    if (initialized_) return;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) { initialized_ = false; return; }

    beepMonitor_       = MakeTone(880.0f, 0.09f, 0.5f, true, 0.0f);
    alarmTone_         = MakeTone(1200.0f, 0.16f, 0.55f, true, 0.3f);
    fireBlip_          = MakeTone(500.0f, 0.07f, 0.4f, true, 0.15f);
    hitTick_           = MakeTone(300.0f, 0.05f, 0.3f, true, 0.0f);
    killChime_         = MakeTone(1046.5f, 0.18f, 0.45f, true, 0.4f); // C6
    backfireBuzz_      = MakeTone(140.0f, 0.22f, 0.5f, true, 0.5f);
    uiMove_            = MakeTone(660.0f, 0.05f, 0.25f, true, 0.0f);
    levelClearJingle_  = MakeTone(784.0f, 0.35f, 0.45f, true, 0.5f);  // G5
    levelFailTone_     = MakeTone(196.0f, 0.5f, 0.5f, true, 0.2f);    // G3

    initialized_ = true;
}

void AudioManager::Shutdown() {
    if (!initialized_) return;
    UnloadSound(beepMonitor_);
    UnloadSound(alarmTone_);
    UnloadSound(fireBlip_);
    UnloadSound(hitTick_);
    UnloadSound(killChime_);
    UnloadSound(backfireBuzz_);
    UnloadSound(uiMove_);
    UnloadSound(levelClearJingle_);
    UnloadSound(levelFailTone_);
    CloseAudioDevice();
    initialized_ = false;
}

void AudioManager::Update(float dt, float heartRate, bool danger, bool muted, float volume) {
    if (!initialized_ || muted) return;
    SetSoundVolume(beepMonitor_, volume);
    SetSoundVolume(alarmTone_, volume);

    beepTimer_ -= dt;
    if (beepTimer_ <= 0.0f) {
        float bpm = Clampf(heartRate, 30.0f, 200.0f);
        beepTimer_ = 60.0f / bpm;
        SetSoundPitch(beepMonitor_, Clampf(heartRate / 75.0f, 0.6f, 1.8f));
        PlaySound(beepMonitor_);
    }

    if (danger) {
        alarmTimer_ -= dt;
        if (alarmTimer_ <= 0.0f) { alarmTimer_ = 0.5f; PlaySound(alarmTone_); }
    } else {
        alarmTimer_ = 0.0f;
    }
}

void AudioManager::PlayFire()        { if (initialized_) PlaySound(fireBlip_); }
void AudioManager::PlayHit()         { if (initialized_) PlaySound(hitTick_); }
void AudioManager::PlayKill()        { if (initialized_) PlaySound(killChime_); }
void AudioManager::PlayBackfire()    { if (initialized_) PlaySound(backfireBuzz_); }
void AudioManager::PlayUiMove()      { if (initialized_) PlaySound(uiMove_); }
void AudioManager::PlayLevelClear()  { if (initialized_) PlaySound(levelClearJingle_); }
void AudioManager::PlayLevelFail()   { if (initialized_) PlaySound(levelFailTone_); }
