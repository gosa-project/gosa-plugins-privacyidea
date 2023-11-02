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
<template id="mfaTanListHeadTemplate">
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
    <style>
@counter-style padded-decimals-parens {
    system: extends decimal;
    prefix: "(";
    suffix: ")\2001";
    pad: 3 "\2007";
}

html, body {
    margin: 0;
    padding: 0;
    height: 100%;
    font-family: sans-serif;
    font-size: 12pt;
    color: #000;
    background-color: #fff;
    line-height: 1.5em;
}

*, ::before, ::after {
    box-sizing: border-box;
}

button {
    font-size: 1rem;
    padding: .25rem;
}

h1:first-child {
    margin-top: 0;
}

h1 {
    font-size: 2rem;
}

h2 {
    font-size: 1.5rem;
}

#mfaConfirmationTan, #mfaConfirmationTan::before {
    display: inline-block;
    margin-left: 1rem;
    padding: .25rem;
    border: 1px solid #000;
}

#mfaTanList {
    padding-left: 0;
    column-width: 16ch; /* adjust to TAN length */
    column-gap: 2rem;
    column-rule: 1px solid #000;
    list-style-type: padded-decimals-parens;
    list-style-position: inside;
}

#mfaTanList li + li {
    margin-top: .5rem;
}

@media screen {
    body {
        margin: 1rem;
    }

    #mfaTanPrintingInstructions {
        text-align: center;
    }

    #mfaTanPage {
        margin: 2rem auto 0 auto;
        padding: 2rem;
        box-shadow: 4px 4px 16px rgba(0,0,0,0.5);
        max-width: 80ch;
        aspect-ratio: 1 / 1.2941;
    }

    #mfaConfirmationTan {
        visibility: hidden;
    }

    #mfaConfirmationTan::before {
        content: '{t}will be visible when printed{/t}';
        color: blue;
        visibility: visible;
    }

}

@media print {
    #mfaTanPrintingInstructions {
        display: none !important;
    }
}
    </style>
</template>
<template id="mfaTanListBodyTemplate">
    <aside id="mfaTanPrintingInstructions">
        <p>{t}Please print this page and then store it in a secure location.{/t}</p>
        <button value="Print page" onClick="window.print()">Print page</button>
    </aside>
    <div id="mfaTanPage">
        <h1>{t escape=no 1=$tokenDescription}TAN list <b>%1</b> (<time id="mfaTanListTimestamp"></time>){/t}</h1>
        <p>{t}If you just generated this list you will need to confirm it by entering the confirmation TAN on the website from which you downloaded this list.{/t}</p>
        <p><span id="mfaConfirmationLabel">{t}Confirmation TAN:{/t}</span> <span id="mfaConfirmationTan"></span></p>
        <h2>TAN</h2>
        <ol id="mfaTanList"></ol>
    </div>
</template>
<template id="mfaTanListItemTemplate">
    <li class="mfaTanItem"></li>
</template>
<script type="text/javascript">
(() => {
    const tanList = JSON.parse("{$mfaTanJSON}");
    const confirmationTan = "{$mfaConfirmationTan}";

    const doc = document.implementation.createHTMLDocument("{t}TAN list {$tokenDescription}{/t}");

    const headContent = document.querySelector("#mfaTanListHeadTemplate").content.cloneNode(true);
    doc.head.appendChild(headContent);

    const bodyContent = document.querySelector("#mfaTanListBodyTemplate").content.cloneNode(true);

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

{* Remove 'add_token' from POST, so that mfaAccount doesn't think we are in a token setup anymore.
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