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
        <li>{t}Install and open a suitable TOTP authentication app{/t}.</li>
        <li>{t}Assign meaningful description for the smartphone and app.{/t}</li>
        <li>{t}Scan a displayed QR code with the app.{/t}</li>
        <li>{t}Verify the setup with a generated code.{/t}</li>
    </ol>
    <div class="row">
        <div class="input-field col s12">
            <label for="tokenDescription">{t}Description{/t}</label>
            <input type="text" id="tokenDescription" name="tokenDescription" size="60" maxlength="60">
            <span class="helper-text">{t}Please enter a meaningful description allowing you to recognize the used device and app. This description may be edited later.{/t}</span>
        </div>
    </div>
</div>

<div class="section">
    <input type="hidden" id="current_phase" name="current_phase" value="0">

    <button class="btn" formnovalidate type="submit">{t}Cancel{/t}</button>
    <button class="btn primary" name="add_token" value="totp" type="submit">{t}Continue{/t}</button>
</div>
