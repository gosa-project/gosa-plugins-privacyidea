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
// Make sure to require 'privacyidea/privacyidea-php-client' and 'setasign/tfpdf' via composer in /usr/share/gosa/ and
// locally. Add excludeFromAutoload files where necessary
require_once("/usr/share/gosa/vendor/setasign/tfpdf/tfpdf.php");
require_once("/usr/share/gosa/vendor/setasign/tfpdf/font/unifont/ttfonts.php");
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
    // WARNING: Setting ENABLE_DEBUG=true leaks PI serviceAccount creds to the client (via JS console).
    private bool $ENABLE_DEBUG = false;

    /** @var array Token counts array ('all' => X, 'paper' => X, 'totp' => X...) */
    public $tokenCounts;

    /** @var string Indicates if the communication with PI had some errors. */
    public $hasPiErrors;

    /**
     * @var string $authToken The authentication token will be used with every API call to privacyIDEA.
     * https://privacyidea.readthedocs.io/en/latest/modules/api/auth.html
     */
    private $authToken;
    /** @var string */
    private $authUsername;
    /** @var string */
    private $authPassword;
    /** @var string */
    private $authRealm;

    /** @var string Version number of privacyIDEA server - gets set via retrievePiServerVersion(). */
    public $piServerVersion;

    /** @param mfaAccount $mfaAccount MFA Account class (likely parent) */
    public function __construct(&$mfaAccount)
    {
        assert(get_class($mfaAccount) == "mfaAccount");
        $this->mfaAccount = $mfaAccount;
        $this->config     = $mfaAccount->config;

        $piServerUrl = $this->getConfigStrValue("piServer");
        $this->authUsername = $this->getConfigStrValue("piServiceAccount");
        $this->authPassword = $this->getConfigStrValue("piServicePass");
        $this->authRealm    = $this->getConfigStrValue("piServiceRealm");

        $this->pi = new PrivacyIDEA($_SERVER['HTTP_USER_AGENT'], $piServerUrl);
        $this->pi->logger = $this; // Needs $this->piDebug() and $this->piError().
    }

    public function piDebug($message)
    {
        if ($this->ENABLE_DEBUG) {
            echo "piDebug (" . date("H:i:s") . "): ";
            echo strip_tags("$message");
            echo "<br><br>";
        }
    }

    public function piError($message)
    {
        if ($this->ENABLE_DEBUG) {
            echo "piError (" . date("H:i:s") . "): ";
            echo strip_tags("$message");
            echo "<br><br>";
        }

        $this->hasPiErrors = true;
    }

    /**
     * Find key recursivly in array (Adapted from GOsa core)
     *
     * @param array $haystack the array which will be searched
     * @param string $needle search string
     * @return mixed result of key search
     */
    public function findRecursive($haystack, $needle): mixed
    {
        assert(is_array($haystack));
        assert(is_string($needle));

        $iterator = new RecursiveArrayIterator($haystack);
        $recursive = new RecursiveIteratorIterator(
            $iterator,
            RecursiveIteratorIterator::SELF_FIRST
        );
        foreach ($recursive as $key => $value) {
            if ($key === $needle) {
                return $value;
            }
        }

        return false;
    }

    /**
     * Sets $this->piServerVersion to version number of privacyIDEA server.
     */
    public function retrievePiServerVersion()
    {
        // ASSUME WE ARE ALREADY AUTHENTICATED.
        assert(!empty($this->authToken));

        $resultArray = $this->piSendRequest(array(), array(), "GET", "/system/versionnumber");

        if (!isset($resultArray["versionnumber"]) || empty($resultArray["versionnumber"])) {
            new log(
                "modify",
                "users/" . get_class($this),
                $this->mfaAccount->dn,
                array(),
                _("Couldn't retrieve version number of privacyIDEA server. Defaulting to legacy API scheme.")
            );
            return;
        }

        $this->piServerVersion = $resultArray["versionnumber"];
    }

    /**
     * Wrapper for version_compare() to check $version against $this->piServerVersion.
     * @param string $version Always second parameter to version_compare().
     * @param string|null $operator [optional] If you specify the second optional operator argument,
     *     you can test for a particular relationship. For possible operators see version_compare().
     * @return int|bool|null See version_compare(). null means $piServerVersion couldn't be retrieved. Please fall back
     *     to legacy API scheme in this case.
     */
    public function piServerVersionCompare($version, $operator = null)
    {
        assert(is_string($version) && !empty($version));
        if (!isset($this->piServerVersion) || empty($this->piServerVersion)) {
            return null;
        }
        return version_compare($this->piServerVersion, $version, $operator);
    }

    /**
     * This function was adapted from privacyidea-php-client.
     * This implementation checks if serviceAccount is an admin.
     *
     * Retrieves an auth token from the server using the service account.
     * An auth token is required for some requests to privacyIDEA.
     *
     * @return string the auth token or empty string if the response did not
     *                contain a token or no service account is configured.
     * @throws PIBadRequestException if an error occurs during the request
     */
    public function getAuthToken()
    {
        if (!$this->pi->serviceAccountAvailable()) {
            $this->pi->errorLog("Cannot retrieve auth token without service account!");
            return "";
        }

        $params = array(
            "username" => $this->pi->serviceAccountName,
            "password" => $this->pi->serviceAccountPass
        );

        if ($this->pi->serviceAccountRealm != null && $this->pi->serviceAccountRealm != "") {
            $params["realm"] = $this->pi->serviceAccountRealm;
        }

        $response = json_decode($this->pi->sendRequest($params, array(''), 'POST', '/auth'), true);

        if (isset($response['result']['value']['token']) && !empty($response['result']['value']['token'])) {
            // Ensure we have an admin account
            if ($this->findRecursive((array)$response, "role") != 'admin') {
                $this->pi->debugLog("auth token was of a user without admin role.");
                return "";
            }

            return $response['result']['value']["token"];
        }

        $this->pi->debugLog("/auth response did not contain a auth token.");
        return "";
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
            // TODO: Use privacyidea-php-client's implementation instead of our own, if they fixed check if
            // serviceAccount is admin upstream.
            $retString = $this->getAuthToken();
        } catch (PIBadRequestException $e) {
            msg_dialog::display(
                _("Internal error"),
                _("Could not get authentification token from privacyIDEA backend server.") . "<br>" .
                $this->pleaseTryAgainMsg()
            );
            $this->authToken = "";
            $this->hasPiErrors = true;
            return false;
        }

        if (!empty($retString)) {
            $this->authToken = $retString;
            $this->retrievePiServerVersion();

            return true;
        } else {
            msg_dialog::display(
                _("Internal Error"),
                _("Could not get authentification token from privacyIDEA backend server.") . "<br>" .
                _("Dear admin: Please ensure that the privacyIDEA service credentials are set correctly.") . "<br>" .
                $this->pleaseTryAgainMsg()
            );
            $this->authToken = "";
            $this->hasPiErrors = true;
        }

        return false;
    }

    /**
     * Counts tokens and caches them in $this->tokenCount.
     * @param array $tokens Tokens to count.
     */
    public function countUserTokens($tokens)
    {
        $allTokenTypes = array_keys($this->mfaAccount->mfaTokens);

        // Get limits
        $tokenLimits = array();
        $tokenLimits["paper"]        = $this->getConfigIntValue("piTokenLimitPaper");
        $tokenLimits["totp"]         = $this->getConfigIntValue("piTokenLimitTotp");
        $tokenLimits["webauthn"]     = $this->getConfigIntValue("piTokenLimitWebAuthn");
        $tokenLimits["registration"] = $this->getConfigIntValue("piTokenLimitRegistration");

        // All tokens. Including deactivated and revoked ones...
        $tokenCountAll = 0;
        $this->tokenCounts = array();

        foreach ($tokens as $key => $token) {
            $tokenType = $token["tokentype"];

            if (!in_array($tokenType, $allTokenTypes)) {
                // We do not support this tokenType.
                continue;
            }

            // We support tokenType, but we forgot to add it in tokenLimits.
            assert(in_array($tokenType, array_keys($tokenLimits)));

            if (!in_array($tokenType, array_keys($this->tokenCounts))) {
                $this->tokenCounts[$tokenType] = 0;
            }

            $this->tokenCounts[$tokenType]++;

            if ($tokenLimits[$tokenType] != 0) {
                // Only increase if this specific token type has a limit at all.
                $tokenCountAll++;
            }
        }

        $this->tokenCounts["all"] = $tokenCountAll;
    }

    /**
     * Enrolls a PI token with type '$tokentype'. in a 2-step-process. Step 1 (creation).
     * @param string $username
     * @param string $realm
     * @param string $tokenType Either one of: ('paper', 'totp', 'webauthn')
     * @param string $description (Default is empty)
     * @return array Token array(...)
     */
    public function enrollTokenStep1($username, $realm, $tokenType, $description = "")
    {
        assert('string' === gettype($username));
        assert(!empty($username));
        assert('string' === gettype($realm));
        assert('string' === gettype($description));
        assert($tokenType == "paper" || $tokenType == "totp" || $tokenType == "webauthn");

        if (!$this->authenticate()) {
            return array();
        }

        if ($tokenType != "webauthn") {
            $params["genkey"] = true;
        }

        $params["user"]        = $username;
        $params["realm"]       = $realm;
        $params["type"]        = $tokenType;
        $params["description"] = $description;

        $requestAnswer = $this->piSendRequest($params, array(), "POST", "/token/init");

        if (!$requestAnswer || !$requestAnswer["result"]["status"]) {
            if ($requestAnswer["result"]["error"]["code"] == 303) {
                // ERROR 303 means "Missing enrollment policy for WebauthnToken: webauthn_relying_party_id"
                msg_dialog::display(
                    _("Internal error"),
                    _("Could not execute step 1 of WebAuthn token enrollment: ") . "<br>" .
                    $requestAnswer["result"]["error"]["message"] . "<br><br>" .
                    $this->pleaseTryAgainMsg()
                );
            }

            return array();
        }

        new log(
            "modify",
            "users/" . get_class($this),
            $this->mfaAccount->dn,
            array(),
            sprintf("A new token of type '%s' has been enrolled for user '%s'.", $tokenType, $this->mfaAccount->getUid())
        );

        // Refresh token countings cache.
        // TODO: Use countUserTokens() but refactor it, so that it doesn't require $tokens.
        //       and it should gather the tokens from endpoint /token/ itself.
        $this->getTokensOfUser($this->mfaAccount->getUid(), $this->mfaAccount->userRealm);

        return $requestAnswer;
    }

    /**
     * Enrolls a PI token with type '$tokentype' in a 2-step-process. Step 2 (verify).
     * @param string $tokenSerial
     * @param string $tokenCode
     * @param string $tokenType Either one of: ('paper', 'totp')
     * @return array Token array(...)
     */
    public function enrollTokenStep2($tokenSerial, $tokenCode, $tokenType)
    {
        assert('string' === gettype($tokenSerial));
        assert('string' === gettype($tokenCode));
        assert(!empty($tokenSerial));
        assert(!empty($tokenCode));
        assert($tokenType == "paper" || $tokenType == "totp");

        if (!$this->authenticate()) {
            return array("results" => array("status" => false));
        }

        $params = array();

        // TODO: privacyIDEA could return different rollout_states in Step 1 (depending on different policies and PI
        // server versions). We need to either give $params["otpkey"] or $params["verify"] based on that.
        // $rollout_state = "verify";
        // if ($rollout_state == "clientwait") {
            // $params["otpkey"] = $tokenCode;
        // } elseif ($rollout_state == "verify") {
            // $params["verify"] = $tokenCode;
        // }

        // We only support 'verify_enrollment' policy for totp and hotp (paper).
        $params["verify"] = $tokenCode;
        $params["serial"] = $tokenSerial;
        $params["type"]   = $tokenType;

        // Call /token/init again to enroll/verify token.
        $resultArray = $this->piSendRequest($params, array(), "POST", "/token/init");

        new log(
            "modify",
            "users/" . get_class($this),
            $this->mfaAccount->dn,
            array(),
            sprintf("New token of type '%s' has been verified for user '%s'.", $tokenType, $this->mfaAccount->getUid())
        );

        // Refresh token countings cache.
        // TODO: Use countUserTokens() but refactor it, so that it doesn't require $tokens.
        //       and it should gather the tokens from endpoint /token/ itself.
        $this->getTokensOfUser($this->mfaAccount->getUid(), $this->mfaAccount->userRealm);

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
                new log(
                    "modify",
                    "users/" . get_class($this),
                    $this->mfaAccount->dn,
                    array(),
                    sprintf("WARNING: Encountered unsupported token type '%s' with serial '%s'.", $piTokenType, $token['serial'])
                );

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

            if (!in_array($token['tokentype'], array_keys($this->mfaAccount->mfaTokens))) {
                // We do not support this token. Don't show it.
                unset($tokens[$key]);
                continue;
            }

            $mustBeValid = array('serial', 'active', 'description','revoked',
                'maxfail', 'failcount', 'tokentype', 'user_realm', 'rollout_state');

            foreach ($mustBeValid as $value) {
                if (!isset($token[$value])) {
                    if (isset($token['serial'])) {
                        new log(
                            "modify",
                            "users/" . get_class($this),
                            $this->mfaAccount->dn,
                            array(),
                            sprintf(
                                "privacyIDEA sent malformed data, '%s' is missing from token with serial %s.",
                                $value,
                                $token['serial']
                            ),
                        );
                    }

                    $valid = false;
                    break;
                }
            }

            if (isset($token["rollout_state"]) && in_array($token["rollout_state"], ["verify", "clientwait"])) {
                $tokens[$key]["status"] = _("Not verified");
            } elseif (isset($token["locked"]) && $token["locked"]) {
                $tokens[$key]["status"] = _("Locked");
            } elseif ($token["active"]) {
                $tokens[$key]["status"] = _("Active");
            } else {
                $tokens[$key]["status"] = _("Deactivated");
            }

            // Convert PI date to browser compatible datetime.
            // See: https://developer.mozilla.org/en-US/docs/Web/HTML/Date_and_time_formats#local_date_and_time_strings
            // Can be used by the browser to convert to localized datetime.
            if (isset($token['info']['last_auth'])) {
                $piString = $token['info']['last_auth'];
                $dateTimeObject = new DateTime($piString, new DateTimeZone('UTC'));

                $dateTimeString = $dateTimeObject->format("Y-m-d\TH:i:s");
                $tokens[$key]['info']['last_auth'] = $dateTimeString;
            }

            if (!$valid) {
                unset($tokens[$key]);
            }
        }

        return $tokens;
    }

    /**
      * @param  array tokens obtained via getTokensOfUser()
      * @return array serials of active tokens
      */
    public function getActiveTokenSerials($tokens)
    {
        return array_reduce($tokens, function ($result, $token) {
            if (
                !(isset($token["rollout_state"]) && $token["rollout_state"] == "verify") &&
                !(isset($token["locked"]) && $token["locked"]) && $token["active"]
            ) {
                $result[] = $token["serial"];
            }
            return $result;
        }, array());
    }

    /**
     * @param string $tokenSerial serial
     * @return bool Whether token is valid
     */
    public function isTokenSerialValid($tokenSerial)
    {
        // TODO: Make sure its a real token (sanitize it) (see prepareTokensForUI).
        return !empty($tokenSerial) && is_string($tokenSerial);
    }

    /** @return string Token serial */
    public function getPOSTTokenSerial()
    {
        $tokenSerial = $_POST["tokenSerial"] ?? null;
        return $this->isTokenSerialValid($tokenSerial) ? $tokenSerial : null;
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
            "user"     => $user,
            "realm"    => $userRealm,
            "sortby"   => "count", // TODO: Sort somehow by last use.
            "pagesize" => 50,
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
            // $returnArray = $this->mapTokenTypes($returnArray);
            $returnArray = $this->prepareTokensForUI($returnArray);

            if ($requestAnswer["result"]["value"]["count"] != count($returnArray)) {
                // There are more tokens hidden behind paging system. (Unlikely to happen since we have token limits)
                // TODO: Show user that more tokens are available.
            }

            if (!isset($tokenSerial)) {
                // Cache tokens countings.
                $this->countUserTokens($returnArray);
            }

            return $returnArray;
        }

        return array();
    }

    public function userExists($user, $userRealm)
    {

        /* Only use if really really necessary... See below. */

        if (!$this->authenticate()) {
            return false;
        }

        $params = array(
            "realm"  => $userRealm,
        );

        /* WARNING: This call is very costly on large user databases in privacyIDEA. */
        $requestAnswer = $this->piSendRequest($params, array(), "GET", $endpoint = "/user/");
        if (!$requestAnswer["result"]["status"]) {
            return false;
        }

        foreach ($requestAnswer["result"]["value"] as $userInfo) {
            if ($userInfo["username"] == $user) {
                return true;
            }
        }

        return false;
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
                _("Could not connect to privacyIDEA backend server.") . "<br>" .
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
        // Don't print anything to js console, if ENABLE_DEBUG isn't 'true'.
        if ($this->ENABLE_DEBUG !== true) {
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

    /**
     * @param string $key
     * @return int Value from GOsa's config system.
     */
    public function getConfigIntValue($key)
    {
        $strValue = $this->getConfigStrValueInternal($key, true);

        // Remove everything which isn't a digit (not even points or commas).
        $strValue = preg_replace('~\D~', '', $strValue);

        return intval($strValue);
    }

    private function getConfigStrValueInternal($key, $debug_to_jsconsole = true)
    {
        assert(is_string($key));

        // Get config value from LDAP tree, gosa config file (gosa.conf) or
        // defaults from class_mfaAccount.inc
        $ret = $this->config->get_cfg_value("core", $key);

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
