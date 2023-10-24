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

class MFARegistrationToken implements MFAToken
{
    private string $renderResult = "";
    private string $TEMPLATE_PREFIX = "mfatype_registration/";
    private PrivacyIdeaUtils $utils;
    private Smarty $smarty;
    private config $config;
    private mfaAccount $mfaAccount;
    /** @var ldapMultiplexer|LDAP */
    private $ldap;

    public function __construct(&$mfaAccount)
    {
        // Crashing is okay here, we want to find errors like this in early development.
        assert(get_class($mfaAccount)  == "mfaAccount");
        $this->mfaAccount = &$mfaAccount;

        $this->utils    = $mfaAccount->utils;
        $this->smarty   = $mfaAccount->smarty;
        $this->ldap     = $mfaAccount->ldap;
        $this->config   = $mfaAccount->config;
        assert(get_class($this->utils)   == "PrivacyIdeaUtils");
        assert(get_class($this->smarty)  == "Smarty");
        assert(get_class($this->config)  == "config");
        assert(get_class($this->ldap)    == "LDAP" ||
               get_class($this->ldap)    == "ldapMultiplexer");
    }

    public function renderSetup()
    {
        $this->renderResult = $this->smarty->fetch(
            get_template_path($this->TEMPLATE_PREFIX . "phase00.tpl", true, dirname(__FILE__) . "/..")
        );

        return true;
    }

    public function getRenderSetupResult()
    {
        return $this->renderResult;
    }

    public function getSetupCardIcon()
    {
        return _("password");
    }

    public function getSetupCardTitle()
    {
        return _("Wiederherstellungs­schlüssel");
    }

    public function getSetupCardDescription()
    {
        return _("Wiederherstellungsschlüssel werden zur einmaligen Anmeldung " .
            "verwendet und verfallen dann. Sie werden nur vergeben, wenn " .
            "keine andere Methode zur Anmeldung verfügbar ist, z.B. bei " .
            "Hardwareverlust.");
    }

    public function getSetupCardButtonText()
    {
        return _("Wiederherstellungsschlüssel hinzufügen");
    }
}
