# macOS Build and Release
Scripts and workflow for building and publishing macOS binaries from Qt projects.

## Files
- `build.sh` — local `.app`/`.dmg`/`.pkg` build with Developer ID or Mac App Store signing.
- `macos-release.yml` — GitHub Actions workflow configurable for direct (Developer ID + notarization) or Mac App Store distribution.

## Local Build (`build.sh`)

### Quick start
- `./scripts/macos/build.sh` — Debug `.app`, unsigned.
- `./scripts/macos/build.sh --release --sign --dmg --notarize` — Release `.app` + Developer ID-signed, notarized `.dmg`.
- `./scripts/macos/build.sh --release --sign-mas --pkg` — Release `.app` + Mac App Store-signed `.pkg`.

### Options
- `--clean` — remove the macOS build directory before configuring.
- `--debug` / `--release` — build type. Defaults to Debug.
- `--unsigned` / `--sign` / `--sign-mas` — signing mode. Defaults to unsigned.
- `--direct` / `--mas` — distribution target. Defaults to `direct`. Implied by the signing mode flag.
- `--dmg` — produce a `.dmg` (direct distribution only).
- `--pkg` — produce a `.pkg` via `productbuild`. Required for Mac App Store.
- `--notarize` — submit the artifact to Apple notarization (direct only; requires `--sign`).
- `--open` — open the built `.app` after the build succeeds.

### Optional local configuration
- `.env.macos.local` in the project root is sourced automatically when present.
- Override the path with `ENV_FILE=<path>`.

### Environment variables (auto-discovered when possible)
- `QT_CMAKE`, `QT_MACOS`, `QT_ROOT`, `QT_VERSION`
- `MACOS_BUILD_DIR` (default: `build/macos-local`)
- `MACOS_BUNDLE_IDENTIFIER`, `APP_BUILD_NUMBER`, `APP_VERSION_STRING`
- `MACOS_SCHEME` — app target name used to locate the `.app` bundle.
- `CMAKE_GENERATOR` (default: Ninja if available, otherwise Unix Makefiles)

### Developer ID signing (`--sign`)
- `APPLE_TEAM_ID` (required)
- `MACOS_SIGN_IDENTITY` (default: `Developer ID Application`)
- `MACOS_INSTALLER_SIGN_IDENTITY` (default: `Developer ID Installer`; used when `--pkg` is combined with `--sign`)
- `MACOS_ENTITLEMENTS` (optional path to entitlements plist)

### Mac App Store signing (`--sign-mas`)
- `APPLE_TEAM_ID` (required)
- `MAS_SIGN_IDENTITY` (default: `Apple Distribution`)
- `MAS_INSTALLER_SIGN_IDENTITY` (default: `3rd Party Mac Developer Installer`)
- `MAS_PROVISIONING_PROFILE` (required; `.provisionprofile` path, embedded into `Contents/embedded.provisionprofile`)
- `MAS_ENTITLEMENTS` (required; sandbox entitlements plist)

### Notarization (`--notarize`)
- `NOTARY_KEYCHAIN_PROFILE` (preferred; created with `xcrun notarytool store-credentials`)
- Or `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID` (defaults to `APPLE_TEAM_ID`), and `NOTARY_PASSWORD` (app-specific password)
- The script runs `notarytool submit --wait` and `stapler staple` against the produced `.dmg`, `.pkg`, or `.app`.

## Release Workflow (`macos-release.yml`)

### Trigger
- `push` events for tags matching `v[0-9]*`.
- `workflow_dispatch` with `distribution_type` input (`direct` or `appstore`).
- Tag-triggered runs require the tagged commit to be an ancestor of the repository default branch.
- Tag-triggered runs use `vars.MACOS_DEFAULT_DISTRIBUTION` (falls back to `direct`). To release both targets per tag, configure two workflows or run `workflow_dispatch` explicitly.

### Distribution modes
- `direct` — archives with Developer ID signing, exports a `.dmg` (or wraps the exported `.app` into one), and notarizes/staples the artifact.
- `appstore` — archives with Apple Distribution signing, exports a Mac App Store `.pkg`, and uploads to App Store Connect via `fastlane pilot upload --app_platform osx --skip_submission true`.

