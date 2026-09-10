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

## Phase 6: First Public Release

Everything that has to be true before the repository goes public and the M5Stack
forum post goes up. Ordered roughly by what blocks what.

**Off-device, on Tim:**

- [ ] **Set up the `peq.me/advhome` redirect on the Caddy server.** The About &
      License page on the device and `THIRD-PARTY-NOTICES.md` both point people
      there, and the LGPL-2.1 relink obligation is satisfied by that link
      resolving to the source. It must exist before any binary is handed out.
      Point it at the GitHub repository once that exists:
      `redir /advhome https://github.com/timpeq/advhome permanent`
- [ ] Create the GitHub repository and push `master`. Currently the only remote
      is `omen`, which is Tim's own machine.
- [ ] Publish the remote-reflex car audio post on peq.me first, then the ADVhome
      post one to two weeks later. Drafts live in `~/code/advhome-blog/posts/`.

**In the repository:**

- [x] Update the README's "Voice roadmap" section. It still describes voice as
      unimplemented and estimates 2-4 weeks; voice shipped on 2026-09-07. Add a
      "this shipped" note rather than deleting the estimate.
- [ ] Add a CONTRIBUTING or issue template asking for the things a bug report
      actually needs: Home Assistant version, entity domain, what the Cardputer
      showed versus what HA shows, serial output, and standalone vs. M5Launcher.
- [ ] Decide what to do about the tracked `advhome.bin`, `advhome_merged.bin`
      and `ptable.bin` at the repository root. Shipping a stale merged image
      next to a warning never to flash it is a foot-gun for a new reader.
- [ ] Take screenshots or photos of the Home, Entities, detail, Chat and Menu
      screens for the README and the forum post.
- [ ] Confirm first-boot works from a fully erased device, not just from an
      already-configured one.

**Polish still open:**

- [x] Audit the remaining hint lines outside `EntityDetailView` for the shared
      footer position and colour.
- [x] Add an on-device Help & Shortcuts page, compiled from the input handlers.
- [x] Tell a new device's empty Favorites screen how to add favorites.
- [x] Make the list SPACE toggle and +/- adjustment optional settings.
- [ ] Decide whether `W`/`S` and `A`/`D` should navigate entity lists. They do
      not today: they feed the type-ahead search like any other letter, and only
      the Wi-Fi picker treats `W`/`S` as up and down. The README claimed
      otherwise until now. Either wire them up and lose those letters from
      type-ahead, or leave it and keep the docs honest.
- [ ] Surface `P` (play/pause) and `S` (stop) in the media player's on-screen
      hint, or drop them. They work but only the Help page mentions them.
- [ ] Check the detail window's layout for every supported domain at both short
      and long state strings, including `unavailable`.
