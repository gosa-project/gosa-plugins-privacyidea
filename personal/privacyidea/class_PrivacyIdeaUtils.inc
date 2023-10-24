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

class PrivacyIdeaUtils
{
    private PrivacyIDEA $pi;
    private config $config;
    private mfaAccount $mfaAccount;
    // TODO: Replace with GOsa's development mode.
    private bool $ENABLE_DEBUG_JS_CONSOLE = true;

    private string $GOSA_SETTINGS_GROUP;

    /** @param mfaAccount $mfaAccount MFA Account class (likely parent) */
    public function __construct(&$mfaAccount)
    {
        assert(get_class($mfaAccount) == "mfaAccount");
        $this->mfaAccount = $mfaAccount;
        $this->config     = $mfaAccount->config;

        $this->GOSA_SETTINGS_GROUP = get_class($mfaAccount);

        // TODO: Load privacyIDEA server URL from settings.
        $pi_server_url = "localhost:5000/";

        $this->pi = new PrivacyIDEA($_SERVER['HTTP_USER_AGENT'], $pi_server_url);
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
