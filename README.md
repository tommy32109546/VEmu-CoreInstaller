# VEmu Core Installer

This is the PS5 core installer for VEmu. It is hosted here instead of Thunderstore.

The Thunderstore addon is config-only because uploads with DLLs and cores kept getting rejected. Without this installer, the Thunderstore ZIP is just config snippets.

## Why this is on GitHub

Thunderstore accepted the old config-only addon. When the PS5 core installer was added, the uploads were rejected with `Invalid submission`. That message did not say exactly what was wrong.

So the addon was split:

- Thunderstore has the config-only ZIP.
- GitHub has the installer.
- You install the GitHub release by hand if you want the PS5 core part.

This installer is not doing anything sneaky. It checks a SHA256 hash, copies one core file if it is missing, and writes a log. It does not overwrite existing files. No ROMs, PKGs, BIOS files, firmware, or runtime downloads are included.

## What it does

When VotV starts, the installer:

1. Looks for your VEmu `user_data_dir` in an installed `settings.ini`.
2. Uses `%LOCALAPPDATA%\VEmu` if it cannot find that setting.
3. Checks the SHA256 of the bundled core.
4. Copies `kytyps5_libretro.dll` to `<VEmu user data>\cores\` only if it is missing.
5. Writes `VEmu-CoreInstaller.log` next to `VotV-Win64-Shipping.exe`.

If a core is already there, it leaves it alone.

## Requirements

- Voices of the Void with UE4SS/shimloader working.
- `Moddy-VEmu-1.1.0`.

## Install

1. Download `VEmu-CoreInstaller-1.0.0.zip` from the Releases page.
2. Extract it so this folder exists:

   `<profile>\shimloader\mod\Theunknown404-VEmuCoreInstaller\`

   Your profile path will be different. On this machine it is:

   `C:\Users\tommy\AppData\Roaming\Thunderstore Mod Manager\DataFolder\VotV\profiles\Default\shimloader\mod\`

3. Check that these files are there:

   - `Theunknown404-VEmuCoreInstaller\enabled.txt`
   - `Theunknown404-VEmuCoreInstaller\dlls\main.dll`
   - `Theunknown404-VEmuCoreInstaller\core\kytyps5_libretro.dll`

4. Start VotV.
5. Check `VEmu-CoreInstaller.log` next to `VotV-Win64-Shipping.exe`.

## Verify a hash

In PowerShell:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath "path\to\file"
```

Expected hashes are in `SHA256SUMS.txt`.

## Uninstall

Delete:

`<profile>\shimloader\mod\Theunknown404-VEmuCoreInstaller\`

Removing the mod does not delete the copied core. To remove the core too, delete:

`<VEmu user data>\cores\kytyps5_libretro.dll`

## Source and license

- Installer source: `src/main.cpp`
- Installer license: `LICENSE`
- KytyPS5 license: `source/KytyPS5-LICENSE.txt`
- Source offer: `source/SOURCE_OFFER.md`
- Exact local patch: `source/MODIFICATIONS.patch`
- Build info: `source/BUILD_INFO.txt`
- Clean source archive: `releases/KytyPS5-libretro-source-clean.zip`

Do not put these DLLs back into a Thunderstore package unless a moderator approves it.
