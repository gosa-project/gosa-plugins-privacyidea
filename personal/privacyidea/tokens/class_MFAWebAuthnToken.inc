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
    public $MAX_SETUP_PHASES = 3;
    public $TEMPLATE_PREFIX = "mfatype_webauthn/";

    /* VARIABLES WHICH WILL BE NEEDED ACROSS PHASES */
    /** @var string */
    private $tokenDescription;
    /** @var string */
    private $tokenSerial;
    /** @var array webAuthnRegisterResponse Object */
    private $mfaWebAuthnRegisterResponse;

    /** @return bool */
    protected function evaluatePhase0()
    {
        $this->tokenDescription = "";
        if (isset($_POST["tokenDescription"])) {
            $this->tokenDescription = $_POST["tokenDescription"];
            $this->tokenDescription = $this->filterTokenDescription($this->tokenDescription);
        } else {
            // Empty descriptions are okay.
        }

        return true;
    }

    /** @return bool */
    protected function evaluatePhase1()
    {
        $tokenSerial = $this->utils->getPOSTTokenSerial();
        if (!isset($tokenSerial) || empty($tokenSerial)) {
            msg_dialog::display(
                _("Internal error"),
                _("Token serial wasn't given or was given malformed.") . "<br>" .
                $this->utils->pleaseTryAgainMsg(),
                ERROR_DIALOG
            );

            return false;
        }
        $this->tokenSerial = $tokenSerial;

        $this->mfaWebAuthnRegisterResponse = "";
        if (isset($_POST["mfaWebAuthnRegisterResponse"])) {
            $mfaWebAuthnRegisterResponse = $_POST["mfaWebAuthnRegisterResponse"];
            $this->mfaWebAuthnRegisterResponse = json_decode($mfaWebAuthnRegisterResponse, true);
        } else {
            // TODO: Error dialog here.
            return false;
        }

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
        $limitReachedMessage = $this->checkUserTokenLimit("webauthn");
        if (!empty($limitReachedMessage)) {
            msg_dialog::display(
                _("Internal error"),
                $limitReachedMessage . "<br>" .
                $this->utils->pleaseTryAgainMsg(),
                ERROR_DIALOG
            );
            return false;
        }

        return true;
    }

    /** @return bool */
    protected function preparePhase1()
    {
        $this->smarty = get_smarty();
        $limitReachedMessage = $this->checkUserTokenLimit("webauthn");
        if (!empty($limitReachedMessage)) {
            msg_dialog::display(
                _("Internal error"),
                $limitReachedMessage . "<br>" .
                $this->utils->pleaseTryAgainMsg(),
                ERROR_DIALOG
            );
            return false;
        }

        $token = $this->utils->enrollTokenStep1(
            $this->mfaAccount->getUid(),
            $this->mfaAccount->userRealm,
            "webauthn",
            $this->tokenDescription
        );

        if (!isset($token) || empty($token)) {
            msg_dialog::display(
                _("Internal error"),
                _("Couldn't enroll token!") . "<br>" .
                $this->utils->pleaseTryAgainMsg(),
                ERROR_DIALOG
            );

            return false;
        }

        if (!isset($token["detail"]["webAuthnRegisterRequest"])) {
            msg_dialog::display(
                _("Internal error"),
                _("privacyIDEA server gave invalid/malformed webAuthnRegisterRequest data.") . "<br>" .
                $this->utils->pleaseTryAgainMsg(),
                ERROR_DIALOG
            );

            return false;
        } else {
            // TODO: Make a few sanity checks here.
        }

        $tokenSerial = $token["detail"]["serial"];
        $webAuthnRegisterRequest = $token["detail"]["webAuthnRegisterRequest"];

        // Please see: https://privacyidea.readthedocs.io/en/latest/modules/lib/tokentypes/webauthn.html
        // before proceeding. :)

        $this->smarty->assign(
            "webAuthnRegisterRequestJSON",
            addslashes(json_encode($webAuthnRegisterRequest))
        );
        $this->smarty->assign("tokenSerial", $tokenSerial);
        $this->smarty->assign("tokenDescription", $this->tokenDescription);

        return true;
    }

    /** @return bool */
    protected function preparePhase2()
    {
        // We just created a token in the phase before this one.
        // So it may happen that we reach the token limit by now..
        // But we don't want to create a new token here in phase 2 but we want
        // to verify the existing one.
        // TODO: Find a good solution to this problem.
        // $limitReachedMessage = $this->checkUserTokenLimit("webauthn");
        // if (!empty($limitReachedMessage)) {
        //     msg_dialog::display(
        //         _("Internal error"),
        //         $limitReachedMessage . "<br>" .
        //         $this->utils->pleaseTryAgainMsg()
        //     );
        //     return false;
        // }

        $this->smarty->assign("tokenDescription", $this->tokenDescription);

        if (isset($this->mfaWebAuthnRegisterResponse)) {
            $resultArray = $this->enrollWebAuthnTokenStep2();
        }

        if (!$resultArray["result"]["status"]) {
            if (isset($resultArray["result"]["error"]["message"])) {
                msg_dialog::display(
                    _("Internal error"),
                    _("Failed to execute step 2 of token enrollment.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg() . "<br>" .
                    sprintf(_("privacyIDEA server error message: %s"), $resultArray["result"]["error"]["message"]),
                    ERROR_DIALOG
                );
            } else {
                msg_dialog::display(
                    _("Internal error"),
                    _("Failed to execute step 2 of token enrollment.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg(),
                    ERROR_DIALOG
                );
            }
            return false;
        }

        return true;
    }

    /**
     * Enrolls a PI token with type '$tokentype' in a 2-step-process. Step 2 (verify).
     * @return array Token array(...)
     */
    public function enrollWebAuthnTokenStep2()
    {
        $params = array();
        $headers = array();

        $params["serial"] = $this->tokenSerial;
        $params["type"] = "webauthn";
        $params = array_merge($params, $this->mfaWebAuthnRegisterResponse);

        $tokenOrigin = $this->utils->getConfigStrValue("piTokenOrigin");
        if (isset($tokenOrigin) && !empty($tokenOrigin)) {
            $headers = array("Origin: " . $tokenOrigin);
        }

        // Call /token/init again to enroll/verify WebAuthn token.
        $resultArray = $this->utils->piSendRequest($params, $headers, "POST", "/token/init");

        // Refresh token countings cache.
        // TODO: Use countUserTokens() but refactor it, so that it doesn't require $tokens.
        //       and it should gather the tokens from endpoint /token/ itself.
        $this->utils->getTokensOfUser($this->mfaAccount->getUid(), $this->mfaAccount->userRealm);

        return $resultArray;
    }

    public function getSetupCardTitle()
    {
        return _("Security key");
    }

    public function getSetupCardDescription()
    {
        return _("FIDO2-compatible hardware security keys are e.g. " .
            "connected via USB or NFC and authentication is started " .
            "using a device-specific mechanism, e.g. touching a sensor.");
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
