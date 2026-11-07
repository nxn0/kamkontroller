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
**Video**, **Camera**, and **Timelapse** modes use FFmpeg and save files in the user's Movies
directory. The preview can be disabled while recording from the control drawer for low-end
devices. Camera permissions are inherited from the user running the application; on Linux,
that user generally needs access to the `video` device group.