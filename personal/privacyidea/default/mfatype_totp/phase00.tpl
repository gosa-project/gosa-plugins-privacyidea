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

<h2>{t}Add authentication app (TOTP){/t}</h2>

<div class="section">
    <p>{t}An authentication app on your smartphone generates time-based one-time passwords (TOTP) which must be entered on each login.{/t}</p>
    <p>{t}Setting up the authentication app requires multiple steps:{/t}</p>
    <ol>
        <li>{t}Install and open a suitable TOTP authentication app.{/t}</li>
        <li>{t}Assign meaningful description for the smartphone and app.{/t}</li>
        <li>{t}Scan a displayed QR code with the app.{/t}</li>
        <li>{t}Verify the setup with a generated code.{/t}</li>
    </ol>
    <div class="row">
        <div class="input-field col s12">
            <label for="tokenDescription">{t}Description{/t}</label>
            <input type="text" id="tokenDescription" name="tokenDescription" size="60" maxlength="120">
            <span class="helper-text">{t}Please enter a meaningful description allowing you to recognize the used device and app. This description may be edited later.{/t}</span>
        </div>
    </div>
</div>

<div class="section">
{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
    <input type="hidden" id="add_token" name="add_token" value="yes">
    <input type="hidden" id="token_type" name="token_type" value="totp">
    <input type="hidden" id="current_phase" name="current_phase" value="0">

{* Remove 'add_token' from POST, so that mfaAccount doesn't think we are in a token setup anymore.
 * Which means we return to the mfa intro page. *}
    <button class="btn" formnovalidate
        onclick="document.getElementById('add_token').remove();"
        name="setup_cancel"
        type="submit">{t}Cancel{/t}
    </button>
    <button class="btn primary"
        name="setup_continue"
        type="submit">{t}Continue{/t}
    </button>
</div>
