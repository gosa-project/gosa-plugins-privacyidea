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

class MFATotpToken extends MFATokenHelper implements MFAToken
{
    private PrivacyIdeaUtils $utils;
    private config $config;
    private mfaAccount $mfaAccount;
    /** @var ldapMultiplexer|LDAP */
    private $ldap;

    public $MAX_SETUP_PHASES = 4;
    public $TEMPLATE_PREFIX = "mfatype_totp/";

    /* VARIABLES WHICH WILL BE NEEDED ACROSS PHASES */
    /** @var string */
    private $tokenDescription;
    /** @var string */
    private $tokenSerial;
    /** @var string */
    private $mfaVerificationTan;
    /** @var string Token serial of a token, which needs verifying (rollout_state = 'verify'). */
    private $needsVerifying;

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
        // Special case for TOTP: Allow regenerating QR code.
        if (isset($_POST["mfa_generate_totp_secret"])) {
            // Make sure to provide needed variables again.
            if (isset($_POST["tokenDescription"])) {
                $this->smarty->assign("tokenDescription", $_POST["tokenDescription"]);
            }

            if ($this->utils->getPOSTTokenSerial() !== null) {
                // Delete old token.
                $this->utils->piSendRequest(array(), array(), "DELETE", "/token/" . $_POST["tokenSerial"]);
            }

            return false;
        }

        $this->tokenSerial = "";

        $tokenSerial = $this->utils->getPOSTTokenSerial();
        if (!isset($tokenSerial) || empty($tokenSerial)) {
            msg_dialog::display(
                _("Internal error"),
                _("Token serial wasn't given or was given malformed.") . "<br>" .
                $this->utils->pleaseTryAgainMsg()
            );

            return false;
        }

        $this->tokenSerial = $tokenSerial;

        return true;
    }

    /** @return bool */
    protected function evaluatePhase2()
    {
        $this->mfaVerificationTan = "";

        $mfaVerificationTan = $_POST["mfaTOTPCode"]; // TODO: Rename mfaTOTPCode to mfaVerificationTan.
        if (isset($mfaVerificationTan) && !empty($mfaVerificationTan)) {
            // Remove everything which isn't a digit.
            $mfaVerificationTan = preg_replace('~\D~', '', $mfaVerificationTan);

            if (!is_numeric($mfaVerificationTan)) {
                // Okay, $mfaTOTPCode is definitely borked.
                msg_dialog::display(
                    _("Internal error"),
                    _("Given validation code was malformed. We couldn't verify your token.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg()
                );

                return false;
            }

            $this->mfaVerificationTan = $mfaVerificationTan;
        }

        return true;
    }

    /** @return bool */
    protected function evaluatePhase3()
    {
        return true;
    }

    /** @return bool */
    protected function preparePhase0()
    {
        return true;
    }

    /** @return bool */
    protected function preparePhase1()
    {
        // TODO: Is serial set? it can be deleted.

        // Actually call utils to enroll TOTP token (in a 2 step process).
        $token = $this->utils->enrollTokenStep1(
            $this->mfaAccount->getUid(),
            mfaAccount::PI_USER_REALM,
            "totp",
            $this->tokenDescription
        );

        if (!isset($token) || empty($token)) {
            msg_dialog::display(
                _("Internal error"),
                _("Couldn't enroll token!") . "<br>" .
                $this->utils->pleaseTryAgainMsg()
            );

            return false;
        }

        $this->needsVerifying = "";
        if ($token["detail"]["rollout_state"] == "verify") {
            $this->needsVerifying = $token["detail"]["serial"];
        }

        $type = "googleurl"; // Use as default.
        // $type = "oathurl";
        // $type = "otpkey";

        // TODO: Replace 'issuer=privacyIDEA' with something else. :)
        // This will be directly visible in the TOTP app.

        $qrImage      = $token["detail"][$type]["img"];
        $qrImageValue = $token["detail"][$type]["value"];
        $tokenSerial  = $token["detail"]["serial"];

        $this->smarty->assign("qrImage", $qrImage);
        $this->smarty->assign("qrImageValue", $qrImageValue);
        $this->smarty->assign("tokenSerial", $tokenSerial);
        $this->smarty->assign("tokenDescription", $this->tokenDescription);

        return true;
    }

    /** @return bool */
    protected function preparePhase2()
    {
        $this->smarty->assign("tokenSerial", $this->tokenSerial);
        return true;
    }

    /** @return bool */
    protected function preparePhase3()
    {
        if ($this->needsVerifying != $this->tokenSerial) {
            // Token doesn't actually need verifying.
            return true;
        }

        $resultArray = $this->utils->enrollTokenStep2($this->tokenSerial, $this->mfaVerificationTan, "totp");
        if (!$resultArray["result"]["status"]) {
            if (isset($resultArray["result"]["error"]["message"])) {
                msg_dialog::display(
                    _("Internal error"),
                    _("Failed to execute step 2 of token enrollment.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg() . "<br>" .
                    sprintf(_("PrivacyIDEA server error message: %s"), $resultArray["result"]["error"]["message"])
                );
            } else {
                msg_dialog::display(
                    _("Internal error"),
                    _("Failed to execute step 2 of token enrollment.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg()
                );
            }
            return false;
        }

        return true;
    }

    public function getSetupCardIcon()
    {
        // Materialize Icon class.
        return "smartphone";
    }

    public function getSetupCardTitle()
    {
        return _("Authentication app (TOTP)");
    }

    public function getSetupCardDescription()
    {
        return _("An authentication app on your smartphone generates " .
            "time-based one-time passwords (TOTP) which must be entered " .
            "on each login.");
    }

    public function getSetupCardButtonText()
    {
        return _("Add authentication app");
    }

    public function getSetupCardPriority()
    {
        return 1;
    }
}
