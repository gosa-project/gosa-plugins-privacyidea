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

// This is a dirty hack for development purposes.
// Make sure to require 'privacyidea/privacyidea-php-client' via composer and
// then copy ./vendor/privacyidea/ to /usr/share/gosa/.
require_once("/usr/share/gosa/vendor/privacyidea/privacyidea-php-client/src/PrivacyIDEA.php");
require_once("/usr/share/gosa/vendor/privacyidea/privacyidea-php-client/src/PIResponse.php");
require_once("/usr/share/gosa/vendor/privacyidea/privacyidea-php-client/src/PILog.php");
require_once("/usr/share/gosa/vendor/privacyidea/privacyidea-php-client/src/PIChallenge.php");
require_once("/usr/share/gosa/vendor/privacyidea/privacyidea-php-client/src/PIBadRequestException.php");
require_once("/usr/share/gosa/vendor/privacyidea/privacyidea-php-client/src/AuthenticationStatus.php");

class PrivacyIdeaUtils implements PILog
{
    private PrivacyIDEA $pi;
    private config $config;
    private mfaAccount $mfaAccount;
    // TODO: Replace with GOsa's development mode.
    private bool $ENABLE_DEBUG_JS_CONSOLE = true;

    private string $GOSA_SETTINGS_GROUP;

    /**
     * @var string $authToken The authentication token will be used with every API call to PrivacyIDEA.
     * https://privacyidea.readthedocs.io/en/latest/modules/api/auth.html
     */
    // TODO: Make static or global or gosaSetting?
    private string $authToken;
    private string $authUsername = "";
    private string $authPassword = "";
    private string $authRealm    = "";

    /** @param mfaAccount $mfaAccount MFA Account class (likely parent) */
    public function __construct(&$mfaAccount)
    {
        assert(get_class($mfaAccount) == "mfaAccount");
        $this->mfaAccount = $mfaAccount;
        $this->config     = $mfaAccount->config;

        $this->GOSA_SETTINGS_GROUP = get_class($mfaAccount);

        // TODO: Load privacyIDEA server URL from settings.
        $pi_server_url = "http://localhost:5000/";

        $this->pi = new PrivacyIDEA($_SERVER['HTTP_USER_AGENT'], $pi_server_url);
        $this->pi->logger = $this;

        $this->authenticate();
    }

    public function piDebug($message)
    {
        echo "piDebug (" . date("H:i:s") . "): ";
        echo "$message";
        echo "<br><br>";
    }

    public function piError($message)
    {
        echo "piError (" . date("H:i:s") . "): ";
        echo "$message";
        echo "<br><br>";
    }

    /**
     * Requests a authentication token which will be stored in $this->authToken if successful.
     * @return bool Indicates if authentication was successful.
     */
    public function authenticate()
    {
        if (!empty($this->authToken)) {
            return true;
        }

        $this->pi->serviceAccountName  = $this->authUsername;
        $this->pi->serviceAccountPass  = $this->authPassword;
        $this->pi->serviceAccountRealm = $this->authRealm;

        $retString = "";
        try {
            $retString = $this->pi->getAuthToken();
        } catch (PIBadRequestException $e) {
            msg_dialog::display(
                _("Internal error"),
                _("Could not get authentification token from PrivacyIDEA backend server.") . "<br>" .
                $this->pleaseTryAgainMsg()
            );
        }

        if (!empty($retString)) {
            $this->authToken = $retString;
            return true;
        } else {
            $this->authToken = "";
        }

        return false;
    }

    /**
     * Enrolls a PI token with type 'paper'. in a 2-step-process. Step 1 (creation).
     * @param string $username
     * @param string $realm
     * @param string $description (Default is empty)
     * @return array array(...)
     */
    public function enrollPaperStep1($username, $realm, $description = "")
    {
        assert('string' === gettype($username));
        assert(!empty($username));
        assert('string' === gettype($realm));
        assert('string' === gettype($description));

        $params["user"]        = $username;
        $params["realm"]       = $realm;
        $params["genkey"]      = "1";
        $params["2stepinit"]   = "1";
        $params["type"]        = "paper";
        $params["description"] = $description;

        $requestAnswer = $this->piSendRequest($params, array(), "POST", "/token/init");

        if ($requestAnswer && $requestAnswer["result"]["status"]) {
            return array(
                "googleurl" => $requestAnswer["detail"]["googleurl"],
                "oathurl"   => $requestAnswer["detail"]["oathurl"],
                "otpkey"    => $requestAnswer["detail"]["otpkey"],
                "otps"      => $requestAnswer["detail"]["otps"],
                "serial"    => $requestAnswer["detail"]["serial"],
            );
        } else {
            return array();
        }
    }

