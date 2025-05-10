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

<h2>{t}Add TAN List{/t}</h2>

<p>{t}Lists with transaction authentication numbers (TAN) are printed out and stored in a secure location. A TAN is a one-time password and a TAN from the list has to be entered on each login.{/t}</p>

<p>{t}Creating a TAN list requires multiple steps:{/t}</p>
<ol>
    <li>{t}Assign a meaningful description which allows you to recognize the generated list.{/t}</li>
    <li>{t}Print the list.{/t}</li>
    <li>{t}Verify the setup by entering a TAN from the list.{/t}</li>
</ol>

<table cellspacing="0">
    <tr>
        <td>
            <label for="tokenDescription">{t}Description{/t}</label>
        </td>
        <td>
            <input type="text" id="tokenDescription" name="tokenDescription" size="60" maxlength="60" pattern="[^<>\x22\x27]+" required
                oninvalid="this.setCustomValidity('{t}Empty descriptions are allowed but discouraged. They help you to identify your tokens later on. You can proceed now.{/t}')"
                oninput="this.setCustomValidity('')">
        </td>
    </tr>
    <tr>
        <td></td>
        <td>{t}Please enter a meaningful description allowing you to recognize the generated list. This description may be edited later.{/t}</td>
    </tr>
</table>

<hr>
<div class="plugin-actions">
    <input type="hidden" id="current_phase" name="current_phase" value="0">

    <button name="add_token" value="paper" type="submit"
        onclick="setTimeout(function() {literal} { {/literal}
            document.getElementById('tokenDescription').removeAttribute('required');
            document.getElementById('tokenDescription').setCustomValidity('');
        }, 200);">
        {t}Continue{/t}
    </button>
    <button formnovalidate type="submit">{t}Cancel{/t}</button>
</div>
