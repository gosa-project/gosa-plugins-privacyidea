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

class MFARegistrationToken extends MFATokenHelper implements MFAToken
{
    public $MAX_SETUP_PHASES = 3;
    public $TEMPLATE_PREFIX = "mfatype_registration/";

    /* VARIABLES WHICH WILL BE NEEDED ACROSS PHASES */
    /** @var string */
    private $tokenDescription;
    /** @var string */
    private $mfaRecoveryKey;

    /** @return bool */
    protected function evaluatePhase0()
    {
        $this->tokenDescription = "";
        if (isset($_POST["tokenDescription"])) {
            $this->tokenDescription = $_POST["tokenDescription"];
            $this->tokenDescription = trim($this->tokenDescription);
        } else {
            // Empty descriptions are okay.
        }

        return true;
    }

    /** @return bool */
    protected function evaluatePhase1()
    {
        return true;
    }


    /** @return bool */
    protected function evaluatePhase2()
    {
        return true;
    }

    /** @return bool */
    protected function preparePhase0()
    {
        $limitReachedMessage = $this->checkUserTokenLimit("registration");
        if (!empty($limitReachedMessage)) {
            msg_dialog::display(
                _("Internal error"),
                $limitReachedMessage . "<br>" .
                $this->utils->pleaseTryAgainMsg()
            );
            return false;
        }

        return true;
    }

    /** @return bool */
    protected function preparePhase1()
    {
        $limitReachedMessage = $this->checkUserTokenLimit("registration");
        if (!empty($limitReachedMessage)) {
            msg_dialog::display(
                _("Internal error"),
                $limitReachedMessage . "<br>" .
                $this->utils->pleaseTryAgainMsg()
            );
            return false;
        }

        // TODO: Refactor all of this. use $this->utils->enrollTokenStep1

        $params["user"]        = $this->mfaAccount->getUid();
        $params["realm"]       = $this->mfaAccount->userRealm;
        $params["type"]        = "registration";
        $params["description"] = $this->tokenDescription;

        $requestAnswer = $this->utils->piSendRequest($params, array(), "POST", "/token/init");

        if (!$requestAnswer || !$requestAnswer["result"]["status"]) {
            // TODO: Check if error message is even available.
            msg_dialog::display(
                _("Internal error"),
                _("Could not enroll recovery key (registration token) in privacyIDEA: ") . "<br>" .
                $requestAnswer["result"]["error"]["message"] . "<br><br>" .
                $this->utils->pleaseTryAgainMsg()
            );
            return false;
        }

        new log(
            "modify",
            "users/" . get_class($this),
            $this->mfaAccount->dn,
            array(),
            sprintf("A new token of type '%s' has been enrolled for user '%s'.", "registration", $this->mfaAccount->getUid())
        );

        // Refresh token countings cache.
        // TODO: Use countUserTokens() but refactor it, so that it doesn't require $tokens.
        //       and it should gather the tokens from endpoint /token/ itself.
        $this->utils->getTokensOfUser($this->mfaAccount->getUid(), $this->mfaAccount->userRealm);

        $this->mfaRecoveryKey = $requestAnswer["detail"]["registrationcode"];

        $this->smarty->assign("tokenDescription", $this->tokenDescription);
        $this->smarty->assign("mfaRecoveryKey", $this->mfaRecoveryKey);

        return true;
    }

