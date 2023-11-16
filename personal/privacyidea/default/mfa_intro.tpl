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

<fieldset style="border: none; padding: 0; margin: 0;" {if !$userExists}disabled{/if}>
<h2>{t}Multifactor authentication requirements{/t}</h2>
<div class="row">
    <p>
        {t}Multifactor authentication protects your account from unauthorized access.{/t}
        {t}The current configuration for this account is that the use of MFA is {/t}
        <b>
            {if $mfaRequired}
                {t}required{/t}
            {else}
                {t}not required{/t}
            {/if}
        </b>.<br>
        {t}This happens due to organizational policy or user choice:{/t}
    </p>
</div>

{render acl=$mfaRequiredByRuleACL}
<div class="row">
    <div class="col">
        <label>
            <input name="mfaRequiredByRule" type="checkbox" {if !$attributesEditMode}disabled{/if} {if $mfaRequiredByRule == "checked"}checked{/if}>
            <span>{t}Additional factors required by organizational policy.{/t}</span>
        </label>
    </div>
</div>
{/render}
{render acl=$mfaRequiredByUserACL}
<div class="row">
    <div class="col">
        <label>
            <input name="mfaRequiredByUser" type="checkbox" {if !$attributesEditMode}disabled{/if} {if $mfaRequiredByUser == "checked"}checked{/if}/>
            <span>
                {t}Always require additional factors.{/t}
            </span>
        </label>
    </div>
</div>
{/render}

{if $allowedTokenTypesACL}
{render acl=$allowedTokenTypesACL}
<h2>{t}Allowed factors{/t}</h2>
<div class="row">
    <div class="input field col s12">
        <select name="allowedTokenTypes[]" {if !$attributesEditMode && $parent!="roletabs"}disabled{/if} multiple>
            <option value="" disabled{if empty(tokenTypes)} selected{/if}>{t}Choose allowed factors{/t}</option>
            {foreach $allTokenTypes as $tokenType}
            <option value="{$tokenType}"{if in_array($tokenType, $tokenTypes)} selected{/if}>{$mfa_{$tokenType}_title}</option>
            {/foreach}
        </select>
        <label></label>
    </div>
</div>
{/render}
{/if}

{* If no token is registered, warn the user! *}
{if ($parent == "usertabs" || $parent == "") && $showWarningNoTokenRegistered}
<div class="card-panel red lighten-4 red-text text-darken-4">
    <b>{t}Attention: {/t}</b>{t}You cannot log in again with this account because there is no multifactor method associated with it. Please add one of the available methods now.{/t}
</div>
{/if}

{if ($parent != "roletabs")}
<div class="row mt-1">
    <div class="col">
        {if $attributesEditMode}
            <button class="btn-small primary" name="edit_apply" type="submit">{t}Save settings{/t}</button>
        {else}
            <button class="btn-small primary" name="attributesEdit" type="submit">{t}Edit settings{/t}</button>
        {/if}
    </div>
</div>
{/if}


{if ($parent == "usertabs" || $parent == "") &&  !$attributesEditMode}
{if $manageTokensACL}
{render acl=$manageTokensACL}
<hr class="divider">
<h2>{t}Add new multifactor token{/t}</h2>
{if count($effectiveTokenTypes) > 0}
    <div class="row">
        <p>
        {t}Here you can add a new login method for your account. You can choose between different methods below.{/t}
        </p>
    </div>

    <div class="row">
        {foreach $effectiveTokenTypes as $tokenType}
            {* TODO: Calculate based on token type count? *}
            <div class="col s12 m12 l6 xl3">
                <div class="card large">
                    <div class="card-content">
                        <i class="material-icons left">{$mfa_{$tokenType}_icon}</i>
                        <span class="card-title">{$mfa_{$tokenType}_title}
                        {if (!in_array($tokenType, $tokenTypes))}
                            {t}[admin]{/t}
                        {/if}
                        </span>

                        <p>
                            {$mfa_{$tokenType}_description}
                        </p>
                    </div>

                    <div class="card-action">
                        {if empty({$mfa_{$tokenType}_limitReachedMessage})}
                            <button
                                class="btn-small primary"
                                name="add_token"
                                value="{$tokenType}"
                                type="submit"
                            >
                            {$mfa_{$tokenType}_button_text}
                            </button>
                        {else}
                            <button
                                class="btn-small primary"
                                title="{$mfa_{$tokenType}_limitReachedMessage}"
                                disabled
                            >
                            {$mfa_{$tokenType}_button_text}
                            </button>
                        {/if}
                    </div>
                </div>
            </div>
        {/foreach}
    </div>
{else}
    <div class="row">
        <p>
        {t}You are not allowed to configure any additional multifactor tokens.{/t}
        </p>
    </div>
{/if}

