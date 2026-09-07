# ADVhome Roadmap

## Phase 1: Core UX & Input Refactoring
- [x] Use Up/Down arrows for volume control on the media detail page.
- [x] Create a code request to allow cover, lock, and alarm control.
- [x] Fix volume debouncing and input unity in EntityDetailView.
- [x] Implement hold-down to fast-forward for media player.
- [x] Fix general time display in media player by tracking time locally instead of waiting for HA updates.
- [ ] Fix the 'modal' view of the details so it doesn't change the top bar and looks like a window below the entity.

## Phase 2: Home Dashboard Completion
- [x] Replace the first `Favs` top-level tab with `Home`.
- [x] Move Favorites into a Home section instead of keeping it as a separate top-level view.
- [x] Make the Home screen show Favorites first, followed by modular widgets.
- [x] Define a widget interface with drawing and input handling.
- [ ] Extract domain-specific detail rendering into reusable widgets (start with media player).
- [ ] Add a Home widget registry so widgets can be enabled and ordered dynamically.
- [ ] Decide whether Favorites should remain duplicated in Entities or be removed once Home is stable.
- [ ] Add a Home visual treatment using a small bitmap or an appropriate Home Assistant mark.
- [ ] Keep the battery indicator and connection state available to Home widgets.

## Phase 3: Hardware Integration
- [ ] Make the device sleep and wake on a button press or gyro movement.
- [ ] Make ESC an escalating back button (close modals -> top of view -> home page -> top of home -> sleep).
- [ ] Add short hardware notes covering speaker, microphone, gyro, sleep, and wake capabilities.
- [ ] Investigate Cardputer speaker and microphone APIs.

## Phase 4: Polish & Stability
- [x] Move the remaining entity categories into the Entities top-level tab as sub-tabs.
- [x] Put Favorites first in the Entities sub-tab sequence, then alphabetize domains with `All` second.
- [ ] Add explicit service capability checks before showing controls for each Home Assistant domain.
- [ ] Add a small widget layout test or rendering fixture for overflow and screen bounds.
- [ ] Add a reset or reconfiguration action for Wi-Fi and Home Assistant credentials.
- [ ] Add optional authenticated Home Assistant album-art thumbnails using a bounded JPEG cache.
- [ ] Keep generated PlatformIO build output out of future feature commits unless a release artifact is intentionally required.