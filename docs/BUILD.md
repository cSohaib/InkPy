# Build and release

The device firmware is C-first on ESP-IDF, with small C++ dependencies for math.
Only ESP32-S3 / Xteink X4 Pro is supported.

## Build

Use Linux and ESP-IDF **v5.5.5**, commit
`b774170ff46c393eeb5e495ea37936038d3f4f4f`.
Install the ESP32-S3 tools using ESP-IDF's installation instructions, then:

```sh
git clone https://github.com/cSohaib/InkPy.git
cd InkPy
. /path/to/esp-idf/export.sh
bash scripts/build-browser.sh
python3 scripts/package-release.py build-browser/firmware.bin release
```

The build scripts fetch exact dependency revisions and reject tracked changes
inside their dependency checkouts. Firmware is `build-browser/firmware.bin`;
the release directory contains that application image, math SD assets, checksums,
and a source archive. Dependencies and build output are ignored by Git.

The official `espressif/idf:v5.5.5` container is also used by the release workflow.
The build script checks the IDF commit even inside that container.

## Release automation

The first release, `v0.1.0`, was published with the exact device-tested Stage43
image and locally verified assets. `.github/workflows/release.yml` provides builds
for subsequent releases: use **Actions → Release → Run workflow** with a new
`vMAJOR.MINOR.PATCH` version, for example `v0.1.1`. Existing releases are never
overwritten. The workflow's initial push trigger was used during setup.

The publish job alone has `contents: write`; build and review jobs use read access.
GitHub Actions must be enabled. A release is available only after the build and
publish jobs succeed. Review the firmware on a device before announcing a release.
The first CI build hit Git's container ownership guard before compilation; the
workflow now trusts only its own checkout path. Its next run was canceled while
queued so the first release could use the tested image. A full CI rebuild remains
to be validated before relying on automation for the next firmware version.

The source archive includes InkPy and the fetched math/Python dependency sources,
without their Git metadata. ESP-IDF is obtained at the exact revision above.
See [dependency notices](../THIRD_PARTY.md). Build scripts and full source allow
rebuilding with a modified FriBidi library; no signature requirement is added.

## GitHub settings

Visibility and security switches are repository settings, not source files.
The owner can make InkPy public under **Settings → General → Danger Zone**.
Enable private vulnerability reporting, Dependabot alerts, and GitHub's available
secret scanning/push protection under **Settings → Security**. Availability
depends on GitHub's current repository/account options. The included Dependabot
configuration checks workflow action updates monthly.
