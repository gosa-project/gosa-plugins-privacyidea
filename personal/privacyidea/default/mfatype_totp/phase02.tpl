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

<h2>{t}Verify authentication app (TOTP){/t}</h2>

<div class="section">
    <div class="row">
        <div class="col s12">
            <p>{t escape=no 1=$tokenDescription}Please verify your authentication app <b>%1</b> by entering a generated code.{/t}</p>
        </div>
        <div class="input-field col s3">
            <label for="mfaTOTPCode">{t}Authentication code{/t}</label>
            <input type="text" id="mfaTOTPCode" class="validate" name="mfaTOTPCode" size="12" maxlength="12" {literal}pattern="[0-9]{6,}"{/literal} placeholder="{t}e.g. 000000{/t}" required>
        </div>
    </div>
</div>

<div class="section">
    <input type="hidden" id="current_phase" name="current_phase" value="2">
    <input type="hidden" id="tokenSerial" name="tokenSerial" value="{$tokenSerial}">

    <button class="btn" formnovalidate type="submit">{t}Cancel{/t}</button>
    <button class="btn primary" name="add_token" value="totp" type="submit">{t}Continue{/t}</button>
</div>
