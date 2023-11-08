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

require_once(dirname(__FILE__) . "/interfaces/interface_MFAToken.inc");

class MFAWebAuthnToken extends MFATokenHelper implements MFAToken
{
    public $MAX_SETUP_PHASES = 1;
    public $TEMPLATE_PREFIX = "mfatype_webauthn/";

    /* VARIABLES WHICH WILL BE NEEDED ACROSS PHASES */

    /** @return bool */
    protected function evaluatePhase0()
    {
        return true;
    }

    /** @return bool */
    protected function preparePhase0()
    {
        return true;
    }

    public function getSetupCardIcon()
    {
        // Materialize Icon class.
        return "usb";
    }

    public function getSetupCardTitle()
    {
        return _("Security key");
    }

    public function getSetupCardDescription()
    {
        return _("FIDO2-compatible hardware security keys are e.g. " .
            "connected via USB of NFC and authentication is started " .
            "using a device-specifcic mechanism, e.g. touching a sensor.");
    }

    public function getSetupCardButtonText()
    {
        return _("Add security key");
    }

    public function getSetupCardPriority()
    {
        return 0;
    }
}
