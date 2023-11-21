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

<h2>{t}Recovery key added{/t}</h2>

<div class="card-panel green lighten-4 green-text text-darken-4">
    {t escape=no 1=$tokenDescription}The recovery key <b>%1</b> was successfully added to this account and can now be used for logging in one time only.{/t}
</div>

<div class="card-panel clue lighten-4 blue-text text-darken-4">
    <p>{t escape=no 1=$mfaRecoveryKey}The recovery key value is: <b>%1</b>{/t}<p>
    <p><a href="data:application/pdf;base64,{$mfaRecoveryKeyPDF_BASE64}" download>
        {t}Download as PDF{/t}<span style="font-size: 20px;" class="material-icons">file_download</span>
    </a></p>
</div>

<div class="section">
    <p>{t}Please ensure that nobody other than yourself is able to access this factor.{/t}</p>
    <p>{t}Recovery keys are fallback factors. Please make sure you have at least one factor configured for daily use.{/t}</p>
</div>

<div class="section">
    <button class="btn primary" type="submit">{t}Back to overview{/t}
    </button>
</div>
