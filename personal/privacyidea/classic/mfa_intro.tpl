{*
 * This code is an addon for GOsa² (https://gosa.gonicus.de)
 * https://github.com/gosa-project/gosa-plugins-privacyidea/
 * Copyright (C) 2023 Daniel Teichmann <daniel.teichmann@das-netzwerkteam.de>
 * Copyright (C) 2023 Guido Berhoerster <guido+freiesoftware@berhoerster.name>
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

{* Define icons *}
{$mfa_registration_icon=password}
{$mfa_paper_icon=file_doc}
{$mfa_totp_icon=pda_alt}
{$mfa_webauthn_icon=usbpendrive_unmount}

<fieldset style="border: none; padding: 0; margin: 0;" {if !$userExists}disabled{/if}>
<h2>{t}Multifactor Authentication Requirements{/t}</h2>
{if !$mfaRequiredByRuleACL && !$mfaRequiredByUserACL && !$allowedTokenTypesACL && !$manageTokensACL}
<p>{t}Insufficient permissions for viewing or editing this user's multifactor authentication properties.{/t}</p>
{else}
<p>
    {t}Multifactor authentication protects your account from unauthorized access.{/t}
    {t}The current configuration for this account is that the use of MFA is {/t}
    {strip}<b>
        {if $mfaRequired}
            {t}required{/t}
        {else}
            {t}not required{/t}
        {/if}
    </b>{/strip}.
</p>
<p>{t}This happens due to organizational policy or user choice:{/t}</p>
{/if}

<table>
{if $mfaRequiredByRuleACL || $mfaRequiredByUserACL || $allowedTokenTypesACL || $manageTokensACL}
{if $mfaRequiredByRuleACL}
{render acl=$mfaRequiredByRuleACL}
    <tr>
        <td>
            <input name="mfaRequiredByRule" id="mfaRequiredByRule" type="checkbox"{if $mfaRequiredByRule == "checked"} checked{/if}/>
        </td>
        <td>
            <label for="mfaRequiredByRule">{t}Additional factors required by organizational policy.{/t}</label>
        <td>
    </tr>
{/render}
{/if}
{if ($parent != "roletabs")}
{if $mfaRequiredByUserACL}
{render acl=$mfaRequiredByUserACL}
    <tr>
        <td>
            <input name="mfaRequiredByUser" id="mfaRequiredByUser" type="checkbox"{if $mfaRequiredByUser == "checked"} checked{/if}/>
        </td>
        <td>
            <label for="mfaRequiredByUser">{t}Always require additional factors.{/t}</label>
        </td>
    </tr>
{/render}
{/if}
{/if}
</table>

{if $allowedTokenTypesACL}
<hr>
<h2>{t}Allowed Multifactor Methods{/t}</h2>
<table>
{foreach $allTokenTypes as $tokenType}
    {render acl=$allowedTokenTypesACL}
    <tr>
        <td>
            <input type="checkbox" name="allowedTokenTypes[]" id="allowedTokenTypes{$tokenType}" value="{$tokenType}"{if in_array($tokenType, $tokenTypes)} checked{/if}/>
        </td>
        <td>
            <label for="allowedTokenTypes{$tokenType}">{$mfa_{$tokenType}_title}</label>
        </td>
    </tr>
    {/render}
{/foreach}
</table>
{/if}

{* If no token is registered, warn the user! *}
{if $parent != "roletabs" && $showWarningNoTokenRegistered}
<table cellspacing="0" cellpadding="0">
    <tr>
        <td>
            {image path="images/warning.png"}
        </td>
        <td style="vertical-align: middle; padding-left: 5px">
        {if $currentUserUID == $currentObjectUID}
            <b>{t}Attention:{/t}</b> {t}You cannot log in again with this account because there is no multifactor method associated to it. Please add one of the available methods now.{/t}
        {else}
            <b>{t}Attention:{/t}</b> {t}The user cannot log in with this account because there is no multifactor method associated to it.{/t}
        {/if}
        </td>
    </tr>
</table>
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
<hr>
<h2>{t}Add new Multifactor Methods{/t}</h2>
{if count($effectiveTokenTypes) > 0}
    <p>
    {t}Here you can add a new login method for your account. You can choose between different methods below.{/t}
    </p>

    <div>
    {foreach $effectiveTokenTypes as $tokenType}
        <table style="width: 25%; min-width: 20em; float: left;" cellspacing="0" cellpadding="0">
            <tr>
                <td style="width: 48px"><img src="plugins/privacyidea/images/{$mfa_{$tokenType}_icon}-large.png" width="48" height="48"></td>
                <td>
                    <table style="width: 25%; min-width: 20em; float: left;" cellspacing="0" cellpadding="0">
                        <colgroup>
                            <col>
                            <col style="width: 1em">
                        </colgroup>
                        <thead>
                            <tr>
                                <th style="border-left: none; text-align: left">{$mfa_{$tokenType}_title}</th>
                                <th style="border-left: none">{if (!in_array($tokenType, $tokenTypes))}<span title="{t}Overridden by ACL{/t}">&#x2757;</span>{/if}</th>
                            </tr>
                        </thead>
                        <tbody>
                            <tr>
                                <td colspan="2">{$mfa_{$tokenType}_description}</td>
                            </tr>
                            <tr>
                                <td colspan="2" style="padding-top: 5px">
                                {render acl=$manageTokensACL}
                                {if empty({$mfa_{$tokenType}_limitReachedMessage})}
                                    <button name="add_token"
                                        value="{$tokenType}"
                                        type="submit"
                                    >
                                    {$mfa_{$tokenType}_button_text}
                                    </button>
                                {else}
                                    <button
                                        title="{$mfa_{$tokenType}_limitReachedMessage}"
                                        disabled
                                    >
                                    {$mfa_{$tokenType}_button_text}
                                    </button>
                                {/if}
                                {/render}
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </td>
            </tr>
        </table>
    {/foreach}
    <div style="clear: both"></div>
    </div>
{else}
    <p>
    {t}You are not allowed to configure any additional multifactor tokens.{/t}
    </p>
{/if}

<style>
.txtonlybtn {
    background: none;
    border: none;
    margin: 0;
    padding: 0;
    min-width: 0;
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

<hr>
<h2>{t}Associated Multifactor Methods{/t}</h2>
    {if count($tokens) > 0}
    <fieldset id="mfaBatchOperation" style="border: none; padding: 0; margin: 0;">
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
    </fieldset>
    {$tokensEditable=(strpos($tokenFailCountResetACL, "w") !== false || strpos($tokenDeactivationACL, "w") !== false ||  strpos($tokenActivationACL, "w") !== false || strpos($tokenRevocationACL, "w") !== false || strpos($tokenRemovalACL, "w") !== false)}
    <div class="sortableListContainer">
        <table id="mfaTokenList" style="width: 100%;" cellspacing="0">
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
                    <th style="border-left: none"><input type="checkbox" id="mfaTokensSelectAll"{if !$tokensEditable} disabled{/if}></th>
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
                {foreach from=$tokens key=$key item=$token}
                    <tr>
                        <td><input type="checkbox" name="mfaTokenSerials[]" value="{$token.serial}"{if !$tokensEditable} disabled{/if}></td>
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
                                <img src="plugins/privacyidea/images/{$mfa_{$token.tokentype}_icon}.png" width="16" height="16">
                                {$mfa_{$token.tokentype}_title}
                            </button>
                        </td>
                        <td><button class="txtonlybtn{if $token.revoked} line-through{/if}" type="submit" name="mfaTokenAction[mfaTokenView]" value="{$token.serial}">
                            {if strpos($tokenDescriptionACL, "r") !== false}{$token.description|escape}{else}{t}not shown{/t}{/if}
                        </button></td>
                        <td>
                            {if strpos($tokenLastUsedACL, "r") !== false}
                                {if !empty($token.info.last_auth)}
                                    <time class="tokenLastUsed{if $token.revoked} line-through{/if}" datetime="{$token.info.last_auth}">{$token.info.last_auth}</time>
                                {else}
                                    <span class="{if $token.revoked} line-through{/if}">{t}Never used before{/t}</span>
                                {/if}
                            {else}
                                {t}not shown{/t}
                            {/if}
                        </td>
                        <td>{if strpos($tokenStatusACL, "r") !== false}{$token.status}{else}{t}not shown{/t}{/if}</td>
                        <td {if $token.revoked}class="line-through"{/if}>{if strpos($tokenFailCountACL, "r") !== false}{$token.failcount}{else}{t}not shown{/t}{/if}/{$token.maxfail}</td>
                        <td>
                            {render acl=$tokenDescriptionACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenEdit]" value="{$token.serial}" title="{t}Edit{/t}">
                                {image path="images/lists/edit.png"}
                            </button>
                            {/render}
                            {render acl=$tokenFailCountResetACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenResetCounter]" value="{$token.serial}" title="{t}Reset error counter{/t}">
                                {image path="images/status_restart_all.png"}
                            </button>
                            {/render}
                        {if $token.active}
                            {render acl=$tokenDeactivationACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenDeactivate]" value=_{$token.serial}" title="{t}Deactivate{/t}">
                                {image path="images/lists/locked.png"}
                            </button>
                            {/render}
                        {else}
                            {render acl=$tokenActivationACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenActivate]" value="{$token.serial}" title="{t}Activate{/t}">
                                {image path="images/lists/unlocked.png"}
                            </button>
                            {/render}
                        {/if}
                            {render acl=$tokenRevocationACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenRevoke]" value="{$token.serial}" title="{t}Revoke{/t}">
                                {image path="images/lists/delete.png"}
                            </button>
                            {/render}
                            {render acl=$tokenRemovalACL}
                            <button class="txtonlybtn" type="submit" name="mfaTokenAction[mfaTokenRemove]" value="{$token.serial}" title="{t}Remove{/t}">
                                {image path="images/lists/trash.png"}
                            </button>
                            {/render}
                        </td>
                    </tr>
                {/foreach}
            </tbody>
        </table>
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
            <table cellspacing="0" cellpadding="0">
                <tr>
                    <td>
                        {image path="images/error.png"}
                    </td>
                    <td style="vertical-align: middle; padding-left: 5px">
                        {t}We had trouble communicating with the privacyIDEA-backend server.{/t}<br>
                        {* Already translated string from backend. It says something
                         * like try it again later or contact the sysadmin.*}
                        {$plsTryAgainMsg}
                    </td>
                </tr>
            </table>
        {else}
            <p>{t}Currently there are no multifactor methods associated.{/t}</p>
        {/if}
    {/if}
{/render}
{/if}
{/if}
{/if}
</fieldset>
