# ADVhome Roadmap

## Phase 1: Core UX & Input Refactoring
- [x] Use Up/Down arrows for volume control on the media detail page.
- [x] Create a code request to allow cover, lock, and alarm control.
- [x] Fix volume debouncing and input unity in EntityDetailView.
- [x] Implement progressive hold-down to fast-forward for media player with Min/Max settings.
- [x] Fix Config view scrolling to support more settings than fit on screen.
- [x] Add heap memory, WiFi RSSI, and build date tracking to Diagnostic View.
- [x] Fix general time display in media player by tracking time locally instead of waiting for HA updates.
- [x] Fix the 'modal' view of the details so it doesn't change the top bar and looks like a window below the entity.
- [ ] Add cursor-based navigation from list boundaries into the top-level tabs.
- [x] Replace the flat Config tab with a Menu tab holding Settings, Wi-Fi, Home Assistant, and About.
- [x] Add an on-device About & License page so attribution survives a firmware-only install.
- [ ] Enhance the climate/thermostat widget in the Entity Detail View to cleanly display both the current ambient temperature and the target set temperature.

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
- [x] Add a Chat top-level tab using Home Assistant's text conversation WebSocket API.
- [x] Make the Chat tab optional for users who do not want to use Home Assistant agents.

## Phase 3: Hardware Integration
- [ ] Make the device sleep and wake on a button press or gyro movement.
- [ ] Make ESC an escalating back button (close modals -> top of view -> home page -> top of home -> sleep).
- [ ] Add short hardware notes covering speaker, microphone, gyro, sleep, and wake capabilities.
- [ ] Investigate Cardputer speaker and microphone APIs.
- [x] Add experimental push-to-talk microphone capture for Assist conversations using the Cardputer GO button.
- [ ] Verify and implement Assist pipeline audio session/framing over the authenticated WebSocket (high effort, about 2-4 weeks).
- [ ] Decode Assist audio responses and play them through the Cardputer speaker, with text fallback (high effort, about 1-2 weeks after transport).

## Phase 4: Polish & Stability
- [x] Move the remaining entity categories into the Entities top-level tab as sub-tabs.
- [x] Put Favorites first in the Entities sub-tab sequence, then alphabetize domains with `All` second.
- [ ] Add explicit service capability checks before showing controls for each Home Assistant domain.
- [ ] Add a small widget layout test or rendering fixture for overflow and screen bounds.
- [x] Add a reset or reconfiguration action for Wi-Fi and Home Assistant credentials.
- [ ] Add optional authenticated Home Assistant album-art thumbnails using a bounded JPEG cache.
- [ ] Keep generated PlatformIO build output out of future feature commits unless a release artifact is intentionally required.

## Phase 5: Connection Management

The basics are in: the Menu tab has Wi-Fi and Home Assistant pages that show the
current connection and can clear it, and the connecting/reconnecting screens
accept `ESC` (Wi-Fi) and `H` (Home Assistant) so a device with a bad saved
network is never stranded. Everything below is deferred.

- [x] Show current SSID, IP, and RSSI on a Wi-Fi page reachable from the Menu.
- [x] Show the Home Assistant URL and version on a Home Assistant page.
- [x] Confirm-before-clearing on both, with an auto-disarming confirmation.
- [x] Offer credential recovery keys from the connecting and reconnecting screens.
- [ ] Reconfigure without rebooting. Clearing credentials currently restarts the
      device, because `HomeAssistantManager` reads config only in `begin()` and
      the views hold references to it. Needs a teardown/rebuild path, or a
      manager that can re-read config and reconnect in place.
- [ ] Store multiple Wi-Fi profiles instead of a single SSID/password pair.
      `ConfigManager` currently keeps one of each in NVS; this needs a list, a
      selected index, and a migration for existing single-profile devices.
- [ ] Auto-try the next profile on connection failure, with a bounded number of
      attempts before falling back to the scan list.
- [ ] Pick the strongest known profile at boot instead of the last one used.
- [ ] Edit Wi-Fi credentials on the device rather than only forgetting and
      re-entering them.
- [ ] Re-open the browser setup portal on demand from the Home Assistant page,
      instead of only reaching it by clearing the config and rebooting.
- [ ] Store multiple Home Assistant server profiles (home vs. remote URL) and
      pick between them.
- [ ] Add a "Test connection" action that validates a URL and token before
      saving, so a typo does not require a reboot to discover.
