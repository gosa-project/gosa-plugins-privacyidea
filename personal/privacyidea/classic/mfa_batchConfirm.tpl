{*
 * This code is an addon for GOsa² (https://gosa.gonicus.de)
 * https://github.com/gosa-project/gosa-plugins-privacyidea/
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

<table cellspacing="0" cellpadding="0">
    <tr>
        <td>{image path='images/warning.png'}</td>
        <td style="padding-left: 5px; vertical-align: middle"><h2>{$confirmationPrompt}</h2></td>
    </tr>
</table>

{if $isBatch}
    <input type="hidden" name="mfaTokenBatchAction" value="{$mfaTokenBatchAction}">
    <ul>
        {foreach $mfaTokenSerials as $tokenSerial}
            <li><input type="hidden" name="mfaTokenSerials[]" value="{$tokenSerial}"><b>{$tokensDescriptions[$tokenSerial]|escape}</b></li>
        {/foreach}
    </ul>
{else}
    <input type="hidden" name="mfaTokenAction[{$mfaTokenAction}]" value="{$tokenSerial}">
    <p><b>{$tokensDescriptions[$tokenSerial]|escape}</b></p>
{/if}

<hr>
<div class="plugin-actions">
    <button type="submit" name="mfaTokenActionConfirm" value="false">{t}Cancel{/t}</button>
    <button type="submit" name="mfaTokenActionConfirm" value="true">{t}OK{/t}</button>
</div>
