# ADVhome Roadmap

## Proposed Navigation And Home Experience

- [x] Replace the first `Favs` top-level tab with `Home`.
- [x] Move Favorites into a Home section instead of keeping it as a separate top-level view.
- [ ] Add a Home visual treatment using a small bitmap or an appropriate Home Assistant mark if licensing and asset size allow; otherwise use a simple device-native home header.
- [x] Make the Home screen show Favorites first, followed by modular widgets.
- [x] Define a widget interface with drawing and input handling.
- [ ] Extract domain-specific detail rendering into reusable widgets, starting with media player now-playing/progress/volume.
- [ ] Add additional Home widgets as their domain renderers become reusable.
- [x] Move the remaining entity categories into the Entities top-level tab as sub-tabs.
- [x] Put Favorites first in the Entities sub-tab sequence, then alphabetize all remaining domain tabs with `All` second.
- [ ] Decide whether Favorites should remain duplicated in Entities during the transition or be removed once Home is stable.

## Feature Requests

- [x] Use Up/Down arrows for volume control on the media detail page.
- [x] Create a code request to allow cover, lock, and alarm control.
- [ ] Make the device sleep and wake on a button press or gyro movement.
- [ ] Make ESC an escalating back button, where it closes modals, then backs to the top of the view, then back to the home page, then back to the top of the home page view, and then make holding the button instantly sleep (and ignore gyro for wake up for (configurable) seconds)
- [ ] Investigate the Cardputer speaker and microphone APIs and hardware behavior.
- [ ] Fix the 'modal' view of the details, it say the entity twice, where i don't actually want it to change the top bar, i just want it to look like a 'window' below that entity's view.

## Quick Follow-Ups

- [ ] Add a Home widget registry so widgets can be enabled and ordered without changing the main controller.
- [ ] Add a small widget layout test or rendering fixture for overflow and screen bounds.
- [ ] Add explicit service capability checks before showing controls for each Home Assistant domain.
- [ ] Keep the battery indicator and connection state available to Home widgets.
- [ ] Add a reset or reconfiguration action for Wi-Fi and Home Assistant credentials.
- [ ] Add a short hardware notes section covering the speaker, microphone, gyro, sleep, and wake capabilities.
- [ ] Add optional authenticated Home Assistant album-art thumbnails using a bounded JPEG cache; keep the native music icon as the fallback.
- [ ] Keep generated PlatformIO build output out of future feature commits unless a release artifact is intentionally required.