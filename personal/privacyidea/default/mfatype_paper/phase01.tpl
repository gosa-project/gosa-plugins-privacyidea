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

<h2>{t}Print TAN list{/t}</h2>

<div class="section">
    <p>{t escape=no 1=$tokenDescription}Please print the TAN list <b>%1</b>, one TAN from the list will be used in the next step to verify the list.{/t}</p>
</div>

<div class="section">
    <p><b>{t}Keep this list secret.{/t}</b></p>
    <p>{t}Please print it and then store it in a secure location.{/t}</p>
    <button class="btn" type="button" id="mfaPaperPrint">{t}Print TAN list{/t}</button>
</div>
<template id="mfaTanListTemplate">
    <aside>
        <p>{t}Please print this page and then store it in a secure location.{/t}</p>
        <button value="Print page" onClick="window.print()">Print page</button>
    </aside>
    <h1>{t escape=no 1=$tokenDescription}TAN list <b>%1</b> (<time id="mfaTanListTimestamp"></time>){/t}</h1>
    <p>{t}If you just generated this list you will need to confirm it by entering the confirmation TAN on the website from which you downloaded this list.{/t}</p>
    <p>{t}Confirmation TAN:{/t} <span id="mfaConfirmationTan"></span></p>
    <h2>TAN</h2>
    <ol id="mfaTanList"></ol>
</template>
<template id="mfaTanListItemTemplate">
    <li class="mfaTanItem"></li>
</template>
<script type="text/javascript">
(() => {
    const tanList = JSON.parse("{$mfaTanJSON}");
    const confirmationTan = "{$mfaConfirmationTan}";

    const doc = document.implementation.createHTMLDocument("{t}TAN list {$tokenDescription}{/t}");

    const bodyContent = document.querySelector("#mfaTanListTemplate").content.cloneNode(true);

    const timeEl = bodyContent.querySelector("#mfaTanListTimestamp");
    const now = new Date();
    timeEl.dateTime = now.toISOString();
    timeEl.textContent = now.toLocaleString();

    bodyContent.querySelector("#mfaConfirmationTan").textContent = confirmationTan;

    const listEl = bodyContent.querySelector("#mfaTanList");

    const itemTemplate = document.querySelector("#mfaTanListItemTemplate");
    for (let tan of tanList) {
        let itemEl = itemTemplate.content.cloneNode(true);
        itemEl.querySelector(".mfaTanItem").textContent = tan;
        listEl.appendChild(itemEl);
    }

    doc.body.appendChild(bodyContent);

    const serializer = new XMLSerializer();
    const blob = new Blob([serializer.serializeToString(doc)], {
        type: "text/html"
    });
    const blobUrl = URL.createObjectURL(blob);

    document.querySelector("#mfaPaperPrint").addEventListener("click", e => {
        e.preventDefault();
        window.open(blobUrl);
    });
})();
</script>

<div class="section">
{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
    <input type="hidden" id="add_token" name="add_token" value="yes">
    <input type="hidden" id="token_type" name="token_type" value="paper">
    <input type="hidden" id="current_phase" name="current_phase" value="1">
    <input type="hidden" id="tokenSerial" name="tokenSerial" value="{$tokenSerial}">

{* Use a little hack, remove 'add_token' from POST, so that mfaAccount doesn't think we are in a token setup anymore.
 * Which means we return to the mfa intro page. *}
    <button class="btn"
        onclick="document.getElementById('add_token').remove();"
        name="setup_cancel"
        type="submit">{t}Cancel{/t}
    </button>
    <button class="btn primary"
        name="setup_continue"
        type="submit">{t}Continue{/t}
    </button>
</div>
