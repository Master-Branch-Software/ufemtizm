# Releasing Ufemtizm

This document covers the full release process: one-time infrastructure setup, per-release steps, and how each platform's update delivery works.

---

## Table of Contents

1. [S3 bucket setup (one-time)](#1-s3-bucket-setup-one-time)
2. [macOS signing setup (one-time)](#2-macos-signing-setup-one-time)
3. [GitHub secrets and variables reference](#3-github-secrets-and-variables-reference)
4. [Cutting a release](#4-cutting-a-release)
5. [Platform update delivery explained](#5-platform-update-delivery-explained)
6. [Local build scripts](#6-local-build-scripts)

---

## 1. S3 bucket setup (one-time)

Create one S3 bucket (e.g. `ufemtizm-releases`) in your preferred region. The workflows write into this key layout:

```
s3://ufemtizm-releases/
  mac/
    releases/
      Ufemtizm-2025.2.2.dmg  ← notarized DMG (direct distribution)
  win/
    releases/
      Ufemtizm-2025.2.2.exe  ← Qt IFW installer
    qt-ifw-repo/
      Updates.xml             ← Qt IFW update repository (synced by CI)
      ufemtizm/
        ...
```

### Bucket policy

Make `mac/` and `win/` publicly readable. Keep IAM credentials write-only, never embedded in client code.

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "PublicRead",
      "Effect": "Allow",
      "Principal": "*",
      "Action": "s3:GetObject",
      "Resource": "arn:aws:s3:::ufemtizm-releases/*"
    }
  ]
}
```

### IAM user for CI

Create an IAM user with this policy scoped to the bucket:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:PutObject",
        "s3:GetObject",
        "s3:DeleteObject",
        "s3:ListBucket"
      ],
      "Resource": [
        "arn:aws:s3:::ufemtizm-releases",
        "arn:aws:s3:::ufemtizm-releases/*"
      ]
    }
  ]
}
```

Generate an access key for this user and save the ID and secret — you'll add them as GitHub secrets next.

---

## 2. macOS signing setup (one-time)

The macOS workflow uses **Fastlane Match** to sync certificates. You need:

- An Apple Developer account with a team ID
- An App Store Connect API key (for Match and notarization)
- A Match Git repository (private, stores encrypted certs)

### App Store Connect API key

1. Go to [App Store Connect → Users and Access → Keys](https://appstoreconnect.apple.com/access/api)
2. Create a key with **Developer** role
3. Download the `.p8` file — it can only be downloaded once
4. Note the **Key ID** (10-char alphanumeric) and **Issuer ID** (UUID)

### Match repository

```bash
# One-time: initialise Match with your chosen Git repo
fastlane match init
```

Run `fastlane match developer_id` (for direct distribution) and/or `fastlane match macappstore` (for App Store) to generate and store certificates.

---

## 3. GitHub secrets and variables reference

Go to **Settings → Secrets and variables → Actions** in your repository.

### Variables (not secret, visible in logs)

| Variable | Example value | Required for |
|---|---|---|
| `ARTIFACT_PREFIX` | `ufemtizm` | All workflows (artifact naming) |
| `S3_BUCKET` | `ufemtizm-releases` | S3 upload (leave unset to skip) |
| `AWS_REGION` | `us-east-1` | S3 upload |
| `MACOS_BUNDLE_IDENTIFIER` | `com.ows.ufemtizm` | macOS workflow |
| `MACOS_SCHEME` | `Ufemtizm` | macOS workflow |
| `MACOS_ARCHIVE_BASENAME` | `Ufemtizm` | macOS workflow |
| `MACOS_DEFAULT_DISTRIBUTION` | `direct` | macOS workflow default |
| `APPLE_TEAM_ID` | `ABCDE12345` | macOS signing |
| `APPSTORE_ISSUER_ID` | `xxxxxxxx-xxxx-...` | macOS App Store Connect |
| `APPSTORE_API_KEY_ID` | `ABCDE12345` | macOS App Store Connect |

### Secrets (hidden in logs)

| Secret | Description | Required for |
|---|---|---|
| `AWS_ACCESS_KEY_ID` | IAM key for S3 writes | S3 upload |
| `AWS_SECRET_ACCESS_KEY` | IAM secret for S3 writes | S3 upload |
| `APPSTORE_API_PRIVATE_KEY` | Contents of the `.p8` file from App Store Connect | macOS signing + notarization |
| `MATCH_GIT_URL` | HTTPS or SSH URL of your Match certificates repo | macOS signing |
| `MATCH_PASSWORD` | Encryption password for the Match repo | macOS signing |
| `MATCH_GIT_BASIC_AUTHORIZATION` | Base64 `user:token` for Match repo auth (optional) | macOS signing (private repo) |

> **S3 upload is opt-in.** Leave `S3_BUCKET` unset and the upload steps are skipped silently. Everything else (build, sign, notarize, GitHub artifact) still runs.

---

## 4. Cutting a release

### Version format

Tags follow `vYYYY.MM.MINOR`, e.g. `v2025.04.0`, `v2025.04.1`.

Update `CMakeLists.txt` before tagging:

```cmake
project(
  Ufemtizm
  VERSION 2025.4.0   # ← bump this
  ...
)
```

Commit, then tag:

```bash
git add CMakeLists.txt
git commit -m "Release 2025.4.0"
git tag v2025.4.0
git push origin development v2025.4.0
```

Pushing the tag triggers both `macos-release.yml` and `windows-release.yml` automatically.

### What each workflow does

#### macOS (`macos-release.yml`)

1. Validates tag format and branch ancestry
2. Installs Qt and fastlane
3. Syncs signing certificates via Match
4. Configures CMake and builds with Xcode
5. Exports a notarized `.dmg` (direct) or `.pkg` (App Store)
6. **Direct only:** uploads the DMG to `s3://BUCKET/mac/releases/`
7. **App Store only:** uploads the `.pkg` to App Store Connect via fastlane

Trigger manually with a distribution override:

- **Actions → macos-release → Run workflow** → choose `direct` or `appstore`

#### Windows (`windows-release.yml`)

1. Validates tag format and branch ancestry
2. Installs Qt + Qt IFW tools
3. Configures CMake — injects the S3 repository URL into the installer
4. Builds with MSVC
5. Packages with `cpack -G IFW` → produces a `.exe` installer
6. Runs `repogen` to generate the Qt IFW online update repository from the package tree
7. Uploads the `.exe` to `s3://BUCKET/win/releases/`
8. Syncs the repository to `s3://BUCKET/win/qt-ifw-repo/`

#### Linux / Snap

Snap is not triggered by the tag workflow — it publishes via the [Snap Store build service](https://snapcraft.io/docs/build-from-github). Connect your repo in the Snap Store dashboard; it will build and release automatically on push to your default branch. No manual steps needed for updates — the snap daemon handles delivery to users.

---

## 5. Platform update delivery explained

### macOS — App Store

Updates are delivered automatically by the App Store. Users are notified in the App Store app and can update with one click. No in-app updater is needed or shown.

### Windows — Qt IFW maintenance tool

The Qt IFW installer places `maintenancetool.exe` next to the app. **Check for Updates** in the Help menu and tray launches `maintenancetool.exe --updater`, which contacts `Updates.xml` on S3, shows available updates, and runs the installer silently. The S3 repository URL is embedded in the installer at build time via `CPACK_IFW_UPDATE_REPO_URL`.

### Linux — Snap

Snap refreshes automatically in the background (default: four times per day). Users can also run `snap refresh ufemtizm` manually. No in-app updater is needed or shown.

---

## 6. Local build scripts

### macOS — `scripts/macos/build.sh`

```bash
# Debug build, unsigned
./scripts/macos/build.sh

# Release, signed, notarized DMG
APPLE_TEAM_ID=ABCDE12345 \
NOTARY_KEYCHAIN_PROFILE=notary-profile \
./scripts/macos/build.sh --release --sign --dmg --notarize

# Mac App Store .pkg
MAS_ENTITLEMENTS=./mas.entitlements.plist \
MAS_PROVISIONING_PROFILE=./mas.provisionprofile \
APPLE_TEAM_ID=ABCDE12345 \
./scripts/macos/build.sh --release --sign-mas --pkg
```

Reads `.env.macos.local` automatically if present — keep it out of version control.

### Windows — `build_and_deploy.ps1`

```powershell
# Release build with Qt IFW installer
.\build_and_deploy.ps1 -BuildType Release

# Specific Qt path
.\build_and_deploy.ps1 -QtPath "C:\Qt\6.9.0\msvc2022_64" -BuildType Release
```

### Linux / macOS (quick) — `build_and_deploy.sh`

`build_and_deploy.sh` detects the OS and runs the appropriate build path. It is not a substitute for `scripts/macos/build.sh` on macOS — it produces an unsigned, un-notarized build for quick local testing only.

| OS | What it produces |
|---|---|
| Linux | DEB package (via CMake + CPack) |
| macOS | Unsigned `.dmg` (via `macdeployqt`, no signing/notarization) |

```bash
# Linux — DEB package
./build_and_deploy.sh

# Linux — Snap package (requires snapcraft)
./build_and_deploy.sh --snap

# macOS — quick unsigned DMG (testing only, not for distribution)
./build_and_deploy.sh
```

For a signed, notarized macOS build use `scripts/macos/build.sh` instead (see above).
