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

<h2>{t}Authentication app (TOTP) added{/t}</h2>

<div class="card-panel green lighten-4 green-text text-darken-4">
    {t escape=no 1=$tokenDescription}The authentication app <b>%1</b> was successfully added and can now be used.{/t}
</div>

<div class="section">
    <p>{t}Please ensure that nobody other than yourself is able to access this factor.{/t}</p>
    <p>{t}Please take precautions in case your device is damaged or lost and set up at least two different factors.{/t}
    </p>
</div>

<div class="section">
    <button class="btn primary" name="setup_continue" type="submit">{t}Back to overview{/t}
    </button>
</div>