    /**
     * Builds address string out of LDAP attributes.
     * @param array $attributes
     * @return string
     */
    protected function buildAddress($attributes)
    {
        $sn            = isset($attributes["sn"][0])            ? trim($attributes["sn"][0])            : "";
        $givenName     = isset($attributes["givenName"][0])     ? trim($attributes["givenName"][0])     : "";

        $homePostalAddress = isset($attributes["homePostalAddress"][0]) ? trim($attributes["homePostalAddress"][0]) : "";
        $workPostalAddress = isset($attributes["postalAddress"][0])     ? trim($attributes["postalAddress"][0])     : "";

        $address = "-" . _("personally") . "-\n";

        if (!empty($academicTitle)) {
            $address .= $academicTitle . " ";
        }

        $address .= "$givenName $sn\n";

        if (!empty($workPostalAddress)) {
            $address .= $workPostalAddress;
            return $address;
        } elseif (!empty($homePostalAddress)) {
            $address .= $homePostalAddress;
            return $address;
        }

        // Downstream specific, shouldn't interfere with GOsa² usptream.
        $academicTitle = isset($attributes["academicTitle"][0]) ? trim($attributes["academicTitle"][0]) : "";
        $addressAddon  = isset($attributes["addressAddon"][0])  ? trim($attributes["addressAddon"][0])  : "";
        $street        = isset($attributes["street"][0])        ? trim($attributes["street"][0])        : "";
        $postalCode    = isset($attributes["postalCode"][0])    ? trim($attributes["postalCode"][0])    : "";
        $l             = isset($attributes["l"][0])             ? trim($attributes["l"][0])             : "";
        $description = "";
        $ou = "";

        if (!empty($description)) {
            if (!empty($ou)) {
                $address .= "$description ($ou)\n";
            } else {
                $address .= "$description\n";
            }
        }

        if (!empty($addressAddon)) {
            $address .= "$addressAddon\n";
        }

        $address .= "$street\n";
        $address .= "$postalCode $l";

        return trim($address);
    }

    /** @return bool */
    protected function preparePhase2()
    {
        if (!isset($this->mfaRecoveryKey) || empty($this->mfaRecoveryKey)) {
            return false;
        }

        require_once(dirname(__FILE__) . "/../PDF/class_RecoveryKeyPDF.inc");

        /* Get givenName and sn via UID */
        $ldap = $this->config->get_ldap_link();
        $ldap->cd($this->config->current['BASE']);
        $uid = $this->mfaAccount->getUid();
        $ldap->search(
            "(uid=$uid)",
            array("sn", "givenName", "academicTitle", "seeAlso", "homePostalAddress", "postalAddress")
        );
        $attributes = $ldap->fetch();

        $uid_dn = $attributes['dn'];
        $person_dn = "";
        if (
            !isset($attributes["givenName"][0]) &&
            !isset($attributes["sn"][0]) &&
            isset($attributes["seeAlso"][0]) && !empty($attributes["seeAlso"][0])
        ) {
            $person_dn = $attributes["seeAlso"][0];
            $ldap->cat(
                $person_dn,
                array("sn", "givenName", "academicTitle", "addressAddon", "street", "postalCode", "l")
            );
            $attributes = $ldap->fetch();
        }

        $address = $this->buildAddress($attributes);

        $infoPage = new RecoveryKeyPDF();
        $infoPage->RecoveryKeyPDF(
            $sn,
            $givenName,
            $academicTitle,
            $address,
            $this->mfaAccount->getUid(),
            $this->mfaRecoveryKey
        );

        ob_end_clean(); // Clean (erase) the output buffer and turn off output buffering.

        if (empty($this->tokenDescription)) {
            $pdfFileName = sprintf(_("recovery_key_%s.pdf"), $this->mfaAccount->getUid());
        } else {
            $tokenDescription = str_replace(" ", "-", $this->tokenDescription);
            $pdfFileName = sprintf(_("recovery_key_%s_%s.pdf"), $this->mfaAccount->getUid(), $tokenDescription);
        }

        $infoPage->output('D', $pdfFileName, true);

        return true;
    }

    public function getSetupCardTitle()
    {
        return _("Recovery key");
    }

    public function getSetupCardDescription()
    {
        return _("A recovery key can be used exactly once for logging in. " .
            "They are only issued if no other authentication method is " .
            "available, e.g. in case of hardware loss.");
    }

    public function getSetupCardButtonText()
    {
        return _("Add recovery key");
    }

    public function getSetupCardPriority()
    {
        return 3;
    }
}
