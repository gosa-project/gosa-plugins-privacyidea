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
<h2><i class="material-icons">{t escape=no 1=$mfa_{$token.tokentype}_icon}%1{/t}</i> {t 2=$mfa_{$token.tokentype}_title}%2 - Details{/t}</h2>
<div class="row">
    <div class="col s12">
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenSerial" value="{$token.serial}" disabled>
                <label for="tokenSerial">{t}Serial{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                {render acl=$tokenDescriptionACL}
                <input type="text" name="tokenDescription" value="{if strpos($tokenDescriptionACL, "r") !== false}{$token.description}{else}{t}not shown{/t}{/if}"{if !$editEnable} disabled{/if}>
                {/render}
                <label for="tokenDescription">{t}Description{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenLastUsed" value="{if strpos($tokenLastUsedACL, "r") !== false}{$token.info.last_auth}{else}{t}not shown{/t}{/if}" disabled>
                <label for="tokenLastUsed">{t}Last use{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenActive" value="{if strpos($tokenStatusACL, "r") !== false}{$token.active}{else}{t}not shown{/t}{/if}" disabled>
                <label for="tokenActive">{t}Status{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenTANIndex" value="4{* FIXME *}" disabled>
                <label for="tokenLastTANIndex">{t}Index of last used TAN{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenLoginAttempts" value="2{* FIXME *}" disabled>
                <label for="tokenLoginAttempts">{t}No. of login attempts{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenSuccessfulLogins" value="2{* FIXME *}" disabled>
                <label for="tokenSuccessfulLogins">{t}Successful logins{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenFailedLogins" value="{if strpos($tokenFailCountACL, "r") !== false}{$token.failcount}{else}{t}not shown{/t}{/if}/{$token.maxfail}" disabled>
                <label for="tokenFailedLogins">{t}Login failure counter (current / max.){/t}</label>
            </div>
        </div>
    </div>
</div>

<input type="hidden" id="tokenSerial" name="tokenSerial" value="{$token.serial}">

<div class="row">
{render acl=$tokenDescriptionACL}
{if $editEnable}
    <input type="hidden" id="editEnable" name="editEnable" value="yes">
    {* Remove 'add_token' from POST, so that mfaAccount doesn't think we are in a token setup anymore.
     * Which means we return to the mfa intro page. *}

    <button class="btn" formnovalidate
        type="submit">{t}Cancel{/t}
    </button>
    <button class="btn primary"
        name="mfaTokenAction" value="mfaTokenSave"
        type="submit">{t}Save{/t}
    </button>
{else}
    <button class="btn primary"
        name="mfaTokenAction" value="mfaTokenEdit"
        type="submit">{t}Edit{/t}
    </button>
{/if}
{/render}
</div>
