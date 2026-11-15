# Contributing to KamKontroller

Thanks for helping improve KamKontroller. Contributions of bug reports, documentation, fixes,
and focused features are welcome.

## Before You Start

- For bugs, search existing reports first and include what you expected, what happened, and the
  steps needed to reproduce it.
- For substantial changes, open an issue or discussion first so the proposed behavior can be
  agreed on before implementation.
- Do not include private camera footage, personal data, or unredacted device logs in an issue or
  pull request.

## Development Setup

KamKontroller requires Linux, CMake 3.21 or newer, a C++17 compiler, Qt 6.2 or newer with
Widgets, `v4l2-ctl` from `v4l-utils`, and `ffmpeg`. Configure and build with:

```sh
cmake -S . -B build
cmake --build build
```

Launch the app with:

```sh
./build/kamkontroller
```

## Making a Change

1. Create a focused branch for your change.
2. Keep changes scoped to one behavior or closely related set of improvements.
3. Follow the surrounding C++ and Qt style: four-space indentation, descriptive names, and the
   existing Qt Widgets and CMake structure.
4. Update the README or other documentation when user-visible behavior or setup steps change.
5. Build the project and run the relevant manual checks before submitting.

There is currently no automated test suite configured in CMake. For changes to camera behavior,
also test with a V4L2 camera where practical: scan for the device, open its controls, and exercise
the affected preview or capture workflow. If hardware testing is not available, say so in the PR.

## Pull Requests

- Use a clear title that describes the user-visible change.
- Explain the problem and the approach, and call out any limitations or behavior changes.
- Include build and manual test results. Add screenshots for meaningful UI changes when they can
  be captured without exposing private footage or data.
- Keep unrelated cleanup out of the PR.
- Be responsive to review feedback and update the PR description when the scope changes.

The pull request template includes a checklist to help reviewers understand and verify the change.
By participating, you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).