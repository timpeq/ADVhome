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
- [x] Enhance the climate/thermostat widget in the Entity Detail View to cleanly display both the current ambient temperature and the target set temperature.
- [x] Smooth out the battery display, it's currently very jumpy. It is now
      sampled once a second, averaged, and only moves past a 1.5% deadband
      (`Battery.cpp`).
- [x] Send volume and seek while the key is still held, not only once it lets
      go. Both are debounced: a send waits for 500 ms with no change, or for the
      key to come up, and every auto-repeat step resets that timer, so a held
      Up/Down or Left/Right sends nothing until release. Make it a throttle
      instead: send the current target at most every ~250 ms during the hold,
      plus a final send on release. The echo handling has to change with it.
      `_volumeChangedLocally` clears as soon as HA reports any level that moved
      off the baseline, and the seek hold clears once the position lands within
      5 s, so the echo of an intermediate send would drop the local target
      mid-gesture and the next step would rebase on a stale value. Only accept
      an echo after the final send.
- [ ] Show and set brightness in the light detail window. Today it draws only
      the bulb icon and ENTER toggles. Brightness is never read: the state
      parser keeps media and climate attributes but drops a light's
      `brightness` (0-255). The only way to change it is +/- from a list, which
      sends a blind ±10 `brightness_step_pct` from
      `HomeAssistantManager::adjustEntity`. Store it in a light state next to
      `MediaPlayerState` and `ClimateState`, draw it as a percentage bar like
      the media volume, and map Up/Down to it with the same optimistic-target
      pattern as volume (see above), sending an absolute `brightness_pct` so the
      target is authoritative. Hide the bar for on/off-only lights
      (`supported_color_modes` of `onoff`). Implemented on 2026-09-10 but not
      yet tried on hardware: the Zigbee coordinator had dropped off Home
      Assistant, so there were no dimmable lights to test against.

## Phase 2: Home Dashboard Completion
- [x] Replace the first `Favs` top-level tab with `Home`.
- [x] Move Favorites into a Home section instead of keeping it as a separate top-level view.
- [x] Make the Home screen show Favorites first, followed by modular widgets.
- [x] Define a widget interface with drawing and input handling.
- [ ] Extract domain-specific detail rendering into reusable widgets (start with media player).
- [ ] Add a Home widget registry so widgets can be enabled and ordered dynamically.
- [ ] Decide whether Favorites should remain duplicated in Entities or be removed once Home is stable. Note that Entities > Favorites is now the only place the order can be edited, so removing it needs somewhere else to put reordering.
- [x] Let the user set the Favorites order that the `ORDER` sort displays.
- [ ] Add a Home visual treatment using a small bitmap or an appropriate Home Assistant mark.
- [ ] Keep the battery indicator and connection state available to Home widgets.
- [x] Add a Chat top-level tab using Home Assistant's text conversation WebSocket API.
- [x] Make the Chat tab optional for users who do not want to use Home Assistant agents.

## Phase 3: Hardware Integration
- [x] Make the device sleep and wake on a button press. Held `ESC` sleeps on
      demand and the idle ladder sleeps automatically; the depth and wake
      source are settings. See Phase 7. Waking on gyro movement is a
      separate problem and is not reachable from firmware alone; see Known
      limitations.
- [ ] Make ESC an escalating back button (close modals -> top of view -> home page -> top of home -> sleep).
- [x] Add short hardware notes covering speaker, microphone, gyro, sleep, and
      wake capabilities. The README now carries the power ladder, the TTS and
      codec notes, and an ADV-versus-original compatibility section.
- [x] Investigate Cardputer speaker and microphone APIs.
- [x] Add experimental push-to-talk microphone capture for Assist conversations using the Cardputer GO button.
- [x] Verify and implement Assist pipeline audio session/framing over the authenticated WebSocket (high effort, about 2-4 weeks).
- [x] Decode Assist audio responses and play them through the Cardputer speaker, with text fallback (high effort, about 1-2 weeks after transport).
- [ ] Let GO barge in on a spoken reply: stop playback and start a new request
      without waiting for the answer to finish. Two halves, and only the first
      is small. Stopping is a few lines — `stopVoicePlayback()` already stops
      the channel, releases the codec and frees the buffers, so a GO press
      during playback can silence it at once. Starting a new turn is the hard
      half: TTS tears the WebSocket down to free the TLS buffers and only
      reconnects from `stopVoicePlayback()`, so `startVoicePipeline()` will fail
      for as long as the reconnect takes. Needs a pending-record state in
      `ChatView` that arms on the GO press and starts the microphone once the
      connection is authenticated again, plus a status line saying why the
      device is not listening yet.
- [ ] "Speaking" outlives the sound by a second or two. The label clears only
      when the HTTP stream is considered finished, which needs the socket closed
      plus a 700 ms grace, AND the speaker idle. Audio can run out well before
      the server drops the connection, so the label is reporting the stream, not
      the sound. Clearing it when the channel goes idle and no bytes have
      arrived for a short while would track what the user actually hears; keep
      the existing condition for the teardown itself, which does need the stream
      to be done.