### Required repository variables
- `ARTIFACT_PREFIX` — artifact name prefix.
- `MACOS_SCHEME` — Xcode scheme used by `xcodebuild archive`.
- `MACOS_ARCHIVE_BASENAME` — base name for archive and export paths.
- `MACOS_BUNDLE_IDENTIFIER` — reverse-DNS bundle identifier.
- `APPLE_TEAM_ID` — 10-character Apple Developer Team ID.
- `APPSTORE_ISSUER_ID` — App Store Connect API issuer ID (UUID).
- `APPSTORE_API_KEY_ID` — App Store Connect API key identifier.
- `MACOS_DEFAULT_DISTRIBUTION` (optional) — `direct` or `appstore`; default for tag-triggered runs.

### Required repository secrets
- `APPSTORE_API_PRIVATE_KEY` — full `.p8` private key content.
- `MATCH_GIT_URL` — Git URL of the Match certificates/profiles repository.
- `MATCH_PASSWORD` — Fastlane Match encryption passphrase.
- `MATCH_GIT_BASIC_AUTHORIZATION` — optional base64 `username:token` for Match HTTP basic auth.

### Signing and certificate management
- Match is invoked with `fastlane match developer_id` (direct) or `fastlane match macappstore` (appstore) with `--platform macos` and `--additional_cert_types mac_installer_distribution` in read-only mode.
- Required profiles in the Match repository:
  - Direct: `DeveloperID <bundle_identifier>`
  - Mac App Store: `MacAppStore <bundle_identifier>`
- A companion write-mode maintenance workflow is required to initialize and rotate certificates/profiles. Model it on the iOS `ios-signing-maintenance.yml` pattern.

### Build configuration
`macos-release.yml` sets these CMake cache variables:
- `APP_VERSION_STRING` — tag value with leading `v` removed (tag runs) or `YYYY.MM.0` (manual runs).
- `APP_BUILD_NUMBER` — `YYYYMM000 + MINOR` (tag runs) or `YYYYMM000` (manual runs).
- `MACOSX_BUNDLE_GUI_IDENTIFIER` — from `MACOS_BUNDLE_IDENTIFIER`.
- `CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM` — from `APPLE_TEAM_ID`.

The target project's `CMakeLists.txt` is expected to:
- Expose `APP_VERSION_STRING` and `APP_BUILD_NUMBER` cache variables.
- Set `MACOSX_BUNDLE_GUI_IDENTIFIER`, `MACOSX_BUNDLE_SHORT_VERSION_STRING`, `MACOSX_BUNDLE_BUNDLE_VERSION` from the cache.
- Support `-GXcode` generation for `xcodebuild archive`.
- For Mac App Store: include a sandbox entitlements file and configure the app target to consume it.

### Pipeline behavior
1. Validate distribution type, release trigger, and release branch ancestry.
2. Install Xcode, Ruby/Fastlane, and the Qt macOS desktop kit.
3. Validate required configuration values and formats.
4. Generate the App Store Connect API key JSON.
5. Create/unlock a CI keychain.
6. Determine Match signing type, identities, and export method based on `DISTRIBUTION_TYPE`.
7. Sync Match signing assets in read-only mode.
8. Configure CI keychain partition access for `codesign`.
9. Configure CMake via `qt-cmake` with the `-GXcode` generator.
10. Archive and export via `xcodebuild` using manual signing. Wrap the exported `.app` into a Developer ID-signed `.dmg` when direct distribution is selected.
11. For direct distribution: notarize and staple with `xcrun notarytool`.
12. Upload the artifact to GitHub Actions.
13. For Mac App Store: upload the `.pkg` to App Store Connect via `fastlane pilot upload --app_platform osx --skip_submission true`.

### Pinned CI baseline
- Runner: `macos-15` (Apple Silicon)
- Qt: `6.11.0`
- Xcode: `26.3`
- `jurplel/install-qt-action@v4` pinned to `aqtinstall` commit `8c3695d4a4e1ceabf6a74dc6c79681656dc6b74b`

## Versioning
- Production tag format: `vYYYY.MM.MINOR`.
- `CFBundleShortVersionString` is the tag with leading `v` removed.
- `CFBundleVersion` is `YYYYMM000 + MINOR`; must always increase for App Store Connect uploads.
