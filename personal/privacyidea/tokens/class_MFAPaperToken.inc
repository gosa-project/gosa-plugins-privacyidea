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

class MFAPaperToken extends MFATokenHelper implements MFAToken
{
    public $MAX_SETUP_PHASES = 4;
    public $TEMPLATE_PREFIX = "mfatype_paper/";

    /* VARIABLES WHICH WILL BE NEEDED ACROSS PHASES */
    /** @var string */
    private $tokenDescription;
    /** @var string */
    private $tokenSerial;
    /** @var string */
    private $mfaVerificationTan;
    /** @var string Token serial of a token, which needs verifying (rollout_state = 'verify'). */
    private $needsVerifying;

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

        $mfaVerificationTan = $_POST["mfaVerificationTan"];
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
        }

        $this->mfaVerificationTan = $mfaVerificationTan;

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
        $token = $this->utils->enrollTokenStep1(
            $this->mfaAccount->getUid(),
            mfaAccount::PI_USER_REALM,
            "paper",
            $this->tokenDescription
        );

        $this->needsVerifying = "";
        if ($token["detail"]["rollout_state"] == "verify") {
            $this->needsVerifying = $token["detail"]["serial"];
        }

        if (!isset($token) || empty($token)) {
            msg_dialog::display(
                _("Internal error"),
                _("Couldn't enroll token!") . "<br>" .
                $this->utils->pleaseTryAgainMsg()
            );

            return false;
        }

        $tokenSerial  = $token["detail"]["serial"];
        $tokenOTPs    = $token["detail"]["otps"];
        $tokenConfTAN = $token["detail"]["otps"][0];

        $amountOfPaperTokenOTPs = $this->utils->getConfigIntValue("amountOfPaperTokenOTPs");

        if ($amountOfPaperTokenOTPs == 0) {
            // This is most likely an error. (Just disable paper tokens / TAN lists?!)
            // Assume healthy default value.
            // TODO: Log error message here (to console).
            $amountOfPaperTokenOTPs = 20;
        }

        // Cut first item. (It is used as a confirmation TAN.)
        // Also have exactly $amountOfPaperTokenOTPs in $tokenOTPs.
        $tokenOTPs = array_splice($tokenOTPs, 1, $amountOfPaperTokenOTPs);

        $this->smarty->assign("mfaTanJSON", addslashes(json_encode($tokenOTPs)));
        $this->smarty->assign("mfaConfirmationTan", $tokenConfTAN);
        $this->smarty->assign("tokenSerial", $tokenSerial);
        $this->smarty->assign("tokenDescription", $this->tokenDescription);

        return true;
    }

    /** @return bool */
    protected function preparePhase2()
    {
        $this->smarty->assign("tokenSerial", $this->tokenSerial);
        $this->smarty->assign("needsVerifying", $this->needsVerifying);
        return true;
    }

    /** @return bool */
    protected function preparePhase3()
    {
        if ($this->needsVerifying != $this->tokenSerial) {
            // Token doesn't actually need verifying.
            return true;
        }

        $resultArray = $this->utils->enrollTokenStep2($this->tokenSerial, $this->mfaVerificationTan, "paper");
        if (!$resultArray["result"]["status"]) {
            msg_dialog::display(
                _("Internal error"),
                _("Failed to execute step 2 of token enrollment.") . "<br>" .
                $this->utils->pleaseTryAgainMsg()
            );

            return false;
        }

        return true;
    }

    public function getSetupCardIcon()
    {
        // Materialize Icon class.
        return "receipt";
    }

    public function getSetupCardTitle()
    {
        return _("TAN list");
    }

    public function getSetupCardDescription()
    {
        return _("Lists with transaction authentication number (TAN) are " .
            "printed out and stored in a secure location. A TAN is a one-time " .
            "password and a TAN from the list has to be entered on each login.");
    }

    public function getSetupCardButtonText()
    {
        return _("Add TAN list");
    }

    public function getSetupCardPriority()
    {
        return 2;
    }
}
