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
<h2>{t}Multifactor Authentication Requirements{/t}</h2>
<div class="row">
{if !$mfaRequiredByRuleACL && !$mfaRequiredByUserACL && !$allowedTokenTypesACL && !$manageTokensACL}
    <p>{t}Insufficient permissions for viewing or editing this user's multifactor authentication properties.{/t}</p>
{else}
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
{/if}
</div>

{if $mfaRequiredByRuleACL || $mfaRequiredByUserACL || $allowedTokenTypesACL || $manageTokensACL}
{if $mfaRequiredByRuleACL}
{render acl=$mfaRequiredByRuleACL}
<div class="row">
    <div class="col">
        <label>
            <input name="mfaRequiredByRule" type="checkbox"{if $mfaRequiredByRule == "checked"} checked{/if}/>
            <span>{t}Additional factors required by organizational policy.{/t}</span>
        </label>
    </div>
</div>
{/render}
{/if}
{if ($parent != "roletabs")}
{if $mfaRequiredByUserACL}
{render acl=$mfaRequiredByUserACL}
<div class="row">
    <div class="col">
        <label>
            <input name="mfaRequiredByUser" type="checkbox"{if $mfaRequiredByUser == "checked"} checked{/if}/>
            <span>
                {t}Always require additional factors.{/t}
            </span>
        </label>
{if $mfaRequiredByUserDisabled}
        <div>{t}You need to add a multifactor method before you can enable this setting.{/t}</div>
{/if}
    </div>
</div>
{/render}
{/if}
{/if}

{if $allowedTokenTypesACL}
<h2>{t}Allowed Multifactor Methods{/t}</h2>
{foreach $allTokenTypes as $tokenType}
<div class="row">
    <div class="input field col s12">
        <label>
            {render acl=$allowedTokenTypesACL}
            <input type="checkbox" name="allowedTokenTypes[]" value="{$tokenType}"{if in_array($tokenType, $tokenTypes)} checked{/if}/>
            {/render}
            <span>{$mfa_{$tokenType}_title}</span>
        </label>
    </div>
</div>
{/foreach}
{/if}

{* If no token is registered, warn the user! *}
{if $parent != "roletabs" && $showWarningNoTokenRegistered}
<div class="card-panel red lighten-4 red-text text-darken-4">
    {if $currentUserUID == $currentObjectUID}
        <b>{t}Attention: {/t}</b> {t}You cannot log in again with this account because there is no multifactor method associated to it. Please add one of the available methods now.{/t}
    {else}
        <b>{t}Attention: {/t}</b> {t}The user cannot log in with this account because there is no multifactor method associated to it.{/t}
    {/if}
</div>
{/if}

{if ($parent != "roletabs")}
{if $manageTokensACL}
<script>
document.forms.mainform.addEventListener("submit", (e) => {
    if (e.submitter.name === "add_token") {
        for (let el of document.forms.mainform.querySelectorAll("input[type='checkbox']")) {
            if (el.checked != (el.getAttribute("checked") !== null)) {
                if (!confirm("{t}You have unsaved changes, are you sure you want to continue? All unsaved changes to this account will be lost.{/t}")) {
                    e.preventDefault();
                }
                break;
            }
        }
    }
});
</script>

{render acl=$manageTokensACL}
<hr class="divider">
<h2>{t}Add new Multifactor Methods{/t}</h2>
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
                            <i class="material-icons" title="{t}Overridden by ACL{/t}">error_outline</i>
                        {/if}
                        </span>

                        <p>
                            {$mfa_{$tokenType}_description}
                        </p>
                    </div>

                    <div class="card-action">
                        {render acl=$manageTokensACL}
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
                        {/render}
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
    background: none;
    border: none;
    margin: 0;
    padding: 0;
    cursor: pointer;
    font-size: inherit;
}

#mfaTokenList th,
#mfaTokenList td {
    vertical-align: top;
}

#mfaTokenList button:disabled {
    visibility: hidden;
}

.line-through {
    text-decoration: line-through;
}
</style>

