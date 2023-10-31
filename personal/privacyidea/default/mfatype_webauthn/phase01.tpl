{*
 * This code is an addon for GOsa² (https://gosa.gonicus.de)
 * https://github.com/gosa-project/gosa-plugins-privacyidea/
 * Copyright (C) 2023 Daniel Teichmann <daniel.teichmann@das-netzwerkteam.de>
 * Copyright (C) 2023 Guido Berhörster <guido@berhoerster.name>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *}

<h1>{t}Confirm security key setup{/t}</h1>

<div class="section">
    <p>{t escape=no 1=$tokenDescription}Please confirm the setup of the security key <b>%1</b>.{/t}</p>
    <p>{t}Your browser or operating system will guide you through the setup of the security key.{/t}</p>

    <ol>
        <li>{t}Connect your security key to your computer, e.g. plug it into a USB port or place it near the computer&apos;s NFC reader.{/t}</li>
        <li>{t}Initiate the setup by pressing the button below.{/t}</li>
        <li>{t}Confirm the setup using a device-specific mechanism, e.g. by touching a sensor on the key.{/t}</li>
        <li>{t}The browser will proceed to the next page if the setup has been sucessful.{/t}</li>
    </ol>
    <button class="btn primary"
        name="mfa_webauthn_confirm"
        type="submit">{t}Start setup{/t}</button>
</div>

<div class="card-paneli red red-lighten4 text-red text-darken-4" id="mfaWebAuthnError"></div>

<div class="section">
{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
    <input type="hidden" id="add_token" name="add_token" value="yes">
    <input type="hidden" id="token_type" name="token_type" value="webauthn">
    <input type="hidden" id="current_phase" name="current_phase" value="1">
    <input type="hidden" id="tokenSerial" name="tokenSerial" value="{$tokenSerial}">

{* Remove 'add_token' from POST, so that mfaAccount doesn't think we are in a token setup anymore.
 * Which means we return to the mfa intro page. *}
    <button class="btn"
        onclick="document.getElementById('add_token').remove();"
        name="setup_cancel"
        type="submit">{t}Cancel{/t}
    </button>
</div>
