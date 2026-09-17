// =====================================================================================
//  Audio.h -- every sound in this game is synthesized at startup rather than loaded
//  from an asset file, so the project has zero external binary dependencies. The
//  "monitor beep" pitch is tied to heart rate the same way a real pulse-oximeter tone
//  tracks HR, and a separate alarm tone plays while any vital sits in a danger band --
//  both are cosmetic/UX touches, not modeled physiology.
// =====================================================================================
#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>

class AudioManager {
public:
    void Init();
    void Shutdown();
    void Update(float dt, float heartRate, bool danger, bool muted, float volume);
    void PlayFire();
    void PlayHit();
    void PlayKill();
    void PlayBackfire();
    void PlayUiMove();
    void PlayLevelClear();
    void PlayLevelFail();

private:
    bool initialized_ = false;
    Sound beepMonitor_{};
    Sound alarmTone_{};
    Sound fireBlip_{};
    Sound hitTick_{};
    Sound killChime_{};
    Sound backfireBuzz_{};
    Sound uiMove_{};
    Sound levelClearJingle_{};
    Sound levelFailTone_{};

    float beepTimer_ = 0.0f;
    float alarmTimer_ = 0.0f;

    Sound MakeTone(float freqHz, float durationSec, float amplitude, bool decay, float harmonicMix = 0.0f);
    Wave MakeToneWave(float freqHz, float durationSec, float amplitude, bool decay, float harmonicMix);
};
