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

class MFAPaperToken implements MFAToken
{
    private string $renderResult = "";
    private string $TEMPLATE_PREFIX = "mfatype_paper/";
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
        $phaseNumber = $this->getNextPhaseNumber();
        $templateFileName = "phase" . str_pad($phaseNumber, 2, "0", STR_PAD_LEFT) . ".tpl";

        $success = true;
        switch ($phaseNumber) {
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

        $tokenInfo = $this->utils->enrollPaperStep1(
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

            // $qrImage      = $tokenInfo[$type]["img"];
            // $qrImageValue = $tokenInfo[$type]["value"];
            $tokenSerial  = $tokenInfo["serial"];
            $tokenOTPs    = $tokenInfo["otps"];
            $tokenConfTAN = $tokenInfo["otps"][0];

            // Cut first item. (It is used as a confirmation TAN.)
            $tokenOTPs = array_splice($tokenOTPs, 1, count($tokenOTPs));

            $this->smarty->assign("mfaTanJSON", addslashes(json_encode($tokenOTPs)));
            $this->smarty->assign("mfaConfirmationTan", $tokenConfTAN);
            $this->smarty->assign("tokenSerial", $tokenSerial);
            $this->smarty->assign("tokenDescription", $tokenDescription);
        } else {
            msg_dialog::display(
                _("Internal error"),
                _("Couldn't enroll token!") . "<br>" .
                $this->utils->pleaseTryAgainMsg()
            );
        }

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
        $mfaVerificationTan = "";
        if (isset($_POST["mfaVerificationTan"])) {
            $mfaVerificationTan = $_POST["mfaVerificationTan"];
            // Remove everything which isn't a digit.
            $mfaVerificationTan = preg_replace('~\D~', '', $mfaVerificationTan);

            if (!is_numeric($mfaVerificationTan)) {
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
        } else {
            return false;
        }

        $resultArray = $this->utils->enrollPaperStep2($tokenSerial, $mfaVerificationTan);
        if ($resultArray["result"]["status"]) {
            return true;
        } else {
            return false;
        }

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
        return "receipt";
    }

    public function getSetupCardTitle()
    {
        return _("TAN-Liste");
    }

    public function getSetupCardDescription()
    {
        return _("TAN-Listen werden ausgedruckt an einem sicheren Ort gelagert. " .
            "Sie enthalten Einmalpasswörter, von denen für jeden Anmeldevorgang " .
            "eines angegeben werden muss.");
    }

    public function getSetupCardButtonText()
    {
        return _("TAN-Liste hinzufügen");
    }

    public function getSetupCardPriority()
    {
        return 2;
    }
}
