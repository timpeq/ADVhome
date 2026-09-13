# ADVhome tasks

One ordered list, most important first. Reorder freely: position is priority.

- **IDs are permanent.** `T-14` means the same task after any reshuffle, and an
  ID is never reused. The next free ID is **T-39**.
- **Tags say who can finish it.** `agent`: the repository and a build are
  enough. `device`: done means checked on the Cardputer, even if an agent writes
  the code. `tim`: needs Tim, for an account, a server, a purchase or a decision.
- **Finishing a task deletes it**, in the same commit that completes it, with
  the ID in the commit subject (`T-14: ...`). `git log --grep T-14` is the
  record; this file only holds open work.

The phase-based roadmap this replaced, including everything already built, is
archived at [docs/archive/2026-09-12-roadmap.md](docs/archive/2026-09-12-roadmap.md).

## Tasks

1. **T-02 · Confirm first boot from a blank state** `device`
    Every test so far started from a configured device. Walk the whole path:
    Wi-Fi scan, password, setup portal, token, first connect, and note anything
    confusing. On the shared M5Launcher device, do not erase the chip: that
    wipes the launcher's partition layout (see AGENTS.md). Clearing ADVhome's
    `advhome` Preferences namespace gives this app the same first-boot state.

2. **T-03 · Add a CONTRIBUTING file or issue template** `agent`
    Ask for what a bug report needs: Home Assistant version, entity domain,
    what the Cardputer showed versus what Home Assistant shows, serial output,
    standalone or M5Launcher, and ADV or original Cardputer.

3. **T-04 · Screenshots or photos for the README and forum post** `tim` `device`
    Home, Entities, a detail window, Chat and Menu. (A desktop simulator build
    using M5GFX's SDL backend could render exact screenshots instead; not
    started.)

4. **T-07 · Publish the blog posts** `tim`
    The remote-reflex car audio post on peq.me first, then the ADVhome post one
    to two weeks later. Drafts are in `~/code/advhome-blog/posts/`.

5. **T-08 · Try light brightness on a dimmable light** `device`
    Built in `18bc413` but never run against a real dimmable light, because the
    Zigbee coordinator had dropped off Home Assistant. Check that the detail
    window draws brightness as a percentage bar, Up/Down move it and send an
    absolute `brightness_pct`, the bar holds steady through Home Assistant's
    echoes, and on/off-only lights (`supported_color_modes` of `onoff`) show no
    bar.

6. **T-09 · Clear "Speaking" when the sound stops, not the stream** `agent` `device`
    The label outlives the audio by a second or two. It clears only when the
    HTTP stream counts as finished (socket closed plus a 700 ms grace) and the
    speaker is idle, so it reports the download rather than what the user
    hears. Clear it once the TTS channel is idle and no bytes have arrived for
    a short while; keep the existing condition for the teardown itself, which
    does need the stream done. See `HomeAssistantManager::pumpTtsWavStream()`.

7. **T-10 · Surface `P` and `S` in the media player's hint, or drop them** `agent`
    Play/pause and stop work, but only the Help page mentions them.

8. **T-11 · Check the detail window layout for every domain** `agent` `device`
    With short and long state strings, and with `unavailable`. T-29's rendering
    fixture could do this without the device.

9. **T-12 · Make ESC an escalating back button** `agent`
    Close modals, then top of view, then Home, then top of Home, then sleep.
    Today ESC closes modals and a held ESC sleeps if enabled; the steps in
    between do not exist.

10. **T-13 · Navigate from a list's top edge into the tab bar** `agent`
    Cursor-based navigation from list boundaries into the top-level tabs.

11. **T-14 · Check service capabilities before showing controls** `agent`
    Per domain, check what the entity supports (for example
    `supported_features`) before offering a control it cannot perform.

12. **T-15 · Add `input_text` and `todo` entities** `agent` `device`
    Both are dropped by `EntityManager::isSupportedDomain`, and each needs a
    sub-tab in `EntitiesView`. `input_text` is the easy one: show the value and
    let ENTER open a text field that calls `input_text.set_value`, capped at the
    entity's `max` length. `todo` is bigger: the state is only the count of open
    items, so the detail view fetches them itself (`todo.get_items`, a service
    call with `return_response`), lists them, ticks them off with
    `todo.update_item` and adds new ones with `todo.add_item`. Keep the fetched
    list bounded; the domain filter exists to save RAM.

13. **T-16 · Reconfigure without rebooting** `agent`
    Clearing credentials restarts the device, because `HomeAssistantManager`
    reads config only in `begin()` and the views hold references to it. Needs a
    teardown and rebuild path, or a manager that can re-read config and
    reconnect in place.

