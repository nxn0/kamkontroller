# KamKontroller

KamKontroller is a Linux desktop camera console for V4L2 cameras. It brings a live preview,
camera controls, video recording, still photos, and timelapse capture into one Qt application.
Controls and menu values are read from the selected device at runtime, so the interface follows
what each camera actually exposes.

## What It Does

- Discovers available V4L2 video devices and switches between cameras.
- Shows a live FFmpeg preview.
- Reads, adjusts, and resets the controls reported by `v4l2-ctl`.
- Captures H.264 MP4 video, JPEG photos, and timelapse video.
- Lets you choose a persistent default folder for captures.
- Shows FFmpeg output in an in-app log window to help diagnose capture problems.

During capture, the preview pauses because FFmpeg takes ownership of the camera stream, then
reconnects when capture finishes. Timelapse output is encoded at 24 fps and has a maximum
capture duration of 75,600 seconds.

## Requirements

- Linux with a V4L2-compatible camera
- CMake 3.21 or newer
- A C++17 compiler
- Qt 6.2 or newer, with the Widgets component
- `v4l2-ctl` (provided by `v4l-utils`)
- `ffmpeg`

The application launches without a camera, but camera features require the runtime tools and a
device the current user can access. On many Linux distributions, camera access is granted through
membership in the `video` group; log out and back in after changing group membership.

## Build and Run

```sh
cmake -S . -B build
cmake --build build
./build/kamkontroller
```

To install the built executable into the configured install prefix:

```sh
cmake --install build
```

## Using KamKontroller

1. Connect a camera and click **Scan** if it is not listed. Use the camera picker to select a
	 device.
2. The live preview starts automatically. Open **Controls** to adjust the controls reported by
	 the selected camera, or choose **Reset defaults** to restore its reported defaults.
3. Choose **Video**, **Camera**, or **Timelapse** mode and use the matching capture button.
4. Open **Settings > Default save location...** to choose one folder for all captures. This
	 setting persists between launches.
5. If FFmpeg reports an error, open the log viewer at the bottom of the window for details.

By default, captures are placed in `~/kamkontroller/videos`, `~/kamkontroller/pictures`, or
`~/kamkontroller/timelapse`, depending on the capture type. The required folders are created
automatically. When a custom default save location is set, all capture types use that folder.

## Troubleshooting

- **No camera appears:** check that the camera is connected and visible to `v4l2-ctl --list-devices`.
- **Permission denied:** confirm the current user can access the camera device (often through the
	Linux `video` group).
- **Preview or capture does not start:** confirm `ffmpeg` and `v4l2-ctl` are installed and on
	`PATH`, and that another application is not using the camera.
- **Controls are missing or unavailable:** the app can only show controls reported by the selected
	device; some devices expose no adjustable controls.
- **Capture fails:** inspect the in-app FFmpeg log and verify that the destination folder is
	writable and that the camera supports the requested stream.

## Contributing

Bug reports, fixes, and focused improvements are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md)
before opening a pull request. By participating, you agree to follow the
[Code of Conduct](CODE_OF_CONDUCT.md).

## License

KamKontroller is distributed under the GNU General Public License, version 3 only. See
[LICENSE](LICENSE) for the complete license text.