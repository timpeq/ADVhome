# Agent Instructions

## Development environment

Use the repository's Nix flake for development tools and dependencies whenever possible.

- Enter the environment with `nix develop`.
- Use the provided PlatformIO from that shell for builds, tests, and device operations.
- Prefer project configuration in `flake.nix` and `platformio.ini` over installing global tools or manually managing dependency versions.
- Keep dependency changes declared in the repository so another developer or agent can reproduce the environment.

## Git workflow

Use Git to track new features as development progresses.

- Check `git status` before starting work and before finishing.
- Make focused commits as coherent features or fixes are completed.
- Use clear commit messages that describe the behavior added or fixed.
- Do not commit secrets, device credentials, or unrelated generated files.
- Do not rewrite or discard existing user changes unless explicitly requested.
- Include relevant documentation and tests in the same feature commit when practical.

## Validation

Before committing a change, run the narrowest relevant checks from the Nix shell. For firmware changes, at minimum run:

```sh
nix develop
pio run
```

Report checks that could not be run, along with any remaining concerns.
