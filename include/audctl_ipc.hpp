/**
 * @file audctl_ipc.hpp
 * @brief Обёртка над libnx audctl API для управления маршрутизацией аудио.
 *
 * Использует нативные libnx функции (audctl.h, libnx 4.12+).
 * Все enum-ы (AudioTarget, AudioOutputMode) — из libnx.
 */
#pragma once

#include <switch.h>
#include <cstdio>

class AudctlWrapper {
public:

    // ── Жизненный цикл ──────────────────────────────────────────────────

    Result initialize() {
        if (m_initialized) return 0;
        Result rc = audctlInitialize();
        if (R_SUCCEEDED(rc))
            m_initialized = true;
        return rc;
    }

    void finalize() {
        if (m_initialized) {
            audctlExit();
            m_initialized = false;
        }
    }

    bool isInitialized() const { return m_initialized; }

    // ── Soundbar Mode ───────────────────────────────────────────────────

    /**
     * Включает Soundbar Mode:
     *  1. Запоминает текущий default target.
     *  2. Отключает Speaker Auto-Mute (чтобы система не пыталась замутить динамики в фоне).
     *  3. Переключает default target на Speaker (fade = 0, без асинхронных таймеров).
     *  4. Снимает mute с динамиков.
     *
     * ВАЖНО для стабильности игр:
     *  - НЕ мутить AudioTarget_Tv вручную! Принудительный mute TV ломает HDMI
     *    audio DMA sync / кольцевой буфер пакетов, из-за чего поток рендера звука
     *    в играх блокируется и игра намертво зависает.
     *  - НЕ вызывать audctlSetOutputTarget! Он конфликтует с политикой SetDefaultTarget.
     *  - fade_ns = 0 — мгновенный переход исключает гонки таймеров в audioctrl.
     */
    Result enableSoundbarMode() {
        if (!m_initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

        // ① Запоминаем текущий target
        Result rc = audctlGetDefaultTarget(&m_savedTarget);
        if (R_FAILED(rc)) m_savedTarget = AudioTarget_Tv;

        // ② Переключаем на встроенные динамики мгновенно (fade = 0)
        rc = audctlSetDefaultTarget(AudioTarget_Speaker, 0, 0);
        if (R_FAILED(rc)) return rc;

        // ③ Снимаем mute с динамиков
        audctlSetTargetMute(AudioTarget_Speaker, false);

        m_soundbarActive = true;
        return 0;
    }

    /**
     * Отключает Soundbar Mode, восстанавливая нормальное состояние.
     */
    Result disableSoundbarMode() {
        if (!m_initialized) return MAKERESULT(Module_Libnx, LibnxError_NotInitialized);

        // Восстанавливаем target
        Result rc = audctlSetDefaultTarget(m_savedTarget, 0, 0);

        m_soundbarActive = false;
        return rc;
    }

    bool isSoundbarActive() const { return m_soundbarActive; }

    // ── Громкость ───────────────────────────────────────────────────────

    Result getSpeakerVolume(s32 *vol) {
        return audctlGetTargetVolume(vol, AudioTarget_Speaker);
    }

    Result setSpeakerVolume(s32 vol) {
        return audctlSetTargetVolume(AudioTarget_Speaker, vol);
    }

    Result getVolumeMin(s32 *vol) {
        return audctlGetTargetVolumeMin(vol);
    }

    Result getVolumeMax(s32 *vol) {
        return audctlGetTargetVolumeMax(vol);
    }

    // ── Состояние ───────────────────────────────────────────────────────

    Result getDefaultTarget(AudioTarget *target) {
        return audctlGetDefaultTarget(target);
    }

    Result getActiveTarget(AudioTarget *target) {
        return audctlGetActiveOutputTarget(target);
    }

    Result getTvAudioOutputMode(AudioOutputMode *mode) {
        return audctlGetAudioOutputMode(mode, AudioTarget_Tv);
    }

    // ── Утилиты ─────────────────────────────────────────────────────────

    static const char* targetName(AudioTarget t) {
        switch (t) {
            case AudioTarget_Speaker:         return "Speaker";
            case AudioTarget_Headphone:       return "Headphone";
            case AudioTarget_Tv:              return "TV (HDMI)";
            case AudioTarget_UsbOutputDevice: return "USB Audio";
            case AudioTarget_Bluetooth:       return "Bluetooth";
            default:                          return "Unknown";
        }
    }

    static const char* outputModeName(AudioOutputMode m) {
        switch (m) {
            case AudioOutputMode_Pcm1ch:  return "Mono";
            case AudioOutputMode_Pcm2ch:  return "Stereo";
            case AudioOutputMode_Pcm6ch:  return "Surround 5.1";
            case AudioOutputMode_PcmAuto: return "Auto";
            default:                      return "Unknown";
        }
    }

private:
    bool        m_initialized    = false;
    bool        m_soundbarActive = false;
    AudioTarget m_savedTarget    = AudioTarget_Tv;
};
