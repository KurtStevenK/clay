# Security

Clay is a public repository. Tokens and passwords live only in GitHub Actions secrets and in gitignored local files.

## Never commit

- **`.env`** and any **`.env.*`** file except **[`.env.example`](.env.example)** (local secrets, including `VIRUSTOTAL_API_KEY`)
- **Credential files** matched by [`.gitignore`](.gitignore) (`*.pem`, `credentials.json`, and similar)

[`.env.example`](.env.example) lists the local variables without values.

## GitHub Actions secrets

| Secret | Used for |
|---|---|
| `VIRUSTOTAL_API_KEY` | Scanning release files on VirusTotal |

Set it with `gh secret set VIRUSTOTAL_API_KEY --repo KurtStevenK/clay` (same key as FeatherShot).

## VirusTotal

After every tagged release published by a workflow named **Build & Release**, the [VirusTotal workflow](.github/workflows/virustotal.yml) downloads the release's installers (Windows Inno `.exe`, and later `.dmg`, `.AppImage`, `.deb`, `.msi`, `.msix`, `.msixbundle`), uploads them to [VirusTotal](https://www.virustotal.com), and appends a table to the GitHub release notes: detection counts per file and a link to each file's full report (`virustotal.com/gui/file/<sha256>`).

The free API allows 4 requests a minute and queues new files, so the workflow runs after Build & Release has published everything and waits up to 45 minutes (`VIRUSTOTAL_WAIT_MINUTES`) for the analyses. A file still queued then is listed as pending, and its report link shows the result once VirusTotal finishes. Detections are reported but do not fail the workflow unless `VIRUSTOTAL_FAIL_ON_DETECTION=1`.

Until Build & Release exists, run the workflow from the Actions tab with a release tag. To scan local builds, set `VIRUSTOTAL_API_KEY` in `.env.local`, then:

```bash
npm run scan:virustotal
# CI: downloads into release-files/. Locally, if that folder is missing, the script
# scans _install/bin/clay-<version>.exe and clay-*-setup.exe in the repo root.
# or: node scripts/virustotal-scan.mjs clay-3.36.0-setup.exe
```

Heuristic flags on large native GTK bundles are common. Read the full report, not a single engine.

### Known detections

None recorded yet. After the first scanned `clay-*-setup.exe` release, list engine names and whether they were false positives here.

## If a secret was committed

1. **Rotate or revoke** the credential right away (VirusTotal API key).
2. Remove the file from history: [GitHub — removing sensitive data](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository).