<hr class="divider">
<h2>{t}Associated Multifactor Methods{/t}</h2>
    {if count($tokens) > 0}
    <div class="row">
        <div class="col s2">
            <fieldset id="mfaBatchOperation" style="border: none; padding: 0; margin: 0;">
                <div class="input-field">
                    <select id="mfaTokenBatchAction" name="mfaTokenBatchAction">
                        <option value="" disabled selected>{t}Choose an action{/t}</option>
                        {render acl=$tokenFailCountResetACL}
                        <option value="mfaTokenResetCounter">{t}Reset error counter{/t}</option>
                        {/render}
                        {render acl=$tokenActivationACL}
                        <option value="mfaTokenActivate">{t}Activate{/t}</option>
                        {/render}
                        {render acl=$tokenDeactivationACL}
                        <option value="mfaTokenDeactivate">{t}Deactivate{/t}</option>
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
    {$tokensEditable=(strpos($tokenFailCountResetACL, "w") !== false || strpos($tokenDeactivationACL, "w") !== false ||  strpos($tokenActivationACL, "w") !== false || strpos($tokenRevocationACL, "w") !== false || strpos($tokenRemovalACL, "w") !== false)}
    <div class="row">
        <div class="col s12">
        <table class="table" id="mfaTokenList">
            <colgroup>
                <col style="width: 2%;">
                <col style="width: 6%;">
                <col style="width: 15%">
                <col style="width: auto;">
                <col style="width: 16%">
                <col style="width: 6%">
                <col style="width: 6%">
                <col style="width: 11%">
            </colgroup>
            <thead>
                <tr>
                    <th><label><input type="checkbox" id="mfaTokensSelectAll"{if !$tokensEditable} disabled{/if}><span></span></label></th>
                    <th>{t}ID{/t}</th>
                    <th>{t}Type{/t}</th>
                    <th>{t}Description{/t}</th>
                    <th>{t}Last use{/t}</th>
                    <th>{t}Status{/t}</th>
                    <th>{t}Error counter{/t}</th>
                    <th>{t}Actions{/t}</th>
                </tr>
            </thead>
            <tbody>
                {foreach from=$tokens key=key item=token}
                    <tr>
                        <td><label><input type="checkbox" name="mfaTokenSerials[]" value="{$token.serial}"{if !$tokensEditable} disabled{/if}><span></span></label></td>
                        <td style="overflow-wrap: break-word;">
                            <button class="txtonlybtn{if $token.revoked} line-through{/if}" type="submit"
                             name="mfaTokenAction[mfaTokenView]" value="{$token.serial}">{$token.serial}</button>
                        </td>
                        <td>
                            {* TODO: Refactor getSetupCard{Icon,Title}, also mfa{tokenType}_{icon,title}.
                             * There are only available if in allowedTokenType or overriden by ACL.
                             * Make them a const in the MFA{Icon, Title}Token class. *}
                            {* TODO: Center text vertically *}
                             <button class="txtonlybtn{if $token.revoked} line-through{/if}" type="submit" name="mfaTokenAction[mfaTokenView]" value="{$token.serial}">
                                <span class="material-icons" style="font-size: inherit;">{$mfa_{$token.tokentype}_icon}</span>
                                {$mfa_{$token.tokentype}_title}
                            </button>
                        </td>
                        <td><button class="txtonlybtn{if $token.revoked} line-through{/if}" type="submit" name="mfaTokenAction[mfaTokenView]" value="{$token.serial}">
                            {if strpos($tokenDescriptionACL, "r") !== false}{$token.description|escape}{else}<i>{t}not shown{/t}</i>{/if}
                        </button></td>
                        <td>
                            {if strpos($tokenLastUsedACL, "r") !== false}
                                {if !empty($token.info.last_auth)}
                                    <time class="tokenLastUsed{if $token.revoked} line-through{/if}" datetime="{$token.info.last_auth}">{$token.info.last_auth}</time>
                                {else}
                                    <span class="{if $token.revoked} line-through{/if}">{t}Never used before{/t}</span>
                                {/if}
                            {else}
                                {t}<i>not shown</i>{/t}
                            {/if}
                        </td>
                        <td>{if strpos($tokenStatusACL, "r") !== false}{$token.status}{else}<i>{t}not shown{/t}</i>{/if}</td>
                        <td {if $token.revoked}class="line-through"{/if}>{if strpos($tokenFailCountACL, "r") !== false}{$token.failcount}{else}<i>{t}not shown{/t}</i>{/if}/{$token.maxfail}</td>
                        <td>
                            {render acl=$tokenDescriptionACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenEdit]" value="{$token.serial}" title="{t}Edit{/t}">
                                <span class="material-icons">edit</span>
                            </button>
                            {/render}
                            {render acl=$tokenFailCountResetACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenResetCounter]" value="{$token.serial}" title="{t}Reset error counter{/t}">
                                <span class="material-icons">restart_alt</span>
                            </button>
                            {/render}
                        {if $token.active}
                            {render acl=$tokenDeactivationACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenDeactivate]" value=_{$token.serial}" title="{t}Deactivate{/t}">
                                <span class="material-icons">lock_open</span>
                            </button>
                            {/render}
                        {else}
                            {render acl=$tokenActivationACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenActivate]" value="{$token.serial}" title="{t}Activate{/t}">
                                <span class="material-icons">lock</span>
                            </button>
                            {/render}
                        {/if}
                            {render acl=$tokenRevocationACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenRevoke]" value="{$token.serial}" title="{t}Revoke{/t}">
                                <span class="material-icons">cancel</span>
                            </button>
                            {/render}
                            {render acl=$tokenRemovalACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenRemove]" value="{$token.serial}" title="{t}Remove{/t}">
                                <span class="material-icons">delete_forever</span>
                            </button>
                            {/render}
                        </td>
                    </tr>
                {/foreach}
            </tbody>
        </table>
        </div>
    </div>
    <script>
        (() => {
        function toLocaleStringSupportsLocales() {
            return (
                typeof Intl === "object" &&
                !!Intl &&
                typeof Intl.DateTimeFormat === "function"
            );
        }

        function updateMfaBatchOperation()
        {
            document.querySelector("#mfaBatchOperation").disabled =
                document.querySelectorAll("input[name='mfaTokenSerials[]']:checked").length === 0;
        }

        function isTokenActionAllowed(action, tokenSerials) {
            switch (action) {
            case "mfaTokenDeactivate": // FALLTHROUGH
            case "mfaTokenRevoke":     // FALLTHROUGH
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

        for (let el of document.querySelectorAll(".tokenLastUsed")) {
            const d = new Date(el.dateTime);
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
    {else}
        {if $hasPiErrors}
            <div class="card-panel red lighten-4 red-text text-darken-4">
                {t}We had trouble communicating with the privacyIDEA-backend server.{/t}<br>
                {* Already translated string from backend. It says something
                 * like try it again later or contact the sysadmin.*}
                {$plsTryAgainMsg}
            </div>
        {else}
            <p>{t}Currently there are no multifactor methods associated.{/t}</p>
        {/if}
    {/if}
{/render}
{/if}
{/if}
{/if}
</fieldset>
