# KamKontroller

A Qt 6 desktop panel for discovering V4L2 camera devices and adjusting the controls reported by `v4l2-ctl`.

## Build

Requirements: Qt 6 Widgets, CMake, a C++17 compiler, and `v4l2-ctl` from `v4l-utils`.

```sh
cmake -S . -B build
cmake --build build
./build/kamkontroller
```

The application reads each device's controls and menu options at runtime. Camera permissions are inherited from the user running the application; on Linux, that user generally needs access to the `video` device group.