# Soundbar Mode — Tesla Overlay для Nintendo Switch

[![Platform](https://img.shields.io/badge/Платформа-Nintendo%20Switch-e60012.svg)](https://www.nintendo.com/)
[![Framework](https://img.shields.io/badge/Фреймворк-libtesla%20%7C%20libnx-blue.svg)](https://github.com/WerWolv/libtesla)
[![License: MIT](https://img.shields.io/badge/Лицензия-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Release](https://img.shields.io/github/v/release/Exxauxted/soundbar-nx)](https://github.com/Exxauxted/soundbar-nx/releases/latest)

[**English Version**](README.md)

Оверлей Tesla / Ultrahand (`.ovl`) для Horizon OS, позволяющий принудительно выводить звук через **встроенные динамики** консоли, когда она находится **в доке**, оставляя видеовыход по HDMI на телевизоре или мониторе. Превратите Switch в компактный саундбар!

---

## Возможности

- **Переключатель «Soundbar Mode»**: Мгновенное перенаправление звука из дока на встроенные динамики.
- **Регулировка громкости**: Дискретный слайдер на 20 шагов (`StepTrackBar`) для управления громкостью динамиков.
- **Мгновенное переключение**: Нулевая задержка без зависающих системных таймеров затухания (`fade_ns = 0`).
- **Защита от зависаний в играх**: Специально спроектированная логика IPC, исключающая переполнение буферов HDMI DMA и дедлоки аудиопотоков в играх.
- **Автоопределение 5.1ch Surround**: Предупреждает, если вывод звука ТВ настроен на объёмный звук (что может перегружать даунмиксер).
- **Сохранение состояния**: Звук продолжает идти через динамики после закрытия меню вплоть до перезагрузки консоли или ручного выключения.

---

## Требования

Для работы оверлея на Nintendo Switch должна быть установлена кастомная прошивка Atmosphere и среда оверлеев Tesla:

| Компонент | Описание |
|---|---|
| **Atmosphere CFW** | Кастомная прошивка (версия 1.0.0+) |
| **[nx-ovlloader](https://github.com/WerWolv/nx-ovlloader)** | Сис-модуль загрузки оверлеев (`sd:/atmosphere/contents/420000000007E51A/`) |
| **[Tesla-Menu](https://github.com/WerWolv/Tesla-Menu)** или **[Ultrahand](https://github.com/ppkantorski/Ultrahand-Overlay)** | Меню оверлеев (`sd:/switch/.overlays/ovlmenu.ovl`) |

---

## Установка

1. Скачайте `soundbar-mode.ovl` со страницы [Последних релизов](https://github.com/Exxauxted/soundbar-nx/releases/latest).
2. Скопируйте файл `soundbar-mode.ovl` на SD-карту Switch в директорию:
   ```
   sd:/switch/.overlays/soundbar-mode.ovl
   ```
3. Вставьте SD-карту в консоль и запустите Atmosphere.

---

## Использование

1. Установите консоль в док с подключённым HDMI-кабелем к телевизору или монитору.
2. Откройте меню Tesla стандартной комбинацией кнопок:  
   **`L` + `Крестовина вниз` + `Нажатие правого стика (R3)`**
3. Выберите пункт **Soundbar Mode**.
4. Переведите переключатель **Soundbar Mode** в положение **ВКЛ**.
5. Настройте комфортную громкость динамиков ползунком.
6. Нажмите **`B`**, чтобы полностью закрыть меню оверлея. Звук продолжит идти через встроенные динамики консоли.

---

## ⚠️ Важно: предотвращение зависаний в играх

Чтобы игры работали плавно, без микрофризов и зависаний:

1. **Установите звук ТВ в режим «Стерео»**:  
   Перейдите в:  
   **Настройки системы → Вывод на ТВ → Звук ТВ → выберите «Стерео»** (не «Объёмный звук» и не «Авто»).  
   *Почему:* Если в доке выбран объёмный звук 5.1, игры передают в систему 6 несжатых каналов PCM. Даунмиксинг 6 каналов в 2-канальные динамики под высокой нагрузкой может приводить к переполнению аудиобуферов и зависанию потока рендеринга игры.
2. **Закрывайте оверлей кнопкой `B`**:  
   Всегда закрывайте меню кнопкой `B`, а не скрывайте его повторным нажатием комбинации кнопок, чтобы процесс оверлея полностью выгрузился из оперативной памяти.

---

## Техническое описание

Horizon OS управляет аудиомаршрутизацией через сервис `audctl` (Audio Controller):

| ID | Target | Описание |
|:--:|---|---|
| `1` | `AudioTarget_Speaker` | Встроенные динамики (кодек Realtek ALC5640 / ALC5639, шина I2S) |
| `2` | `AudioTarget_Headphone` | Разъём для наушников 3.5 мм |
| `3` | `AudioTarget_Tv` | HDMI-аудио контроллера дисплея Tegra X1 |

В штатном режиме при подключении дока система принудительно назначает вывод на `AudioTarget_Tv`.

### Особенности реализации

- **`audctlSetDefaultTarget(AudioTarget_Speaker, 0, 0)`**: Назначает целевой аудиовыход по умолчанию на уровне системной политики.
- **ТВ НЕ мутится принудительно**: Принудительный вызов `SetTargetMute(Tv, true)` останавливает приём пакетов HDMI Audio DMA. Запущенные игры, ожидающие подтверждения освобождения буферов через `svcWaitSynchronization`, блокируются и зависают. Отсутствие ручного mute ТВ полностью предотвращает эту проблему.
- **Исключён `audctlSetOutputTarget`**: Не используется низкоуровневый оверрайд, конфликтующий с диспетчером политик Horizon OS.

---

## Сборка из исходников

### 1. Установка devkitPro и switch-dev

```bash
# macOS (через Homebrew)
brew install devkitpro-pacman
sudo dkp-pacman -S switch-dev

# Arch Linux / Ubuntu (dkp-pacman)
sudo dkp-pacman -S switch-dev
```

### 2. Настройка переменных окружения

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITPRO/tools/bin:$DEVKITA64/bin:$PATH
```

### 3. Клонирование репозитория

```bash
git clone --recursive https://github.com/Exxauxted/soundbar-nx.git
cd soundbar-nx
```

### 4. Сборка

```bash
make
```

Готовый файл: `out/soundbar-mode.ovl`.

---

## Структура проекта

```
soundbar-nx/
├── include/
│   └── audctl_ipc.hpp       # C++ обёртка над libnx audctl API
├── source/
│   └── main.cpp             # Tesla overlay GUI и логика жизненного цикла
├── lib/
│   └── libtesla/            # Фреймворк оверлеев (submodule)
├── Makefile                 # Сборка на switch_rules
├── LICENSE                  # Лицензия MIT
├── README.md                # Английская документация
└── README_RU.md             # Русская документация
```

---

## Лицензия

Проект распространяется под лицензией [MIT](LICENSE).
