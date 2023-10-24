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

    /** @var bool This is a gosaSetting telling if all accounts need an additional factor. */
    private $additionalFactorRequiredByOrg;

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

        $this->additionalFactorRequiredByOrg = $this->utils->getConfigBoolValue("additionalFactorRequiredByOrg");

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
        $allTokenNames = array_keys($this->mfaTokens);

        // Only enroll tokens which are allowed by allowedTokenTypes attribute.
        // Or which are overriden by ACLs.
        $allowedTokenNames = $this->attrs['allowedTokenTypes'];

        if ($allowedTokenNames['count'] <= 0) {
            // TODO: No tokens allowed???
            // Figure out what we should do now.
        }

        unset($allowedTokenNames['count']);
        $allowedTokenNames = array_map('strtolower', $allowedTokenNames);

        // TODO: Check if there are ACLs which override the token's allowance.

        foreach ($allowedTokenNames as $key => $tokenName) {
            if (!in_array_ics($tokenName, $allTokenNames)) {
                // Malformed LDAP data?!
                // TODO: Log error message here.

                // Don't show empty setup card.
                unset($allowedTokenNames[$key]);
                continue;
            }
            $token = $this->mfaTokens[$tokenName];

            $this->smarty->assign("mfa_" . $tokenName . "_icon", $token->getSetupCardIcon());
            $this->smarty->assign("mfa_" . $tokenName . "_title", $token->getSetupCardTitle());
            $this->smarty->assign("mfa_" . $tokenName . "_description", $token->getSetupCardDescription());
            $this->smarty->assign("mfa_" . $tokenName . "_button_text", $token->getSetupCardButtonText());
        }
        $this->smarty->assign('tokenTypes', $allowedTokenNames);
    }

    /**
     * Executes plugin logic, renders template, etc...
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

        /*
         * SMARTY ASSIGN VARIABLES
         */
        // Extract all ACLs and give them to Smarty, so that we can enable/disable
        // UI elements based on ACLs.
        $providedACLs = $this->plInfo()['plProvidedAcls'];
        foreach ($providedACLs as $name => $transl) {
            $aclVal  = $this->getacl($name);
            $aclName = "$name" . "ACL";
            $this->smarty->assign($aclName, $aclVal);
        }

        $this->smarty->assign(
            "additionalFactorRequiredByOrg",
            $this->additionalFactorRequiredByOrg ? "checked" : ""
        );

        $this->smarty->assign(
            "wishMfaRequired",
            ($this->attrs['mfaRequired'] == 'by_user') ? "checked" : ""
        );

        $this->prepareSetupCards();

        /*
         * MFA Setup logic
         */
        $setup = null;
        if (in_array_ics('add_token', array_keys($_POST))) {
            // Okay, user clicked on a button, which should add a token.
            // Starting MFA setup for this token type now.
            $token_type = "";
            if (in_array_ics('token_type', array_keys($_POST))) {
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
     * @return string Always returnes the MFA intro template.
     */
    private function getMFAIntroTemplate()
    {
        // Show introduction page…
        return $this->smarty->fetch(get_template_path("mfa_intro.tpl", true, dirname(__FILE__)));
    }

    /**
     * Save data to object
     */
    public function save_object()
    {
        $wishMfaRequired = false;
        if (isset($_POST['wishMfaRequired'])) {
            $wishMfaRequired = true;
        }

        // If user clicked safe settings button.
        if (isset($_POST['safe_settings'])) {
            $wishMfaRequired = false;
            if (isset($_POST['wishMfaRequired'])) {
                // TODO: Check for ACL! Is the user allowed to do this?
                $wishMfaRequired = true;
            }

            // Set mfaRequired to by_user if user wishes to.
            // If org. requires all users to use MFA then set to 'by_rule'.
            $mfaRequired = $this->attrs['mfaRequired'];
            if ($wishMfaRequired) {
                $this->attrs['mfaRequired'] = "by_user";
            } elseif ($this->additionalFactorRequiredByOrg) {
                $this->attrs['mfaRequired'] = "by_rule";
            } else {
                $this->attrs['mfaRequired'] = "";
            }
        }

        plugin::save_object();
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

        // Fill $this->attrs from $this's fields.
        // $this->attrs['mail'] = $this->mail;

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

        /* Optionally execute a command after we are done */
        if ($this->initially_was_account == $this->is_account) {
            if ($this->is_modified) {
                $this->handle_post_events("modify", array ("uid" => $this->uid));
            }
        } else {
            $this->handle_post_events("add", array ("uid" => $this->uid));
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
                                   "mfaWebAuthn"     => _("Allowed token type: WebAuthn"),
                                   "mfaTOTP"         => _("Allowed token type: TOTP"),
                                   "mfaPaper"        => _("Allowed token type: Paper"),
                                   "mfaRegistration" => _("Allowed token type: Registration"),
                                   "wishMfaRequired" => _("Let the user decide if an additional factor should " .
                                                          "voluntarily always be required."),
            ),
        ));
    }

    /**
     * Define properties (saved in LDAP as 'gosaSettings' or in gosa.conf).
     * @return array Properties information.
     */
    public static function getProperties()
    {
        return array(
            array(
                "name"        => "additionalFactorRequiredByOrg",
                "type"        => "bool",
                "check"       => "gosaProperty::isBool",
                "group"       => "privacyidea",
                "default"     => "false",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Every user needs to have at least one " .
                                   "additional factor (besides password)."),
            ),
        );
    }
}
