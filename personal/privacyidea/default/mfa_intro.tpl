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
<h2>Multifactor authentication requirements</h2>
<div class="row">
    <p>
        {t}Multifactor authentication protects your account from unauthorized access.{/t}
        {t}The current configuration for this account is that the use of MFA is {/t}
        <b>
            {if $mfaRequired}
                {t}necessary{/t}
            {else}
                {t}not necessary{/t}
            {/if}
        </b>.<br>
        {t}This happens due to organization policy or when you choose to do so:{/t}
    </p>
</div>

<div class="row">
    <div class="col">
        <label>
            <input type="checkbox" {if $mfaRequiredByRule == "checked"}checked{/if} disabled>
            <span>{t}Additional factor necessary due to organisation requirements.{/t}</span>
        </label>
    </div>
</div>
<div class="row">
    <div class="col">
        <label>
            {render acl=$wishMfaRequired_ACL}
                <input name="wishMfaRequired" type="checkbox" {if $wishMfaRequired == "checked"}checked{/if}/>
            {/render}
            <span>
                {render acl=$wishMfaRequired_ACL}
                    {t}Additional factors should always be asked voluntarily.{/t}
                {/render}
            </span>
        </label>
    </div>
</div>

{* If no token is registered, warn the user! *}
{if $showWarningNoTokenRegistered}
<div class="card-panel red lighten-4 red-text text-darken-4">
    <b>{t}Attention: {/t}</b>{t}You cannot log in again with this account because there is no multifactor method associated with it. Please add one of the available methods now.{/t}
</div>
{/if}

<div class="row mt-1">
    <div class="col">
        <button class="btn-small primary" name="edit_apply" type="submit">{t}Save settings above{/t}</button>
    </div>
</div>


<hr class="divider">
<h2>{t}Add new multifactor token{/t}</h2>
{if count($tokenTypes) > 0}
    <div class="row">
        <p>
        {t}Here you can add a new login method for your account. You can choose between different methods below.{/t}
        </p>
    </div>

    <input type="hidden" id="token_type" name="token_type" value="">

    <div class="row">
        {foreach $tokenTypes as $tokenType}
            {render acl=$overrideAllowToken_{$tokenType}_ACL}
            {* TODO: Calculate based on token type count? *}
            <div class="col s12 m12 l6 xl3">
                <div class="card large">
                    <div class="card-content">
                        <i class="material-icons left">{$mfa_{$tokenType}_icon}</i>
                        <span class="card-title">{t}{$mfa_{$tokenType}_title}{/t}</span>
                        <p>
                            {t}
                            {$mfa_{$tokenType}_description}
                            {/t}
                        </p>
                    </div>

                    <div class="card-action">
                        <input type="hidden" name="php_c_check" value="1">
                        <button
                            class="btn-small primary"
                            {* Changing #token_type's value to $tokenType *}
                            onclick="document.getElementById('token_type').value = '{$tokenType}';"
                            name="add_token"
                            value="yes"
                            type="submit"
                        >
                            {t}{$mfa_{$tokenType}_button_text}{/t}
                        </button>
                    </div>
                </div>
            </div>
            {/render}
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
<div class="row">
    {if count($tokens) > 0}
        <div class="col-md-12 col-xl-9">
        <table class="table">
            <thead>
                <tr>
                    <th style="width: 2%" ><input type="checkbox"></th>
                    <th style="width: 5%" >{t}ID{/t}</th>
                    <th style="width: 20%">{t}Type{/t}</th>
                    <th>{t}Description{/t}</th>
                    <th style="width: 15%">{t}Last use{/t}</th>
                    <th style="width: 5%" >{t}Status{/t}</th>
                    <th style="width: 5%" >{t}Error counter{/t}</th>
                    <th style="width: 10%">{t}Actions{/t}</th>
                </tr>
            </thead>
            <tbody>
                {foreach from=$tokens key=$key item=$token}
                    <tr>
                        <td><input type="checkbox"></td>
                        <td><button class="txtonlybtn" type="submit" name="mfaTokenEdit">{$token.serial}</button></td>
                        <td>
                            {* TODO: Refactor getSetupCard{Icon,Title}, also mfa{tokenType}_{icon,title}.
                             * There are only available if in allowedTokenType or overriden by ACL.
                             * Make them a const in the MFA{Icon, Title}Token class. *}
                            {* TODO: Center text vertically *}
                             <button class="txtonlybtn" type="submit" name="mfaTokenEdit">
                                <span class="material-icons">{$mfa_{$token.tokentype}_icon}</span>
                                {$mfa_{$token.tokentype}_title}
                            </button>
                        </td>
                        <td><button class="txtonlybtn" type="submit" name="mfaTokenEdit">{$token.description}</button></td>
                        <td>{$token.info.last_auth}</td>
                        <td>{$token.active}</td>
                        <td>{$token.failcount} / {$token.maxfail}</td>
                        <td>
                            <button class="txtonlybtn" type="submit" name="mfaTokenEdit" title="{t}Edit{/t}">
                                <span class="material-icons">edit</span>
                            </button>
                            <button class="txtonlybtn" type="submit" name="mfaTokenResetCounter" title="{t}Reset error counter{/t}">
                                <span class="material-icons">restart_alt</span>
                            </button>
                            <button class="txtonlybtn" type="submit" name="mfaTokenDeactivate" title="{t}Deactivate{/t}">
                                <span class="material-icons">lock</span>
                            </button>
                            <button class="txtonlybtn" type="submit" name="mfaTokenRecall" title="{t}Recall{/t}">
                                <span class="material-icons">cancel</span>
                            </button>
                            <button class="txtonlybtn" type="submit" name="mfaTokenRemove" title="{t}Remove{/t}">
                                <span class="material-icons">delete_forever</span>
                            </button>
                        </td>
                    </tr>
                {/foreach}
            </tbody>
        </table>
        </div>
    {else}
        <span>{t}Currently there are no multifactor methods associated.{/t}</span>
    {/if}
</div>
