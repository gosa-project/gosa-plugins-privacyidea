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

<h2>{t}Add a Recovery Key{/t}</h2>

<div class="section">
    <p>{t}A recovery key can be used exactly once for logging in.{/t}</p>
    <p>{t}Recovery keys are created through the following procedure:{/t}</p>
    <ol>
        <li>{t}Assign a meaningful description for the recovery key.{/t}</li>
        <li>{t}The key will be shown immediately.{/t}</li>
    </ol>
    <div class="row">
        <div class="input-field col s12">
            <label for="tokenDescription">{t}Description{/t}</label>
            <input type="text" id="tokenDescription" name="tokenDescription" size="60" maxlength="60" pattern="[^<>\x22\x27]+" required
                oninvalid="this.setCustomValidity('{t}Empty descriptions are allowed but discouraged. They help you to identify your tokens later on. You can proceed now.{/t}')"
                oninput="this.setCustomValidity('')">
            <span class="helper-text">
                {t}Please enter a meaningful description allowing you to recognize the recovery key. This description may be edited later.{/t}
            </span>
        </div>
    </div>
</div>

<div class="section" style="display: flex;">
    <input type="hidden" id="current_phase" name="current_phase" value="0">

    <button class="btn primary" style="margin-left: 10px;" name="add_token" value="registration" type="submit"
        onclick="setTimeout(function() {literal} { {/literal}
            document.getElementById('tokenDescription').removeAttribute('required');
            document.getElementById('tokenDescription').setCustomValidity('');
        }, 200);">
        {t}Continue{/t}
    </button>
    <button class="btn" style="order: -1;" formnovalidate type="submit">{t}Cancel{/t}</button>
</div>
