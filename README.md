# VEmu Core Installer

Manual-install workaround for the VEmu PS5 core installer.

Thunderstore rejected binary-containing uploads as `Invalid submission`, so the public Thunderstore `VEmu_Addon` stays config-only. This repository hosts the installer separately for manual installation.

## Why this is on GitHub

The old config-only `VEmu_Addon` uploads worked on Thunderstore. When the PS5 core installer DLL was added, Thunderstore rejected those versions with `Invalid submission`.

`Invalid submission` means Thunderstore wanted a manual review. The message did not say exactly which file or rule caused it. Rather than keep guessing and keep uploading rejected versions, we split it:

- Thunderstore gets a clean config-only addon with no DLLs or cores.
- GitHub hosts the installer as a manual download.
- People who want the PS5 core installer can choose to install it themselves.

This is a workaround to keep both parts available. It does not mean the installer is a virus or that the core is illegal. The installer is small and narrow: it verifies a SHA256 hash, copies the core only if it is missing, never overwrites, and writes a log. No ROMs, PKGs, BIOS files, firmware, or runtime downloads are included.

If Thunderstore moderators later approve a binary version, this GitHub installer can stay as a fallback.

## What it does

On VotV launch, the installer:

1. Finds your VEmu `user_data_dir` from an installed VEmu `settings.ini`.
2. Falls back to `%LOCALAPPDATA%\VEmu`.
3. Verifies the bundled core SHA256.
4. Copies `kytyps5_libretro.dll` into `<VEmu user data>\cores\` only if it is missing.
5. Writes `VEmu-CoreInstaller.log` next to `VotV-Win64-Shipping.exe`.

It never overwrites an existing core.

## Requirements

- Voices of the Void with UE4SS/shimloader working.
- `Moddy-VEmu-1.1.0`.

## Install

1. Close VotV.
2. Download `releases/VEmu-CoreInstaller-1.0.0.zip` from this repository's Releases page.
3. Extract it so this folder exists:

   `<profile>\shimloader\mod\Theunknown404-VEmuCoreInstaller\`

   On this machine that profile path is normally:

   `C:\Users\tommy\AppData\Roaming\Thunderstore Mod Manager\DataFolder\VotV\profiles\Default\shimloader\mod\`

4. Make sure these files exist after extraction:

   - `Theunknown404-VEmuCoreInstaller\enabled.txt`
   - `Theunknown404-VEmuCoreInstaller\dlls\main.dll`
   - `Theunknown404-VEmuCoreInstaller\core\kytyps5_libretro.dll`

5. Start VotV.
6. Check `VEmu-CoreInstaller.log` next to `VotV-Win64-Shipping.exe`.

## Verify a hash

In PowerShell:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath "path\to\file"
```

Expected hashes are in `SHA256SUMS.txt`.

## Uninstall

Delete:

`<profile>\shimloader\mod\Theunknown404-VEmuCoreInstaller\`

Removing the mod does not delete the copied core. To remove the core manually, delete:

`<VEmu user data>\cores\kytyps5_libretro.dll`

## Source and license

- Installer source: `src/main.cpp`
- Installer license: `LICENSE`
- KytyPS5 license: `source/KytyPS5-LICENSE.txt`
- Source offer: `source/SOURCE_OFFER.md`
- Exact local patch: `source/MODIFICATIONS.patch`
- Build info: `source/BUILD_INFO.txt`
- Clean corresponding source archive: `releases/KytyPS5-libretro-source-clean.zip`

Do not add these DLLs back into a Thunderstore package unless a moderator approves it.
