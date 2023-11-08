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

class mfaAccount extends plugin
{
    // Plugin definitions
    public $plHeadline    = "Multifactor authentication";
    public $plDescription = "Manage Multifactor authentication";
    public $access        = "";

    // Attribute list for save action
    public $attributes    = array(
        "mfaRequired",
        "allowedTokenTypes",
    );
    public $objectclasses = array(
        "mfaService",
    );
    private $view_logged = false;

    /*
     * ATTRIBUTES
     */
    public $mfaRequired;
    public $allowedTokenTypes;

    public $config = null;
    public $ldap   = null;
    /** @var PrivacyIdeaUtils */
    public $utils = null;
    /** @var MFAToken[] Save an instance of all MFATokens. */
    public $mfaTokens = array();
    /** @var &MFAToken Used to save which setup we are currently running. */
    public $currentSetup = null;
    /** @var Smarty */
    public $smarty = null;

    public $initTime  = null;
    public $ui        = null;
    private $ldapinfo = null;

    /** @var string Used to find users in PrivacyIDEA */
    public const PI_USER_REALM = "nwt-ldap";

    public function __construct(&$config, $dn = null)
    {
        plugin::__construct($config, $dn);

        $this->initTime = microtime(true);

        /* Include config object */
        $this->config = &$config;

        // Initialize utils class
        $this->utils = new PrivacyIdeaUtils($this);

        // Initialize LDAP
        $this->ldap = $this->config->get_ldap_link();
        assert(get_class($this->ldap) == "LDAP" || get_class($this->ldap) == "ldapMultiplexer");
        if (!is_array($this->ldapinfo)) {
            $this->ldapinfo = [];
        }

        $this->allowedTokenTypes = $this->attrs['allowedTokenTypes'];
        $this->mfaRequired       = $this->attrs['mfaRequired'];

        $this->ui = get_userinfo();

        stats::log(
            'plugin',
            $class = get_class($this),
            $category = array($this->acl_category),
            $action = 'open',
            $amount = 1,
            $duration = (microtime(true) - $this->initTime),
        );

        $this->smarty = get_smarty();
        assert(get_class($this->smarty) == "Smarty");
        $this->mfaTokens['webauthn']     = new MFAWebAuthnToken($this);
        $this->mfaTokens['totp']         = new MFATotpToken($this);
        $this->mfaTokens['paper']        = new MFAPaperToken($this);
        $this->mfaTokens['registration'] = new MFARegistrationToken($this);
    }

    /**
     * This function decides which PrivacyIDEA tokens should be displayed and
     * prepares Smarty variables accordingly.
     */
    private function prepareSetupCards()
    {
        $allRegisteredTokenTypes = array_keys($this->mfaTokens);

        // Only enroll tokens which are allowed by allowedTokenTypes attribute.
        // Or which are overriden by ACLs.

        $this->updateAttrsAllowedTokenTypes();

        if (empty($this->allowedTokenTypes)) {
            // TODO: No tokens allowed???
            // Figure out what we should do now.
        }

        // TODO: Check if there are ACLs which override the token's allowance.

        foreach ($this->allowedTokenTypes as $key => $tokenType) {
            if (!in_array_ics($tokenType, $allRegisteredTokenTypes)) {
                // Malformed LDAP data?!
                // TODO: Log error message here.

                // Don't show empty setup card.
                unset($this->allowedTokenTypes[$key]);
                continue;
            }

            // We should be able to map $tokenType to a MFAToken now.
            $token = $this->mfaTokens[$tokenType];

            $limitReachedMessage = $token->checkUserTokenLimit($tokenType);
            $this->smarty->assign("mfa_" . $tokenType . "_limitReachedMessage", $limitReachedMessage);

            $this->smarty->assign("mfa_" . $tokenType . "_icon", $token->getSetupCardIcon());
            $this->smarty->assign("mfa_" . $tokenType . "_title", $token->getSetupCardTitle());
            $this->smarty->assign("mfa_" . $tokenType . "_description", $token->getSetupCardDescription());
            $this->smarty->assign("mfa_" . $tokenType . "_button_text", $token->getSetupCardButtonText());
        }

        // Sort $this->allowedTokenTypes by priority.
        usort($this->allowedTokenTypes, function (mixed $a, mixed $b) {
            $tokenA = $this->mfaTokens[$a];
            $tokenB = $this->mfaTokens[$b];
            $prioA = $tokenA->getSetupCardPriority();
            $prioB = $tokenB->getSetupCardPriority();
            if ($prioA == $prioB) {
                return 0;
            }
            return ($prioA < $prioB) ? -1 : 1;
        });

        $this->smarty->assign('tokenTypes', $this->allowedTokenTypes);
    }

