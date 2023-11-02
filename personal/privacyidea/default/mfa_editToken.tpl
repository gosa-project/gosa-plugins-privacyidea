{*
 * This code is an addon for GOsa² (https://gosa.gonicus.de)
 * https://github.com/gosa-project/gosa-plugins-privacyidea/
 * Copyright (C) 2023 Daniel Teichmann <daniel.teichmann@das-netzwerkteam.de>
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
 
<h2>{t}Details of multifactor method{/t}</h2>

<div class="row content">
    <div class="col-sm-12 col-xl-6">

        <div class="row">
            <label class="col-sm-3 col-form-label">{t}Type{/t}</label>
            <div class="col-sm-9 input-field"><div style="padding: 0.5rem 0 0 0.5rem">
                <span class="material-icons">{$mfa_{$token.tokentype}_icon}</span>
                {$mfa_{$token.tokentype}_title}
            </div></div>
        </div>
    </div>
</div>

<hr>

<div class="row content">
    <div class="col s12 m12 xl6">

        <div class="row">
            <label class="col s3 m3">{t}Serial{/t}</label>
            <div class="col s9 m9 input-field"><input class="form-control" type="text" value="{$token.serial}" disabled></div>
        </div>
        
        <div class="row">
            <label class="col s3 m3">{t}Description{/t}</label>
            <div class="col s9 m9 input-field">
            <input
                class="form-control"
                type="text"
                value="{$token.description}"
                name="tokenDescription"
                {if !$editEnable}disabled{/if}>
            </div>
        </div>

        <div class="row">
            <label class="col s3 m3">{t}Last use{/t}</label>
            <div class="col s9 m9 input-field"><input class="form-control" type="text" value="{$token.info.last_auth}" disabled></div>
        </div>

        <div class="row">
            <label class="col s3 m3">{t}Status{/t}</label>
            <div class="col s9 m9 input-field"><input class="form-control" type="text" value="{$token.active}" disabled></div>
        </div>        

        <div class="row">
            <label class="col s3 m3">{t}Index der letzten genutzten TAN{/t}</label>
            <div class="col s9 m9 input-field"><input class="form-control" type="text" value="4" disabled></div>
        </div>

        <div class="row">
            <label class="col s3 m3">{t}Anzahl Anmeldungen{/t}</label>
            <div class="col s9 m9 input-field"><input class="form-control" type="text" value="2" disabled></div>
        </div>

        <div class="row">
            <label class="col s3 m3">{t}Erfolgreiche Anmeldungen{/t}</label>
            <div class="col s9 m9 input-field"><input class="form-control" type="text" value="2" disabled></div>
        </div>

        <div class="row">
            <label class="col s3 m3">{t}Fehlerzähler (aktuell / maximal){/t}</label>
            <div class="col s9 m9 input-field"><input class="form-control" type="text" value="{$token.failcount} / {$token.maxfail}" disabled></div>
        </div>      
    </div>
</div>

{* Remove 'add_token' from POST, so that mfaAccount doesn't think we are in a token setup anymore.
 * Which means we return to the mfa intro page. *}
    <button class="btn"
        onclick="document.getElementById('add_token').remove();"
        name="setup_cancel"
        type="submit">{t}Cancel{/t}
    </button>
    <button class="btn primary"
        name="setup_continue"
        type="submit">{t}Save{/t}
    </button>
</div>