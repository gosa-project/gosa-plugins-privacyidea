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

class MFAWebAuthnToken implements MFAToken
{
    private string $renderResult = "";
    private string $TEMPLATE_PREFIX = "mfatype_webauthn/";
    private PrivacyIdeaUtils $utils;
    private Smarty $smarty;
    private config $config;
    private mfaAccount $mfaAccount;
    /** @var ldapMultiplexer|LDAP */
    private $ldap;

    /** @var int Indicates how many phases the token setup has. */
    public const MAX_SETUP_PHASES = 1;

    public function __construct(&$mfaAccount)
    {
        // Crashing is okay here, we want to find errors like this in early development.
        assert(get_class($mfaAccount)  == "mfaAccount");
        $this->mfaAccount = &$mfaAccount;

        $this->utils  = $mfaAccount->utils;
        $this->smarty = $mfaAccount->smarty;
        $this->ldap   = $mfaAccount->ldap;
        $this->config = $mfaAccount->config;
        assert(get_class($this->utils)  == "PrivacyIdeaUtils");
        assert(get_class($this->smarty) == "Smarty");
        assert(get_class($this->config) == "config");
        assert(get_class($this->ldap)   == "LDAP" ||
               get_class($this->ldap)   == "ldapMultiplexer");
    }

    public function renderSetup()
    {
        $phaseNumber = $this->getNextPhaseNumber();
        $templateFileName = "phase" . str_pad($phaseNumber, 2, "0", STR_PAD_LEFT) . ".tpl";

        $success = true;
        switch ($phaseNumber) {
            default:
                # PHASE 0 #
                $success = $success && $this->preparePhase0();
                break;
        }

        $this->renderResult = $this->smarty->fetch(
            get_template_path($this->TEMPLATE_PREFIX . $templateFileName, true, dirname(__FILE__) . "/..")
        );

        return true;
    }

    /** @return bool */
    private function preparePhase0()
    {
        // TODO: Prepare for phase 0.
        return true;
    }

    public function getRenderSetupResult()
    {
        return $this->renderResult;
    }

    public function getNextPhaseNumber()
    {
        if (!isset($_POST["current_phase"])) {
            return 0;
        }

        if (!is_numeric($_POST["current_phase"])) {
            return 0;
        }

        $currentPhase = $_POST["current_phase"];

        if ($currentPhase < 0 || $currentPhase >= self::MAX_SETUP_PHASES - 1) {
            return 0;
        }

        return $_POST["current_phase"] + 1;
    }

    public function getSetupCardIcon()
    {
        // Materialize Icon class.
        return "usb";
    }

    public function getSetupCardTitle()
    {
        return _("Sicherheitsschlüssel");
    }

    public function getSetupCardDescription()
    {
        return _("FIDO2-kompatible Sicherheitsschlüssel werden in einen " .
            "USB-Anschluss gesteckt oder kontaktlos über NFC angesprochen. " .
            "Während des Anmeldevorgangs wird je nach Modell eine Sensortaste " .
            "berührt oder ein Knopf gedrückt.");
    }

    public function getSetupCardButtonText()
    {
        return _("Sicherheitsschlüssel hinzufügen");
    }

    public function getSetupCardPriority()
    {
        return 0;
    }
}
