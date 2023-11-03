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
<h2>{t}Set up authentication app (TOTP){/t}</h2>

<div class="section row">
    <div class="col m12 l4">
        <div class="section row">
            <p>{t escape=no 1=$tokenDescription}Please set up your authentication app <b>%1</b> as follows:{/t}</p>
            <ol>
                <li>{t}Open your authentication app.{/t}</li>
                <li>{t}Find an option to add a new account (this is often a button with a plus or QR code symbol).{/t}</li>
                <li>{t}Scan the shown QR code.{/t}</li>
            </ol>
        </div>
        <div class="section row">
            <div class="col s12 m8 l4 xl3" id="qrcode">
                <img style="display: block; object-fit: contain; width: 100%; image-rendering: pixelated;" src="{$qrImage}" alt="{$qrImageValue}" />
            </div>
        </div>
        <div class="section">
            <p>{t}Please ensure nobody has seen the QR code. If unsure, generate a new code.{/t}</p>
            <button class="btn primary"
                name="mfa_generate_totp_secret"
                type="submit">{t}Generate new QR code{/t}
            </button>
            {* Make sure to provide needed variables again. *}
            <input type="hidden" id="tokenDescription" name="tokenDescription" value="{$tokenDescription}">
            <input type="hidden" id="tokenSerial" name="tokenSerial" value="{$tokenSerial}">
        </div>
    </div>
    <div class="col m12 l8" style="background-color: #dfdfdf; text-align:center;">
        <video class="responsive-video" autoplay loop muted>
            <source src="plugins/privacyidea/videos/totp.mp4" type="video/mp4">
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
    </div>
</div>

<div class="section">
{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
    <input type="hidden" id="add_token" name="add_token" value="yes">
    <input type="hidden" id="token_type" name="token_type" value="totp">
    <input type="hidden" id="current_phase" name="current_phase" value="1">

{* Remove 'add_token' from POST, so that mfaAccount doesn't think we are in a token setup anymore.
 * Which means we return to the mfa intro page. *}
    <button class="btn"
        onclick="document.getElementById('add_token').remove();"
        name="setup_cancel"
        type="submit">{t}Cancel{/t}
    </button>
    <button class="btn primary"
        name="setup_continue"
        type="submit">{t}Continue{/t}
    </button>
</div>