    /**
     * Enrolls a PI token with type 'PAPER' in a 2-step-process. Step 2 (verify).
     * @param string $tokenSerial
     * @param string $tokenCode
     * @return array array(...)
     */
    public function enrollPaperStep2($tokenSerial, $tokenCode)
    {
        assert('string' === gettype($tokenSerial));
        assert('string' === gettype($tokenCode));
        assert(!empty($tokenSerial));
        assert(!empty($tokenCode));

        $params = array();
        $params["serial"] = $tokenSerial;
        $params["otpkey"] = $tokenCode;
        $params["type"]   = "paper";

        // Call /token/init again to enroll TOTP token.
        $resultArray = $this->piSendRequest($params, array(), "POST", "/token/init");

        return $resultArray;
    }

    /**
     * Enrolls a PI token with type 'TOTP' in a 2-step-process.
     * @param string $username
     * @param string $realm
     * @param string $description (Default is empty)
     * @return array array(...)
     */
    public function enrollTotpStep1($username, $realm, $description = "")
    {
        assert('string' === gettype($username));
        assert(!empty($username));
        assert('string' === gettype($realm));
        assert('string' === gettype($description));

        $params["user"]        = $username;
        $params["realm"]       = $realm;
        $params["genkey"]      = "1";
        $params["2stepinit"]   = "1";
        $params["type"]        = "totp";
        $params["description"] = $description;

        $requestAnswer = $this->piSendRequest($params, array(), "POST", "/token/init");

        if ($requestAnswer && $requestAnswer["result"]["status"]) {
            return array(
                "googleurl" => $requestAnswer["detail"]["googleurl"],
                "oathurl"   => $requestAnswer["detail"]["oathurl"],
                "otpkey"    => $requestAnswer["detail"]["otpkey"],
                "serial"    => $requestAnswer["detail"]["serial"],
            );
        } else {
            return array();
        }
    }

    /**
     * Enrolls a PI token with type 'TOTP' in a 2-step-process. Step 2 (verify).
     * @param string $tokenSerial
     * @param string $tokenCode
     * @return array array(...)
     */
    public function enrollTotpStep2($tokenSerial, $tokenCode)
    {
        assert('string' === gettype($tokenSerial));
        assert('string' === gettype($tokenCode));
        assert(!empty($tokenSerial));
        assert(!empty($tokenCode));

        $params = array();
        $params["serial"] = $tokenSerial;
        $params["otpkey"] = $tokenCode;
        $params["type"]   = "totp";

        // Call /token/init again to enroll TOTP token.
        $resultArray = $this->piSendRequest($params, array(), "POST", "/token/init");

        return $resultArray;
    }

    /**
     * @param array $tokens Array of tokens.
     * @return array Returns $tokens back with $token["MFATokenClass"] mapped to the specific MFAToken class.
     */
    public function mapTokenTypes($tokens)
    {
        if (empty($tokens)) {
            // Can't map tokens, if there are none.
            return $tokens;
        }

        $allTokenTypes = array_keys($this->mfaAccount->mfaTokens);
        foreach ($tokens as $key => $token) {
            $piTokenType = strtolower($token["tokentype"]);

            if (!in_array($piTokenType, $allTokenTypes)) {
                // Token is not supported by this plugin, remove it?
                // TODO: Do a log here!
                // See class_MFAUnknownToken.inc.
                unset($tokens[$key]);
            } else {
                $token = $this->mfaAccount->mfaTokens[$piTokenType];
                $tokens[$key]["MFATokenClass"] = $token;
            }
        }

        return $tokens;
    }

    /**
     * @param array $tokens
     * @return array Sanitized tokens which are ready to be given to Smarty templates (UI).
     */
    public function prepareTokensForUI($tokens)
    {
        foreach ($tokens as $key => $token) {
            $valid = true;

            $mustBeValid = array('serial', 'active', 'description','revoked',
                'maxfail', 'failcount', 'tokentype', 'user_realm', 'rollout_state');

            foreach ($mustBeValid as $value) {
                if (!isset($token[$value])) {
                    // TODO: Log here that $value is not set in $token.
                    $valid = false;
                    break;
                }
            }

            if (isset($token["rollout_state"]) && $token["rollout_state"] == "verify") {
                $tokens[$key]["status"] = _("Not verified");
            } elseif (isset($token["locked"]) && $token["locked"]) {
                $tokens[$key]["status"] = _("Locked");
            } elseif ($token["active"]) {
                $tokens[$key]["status"] = _("Active");
            } else {
                $tokens[$key]["status"] = _("Deactivated");
            }

            if (!$valid) {
                unset($tokens[$key]);
            }
        }

        return $tokens;
    }

    /** @return string Token serial */
    public function getPOSTTokenSerial()
    {
        $tokenSerial = null;
        if (isset($_POST["tokenSerial"])) {
            // TODO: Make sure its a real token (sanitize it).
            $tokenSerial = $_POST["tokenSerial"];
        }

        return $tokenSerial;
    }