## Phase 4: Polish & Stability
- [x] Move the remaining entity categories into the Entities top-level tab as sub-tabs.
- [x] Put Favorites first in the Entities sub-tab sequence, then alphabetize domains with `All` second.
- [ ] Add explicit service capability checks before showing controls for each Home Assistant domain.
- [ ] Add `input_text` and `todo` to the entity domains. Both are dropped today
      by `EntityManager::isSupportedDomain`, and each needs a sub-tab in
      `EntitiesView`'s list. `input_text` is the easy one: show the value, and
      let ENTER open a text field that calls `input_text.set_value`, capped at
      the entity's `max` length. `todo` is bigger. The entity's state is only
      the count of open items, so the detail view has to fetch the items itself
      (`todo.get_items`, a service call with `return_response`), list them,
      tick them off with `todo.update_item` and add new ones with
      `todo.add_item`. Keep the fetched list bounded; the domain filter exists
      to save RAM.
- [ ] Add a small widget layout test or rendering fixture for overflow and screen bounds.
- [x] Add a reset or reconfiguration action for Wi-Fi and Home Assistant credentials.
- [ ] Add optional authenticated Home Assistant album-art thumbnails using a bounded JPEG cache.

## Phase 5: Connection Management

The basics are in: Menu > Settings has Wi-Fi and Home Assistant pages that show
the current connection and can clear it, and the connecting/reconnecting screens
accept `ESC` (Wi-Fi) and `H` (Home Assistant) so a device with a bad saved
network is never stranded. Everything below is deferred.

- [x] Show current SSID, IP, and RSSI on a Wi-Fi page, in Menu > Settings.
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
- [x] Decide whether `W`/`S` and `A`/`D` should navigate entity lists. Decided
      against: they stay type-ahead letters, because a list you search by name
      is worth more than a second set of arrows. `W`/`S` (and `J`/`K`) still
      scroll the first-setup Wi-Fi picker, which has no type-ahead to lose, and
      `S` still stops playback in the media player. The Help page no longer
      carries a note denying any of this.
- [ ] Surface `P` (play/pause) and `S` (stop) in the media player's on-screen
      hint, or drop them. They work but only the Help page mentions them.
- [ ] Check the detail window's layout for every supported domain at both short
      and long state strings, including `unavailable`.

## Phase 7: Power

Done: the panel is put into sleep-in rather than only blanked, the ES8311 codec
is released whenever the screen goes off, the radio drops to `WIFI_PS_MAX_MODEM`
while the display is off, and sleep depth and wake source are now
separate settings: "Deep Sleep" (OFF / LIGHT / DEEP) and "Wake On GO Only".

- [x] Give the held-ESC gesture visible feedback; it was silent until the key
      came up, so there was no way to know when to let go.
- [x] Drain the TCA8418 event FIFO before arming a wake. The queued release
      event held INT low and woke the device the instant it slept.
- [x] Count held keys as activity, so holding an arrow to scroll a long list no
      longer lets the screen dim mid-gesture.
- [x] Test DEEP. Confirmed working on a Cardputer ADV, 2026-09-09.
- [ ] **Measure it.** None of the above has been verified with a meter, only
      reasoned from the datasheets and the driver source. Get a USB power meter
      or an inline shunt and record actual draw at each level: NORMAL, DIM,
      DISPLAY_OFF, SOFT_SLEEP, light sleep, deep sleep. Without numbers there is
      no way to know which of these changes actually mattered.
- [ ] Consider whether `WIFI_PS_MAX_MODEM` delays Home Assistant state pushes
      enough to be noticeable when the screen comes back on, and whether the
      websocket survives long idle periods under it.
- [x] Document that IMU movement counts as activity only while the firmware is
      still running, so it holds off DIM, DISPLAY_OFF and SOFT_SLEEP but cannot
      wake the device once it is actually asleep. Stated in the README's power
      section. The interrupt alternative is in Known limitations.
- [ ] The IMU runs continuously even when the screen is off. Check whether it
      can be put into low-power mode between polls.
- [ ] Deep sleep loses the Home Assistant connection and re-fetches all entity
      state on wake. Measure how long that takes; if it is slow, the GO-only
      mode may be worse overall than staying in light sleep.
- [ ] Consider a timed wake so the device can refresh state periodically without
      user input (`M5.Power.timerSleep`), if that is ever wanted.
- [ ] Check whether the TCA8418 keyboard controller can be put into its own
      low-power mode while the device sleeps.

## Known limitations

Not tasks. These are things the project has decided it cannot close from here,
recorded so they stop being rediscovered. They carry no checkbox because nobody
working on the firmware can tick them.

**Wake on movement is not reachable from firmware.** The ADV's BMI270 is polled
as an activity source, which is why picking the device up holds off the idle
ladder. Making it a genuine wake source is blocked twice over. M5Unified's
BMI270 driver has no interrupt support at all — it never writes an `INT_MAP` or
feature register, and `IMU_Base::setINTPinActiveLogic` is a stub returning
`false` that only the MPU6886 overrides — so any-motion detection would have to
be written against the part's feature engine directly. Separately, nothing in
M5Unified names an IMU interrupt GPIO for the ADV, so whether `INT1` is routed to
the ESP32-S3 at all is unverified and needs M5Stack's schematic; if it is not,
this is board rework rather than code. Even with a routed pin, deep sleep wakes
through `ext1`, which requires GPIO0-21, so a pin above that would give movement
wake from light sleep only.

**Deep sleep on the original Cardputer is untested.** `DEEP` with keyboard wake
latches the matrix rows low with `gpio_hold_en()` so a press can still pull a
column down while the digital core is off. That path was written from the
ESP32-S3 reference and has never been run, because nobody working on the project
owns an original Cardputer. `Wake On GO Only` avoids the path entirely. This
closes the first time someone with the hardware reports back.
