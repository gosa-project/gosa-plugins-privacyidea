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
    public $MAX_SETUP_PHASES = 2;
    public $TEMPLATE_PREFIX = "mfatype_registration/";

    /* VARIABLES WHICH WILL BE NEEDED ACROSS PHASES */
    /** @var string */
    private $tokenDescription;

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
            sprintf(_("User enrolled new token with type %s."), "registration")
        );

        // Refresh token countings cache.
        // TODO: Use countUserTokens() but refactor it, so that it doesn't require $tokens.
        //       and it should gather the tokens from endpoint /token/ itself.
        $this->utils->getTokensOfUser($this->mfaAccount->getUid(), $this->mfaAccount->userRealm);

        $mfaRecoveryKey = $requestAnswer["detail"]["registrationcode"];

        require_once(dirname(__FILE__) . "/../PDF/class_RecoveryKeyPDF.inc");

        $infoPage = new RecoveryKeyPDF();
        $infoPage->RecoveryKeyPDF(
            $this->mfaAccount->givenName,
            $this->mfaAccount->sn,
            $this->mfaAccount->getUid(),
            $mfaRecoveryKey
        );

        $pdfFileName = sprintf(_("recovery_key_%s.pdf"), $this->mfaAccount->getUid());
        $output = $infoPage->output('S', $pdfFileName, true);

        $this->smarty->assign("tokenDescription", $this->tokenDescription);
        $this->smarty->assign("mfaRecoveryKeyPDF_BASE64", base64_encode($output));
        $this->smarty->assign("mfaRecoveryKeyPDF_fileName", $pdfFileName);
        $this->smarty->assign("mfaRecoveryKey", $mfaRecoveryKey);

        // DEBUG DEBUG DEBUG
        // $pdfFilePath = sys_get_temp_dir() . PATH_SEPARATOR . $pdfFileName;
        // file_put_contents($pdfFilePath, $output);

        return true;
    }

    public function getSetupCardIcon()
    {
        // Materialize Icon class.
        return "password";
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