<style>
.txtonlybtn {
        background:none;
        border:none;
        margin:0;
        padding:0;
        cursor: pointer;
        font-size: 1em;
}
</style>

<hr class="divider">
<h2>{t}Associated multifactor methods{/t}</h2>
    {if count($tokens) > 0}
    <div class="row">
        <div class="col s2">
            <fieldset id="mfaBatchOperation" style="border: none; padding: 0; margin: 0;">
                <div class="input-field">
                    <select id="mfaTokenBatchAction" name="mfaTokenBatchAction">
                        <option value="" disabled selected>{t}Choose an action{/t}</option>
                        {render acl=$tokenFailCountACL}
                        <option value="mfaTokenResetCounter">{t}Reset error counter{/t}</option>
                        {/render}
                        {render acl=$tokenStatusACL}
                        <option value="mfaTokenEnable">{t}Activate{/t}</option>
                        <option value="mfaTokenDisable">{t}Deactivate{/t}</option>
                        {/render}
                        {render acl=$tokenRevocationACL}
                        <option value="mfaTokenRevoke">{t}Revoke{/t}</option>
                        {/render}
                        {render acl=$tokenRemovalACL}
                        <option value="mfaTokenRemove">{t}Remove{/t}</option>
                        {/render}
                    </select>
                </div>
            </fieldset>
        </div>
    </div>
    <div class="row">
        <div class="col s12">
        <table class="table" id="mfaTokenList">
            <thead>
                <tr>
                    <th style="width: 2%" ><label><input type="checkbox" id="mfaTokensSelectAll"><span></span></label></th>
                    <th style="width: 6%;" >{t}ID{/t}</th>
                    <th style="width: 15%">{t}Type{/t}</th>
                    <th>{t}Description{/t}</th>
                    <th style="width: 16%">{t}Last use{/t}</th>
                    <th style="width: 6%" >{t}Status{/t}</th>
                    <th style="width: 6%" >{t}Error counter{/t}</th>
                    <th style="width: 11%">{t}Actions{/t}</th>
                </tr>
            </thead>
            <tbody>
                {foreach from=$tokens key=$key item=$token}
                    <form method="post">
                    {* Indicate mfaAccount that we want to execute an action on a token *}
                    <input type="hidden" id="tokenSerial" name="tokenSerial" value="{$token.serial}">
                    <input type="hidden" name="php_c_check" value="1">
                    <tr>
                        <td><label><input type="checkbox" name="mfaTokenSerials[]" value="{$token.serial}"><span></span></label></td>
                        <td><button style="font-size: 0.8em;" class="txtonlybtn" type="submit"
                                    name="mfaTokenAction" value="mfaTokenView">
                            {$token.serial}
                        </button></td>
                        <td>
                            {* TODO: Refactor getSetupCard{Icon,Title}, also mfa{tokenType}_{icon,title}.
                             * There are only available if in allowedTokenType or overriden by ACL.
                             * Make them a const in the MFA{Icon, Title}Token class. *}
                            {* TODO: Center text vertically *}
                             <button class="txtonlybtn" type="submit" name="mfaTokenAction" value="mfaTokenView">
                                <span class="material-icons">{$mfa_{$token.tokentype}_icon}</span>
                                {$mfa_{$token.tokentype}_title}
                            </button>
                        </td>
                        <td><button class="txtonlybtn" type="submit" name="mfaTokenAction" value="mfaTokenView">
                            {if strpos($tokenDescriptionACL, "r") !== false}{$token.description}{else}{t}not shown{/t}{/if}
                        </button></td>
                        <td>{if strpos($tokenLastUsedACL, "r") !== false}{$token.info.last_auth}{else}{t}not shown{/t}{/if}</td>
                        <td>{if strpos($tokenStatusACL, "r") !== false}{$token.status}{else}{t}not shown{/t}{/if}</td>
                        <td>{if strpos($tokenFailCountACL, "r") !== false}{$token.failcount}{else}{t}not shown{/t}{/if}/{$token.maxfail}</td>
                        <td>
                            {render acl=$tokenDescriptionACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction" value="mfaTokenEdit" title="{t}Edit{/t}">
                                <span class="material-icons">edit</span>
                            </button>
                            {/render}
                    {if !$token.revoked}
                            {render acl=$tokenFailCountACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction" value="mfaTokenResetCounter" title="{t}Reset error counter{/t}">
                                <span class="material-icons">restart_alt</span>
                            </button>
                            {/render}
                        {if $token.active}
                            {render acl=$tokenStatusACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction" value="mfaTokenDisable" title="{t}Deactivate{/t}">
                                <span class="material-icons">lock</span>
                            </button>
                            {/render}
                        {else}
                            {render acl=$tokenStatusACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction" value="mfaTokenEnable" title="{t}Activate{/t}">
                                <span class="material-icons">lock_open</span>
                            </button>
                            {/render}
                        {/if}
                            {render acl=$tokenRevocationACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction" value="mfaTokenRevoke" title="{t}Revoke{/t}">
                                <span class="material-icons">cancel</span>
                            </button>
                            {/render}
                    {/if}
                            {render acl=$tokenRemovalACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction" value="mfaTokenRemove" title="{t}Remove{/t}">
                                <span class="material-icons">delete_forever</span>
                            </button>
                            {/render}
                        </td>
                    </tr>
                    </form>
                {/foreach}
            </tbody>
        </table>
        </div>
    </div>
<script>
(() => {
function updateMfaBatchOperation()
{
    document.querySelector("#mfaBatchOperation").disabled =
        document.querySelectorAll("input[name='mfaTokenSerials[]']:checked").length === 0;
}

function isTokenActionAllowed(action, tokenSerials) {
    switch (action) {
    case "mfaTokenDisable": // FALLTHROUGH
    case "mfaTokenRevoke":  // FALLTHROUGH
    case "mfaTokenRemove":
        let activeTokenCount = ACTIVE_TOKEN_SERIALS.size;
        for (tokenSerial of tokenSerials) {
            if (ACTIVE_TOKEN_SERIALS.has(tokenSerial)) {
                activeTokenCount--;
            }
        }
        return activeTokenCount > 0;
    default:
        return true;
    }
}

document.querySelector("#mfaTokenList").addEventListener("change", (e) => {
    if (e.target.id === "mfaTokensSelectAll") {
        for (el of document.querySelectorAll("input[name='mfaTokenSerials[]']")) {
            el.checked = e.target.checked;
        }
    } else if (e.target.name === "mfaTokenSerials[]") {
        document.querySelector("#mfaTokensSelectAll").checked = false;
    }
    updateMfaBatchOperation();
});

document.querySelector("#mfaBatchOperation").addEventListener("change", (e) => {
    let data = new FormData(document.forms.mainform);
    let tokenSerials = new Set(data.getAll("mfaTokenSerials[]"));
    if (!isTokenActionAllowed(e.target.value, tokenSerials)) {
        document.querySelector("#mfaTokenBatchAction").value = "";
        alert("{t}At least one token needs to remain active.{/t}");
        return;
    }

    e.target.form.submit();
});

document.forms.mainform.addEventListener("submit", (e) => {
    let data = new FormData(document.forms.mainform);
    let tokenSerials = new Set([data.get("tokenSerial")]);
    if (e.submitter.name === "mfaTokenAction" && !isTokenActionAllowed(e.submitter.value, tokenSerials)) {
        e.preventDefault();
        alert("{t}At least one token needs to remain active.{/t}");
        return;
    }
});

const ACTIVE_TOKEN_SERIALS = new Set([
{foreach $activeTokenSerials as $tokenSerial}
    "{$tokenSerial}",
{/foreach}
]);

updateMfaBatchOperation();
})();
</script>
    {else}
        <p>{t}Currently there are no multifactor methods associated.{/t}</p>
    {/if}
{/render}
{/if}
{/if}
</fieldset>
