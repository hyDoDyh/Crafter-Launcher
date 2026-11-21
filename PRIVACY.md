# Privacy Policy — Crafter Launcher

Last updated: November 21, 2026

Crafter Launcher ("the launcher") is an open-source launcher for Minecraft: Java Edition.
This policy explains what information the launcher processes when you sign in with a
Microsoft account, how that information is stored, and what is never collected.

## 1. Information the launcher processes

When you choose **Microsoft sign-in**, the launcher uses Microsoft's OAuth 2.0 device code
flow. During this process the launcher receives and handles:

- Microsoft account access and refresh tokens;
- Xbox Live and XSTS tokens derived from the Microsoft token;
- a Minecraft access token issued by `api.minecraftservices.com`;
- your Minecraft profile information: in-game name and UUID.

This information is required to authenticate you with Minecraft and to start the game.
The launcher does not request your Microsoft account password, and the password is entered
only on Microsoft's own login page.

## 2. How information is stored

- The Microsoft **refresh token** and the **last signed-in account name** are stored locally
  in the launcher's settings file (`QSettings` INI file, keys `Auth/Microsoft/RefreshToken`
  and `Auth/Microsoft/LastName`), so that you do not have to sign in again every time.
- Access tokens, Xbox Live/XSTS tokens and the Minecraft access token are kept in memory
  only and are discarded when the launcher exits or you switch accounts.
- The launcher does not upload, sync or back up any of these tokens to the developer or to
  any third party. Tokens are sent only to Microsoft, Xbox Live and Minecraft services as
  part of the sign-in process.

## 3. No collection, tracking or sharing

The launcher does **not** collect telemetry, analytics, crash reports, advertising
identifiers or usage statistics. It does not sell, rent or share personal information.
Apart from the authentication requests described above, the launcher only contacts
Minecraft/Mojang servers and the download sources you configure in order to download game
files, libraries and assets.

## 4. Third-party services

Sign-in relies on services operated by Microsoft (Microsoft identity platform, Xbox Live)
and Mojang/Microsoft (Minecraft services). Your use of those services is governed by their
own privacy statements:

- Microsoft Privacy Statement: https://privacy.microsoft.com/privacystatement
- Xbox Live / Microsoft account privacy controls: https://account.microsoft.com/privacy

## 5. Your choices and control

- **Sign out / forget account:** removes the stored refresh token and account name from the
  local settings file.
- **Delete the settings file:** removes all locally stored account information.
- **Revoke access:** you can revoke the launcher's access to your Microsoft account at
  https://account.live.com/consent/Manage at any time.
- Game files, logs and configurations created by the launcher remain on your computer and
  can be deleted manually.

## 6. Children

The launcher is not directed to children under 13 (or the minimum age required in your
country). Sign-in requires a Microsoft account, which is subject to Microsoft's own age
requirements.

## 7. Security

Account tokens are stored locally on your device. You are responsible for protecting your
computer account and the launcher's settings file. If you believe your tokens have been
compromised, sign out in the launcher and revoke access at
https://account.live.com/consent/Manage.

## 8. Changes to this policy

This policy may be updated as the launcher evolves. The updated version will be published
in this repository with a new "Last updated" date.

## 9. Contact

For privacy questions or requests, open an issue at
https://github.com/hyDoDyh/Crafter-Launcher/issues or contact
[hyDoDyh@hotmail.com](mailto:hyDoDyh@hotmail.com).
