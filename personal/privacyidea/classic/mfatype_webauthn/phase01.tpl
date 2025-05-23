{*
 * This code is an addon for GOsa² (https://gosa.gonicus.de)
 * https://github.com/gosa-project/gosa-plugins-privacyidea/
 * Copyright (C) 2023 Daniel Teichmann <daniel.teichmann@das-netzwerkteam.de>
 * Copyright (C) 2023 Guido Berhörster <guido+freiesoftware@berhoerster.name>
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

<!-- Load WebAuthn logic stuff -->
<script type="text/javascript" async="true" src="plugins/privacyidea/js/WebAuthn.js"></script>

<h2>{t}Confirm Security Key Setup{/t}</h2>

<table cellspacing="0" cellpadding="0" style="width: 100%">
    <tr>
        <td style="width: 33%">
            <p>{t escape=no 1=$tokenDescription|escape}Please confirm the setup of the security key <b>%1</b>.{/t}</p>
            <p>{t}Your browser or operating system will guide you through the setup of the security key.{/t}</p>

            <ol>
                <li>{t}Connect your security key to your computer, e.g. plug it into a USB port or place it near the computer's NFC reader.{/t}</li>
                <li>{t}Initiate the setup by pressing the button below.{/t}</li>
                <li>{t}Confirm the setup using a device-specific mechanism, e.g. by touching a sensor on the key.{/t}</li>
                <li>{t}The browser will proceed to the next page if the setup has been sucessful.{/t}</li>
            </ol>

            <a style="padding: 2px 6px; margin: 0 0 6px 0; background: linear-gradient(to bottom, #FFF, #BBB); border: 1px solid #BBB; border-radius: 2px; outline: none; min-width: 60px; pointer-events: none;" {* Enable this link again when JS script is finished loading. *}
                id="startWebAuthnSetupButton"
                title="{t}Start the setup for your WebAuthn device.{/t}"
                onclick='startWebAuthnSetup("{$webAuthnRegisterRequestJSON}");'>{t}Start setup{/t}
            </a>
        </td>
        <td>
        <td style="background-color: #dfdfdf; text-align:center; padding: 5px; width: 66%">
            <video autoplay loop muted>
                <source src="plugins/privacyidea/videos/fido.mp4" type="video/mp4">
            </video>
            <p xmlns:cc="http://creativecommons.org/ns#" style="text-align: right">&copy; 2023
                <span property="cc:attributionName">Thomas Häpp, Universität Bonn</span>,
                licensed under <a
                href="http://creativecommons.org/licenses/by-sa/4.0/?ref=chooser-v1"
                target="_blank" rel="license noopener noreferrer"
                style="display:inline-block;">CC BY-SA 4.0<img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;"
                src="plugins/privacyidea/images/cc.svg"><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;"
                src="plugins/privacyidea/images/by.svg"><img style="height:22px!important;margin-left:3px;vertical-align:text-bottom;"
                src="plugins/privacyidea/images/sa.svg"></a></p>
        </td>
    </tr>
</table>

<hr>
<div class="plugin-actions">
    <input type="hidden" id="current_phase" name="current_phase" value="1">
    <input type="hidden" id="add_token" name="add_token" value="webauthn">
    <input type="hidden" id="tokenSerial" name="tokenSerial" value="{$tokenSerial}">
    <input type="hidden" id="mfaWebAuthnRegisterResponse" name="mfaWebAuthnRegisterResponse" value="{* TO BE FILLED BY JAVASCRIPT *}">

    {* <button name="add_token" value="webauthn" type="submit">{t}Continue{/t}</button> *}
    <button formnovalidate name="cancel_setup" value="yes" type="submit">{t}Cancel{/t}</button>
</div>
