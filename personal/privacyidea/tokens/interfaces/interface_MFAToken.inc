<?php

/*
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
 */

interface MFAToken
{
    /** @return string Icon class/name/path wathever the template needs. */
    public function getSetupCardIcon();
    /** @return string Title of setup card. */
    public function getSetupCardTitle();
    /** @return string Description of setup card. */
    public function getSetupCardDescription();
    /** @return string Button text of setup card. */
    public function getSetupCardButtonText();

    /**
     * Contructor
     * @param mfaAccount $mfaAccount
     */
    public function __construct(&$mfaAccount);

    /**
     * Renders setup steps/phases.
     *
     * @return bool 'False' is indicating that something did go wrong.
     */
    public function renderSetup();

    /**
     * @return string Rendered template, ready to be given to GOsa.
     *                e.g. mfaAccount's execute()
     */
    public function getRenderSetupResult();
}
