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

{* Define materialize icons *}
{$mfa_registration_icon=password}
{$mfa_paper_icon=receipt}
{$mfa_totp_icon=smartphone}
{$mfa_webauthn_icon=usb}

{assign var=tokenLastUsed value="{t}not shown{/t}"}
{if strpos($tokenLastUsedACL, "r") !== false}
    {if !empty($token.info.last_auth)}
        {assign var=tokenLastUsed value=$token.info.last_auth}
    {else}
        {assign var=tokenLastUsed value="{t}Never used before{/t}"}
    {/if}
{/if}

<h2><i class="material-icons">{$mfa_{$token.tokentype}_icon}</i> {t 1=$mfa_{$token.tokentype}_title}%1 - Details{/t}</h2>
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
                <input type="text" name="tokenDescription"
                    {* If in the future more details can be edited, copy this snippet here... *}
                    {if strpos($tokenDescriptionACL, "r") !== false}
                        value="{if !$editEnable && empty($token.description)}{t}(empty){/t}{else}{$token.description}{/if}"
                    {else}
                        {if $editEnable}
                            placeholder="{t}not shown but editable{/t}"
                        {else}
                            value="{t}not shown{/t}"
                        {/if}
                    {/if}
                    {if !$editEnable || !(strpos($tokenDescriptionACL, "w") !== false)} disabled{/if}
                >
                <label for="tokenDescription">{t}Description{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" id="tokenLastUsed" name="tokenLastUsed" value="{$tokenLastUsed}" disabled>
                <label for="tokenLastUsed">{t}Last use{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenStatus" value="{if strpos($tokenStatusACL, "r") !== false}{$token.status}{else}{t}not shown{/t}{/if}" disabled>
                <label for="tokenStatus">{t}Status{/t}</label>
            </div>
        </div>
        {* TODO: This data isn't provided by privacyIDEA?! *}
        {* <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenTANIndex" value="4" disabled>
                <label for="tokenLastTANIndex">{t}Last used TAN no.{/t}</label>
            </div>
        </div> *}
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenLoginAttempts" value="{if strpos($tokenCountAuthACL, "r") !== false}{if empty($token.info.count_auth)}{t}Never used before{/t}{else}{$token.info.count_auth}{/if}{else}{t}not shown{/t}{/if}" disabled>
                <label for="tokenLoginAttempts">{t}No. of login attempts{/t}</label>
            </div>
        </div>
        <div class="row">
            <div class="input-field col s12 xl6">
                <input type="text" name="tokenSuccessfulLogins" value="{if strpos($tokenCountAuthSuccessACL, "r") !== false}{if empty($token.info.count_auth_succes)}{t}Never used before{/t}{else}{$token.info.count_auth_success}{/if}{else}{t}not shown{/t}{/if}" disabled>
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

<script>
function toLocaleStringSupportsLocales() {
    return (
        typeof Intl === "object" &&
        !!Intl &&
        typeof Intl.DateTimeFormat === "function"
    );
}

(() => {
const el = document.querySelector("#tokenLastUsed");
if (el.value.match(/^[0-9]/)) {
    const d = new Date(el.value);
    if (toLocaleStringSupportsLocales()) {
        el.textContent = d.toLocaleString("default", {
            year: "numeric",
            month: "2-digit",
            day: "2-digit",
            hour: "2-digit",
            minute: "2-digit",
            second: "2-digit",
        });
    } else {
        el.textContent = d.toLocaleString();
    }
}
})();
</script>

<input type="hidden" id="tokenSerial" name="tokenSerial" value="{$token.serial}">

<div class="row">
{* If in the future more details can be edited, add ACLs here too, then. *}
{$allowEdit = (strpos($tokenDescriptionACL, "w") !== false)}

{if $editEnable}
    <input type="hidden" id="editEnable" name="editEnable" value="yes">


    <button class="btn primary"
        name="mfaTokenAction[mfaTokenSave]" value="{$token.serial}"
        type="submit">{t}Save{/t}
    </button>
    <button class="btn" formnovalidate style="order: -1;"
        type="submit">{t}Cancel{/t}
    </button>
{elseif $allowEdit}
    <button class="btn primary"
        name="mfaTokenAction[mfaTokenEdit]" value="{$token.serial}"
        type="submit">{t}Edit{/t}
    </button>
{else}
    <button class="btn" formnovalidate
        type="submit">{t}Back{/t}
    </button>
{/if}
</div>
