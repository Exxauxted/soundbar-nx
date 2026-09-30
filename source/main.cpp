/**
 * @file main.cpp
 * @brief «Soundbar Mode» — Tesla / Ultrahand overlay (.ovl) для Nintendo Switch.
 *
 * Маршрутизирует аудио на встроенные динамики консоли в доке,
 * оставляя видеовыход по HDMI.
 *
 * Сборка: devkitPro (libnx 4.12+ / libtesla).
 */

#define TESLA_INIT_IMPL
#include <tesla.hpp>

#include "audctl_ipc.hpp"

#include <cstdio>
#include <string>

// ─── Глобальный IPC-объект ───────────────────────────────────────────────────
static AudctlWrapper g_audctl;

// ─── Утилиты ─────────────────────────────────────────────────────────────────

static int volumeToPercent(s32 value, s32 vmin, s32 vmax) {
    if (vmax <= vmin) return 0;
    int pct = static_cast<int>((static_cast<float>(value - vmin) / (vmax - vmin)) * 100.0f);
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

static s32 percentToVolume(int pct, s32 vmin, s32 vmax) {
    if (pct <= 0)   return vmin;
    if (pct >= 100) return vmax;
    return vmin + static_cast<s32>(static_cast<float>(pct) / 100.0f * (vmax - vmin));
}

// ═════════════════════════════════════════════════════════════════════════════
//  GUI
// ═════════════════════════════════════════════════════════════════════════════

class SoundbarGui : public tsl::Gui {
public:

    SoundbarGui() = default;

    virtual tsl::elm::Element* createUI() override {

        auto *frame = new tsl::elm::OverlayFrame("Soundbar Mode", "v1.1.0");
        auto *list  = new tsl::elm::List();

        // ── Диапазон громкости ───────────────────────────────────────────

        g_audctl.getVolumeMin(&m_volMin);
        g_audctl.getVolumeMax(&m_volMax);
        if (m_volMax <= m_volMin) {
            m_volMin = 0;
            m_volMax = 15;
        }

        // Текущая громкость
        s32 curVol = m_volMax;
        g_audctl.getSpeakerVolume(&curVol);
        m_currentPercent = volumeToPercent(curVol, m_volMin, m_volMax);
        m_lastDrawnPercent = m_currentPercent;

        // Определяем текущее состояние toggle
        AudioTarget curDefault = AudioTarget_Tv;
        g_audctl.getDefaultTarget(&curDefault);
        bool soundbarOn = (curDefault == AudioTarget_Speaker)
                       || g_audctl.isSoundbarActive();

        // ── 1) Секция: статус ────────────────────────────────────────────
        list->addItem(new tsl::elm::CategoryHeader("Audio Routing"));

        m_statusItem = new tsl::elm::ListItem("Output");
        updateStatusText();
        list->addItem(m_statusItem);

        // Проверка формата TV: если стоит Surround (5.1ch), предупреждаем
        AudioOutputMode tvMode = AudioOutputMode_Invalid;
        g_audctl.getTvAudioOutputMode(&tvMode);
        if (tvMode == AudioOutputMode_Pcm6ch) {
            auto *warnItem = new tsl::elm::ListItem("\u26A0 TV Sound: 5.1ch");
            warnItem->setValue("Set Stereo in Settings!");
            list->addItem(warnItem);
        }

        // ── 2) Тоггл ────────────────────────────────────────────────────
        list->addItem(new tsl::elm::CategoryHeader("Control"));

        auto *toggle = new tsl::elm::ToggleListItem(
            "\uE14B  Soundbar Mode", soundbarOn
        );

        toggle->setStateChangedListener([this](bool state) {
            Result rc = state
                ? g_audctl.enableSoundbarMode()
                : g_audctl.disableSoundbarMode();
            if (R_FAILED(rc)) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "Error: 0x%X", rc);
                if (m_statusItem) m_statusItem->setValue(buf);
            } else {
                updateStatusText();
            }
        });
        list->addItem(toggle);

        // ── 3) Громкость ─────────────────────────────────────────────────
        list->addItem(new tsl::elm::CategoryHeader("Speaker Volume"));

        auto *volumeBar = new tsl::elm::StepTrackBar("\uE13C", kVolumeSteps);
        m_volumeBar = volumeBar;

        u8 initStep = static_cast<u8>(
            static_cast<float>(m_currentPercent) / 100.0f * kVolumeSteps
        );
        volumeBar->setProgress(initStep);

        volumeBar->setValueChangedListener([this](u8 step) {
            int pct = static_cast<int>(
                static_cast<float>(step) / kVolumeSteps * 100.0f
            );
            if (pct > 100) pct = 100;
            m_currentPercent = pct;
            s32 vol = percentToVolume(pct, m_volMin, m_volMax);
            g_audctl.setSpeakerVolume(vol);
            updateStatusText();
        });
        list->addItem(volumeBar);

        // Текстовый индикатор процента
        m_volLabel = new tsl::elm::ListItem("Volume");
        {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%d %%", m_currentPercent);
            m_volLabel->setValue(buf);
        }
        list->addItem(m_volLabel);

        // ── 4) Подсказка ─────────────────────────────────────────────────
        list->addItem(new tsl::elm::CategoryHeader("Info"));

        auto *hintClose = new tsl::elm::ListItem("Tip");
        hintClose->setValue("Press B to close menu");
        list->addItem(hintClose);

        frame->setContent(list);
        return frame;
    }

    virtual bool handleInput(
        u64 /*keysDown*/, u64 /*keysHeld*/,
        const HidTouchState& /*touchPos*/,
        HidAnalogStickState  /*joyStickPosLeft*/,
        HidAnalogStickState  /*joyStickPosRight*/
    ) override {
        return false;
    }

    virtual void update() override {
        // Обновляем строку громкости только при фактическом изменении (не каждый кадр)
        if (m_volLabel && m_lastDrawnPercent != m_currentPercent) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%d %%", m_currentPercent);
            m_volLabel->setValue(buf);
            m_lastDrawnPercent = m_currentPercent;
        }
    }

private:

    void updateStatusText() {
        if (!m_statusItem) return;
        AudioTarget def = AudioTarget_Tv;
        g_audctl.getDefaultTarget(&def);
        char buf[80];
        std::snprintf(buf, sizeof(buf), "%s  |  Vol: %d %%",
            AudctlWrapper::targetName(def), m_currentPercent);
        m_statusItem->setValue(buf);
    }

    tsl::elm::ListItem     *m_statusItem = nullptr;
    tsl::elm::ListItem     *m_volLabel   = nullptr;
    tsl::elm::StepTrackBar *m_volumeBar  = nullptr;

    s32 m_volMin = 0;
    s32 m_volMax = 15;
    int m_currentPercent   = 100;
    int m_lastDrawnPercent = -1;

    static constexpr size_t kVolumeSteps = 20;
};

// ═════════════════════════════════════════════════════════════════════════════
//  Overlay
// ═════════════════════════════════════════════════════════════════════════════

class SoundbarOverlay : public tsl::Overlay {
public:

    virtual void initServices() override {
        g_audctl.initialize();
    }

    virtual void exitServices() override {
        g_audctl.finalize();
    }

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<SoundbarGui>();
    }
};

int main(int argc, char **argv) {
    return tsl::loop<SoundbarOverlay>(argc, argv);
}