    /**
     * @param string $user Username/UID of user to request a list of tokens for.
     * @param string $userRealm
     * @param string $tokenSerial If specified, only ONE token will be returned (if available).
     * @return array Array of PI tokens (can be empty).
     */
    public function getTokensOfUser($user, $userRealm, $tokenSerial = null)
    {
        if (!$this->authenticate()) {
            return array();
        }

        $params = array(
            "user"   => $user,
            "realm"  => $userRealm,
            "sortby" => "count", // TODO: Sort somehow by last use.
        );

        if (isset($tokenSerial) && !empty($tokenSerial)) {
            $tokenSerial = trim($tokenSerial);
            $params["serial"] = $tokenSerial;
        }

        // $endpoint must be written like this or else we get a HTTP redirect.
        $requestAnswer = $this->piSendRequest($params, array(), "GET", $endpoint = "/token/");

        // Extract token information now.
        if ($requestAnswer["result"]["status"]) {
            $returnArray = $requestAnswer["result"]["value"]["tokens"];
            $returnArray = $this->mapTokenTypes($returnArray);
            $returnArray = $this->prepareTokensForUI($returnArray);

            return $returnArray;
        }

        return array();
    }

    /**
     * Send a request to an endpoint with the specified parameters and headers.
     *
     * @param array $params request parameters
     * @param array $headers headers fields (Authentication field gets added automatically)
     * @param string $httpMethod GET or POST
     * @param string $endpoint endpoint of the privacyIDEA API (e.g. /validate/check)
     * @return ?array returns json-decoded array with the response from server
     */
    public function piSendRequest(array $params, array $headers, $httpMethod, $endpoint)
    {
        if (!$this->authenticate()) {
            return array(
                "result" => array(
                    "status" => false
                )
            );
        }

        $authTokenHeader = array("authorization:" . $this->authToken);
        if (!empty($headers)) {
            $fullHeaders = array_merge($headers, $authTokenHeader);
        } else {
            $fullHeaders = $authTokenHeader;
        }

        try {
            $returnedJSON = $this->pi->sendRequest($params, $fullHeaders, $httpMethod, $endpoint);
        } catch (PIBadRequestException $e) {
            msg_dialog::display(
                _("Internal error"),
                _("Could not connect to PrivacyIDEA backend server.") . "<br>" .
                $this->pleaseTryAgainMsg()
            );

            return array(
                "result" => array(
                    "status" => false
                )
            );
        }

        $returnArray = json_decode($returnedJSON, true, $flags = JSON_PRETTY_PRINT);

        if (isset($returnArray["result"]["error"]["code"])) {
            if ($returnArray["result"]["error"]["code"] == 4305) {
                // This is the code for "Authentication failure. Your token has expired: Signature has expired"
                $this->authToken = "";
                $this->authenticate();
                // Don't bother to check if we loop recursively. Just do it and hope for the best.
                $this->piSendRequest($params, $headers, $httpMethod, $endpoint);
            }
        }

        return $returnArray;
    }

    /** @return string Always returns translated string (see source code for actual message). */
    public function pleaseTryAgainMsg()
    {
        return _("Please try logging out and logging in again and contact your " .
                 "system administrator, if the problem persists. Thank you.");
    }

    /**
     * @param string|array $data Data to show inside of the Javascript console (in the client browser).
     */
    public function debugToJsConsole($data)
    {
        // Don't print anything to js console, if ENABLE_DEBUG_JS_CONSOLE isn't 'true'.
        if ($this->ENABLE_DEBUG_JS_CONSOLE !== true) {
            return;
        }

        if (is_array($data)) {
            $output = implode('', $data);
        }

        echo "<script>console.log('" . addslashes($data) . "');</script>";
    }

    /**
     * @param string $key
     * @return string Value from GOsa's config system.
     */
    public function getConfigStrValue($key)
    {
        return $this->getConfigStrValueInternal($key, true);
    }

    private function getConfigStrValueInternal($key, $debug_to_jsconsole = true)
    {
        assert(is_string($key));

        // Get config value from LDAP-Tree, gosa config file (gosa.conf) or
        // defaults from class_schoolmgr.inc
        $ret = $this->config->get_cfg_value($this->GOSA_SETTINGS_GROUP, $key);

        if ($debug_to_jsconsole) {
            // Debug property to js console (if debugging is enabled!).
            $debug_str = "Got string property '" . str_pad($key . "':", 58) . "'" . $ret . "'";
            $this->debugToJsConsole($debug_str);
        }

        return $ret;
    }

    /**
     * @param string $key
     * @return bool Value from GOsa's config system.
     */
    public function getConfigBoolValue($key)
    {
        assert(is_string($key));

        // Get config value, but pause debugging output.
        $conf_value = strtolower($this->getConfigStrValueInternal($key, false));

        $ret = false;

        // Only return TRUE if value is literally "true"…
        if ($conf_value === "true") {
            $ret = true;
        }

        // Debug property to js console (if debugging is enabled!).
        $ret_str = $ret ? "✅" : "❌";
        $debug_str = "Got bool property '" . str_pad($key . "':", 60) . "'" . $ret_str . "'";
        $this->debugToJsConsole($debug_str);

        return $ret;
    }
}