14. **T-17 · Re-open the setup portal on demand** `agent`
    From the Home Assistant page in Settings, instead of only by clearing the
    config and rebooting.

15. **T-18 · Add a "Test connection" action** `agent`
    Validate a URL and token before saving, so a typo does not take a reboot to
    discover.

16. **T-19 · Store multiple Wi-Fi profiles** `agent`
    `ConfigManager` keeps one SSID and password in NVS. This needs a list, a
    selected index, and a migration for existing single-profile devices.

17. **T-20 · Try the next Wi-Fi profile on failure** `agent`
    With a bounded number of attempts before falling back to the scan list.
    Needs T-19.

18. **T-21 · Pick the strongest known Wi-Fi at boot** `agent`
    Instead of the last one used. Needs T-19.

19. **T-22 · Edit Wi-Fi credentials on the device** `agent`
    Rather than only forgetting and re-entering them.

20. **T-23 · Store multiple Home Assistant server profiles** `agent`
    Home and remote URLs, with a way to pick between them. A remote `https`
    profile will not have voice barge-in; see T-36.

21. **T-25 · Extract detail rendering into reusable widgets** `agent`
    Domain-specific detail rendering as widgets, starting with the media player.

22. **T-26 · Add a Home widget registry** `agent`
    So widgets can be enabled and ordered. Needs T-25.

23. **T-27 · Give Home widgets the battery and connection state** `agent`
    `HomeWidget` exposes only `draw` and `handleInput` today.

24. **T-28 · Give Home a visual treatment** `tim` `agent`
    A small bitmap or an appropriate Home Assistant mark. Tim picks the look.

25. **T-29 · Add a layout test or rendering fixture** `agent`
    For overflow and screen bounds in widgets and views. Would also cover T-11.

26. **T-30 · Album-art thumbnails** `agent` `device`
    Optional and authenticated, through a bounded JPEG cache. Mind the RAM
    ceiling in AGENTS.md.

27. **T-31 · Measure power draw** `tim` `device`
    None of the power work has been checked with a meter, only reasoned from
    datasheets and driver source. Record actual draw at NORMAL, DIM,
    DISPLAY_OFF, SOFT_SLEEP, light sleep and deep sleep; without numbers there
    is no way to know which changes mattered. Needs a USB power meter or an
    inline shunt.

28. **T-32 · Measure the reconnect after deep sleep** `device`
    Deep sleep loses the Home Assistant connection and re-fetches all entity
    state on wake. If that is slow, GO-only wake may be worse overall than
    staying in light sleep.

29. **T-33 · Check what `WIFI_PS_MAX_MODEM` costs** `device`
    Whether it delays state pushes enough to notice when the screen comes back
    on, and whether the WebSocket survives long idle periods under it.

30. **T-34 · Put the IMU in low-power mode between polls** `agent` `device`
    It runs continuously, even with the screen off.

31. **T-35 · Put the TCA8418 in low-power mode while asleep** `agent` `device`
    If the keyboard controller supports it.

32. **T-36 · Barge-in over `wss`** `agent` `device`
    Over `wss` a spoken reply still tears the socket down, so holding GO
    silences the reply but the new request fails until the connection is back.
    Needs a pending-record state in `ChatView` (arm on GO, start the mic once
    re-authenticated) or a smaller TLS footprint. Low priority while plain `ws`
    is the recommended setup.

33. **T-37 · Timed wake, if it is ever wanted** `tim`
    Refresh state periodically without user input (`M5.Power.timerSleep`).
    Decide whether it is wanted before building it.

34. **T-38 · Skip the USB wait on deep-sleep wakes that pass through M5Launcher** `agent` `device`
    `setup()` skips its 1.5 s CDC re-enumeration wait when
    `esp_sleep_get_wakeup_cause()` says the chip woke from sleep, but on the
    shared device every wake runs M5Launcher first, which relaunches ADVhome
    with a software reset, so the cause reads as a cold boot and the wait
    stays: five boots on 2026-09-12 all measured ~5.08 s to ready. Set an
    `RTC_DATA_ATTR` flag before `esp_deep_sleep_start()` (RTC memory survives
    both the sleep and the launcher's reset), skip the wait when it is set,
    and clear it. Check the flag is cleared on the paths that do not go
    through deep sleep.

## Known limitations

Not tasks. Things the project has decided it cannot close from here, recorded so
they stop being rediscovered.

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
