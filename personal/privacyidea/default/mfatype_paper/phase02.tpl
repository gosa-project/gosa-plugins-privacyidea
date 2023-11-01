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

<h1>{t}Verify TAN list{/t}</h1>

<div class="section">
    <div class="row">
        <div class="col s12">
            <p>{t escape=no 1=$tokenDescription}Please verify your TAN list <b>%1</b> by entering the verification TAN.{/t}</p>
        </div>
        <div class="input-field col s3">
            <label for="mfaVerificationTan">{t}Verification TAN{/t}</label>
            <input type="text" id="mfaVerificationTan" class="validate" name="mfaVerificationTan" size="12" maxlength="12" {literal}pattern="[0-9]{6,}"{/literal} placeholder="{t}e.g. 000000{/t}" required>
        </div>
    </div>
</div>

<div class="section">
{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
    <input type="hidden" id="add_token" name="add_token" value="yes">
    <input type="hidden" id="token_type" name="token_type" value="paper">
    <input type="hidden" id="current_phase" name="current_phase" value="2">
    <input type="hidden" id="tokenSerial" name="tokenSerial" value="{$tokenSerial}">

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
