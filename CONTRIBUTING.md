# Contributing to CardMesh

Thank you for your interest in contributing to CardMesh! We welcome contributions from the community and want to make the process clear and straightforward.

## Ways to Contribute

- Report bugs or request features using [GitHub Issues](../../issues).
- Improve documentation (README, code comments, examples).
- Submit Pull Requests (PRs) with bug fixes, improvements, or new features.

## Code Style

- Target **C++17**, matching the existing codebase.
- Keep networking (`HttpClient`), the MeshMonitor API layer (`MeshMonitorClient`), storage (`Settings`, `Database`), and UI cleanly separated — see the "Application Layers" section of the README.
- Do not parse raw MeshMonitor JSON inside UI screens; parse it in `src/models/` and consume typed models instead.
- Never fabricate a value MeshMonitor didn't provide (e.g. missing RSSI/SNR/battery) — leave it unset (`std::optional`) rather than defaulting to `0`.
- Keep things simple (KISS principle); add comments only where the *why* isn't obvious from the code.

## Pull Request Guidelines

1. **Fork** the repository.
2. Create a new branch for your feature or bugfix:  
   ```bash
   git checkout -b feature/my-new-feature
   ```
3. Commit changes with clear, descriptive messages.
4. Build and run the test suite locally before opening a PR:
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ctest --test-dir build --output-on-failure
   ```
5. Open a Pull Request and describe your changes.

## Tests

- Please test your changes locally before submitting a PR.
- Add or update unit tests under `tests/` for new API parsing, models, or storage logic.

## Security-Sensitive Changes

- Never commit API tokens, credentials, or real MeshMonitor server details.
- Changes touching `Settings`, `HttpClient`, or credential handling should follow [SECURITY.md](SECURITY.md) and may receive additional review.

## Community

We value respectful collaboration. By contributing, you agree to follow our [Code of Conduct](CODE_OF_CONDUCT.md).
