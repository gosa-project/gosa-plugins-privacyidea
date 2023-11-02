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

class MFATotpToken implements MFAToken
{
    private string $renderResult = "";
    private string $TEMPLATE_PREFIX = "mfatype_totp/";
    private PrivacyIdeaUtils $utils;
    private Smarty $smarty;
    private config $config;
    private mfaAccount $mfaAccount;
    /** @var ldapMultiplexer|LDAP */
    private $ldap;

    /** @var int Indicates how many phases the token setup has. */
    public const MAX_SETUP_PHASES = 4;

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
        $currentPhase = $this->getCurrentPhaseNumber();
        $nextPhaseNumber = $this->getNextPhaseNumber();

        $success = true;
        switch ($nextPhaseNumber) {
            case 1:
                # PHASE 1 #
                $success = $success && $this->preparePhase1();
                break;

            case 2:
                # PHASE 2 #
                $success = $success && $this->preparePhase2();
                break;

            case 3:
                # PHASE 3 #
                $success = $success && $this->preparePhase3();
                break;

            default:
                # PHASE 0 #
                $success = $success && $this->preparePhase0();
                break;
        }

        if (!$success) {
            // Oh, something did go wrong. Do not show next phase.
            $nextPhaseNumber = $currentPhase;
        }

        $templateFileName = "phase" . str_pad($nextPhaseNumber, 2, "0", STR_PAD_LEFT) . ".tpl";
        $this->renderResult = $this->smarty->fetch(
            get_template_path($this->TEMPLATE_PREFIX . $templateFileName, true, dirname(__FILE__) . "/..")
        );

        return true;
    }

    /** @return bool */
    private function preparePhase0()
    {
        return true;
    }

    /** @return bool */
    private function preparePhase1()
    {
        $tokenDescription = "";
        if (isset($_POST["tokenDescription"])) {
            $tokenDescription = $_POST["tokenDescription"];
            $tokenDescription = trim($tokenDescription);
        }

        // Actually call utils to enroll TOTP token (in a 2 step process).
        $tokenInfo = $this->utils->enrollTotpStep1(
            $this->mfaAccount->getUid(),
            mfaAccount::PI_USER_REALM,
            $tokenDescription
        );

        if (isset($tokenInfo) && !empty($tokenInfo)) {
            // TODO: Which should we use as default?
            $type = "googleurl"; // This works for me.
            // $type = "oathurl";
            // $type = "otpkey";

            // TODO: Replace 'issuer=privacyIDEA' with something else. :)
            // This will be directly visible in the TOTP app.

            $qrImage      = $tokenInfo[$type]["img"];
            $qrImageValue = $tokenInfo[$type]["value"];
            $tokenSerial  = $tokenInfo["serial"];

            $this->smarty->assign("qrImage", $qrImage);
            $this->smarty->assign("qrImageValue", $qrImageValue);
            $this->smarty->assign("tokenSerial", $tokenSerial);
            $this->smarty->assign("tokenDescription", $tokenDescription);
        } else {
            msg_dialog::display(
                _("Internal error"),
                _("Couldn't enroll token!") . "<br>" .
                $this->utils->pleaseTryAgainMsg()
            );

            return false;
        }

        // $this->utils->piSendRequest(array(), array(), "DELETE", "/token/" . $tokenInfo["serial"]);

        return true;
    }

    /** @return bool */
    private function preparePhase2()
    {
        $tokenSerial = "";
        if (isset($_POST["tokenSerial"])) {
            // TODO: Make sure its a real token (sanitize it).
            $tokenSerial = $_POST["tokenSerial"];
            $this->smarty->assign("tokenSerial", $tokenSerial);
        }

        return true;
    }

    /** @return bool */
    private function preparePhase3()
    {
        $mfaTOTPCode = "";
        if (isset($_POST["mfaTOTPCode"])) {
            $mfaTOTPCode = $_POST["mfaTOTPCode"];
            // Remove everything which isn't a digit.
            $mfaTOTPCode = preg_replace('~\D~', '', $mfaTOTPCode);

            if (!is_numeric($mfaTOTPCode)) {
                // Okay, $mfaTOTPCode is definitely borked.
                msg_dialog::display(
                    _("Internal error"),
                    _("Given TOTP code is malformed and could not be read.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg()
                );

                return;
            }
        }

        $tokenSerial = "";
        if (isset($_POST["tokenSerial"])) {
            // TODO: Make sure its a real token (sanitize it).
            $tokenSerial = $_POST["tokenSerial"];
        }

        $resultArray = $this->utils->enrollTotpStep2($tokenSerial, $mfaTOTPCode);
        if ($resultArray["result"]["status"]) {
            return true;
        } else {
            return false;
        }
    }

    public function getRenderSetupResult()
    {
        return $this->renderResult;
    }

    /** @return int Sanitized and cleaned $_POST['current_phase']. */
    public function getCurrentPhaseNumber()
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

        return $currentPhase;
    }

    /** @return int Next phase. */
    public function getNextPhaseNumber()
    {
        $currentPhase = $this->getCurrentPhaseNumber();

        // Special case for TOTP: Allow regenerating QR code.
        if (isset($_POST["mfa_generate_totp_secret"])) {
            // Make sure to provide needed variables again.
            if (isset($_POST["tokenDescription"])) {
                $this->smarty->assign("tokenDescription", $_POST["tokenDescription"]);
            }
            if (isset($_POST["tokenSerial"])) {
                // Delete old token.
                $this->utils->piSendRequest(array(), array(), "DELETE", "/token/" . $_POST["tokenSerial"]);
            }

            return $currentPhase;
        }

        return $currentPhase + 1;
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