    /**
     * Executes plugin logic, control flow, renders template, etc...
     * @return string Complete template, ideally loaded by Smarty to display to the user.
     */
    public function execute()
    {
        /* Call parent execute */
        plugin::execute();

        // Always mark this as an account.
        if (!$this->is_account && $this->acl_is_createable()) {
            $this->is_account = true;
        }

        /* Log view */
        if ($this->is_account && !$this->view_logged) {
            $this->view_logged = true;
            new log("view", "users/" . get_class($this), $this->dn);
        }

        // Extract all ACLs and give them to Smarty, so that we can enable/disable
        // UI elements based on ACLs.
        $providedACLs = $this->plInfo()['plProvidedAcls'];
        foreach ($providedACLs as $name => $transl) {
            $aclVal  = $this->getacl($name);
            $aclName = "$name" . "_ACL";
            $this->smarty->assign($aclName, $aclVal);
        }

        $this->updateAttrsMfaRequired();
        $this->updateAttrsAllowedTokenTypes();

        /*
         * MFA control flow
         */
        $setup = null;
        $editToken = null;
        $array_keys_post = array_keys($_POST);
        if (in_array_ics('add_token', $array_keys_post)) {
            // Okay, user clicked on a button, which should add a token.
            // Starting MFA setup for this token type now.
            $token_type = "";
            if (in_array_ics('token_type', $array_keys_post)) {
                $token_type = get_post("token_type");
            }

            // Detect if $token_type is not registered in $mfaTokens.
            // This probably means the user sent some garbage _POST request!
            // Just showing intro site again to avoid being susceptible for proxy hacks.
            if (empty($token_type) || !in_array_ics($token_type, array_keys($this->mfaTokens))) {
                $setup = null;
                msg_dialog::display(
                    _("Internal error"),
                    _("Could not process POST data of your request.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg()
                );

                return $this->getMFAIntroTemplate();
            }

            $setup = $this->mfaTokens[$token_type];
        } elseif (in_array_ics('mfaTokenAction', $array_keys_post)) {
            $tokenSerial = $this->utils->getPOSTTokenSerial();
            if (!isset($tokenSerial)) {
                // Oops, user requested an action but no tokenSerial was given?
                return $this->getMFAIntroTemplate();
            }

            $mfaTokenAction = $_POST["mfaTokenAction"];
            if ($mfaTokenAction != "mfaTokenView" && $mfaTokenAction != "mfaTokenEdit") {
                $this->executeTokenAction($mfaTokenAction, $tokenSerial);
                return $this->getMFAIntroTemplate();
            }

            return $this->getTokenEditTemplate($mfaTokenAction);
        }

        // Either return template from a setup class or the MFA intro.
        if (isset($setup)) {
            $renderSuccess = $setup->renderSetup();

            if ($renderSuccess) {
                return $setup->getRenderSetupResult();
            } else {
                // Oof, something did go terribly wrong.
                msg_dialog::display(
                    _("Internal error"),
                    _("Could not load/render template for MFA setup.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg()
                );

                return $this->getMFAIntroTemplate();
            }
        }

        return $this->getMFAIntroTemplate();
    }

    /**
     * This function makes sure that $this->mfaRequired is always cleaned up.
     * @param bool|null $wishMfaRequired
     */
    private function updateAttrsMfaRequired($wishMfaRequired = null)
    {
        // Keep $this->mfaRequired an array.
        if (!is_array($this->mfaRequired)) {
            $this->mfaRequired = array($this->mfaRequired);
        }

        // Clean up $this->mfaRequired.
        if (isset($this->mfaRequired['count'])) {
            unset($this->mfaRequired['count']);
        }

        if (!empty($this->mfaRequired)) {
            $this->mfaRequired = array_filter($this->mfaRequired, function ($value) {
                return ($value !== null && $value !== false && $value !== '');
            });
            $this->mfaRequired = array_map('strtolower', $this->mfaRequired);
            $this->mfaRequired = array_map('trim', $this->mfaRequired);
        }

        if (isset($wishMfaRequired)) {
            if ($wishMfaRequired) {
                if (!in_array("by_user", $this->mfaRequired)) {
                    $this->mfaRequired[] = "by_user";
                }
            } else {
                $this->mfaRequired = array_diff($this->mfaRequired, array("by_user"));
            }
        }
    }

    /**
     * This function makes sure that $this->allowedTokenTypes is always cleaned up.
     */
    private function updateAttrsAllowedTokenTypes()
    {
        // Keep $this->allowedTokenTypes an array.
        if (!is_array($this->allowedTokenTypes)) {
            $this->allowedTokenTypes = array($this->allowedTokenTypes);
        }

        // Clean up $this->allowedTokenTypes.
        if (isset($this->allowedTokenTypes['count'])) {
            unset($this->allowedTokenTypes['count']);
        }

        if (!empty($this->allowedTokenTypes)) {
            $this->allowedTokenTypes = array_filter($this->allowedTokenTypes, function ($value) {
                return ($value !== null && $value !== false && $value !== '');
            });
            $this->allowedTokenTypes = array_map('strtolower', $this->allowedTokenTypes);
            $this->allowedTokenTypes = array_map('trim', $this->allowedTokenTypes);
        }
    }

    /**
     * Execute an action $mfaTokenAction using token $tokenSerial.
     * @param string $tokenSerial
     * @param string $mfaTokenAction An action can be one of the following:
     * - mfaTokenSave
     * - mfaTokenResetCounter
     * - mfaTokenDisable
     * - mfaTokenEnable
     * - mfaTokenRevoke
     * - mfaTokenRemove
     */
    private function executeTokenAction($mfaTokenAction, $tokenSerial)
    {
        assert(is_string($mfaTokenAction));
        assert(is_string($tokenSerial));

        switch ($mfaTokenAction) {
            case 'mfaTokenSave':
                $tokenDescription = "";
                if (isset($_POST["tokenDescription"]) && !empty($_POST["tokenDescription"])) {
                    $tokenDescription = $_POST["tokenDescription"];
                    $tokenDescription = trim($tokenDescription);
                } else {
                    // Empty descriptions are okay.
                }

                $params = array();
                $params["serial"] = $tokenSerial;
                $params["description"] = $tokenDescription;

                $this->utils->piSendRequest($params, array(), "POST", "/token/description");

                break;

            case 'mfaTokenResetCounter':
                $this->utils->piSendRequest(array(), array(), "POST", "/token/reset/" . $tokenSerial);
                break;

            case 'mfaTokenDisable':
                $this->utils->piSendRequest(array(), array(), "POST", "/token/disable/" . $tokenSerial);
                break;

            case 'mfaTokenEnable':
                $this->utils->piSendRequest(array(), array(), "POST", "/token/enable/" . $tokenSerial);
                break;

            case 'mfaTokenRevoke':
                $this->utils->piSendRequest(array(), array(), "POST", "/token/revoke/" . $tokenSerial);
                break;

            case 'mfaTokenRemove':
                $this->utils->piSendRequest(array(), array(), "DELETE", "/token/" . $tokenSerial);
                break;

            default:
                // mfaTokenAction is malformed or not implemented yet.
                msg_dialog::display(
                    _("Internal error"),
                    _("Selected token action is not implemented yet.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg()
                );
                break;
        }
    }

    /**
     * Prepares Smarty to show the mfa token edit page, renders and returns it.
     * @return string Always returnes the token edit page for $token.
     */
    private function getTokenEditTemplate($mfaTokenAction)
    {
        $tokenSerial = $this->utils->getPOSTTokenSerial();
        $tokens = $this->utils->getTokensOfUser($this->getUid(), mfaAccount::PI_USER_REALM, $tokenSerial);
        $token = $tokens[0]; // We only want to execute an action on ONE token.

        $editEnable = false;
        $array_keys_post = array_keys($_POST);
        if ($mfaTokenAction == 'mfaTokenEdit') {
            $editEnable = true;
        } elseif ($mfaTokenAction == 'mfaTokenView') {
            // Nothing special to prepare...
        }

        $this->smarty->assign("token", $token);
        $this->smarty->assign("tokenSerial", $tokenSerial);
        $this->smarty->assign("editEnable", $editEnable);

        // Show introduction page…
        return $this->smarty->fetch(get_template_path("mfa_editToken.tpl", true, dirname(__FILE__)));
    }

    /**
     * Prepares Smarty to show the MFA intro, renders and returns it.
     * @return string Always returnes the MFA intro template.
     */
    private function getMFAIntroTemplate()
    {
        $this->smarty->assign("mfaRequiredByRule", in_array("by_rule", $this->mfaRequired) ? "checked" : "");
        $this->smarty->assign("wishMfaRequired", in_array("by_user", $this->mfaRequired) ? "checked" : "");

        $this->smarty->assign("mfaRequired", empty($this->mfaRequired) ? false : true);

        $tokens = $this->utils->getTokensOfUser($this->getUid(), mfaAccount::PI_USER_REALM);
        $this->smarty->assign("tokens", $tokens);

        $this->prepareSetupCards();

        $showWarningNoTokenRegistered = !empty($this->mfaRequired) && empty($tokens);
        $this->smarty->assign("showWarningNoTokenRegistered", $showWarningNoTokenRegistered);

        // Show introduction page…
        return $this->smarty->fetch(get_template_path("mfa_intro.tpl", true, dirname(__FILE__)));
    }

    /** @return string The uid of the current user. */
    public function getUid()
    {
        if (isset($this->attrs["uid"][0])) {
            return $this->attrs["uid"][0];
        }

        return "";
    }

    /**
     * Save data to object
     */
    public function save_object()
    {
        plugin::save_object();

        // If user clicked safe settings button.
        if (isset($_POST['edit_apply'])) {
            $wishMfaRequired = false;
            if (isset($_POST['wishMfaRequired'])) {
                // TODO: Check for ACL! Is the user allowed to do this?
                $wishMfaRequired = true;
            }

            $this->updateAttrsMfaRequired($wishMfaRequired);
        }
    }

    /**
     * Removes the mailAccount extension from ldap
     */
    public function remove_from_parent()
    {
        // Cancel if there's nothing to do here
        if (!$this->initially_was_account) {
            return;
        }

        // Remove GOsa attributes
        plugin::remove_from_parent();

        $this->cleanup();

        @DEBUG(DEBUG_LDAP, __LINE__, __FUNCTION__, __FILE__, $this->attributes, "Save");
        $ldap = $this->config->get_ldap_link();
        $ldap->cd($this->dn);
        $ldap->modify($this->attrs);

        /* Add "view" to logging class */
        new log("remove", "users/" . get_class($this), $this->dn, array_keys($this->attrs), $ldap->get_error());
        if (!$ldap->success()) {
            msg_dialog::display(
                _("LDAP error"),
                msgPool::ldaperror($ldap->get_error(), $this->dn, LDAP_MOD, get_class())
            );
        }

        /* Optionally execute a command after we are done */
        $this->handle_post_events("remove", array("uid" => $this->uid));
    }

    /**
     * Save the mfaAccount settings to the ldap database.
     */
    public function save()
    {
        $ldap = $this->config->get_ldap_link();

        // if ($this->initially_was_account) {}

        // Call parents save to prepare $this->attrs
        plugin::save();

        // Fill $this->attrs with $this's fields.
        $this->attrs['allowedTokenTypes'] = $this->allowedTokenTypes;
        $this->attrs['mfaRequired'] = $this->mfaRequired;

        // Clean up $this->attrs.
        if (is_string($this->attrs['allowedTokenTypes']) && empty($this->attrs['allowedTokenTypes'])) {
            $this->attrs['allowedTokenTypes'] = array();
        }
        if (is_string($this->attrs['mfaRequired']) && empty($this->attrs['mfaRequired'])) {
            $this->attrs['mfaRequired'] = array();
        }

        // Save data to LDAP
        $ldap->cd($this->dn);
        $this->cleanup();
        $ldap->modify($this->attrs);

        if (!$ldap->success()) {
            msg_dialog::display(
                _("LDAP error"),
                msgPool::ldaperror($ldap->get_error(), $this->dn, LDAP_MOD, get_class())
            );
        }

        /* Log last action */
        if ($this->initially_was_account) {
            new log("modify", "users/" . get_class($this), $this->dn, array_keys($this->attrs), $ldap->get_error());
        } else {
            new log("create", "users/" . get_class($this), $this->dn, array_keys($this->attrs), $ldap->get_error());
        }
    }

    /**
     * Check given values
     * @return array Strings of error messages (should be translated already).
     */
    public function check()
    {
        if (!$this->is_account) {
            return (array ());
        }

        $ldap = $this->config->get_ldap_link();

        // Call common method to give check the hook
        $message = plugin::check();

        // TODO: If user looks at it's own page AND mfa method must be activated
        // then don't let user leave!
        // $message[] = msgPool::required(_("A MFA method must be initialized or else you can't login anymore!"));

        // $message[] = msgPool::required(_("Mail address"));
        // $message[] = "check()";

        return ($message);
    }

    /**
     * Adapt from template
     *
     * @param string $dn   DN of the template to adapt from.
     * @param array  $skip
     */
    public function adapt_from_template($dn, $skip = array ())
    {
        // TODO: Adapt allowed token types.
        plugin::adapt_from_template($dn, $skip);
    }

    /**
     * Provide plugin information
     * @return array Plugin settings.
     */
    public static function plInfo()
    {
        $properties = mfaAccount::getProperties();

        return (array (
            "plShortName"    => _("MFA"),
            "plDescription"  => _("Multifactor Authentification"),
            "plSelfModify"   => true,
            "plDepends"      => array ("user"),           // This plugin depends on
            "plPriority"     => 4,                        // Position in tabs
            "plSection"      => array ("personal"   => _("My account")),
            "plCategory"     => array ("users"),
            "plOptions"      => array (),
            "plProperties"   => $properties,
            "plRequirements" => array ('onFailureDisablePlugin' => array (get_class())),
            "plProvidedAcls" => array (
                "overrideAllowToken_webauthn"     => _("Override token type: WebAuthn. This allows the user to add a " .
                                                       "token with this type."),
                "overrideAllowToken_totp"         => _("Override token type: TOTP. This allows the user to add a " .
                                                       "token with this type."),
                "overrideAllowToken_paper"        => _("Override token type: Paper. This allows the user to add a " .
                                                       "token with this type."),
                "overrideAllowToken_registration" => _("Override token type: Registration. This allows the user to " .
                                                       "add a token with this type."),
                "wishMfaRequired" => _("Let the user decide if an additional factor should " .
                                       "voluntarily always be required."),
            ),
            // TODO: Outsource ACLs overrideAllowToken_* to MFAToken classes!!
        ));
    }

    /**
     * Define properties (saved in LDAP as 'gosaSettings' or in gosa.conf).
     * @return array Properties information.
     */
    public static function getProperties()
    {
        $privacyidea_settings = array(
            array(
                "name"        => "PRIVACYIDEA_SERVER_URL",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "privacyidea",
                "default"     => "http://localhost:5000/",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("PrivacyIDEA server URL.")
            ),
            array(
                "name"        => "SERVICE_ACC_USERNAME",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "privacyidea",
                "default"     => "username",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("PrivacyIDEA service account username. Used to authenticate against PrivacyIDEA.")
            ),
            array(
                "name"        => "SERVICE_ACC_PASSWORD",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "privacyidea",
                "default"     => "password",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("PrivacyIDEA service account password. Used to authenticate against PrivacyIDEA.")
            ),
            array(
                "name"        => "SERVICE_ACC_REALM",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "privacyidea",
                "default"     => "defrealm",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("PrivacyIDEA service account realm. Used to authenticate against PrivacyIDEA.")
            ),
        );

        $token_limits = array(
            array(
                "name"        => "TOKEN_LIMIT_ALL",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "privacyidea",
                "default"     => "10",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed to link a total of X tokens from the available MFA methods.")
            ),
            array(
                "name"        => "TOKEN_LIMIT_PAPER",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "privacyidea",
                "default"     => "6",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed have a total of X paper tokens.")
            ),
            array(
                "name"        => "TOKEN_LIMIT_TOTP",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "privacyidea",
                "default"     => "6",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed have a total of X TOTP tokens.")
            ),
            array(
                "name"        => "TOKEN_LIMIT_WEBAUTHN",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "privacyidea",
                "default"     => "4",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed have a total of X WebAuthn tokens.")
            ),
            array(
                "name"        => "TOKEN_LIMIT_REGISTRATION",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "privacyidea",
                "default"     => "0",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed have a total of X Registration tokens.")
            ),
            // Dear admin, 0 doesn't mean that the user can't create this token type but that there is no limit for
            // this type. Please use allowedTokenTypes or it's ACL for this purpouse.
        );

        $misc = array(
            array(
                "name"        => "amountOfPaperTokenOTPs",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "privacyidea",
                "default"     => "20",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Amount of OTPs which will be printed on a single TAN list."),
            ),
        );

        return array_merge($privacyidea_settings, $token_limits, $misc);
    }
}
