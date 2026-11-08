# KamKontroller

A Qt 6 desktop camera app for discovering V4L2 devices, viewing a live FFmpeg preview,
recording video, taking photos, creating timelapses, and adjusting every control reported
by `v4l2-ctl`.

## Build

Requirements: Qt 6 Widgets, CMake, a C++17 compiler, `v4l2-ctl` from `v4l-utils`, and
`ffmpeg`.

```sh
cmake -S . -B build
cmake --build build
./build/kamkontroller
```

The main screen is the live camera preview. Use **Controls** to open the semi-transparent
control drawer; its controls and menu values are read from the selected device at runtime.
**Video**, **Camera**, and **Timelapse** modes use FFmpeg. Timelapse records a video at 24 fps
with a 75,600-second maximum duration. By default, videos, timelapses, and pictures are saved
under `~/kamkontroller/videos`, `~/kamkontroller/timelapse`,
and `~/kamkontroller/pictures`, respectively. Missing folders are created automatically. Use
**Settings > Default save location...** to choose a custom folder for all captures; the choice
is persisted between launches. The preview pauses with a blurred still frame while FFmpeg is
capturing from the camera. Camera permissions are inherited from the user
running the application; on Linux, that user generally needs access to the `video` device
group.