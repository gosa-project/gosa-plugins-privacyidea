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
    public $plDescription = "Manage multifactor authentication";
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
    public $effectiveTokenTypes;

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

    /** @var string Used to find users in privacyIDEA */
    public $userRealm;

    public $orig_dn = "";

    // Indicate to parent that its footer should be hidden
    public $skipFooter    = false;
    public $skipWrite;

    public function __construct(&$config, $dn = null)
    {
        parent::__construct($config, $dn);

        $this->orig_dn = $dn;

        $this->initTime = microtime(true);

        /* Include config object */
        $this->config = &$config;

        // Initialize utils class
        $this->utils = new PrivacyIdeaUtils($this);

        $this->userRealm = $this->utils->getConfigStrValue("piUserRealm");

        // Initialize LDAP
        $this->ldap = $this->config->get_ldap_link();
        assert(get_class($this->ldap) == "LDAP" || get_class($this->ldap) == "ldapMultiplexer");
        if (!is_array($this->ldapinfo)) {
            $this->ldapinfo = [];
        }

        $this->allowedTokenTypes = $this->attrs['allowedTokenTypes'] ?? array();
        $this->mfaRequired       = $this->attrs['mfaRequired'] ?? array();

        $this->effectiveTokenTypes = [];

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
        assert(in_array(get_class($this->smarty), ["Smarty","Smarty\Smarty"]));
        $this->mfaTokens['webauthn']     = new MFAWebAuthnToken($this);
        $this->mfaTokens['totp']         = new MFATotpToken($this);
        $this->mfaTokens['paper']        = new MFAPaperToken($this);
        $this->mfaTokens['registration'] = new MFARegistrationToken($this);

        // Detect initial account state - This value will be overridden later in the code.
        //  - We may inherit our account state from a role.
        $this->initially_was_account = $this->is_account;
    }

    /**
     * This function decides which privacyIDEA tokens should be displayed and
     * prepares Smarty variables accordingly.
     */
    private function prepareSetupCards()
    {
        $allRegisteredTokenTypes = array_keys($this->mfaTokens);

        // Only enroll tokens which are allowed by allowedTokenTypes attribute.
        // Or which are overriden by ACLs.

        $this->updateAttrsAllowedTokenTypes();

        if (empty($this->effectiveTokenTypes)) {
            // TODO: No tokens allowed???
            // Figure out what we should do now.
        }

        foreach ($this->effectiveTokenTypes as $key => $tokenType) {
            if (!in_array_ics($tokenType, $allRegisteredTokenTypes)) {
                // Don't show empty setup card.
                unset($this->effectiveTokenTypes[$key]);
                continue;
            }

            // We should be able to map $tokenType to a MFAToken now.
            $token = $this->mfaTokens[$tokenType];

            $limitReachedMessage = $token->checkUserTokenLimit($tokenType);
            $this->smarty->assign("mfa_" . $tokenType . "_limitReachedMessage", $limitReachedMessage);
        }

        foreach ($this->mfaTokens as $tokenType => $token) {
            $this->smarty->assign("mfa_" . $tokenType . "_title", $token->getSetupCardTitle());
            $this->smarty->assign("mfa_" . $tokenType . "_description", $token->getSetupCardDescription());
            $this->smarty->assign("mfa_" . $tokenType . "_button_text", $token->getSetupCardButtonText());
        }

        // Sort $this->effectiveTokenTypes by priority.
        usort($this->effectiveTokenTypes, function (mixed $a, mixed $b) {
            $tokenA = $this->mfaTokens[$a];
            $tokenB = $this->mfaTokens[$b];
            $prioA = $tokenA->getSetupCardPriority();
            $prioB = $tokenB->getSetupCardPriority();
            if ($prioA == $prioB) {
                return 0;
            }
            return ($prioA < $prioB) ? -1 : 1;
        });
    }

    /**
     * Executes plugin logic, control flow, renders template, etc...
     * @return string Complete template, ideally loaded by Smarty to display to the user.
     */
    public function execute()
    {
        /* Before using smarty get the up-to-date handle */
        $this->smarty = get_smarty();

        $parentName = is_object($this->parent) ? get_class($this->parent) : '';

        /* Call parent execute */
        parent::execute();

        $this->utils->hasPiErrors = false;

        $this->uid = $this->getUid();

        if (!$this->is_account && $this->acl_is_writeable("")) {
            $this->is_account = true;
        }

        /* Do we represent a valid mfaAccount? */
        if (!$this->is_account) {
            switch ($theme) {
            case 'classic':
                // TODO: implement for classic theme
                $display = "";
                break;
            default:
                $display = " <div class='card-panel red lighten-4 red-text text-darken-4 valign-wrapper'>
                                <p><i style='font-size: 2em; margin-right: 1em;' class='material-icons'>error_outline</i> " . msgPool::noValidExtension(_($this->plHeadline)) . "</p>
                            </div>";
            }
            if ($this->parent === null) {
                $display .= back_to_main();
            }
            return ($display);
        }

        /* In 'MyAccount' mode, we must remove write acls if we are not in editing mode (plus exception for roletabs mode). */
        $this->skipWrite = $parentName != "roletabs" && !session::is_set('edit');

        /* Log view */
        if ($this->is_account && !$this->view_logged) {
            $this->view_logged = true;
            new log("view", "users/" . get_class($this), $this->dn);
        }

        // Extract all ACLs and give them to Smarty, so that we can enable/disable
        // UI elements based on ACLs.
        $providedACLs = $this->plInfo()['plProvidedAcls'];
        foreach ($providedACLs as $name => $transl) {
            switch ($name) {
                case "allowedTokenTypes":   // FALLTHROUGH
                case "mfaRequiredByRule":   // FALLTHROUGH
                case "mfaRequiredByUser":
                    $mfaSkipWrite = $this->skipWrite;
                    break;
                default:
                    $mfaSkipWrite = false;
            }
            $aclVal  = $this->getacl($name, $mfaSkipWrite);
            $aclName = "$name" . "ACL";
            $this->smarty->assign($aclName, $aclVal);
        }

        $this->smarty->assign("currentUserUID", $this->ui->uid);
        $this->smarty->assign("currentObjectUID", $this->attrs["uid"][0] ?? '');

        $this->smarty->assign("parent", $parentName);

        $this->updateAttrsMfaRequired();
        $this->updateAttrsAllowedTokenTypes();

        /*
         * MFA control flow
         *
         * TODO: refactor and split up into methods
         */
        $show_mfa_manager  = $parentName != "roletabs";
        if ($show_mfa_manager) {
            $setup = null;
            $editToken = null;
            $array_keys_post = array_keys($_POST);

            // If we are in a TOTP flow and press regenerate QR, readd the required context
            if (in_array_ics('mfa_generate_totp_secret', $array_keys_post)) {
                $_POST['add_token'] = 'totp';
                $array_keys_post[] = 'add_token';
            }

            if (in_array_ics('add_token', $array_keys_post)) {
                if (in_array_ics('cancel_setup', $array_keys_post)) {
                    return $this->getMFAIntroTemplate();
                }

                if (!$this->acl_is_writeable("manageTokens")) {
                    $setup = null;
                    msg_dialog::display(
                        _("Insufficient permissions"),
                        _("You are not allowed to add a new factor.") . "<br>" .
                        $this->utils->pleaseTryAgainMsg(),
                        ERROR_DIALOG
                    );

                    return $this->getMFAIntroTemplate();
                }
                // Okay, user clicked on a button, which should add a token.
                // Starting MFA setup for this token type now.
                $token_type = get_post("add_token");

                // Detect if $token_type is not registered in $mfaTokens.
                // This probably means the user sent some garbage _POST request!
                // Just showing intro site again to avoid being susceptible for proxy hacks.
                if (empty($token_type) || !in_array_ics($token_type, $this->effectiveTokenTypes)) {
                    $setup = null;
                    msg_dialog::display(
                        _("Internal error"),
                        _("Could not process POST data of your request.") . "<br>" .
                        $this->utils->pleaseTryAgainMsg(),
                        ERROR_DIALOG
                    );

                    return $this->getMFAIntroTemplate();
                }

                $setup = $this->mfaTokens[$token_type];
            } elseif (in_array_ics('mfaTokenAction', $array_keys_post)) {
                if (!$this->acl_is_writeable("manageTokens")) {
                    msg_dialog::display(
                        _("Insufficient permissions"),
                        _("You are not allowed to manage MFA factors.") . "<br>" .
                        $this->utils->pleaseTryAgainMsg(),
                        ERROR_DIALOG
                    );

                    return $this->getMFAIntroTemplate();
                }

                // Idea: refactor this to use multiple mfaTokenActions.
                // if count($mfaTokenActions) > 1, then this could be treated as a batch operation.
                $mfaTokenActions = $_POST["mfaTokenAction"];
                if (!is_array($mfaTokenActions) || count($mfaTokenActions) != 1) {
                    return $this->getMFAIntroTemplate(); // Got malformed data, straight to MFA intro again.
                } else {
                    $mfaTokenAction = array_keys($mfaTokenActions)[0]; // Get the mfaTokenAction type.
                    $tokenSerial    = $mfaTokenActions[$mfaTokenAction]; // Get the tokenSerial part.

                    if (!isset($tokenSerial) || !$this->utils->isTokenSerialValid($tokenSerial)) {
                        // Oops, user requested an action but no tokenSerial was given?
                        return $this->getMFAIntroTemplate();
                    }
                }

                switch ($mfaTokenAction) {
                    case 'mfaTokenDeactivate':   // FALLTHROUGH
                    case 'mfaTokenRevoke':       // FALLTHROUGH
                    case 'mfaTokenRemove':
                        $tokens = $this->utils->getTokensOfUser($this->getUid(), $this->userRealm);
                        $activeTokenSerials = $this->utils->getActiveTokenSerials($tokens);
                        if (count(array_diff($activeTokenSerials, array($tokenSerial))) == 0) {
                            // Prevent users from locking themselves out
                            msg_dialog::display(
                                _("Unsupported action"),
                                _("It is not supported to deactivate, revoke or remove your last token.") . "<br>" .
                                _("Please add another token to ensure that you always have at least one working " .
                                  "token.") . "<br>" .
                                $this->utils->pleaseTryAgainMsg(),
                                ERROR_DIALOG
                            );

                            return $this->getMFAIntroTemplate();
                        }

                        if (!isset($_POST['mfaTokenActionConfirm'])) {
                            // not confirmed yet by user
                            $this->skipFooter = true;
                            return $this->getMFAConfirmTemplate($mfaTokenAction, $tokenSerial);
                        } elseif ($_POST['mfaTokenActionConfirm'] !== "true") {
                            // cancelled by user
                            return $this->getMFAIntroTemplate();
                        }
                        break;
                    case "mfaTokenView":    // FALLTHROUGH
                    case "mfaTokenEdit":
                        $this->skipFooter = true;
                        return $this->getTokenEditTemplate($mfaTokenAction, $tokenSerial);
                    default:
                        break;
                }

                $this->executeTokenAction($mfaTokenAction, $tokenSerial);
                return $this->getMFAIntroTemplate();
            } elseif (in_array_ics('mfaTokenBatchAction', $array_keys_post)) {
                if (!$this->acl_is_writeable("manageTokens")) {
                    msg_dialog::display(
                        _("Insufficient permissions"),
                        _("You are not allowed to manage MFA factors.") . "<br>" .
                        $this->utils->pleaseTryAgainMsg(),
                        ERROR_DIALOG
                    );

                    return $this->getMFAIntroTemplate();
                }

                $mfaTokenSerials = [];
                if (isset($_POST['mfaTokenSerials']) && is_array($_POST['mfaTokenSerials'])) {
                    $mfaTokenSerials = array_unique($_POST['mfaTokenSerials']);
                    array_filter($mfaTokenSerials, [$this->utils, 'isTokenSerialValid']);
                }

                $mfaTokenAction = $_POST["mfaTokenBatchAction"] ?? null;
                switch ($mfaTokenAction) {
                    case 'mfaTokenResetCounter': // FALLTHROUGH
                    case 'mfaTokenActivate':
                        break;
                    case 'mfaTokenDeactivate':   // FALLTHROUGH
                    case 'mfaTokenRevoke':       // FALLTHROUGH
                    case 'mfaTokenRemove':
                        $tokens = $this->utils->getTokensOfUser($this->getUid(), $this->userRealm);
                        $activeTokenSerials = $this->utils->getActiveTokenSerials($tokens);
                        if (count(array_diff($activeTokenSerials, $mfaTokenSerials)) == 0) {
                            // Prevent users from locking themselves out
                            msg_dialog::display(
                                _("Unsupported action"),
                                _("It is not supported to deactivate, revoke or remove your last token.") . "<br>" .
                                _("Please add another token to ensure that you always have at least one working " .
                                  "token.") . "<br>" .
                                $this->utils->pleaseTryAgainMsg(),
                                ERROR_DIALOG
                            );

                            return $this->getMFAIntroTemplate();
                        }

                        if (!isset($_POST['mfaTokenActionConfirm'])) {
                            // not confirmed yet by user
                            $this->skipFooter = true;
                            return $this->getMFAConfirmTemplate($mfaTokenAction, $mfaTokenSerials);
                        } elseif ($_POST['mfaTokenActionConfirm'] !== "true") {
                            // cancelled by user
                            return $this->getMFAIntroTemplate();
                        }
                        break;
                    default:
                        // invalid action
                        return $this->getMFAIntroTemplate();
                }

                foreach ($mfaTokenSerials as $mfaTokenSerial) {
                    // Stop on first error
                    if (!$this->executeTokenAction($mfaTokenAction, $mfaTokenSerial)) {
                        msg_dialog::display(
                            _("Internal error"),
                            sprintf(_("A batch operation failed on token '%s'. Skipping remaining operations."), $mfaTokenSerial) . "<br>" .
                            $this->utils->pleaseTryAgainMsg(),
                            ERROR_DIALOG
                        );
                        break;
                    }
                }
                return $this->getMFAIntroTemplate();
            }
        }

        // Either return template from a setup class or the MFA intro.
        if (isset($setup)) {
            $renderSuccess = $setup->renderSetup();

            if ($renderSuccess) {
                $this->skipFooter = true;
                return $setup->getRenderSetupResult();
            } else {
                // Oof, something did go terribly wrong.
                msg_dialog::display(
                    _("Internal error"),
                    _("Could not load/render template for MFA setup.") . "<br>" .
                    $this->utils->pleaseTryAgainMsg(),
                    ERROR_DIALOG
                );

                return $this->getMFAIntroTemplate();
            }
        }

        return $this->getMFAIntroTemplate();
    }

    /**
     * This function makes sure that $this->mfaRequired is always cleaned up.
     * @param bool|null $mfaRequiredByRule
     * @param bool|null $mfaRequiredByUser
     */
    private function updateAttrsMfaRequired($mfaRequiredByRule = null, $mfaRequiredByUser = null)
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

        if (isset($mfaRequiredByRule)) {
            if ($mfaRequiredByRule) {
                if (!in_array("by_rule", $this->mfaRequired)) {
                    $this->mfaRequired[] = "by_rule";
                }
            } else {
                $this->mfaRequired = array_diff($this->mfaRequired, array("by_rule"));
            }
        }
        if (isset($mfaRequiredByUser)) {
            if ($mfaRequiredByUser) {
                if (!in_array("by_user", $this->mfaRequired)) {
                    $this->mfaRequired[] = "by_user";
                }
            } else {
                $this->mfaRequired = array_diff($this->mfaRequired, array("by_user"));
            }
        }

        // Re-index the array, so index numbers always start with number 0.
        $this->mfaRequired = array_values($this->mfaRequired);
    }

    /**
     * This function makes sure that $this->allowedTokenTypes is always cleaned up.
     * @param array|null $allowedTokenTypes
     */
    private function updateAttrsAllowedTokenTypes($allowedTokenTypes = null)
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

        // ensure $allowedTokenTypes are valid
        if (isset($allowedTokenTypes) && is_array($allowedTokenTypes)) {
            $this->allowedTokenTypes = array_filter($allowedTokenTypes, function ($value) {
                return array_key_exists($value, $this->mfaTokens);
            });
        }

        $this->allowedTokenTypes = array_unique($this->allowedTokenTypes);

        // Re-index the array, so index numbers always start with number 0.
        $this->allowedTokenTypes = array_values($this->allowedTokenTypes);

        /*
         * Determine the effectively allowed token types which is the union of
         * types allowed via the LDAP attribute allowedTokenTypes and those
         * allowed via ACLs
         */
        $tokenTypesAcls = array(
            "paper"        => "addTokenPaper",
            "registration" => "addTokenRegistration",
            "webauthn"     => "addTokenWebAuthn",
            "totp"         => "addTokenTotp",
        );
        $overrideTokenTypes = [];
        foreach ($this->mfaTokens as $tokenType => $tokenType) {
            assert(array_key_exists($tokenType, $tokenTypesAcls));

            if ($this->acl_is_writeable("addTokenAny") || $this->acl_is_writeable($tokenTypesAcls[$tokenType])) {
                $overrideTokenTypes[] = $tokenType;
            }
        }

        $this->effectiveTokenTypes = array_unique(array_merge($this->allowedTokenTypes, $overrideTokenTypes));
    }

    /**
     * Execute an action $mfaTokenAction using token $tokenSerial.
     * @param string $tokenSerial
     * @param string $mfaTokenAction An action can be one of the following:
     * - mfaTokenSave
     * - mfaTokenResetCounter
     * - mfaTokenDeactivate
     * - mfaTokenActivate
     * - mfaTokenRevoke
     * - mfaTokenRemove
     * @return bool Wether the action succeeded
     */
    private function executeTokenAction($mfaTokenAction, $tokenSerial)
    {
        assert(is_string($mfaTokenAction));
        assert(is_string($tokenSerial));

        switch ($mfaTokenAction) {
            case 'mfaTokenSave':
                if (!$this->acl_is_writeable("tokenDescription")) {
                    break;
                }

                $tokenDescription = "";
                if (isset($_POST["tokenDescription"]) && !empty($_POST["tokenDescription"])) {
                    $tokenDescription = $_POST["tokenDescription"];
                    $tokenDescription = trim($tokenDescription);
                    // cut off at 60 characters
                    if (function_exists("mb_substr")) {
                        $tokenDescription = mb_substr($tokenDescription, 0, 60, "UTF-8");
                    } else {
                        $tokenDescription = substr($tokenDescription, 0, 60);
                    }
                } else {
                    // Empty descriptions are okay.
                }

                $params = array();
                $params["serial"] = $tokenSerial;
                $params["description"] = $tokenDescription;

                /*
                 * /token/description requires that you have the admin.setdescription policy set.
                 * However, this policy is only available as an admin policy from PI 3.9; previously it was only
                 * available as a user policy: https://github.com/privacyidea/privacyidea/issues/3609
                 * In order for the plugin to work for PI versions before 3.9, the /token/set API must be used,
                 * which responds to the admin.set policy, which can also be set with 3.8.
                 * See internal discussion at (login-protected):
                 * https://gitlab.gonicus.de/uni-bonn/unibonn-gosa_v2-8/gosa_plugin_privacyidea/-/issues/15
                 */
                if ($this->utils->piServerVersionCompare("3.8.1", "gt") === true) {
                    $this->utils->piSendRequest($params, array(), "POST", "/token/description");
                } else {
                    $this->utils->piSendRequest($params, array(), "POST", "/token/set");
                }

                new log(
                    "modify",
                    "users/" . get_class($this),
                    $this->dn,
                    array(),
                    sprintf("Description of token '%s' associated with user '%s' has been updated.", $tokenSerial, $this->getUid())
                );

                return true;

            case 'mfaTokenResetCounter':
                if (!$this->acl_is_writeable("tokenFailCountReset")) {
                    break;
                }

                $this->utils->piSendRequest(array(), array(), "POST", "/token/reset/" . $tokenSerial);
                new log(
                    "modify",
                    "users/" . get_class($this),
                    $this->dn,
                    array(),
                    sprintf("Error counter of token '%s' associated with user '%s' has been reset.", $tokenSerial, $this->getUid())
                );
                return true;

            case 'mfaTokenDeactivate':
                if (!$this->acl_is_writeable("tokenDeactivation")) {
                    break;
                }

                $this->utils->piSendRequest(array(), array(), "POST", "/token/disable/" . $tokenSerial);
                new log(
                    "modify",
                    "users/" . get_class($this),
                    $this->dn,
                    array(),
                    sprintf("Token '%s' associated with user '%s' has been disabled.", $tokenSerial, $this->getUid())
                );
                return true;

            case 'mfaTokenActivate':
                if (!$this->acl_is_writeable("tokenActivation")) {
                    break;
                }

                $this->utils->piSendRequest(array(), array(), "POST", "/token/enable/" . $tokenSerial);
                new log(
                    "modify",
                    "users/" . get_class($this),
                    $this->dn,
                    array(),
                    sprintf("Token '%s' associated with user '%s' has been enabled.", $tokenSerial, $this->getUid())
                );
                return true;

            case 'mfaTokenRevoke':
                if (!$this->acl_is_writeable("tokenRevocation")) {
                    break;
                }

                $this->utils->piSendRequest(array(), array(), "POST", "/token/revoke/" . $tokenSerial);
                new log(
                    "modify",
                    "users/" . get_class($this),
                    $this->dn,
                    array(),
                    sprintf("Token '%s' associated with user '%s' has been revoked.", $tokenSerial, $this->getUid())
                );
                return true;

            case 'mfaTokenRemove':
                if (!$this->acl_is_writeable("tokenRemoval")) {
                    break;
                }

                $this->utils->piSendRequest(array(), array(), "DELETE", "/token/" . $tokenSerial);
                new log(
                    "remove",
                    "users/" . get_class($this),
                    $this->dn,
                    array(),
                    sprintf("Token '%s' associated with user '%s' has been removed.", $tokenSerial, $this->getUid())
                );
                return true;
        }

        // mfaTokenAction is malformed or not implemented yet.
        msg_dialog::display(
            _("Internal error"),
            _("Token action malformed or not allowed.") . "<br>" .
            $this->utils->pleaseTryAgainMsg(),
            ERROR_DIALOG
        );
    }

    /**
     * Prepares Smarty to show the mfa token edit page, renders and returns it.
     * @param string $mfaTokenAction
     * @param string $tokenSerial
     * @return string Always returnes the token edit page for $token.
     */
    private function getTokenEditTemplate($mfaTokenAction, $tokenSerial)
    {
        $tokens = $this->utils->getTokensOfUser($this->getUid(), $this->userRealm, $tokenSerial);
        if (empty($tokens)) {
            return $this->getMFAIntroTemplate();
        }
        $token = $tokens[0]; // We only want to execute an action on ONE token.

        $editEnable = false;
        $array_keys_post = array_keys($_POST);
        if ($mfaTokenAction == 'mfaTokenEdit') {
            $editEnable = true;
        } elseif ($mfaTokenAction == 'mfaTokenView') {
            // Nothing special to prepare...
        }

        $tokenType = $token['tokentype'];
        $tokenTitle = "";
        if (isset($this->mfaTokens[$token['tokentype']])) {
            $tokenTitle = $this->mfaTokens[$token['tokentype']]->getSetupCardTitle();
        }
        $this->smarty->assign("mfa_" . $token['tokentype'] . "_title", $tokenTitle);
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
        $this->smarty->assign("userExists", ($this->orig_dn != "new"));
        $this->smarty->assign("mfaRequiredByRule", in_array("by_rule", $this->mfaRequired) ? "checked" : "");
        $this->smarty->assign("mfaRequiredByUser", in_array("by_user", $this->mfaRequired) ? "checked" : "");

        $this->smarty->assign("mfaRequired", empty($this->mfaRequired) ? false : true);

        $this->smarty->assign("allTokenTypes", array_keys($this->mfaTokens));

        $parentName = is_object($this->parent) ? get_class($this->parent) : "";
        if ($parentName != "roletabs") {
            $tokens = $this->utils->getTokensOfUser($this->getUid(), $this->userRealm);
            $this->smarty->assign("tokens", $tokens);
            $this->smarty->assign("activeTokenSerials", $this->utils->getActiveTokenSerials($tokens));

            $this->prepareSetupCards();

            $this->smarty->assign('hasPiErrors', $this->utils->hasPiErrors);
            $this->smarty->assign('plsTryAgainMsg', $this->utils->pleaseTryAgainMsg());

            $this->smarty->assign('tokenTypes', $this->allowedTokenTypes);

            $showWarningNoTokenRegistered = !empty($this->mfaRequired) && empty($tokens);
            $this->smarty->assign("showWarningNoTokenRegistered", $showWarningNoTokenRegistered);

            $this->smarty->assign('effectiveTokenTypes', $this->effectiveTokenTypes);
        } else {
            $this->smarty->assign('tokenTypes', $this->allowedTokenTypes);
            foreach ($this->mfaTokens as $tokenType => $token) {
                $this->smarty->assign("mfa_" . $tokenType . "_title", $token->getSetupCardTitle());
            }
        }

        // Show introduction page…
        return $this->smarty->fetch(get_template_path("mfa_intro.tpl", true, dirname(__FILE__)));
    }

    /**
     * Prepares Smarty to show the MFA confirmation page for destructive
     * actions, renders and returns it.
     * @param string $mfaTokenAction
     * @param array|string $mfaTokens
     * @return string Always returnes the MFA confirmation page template.
     */
    private function getMFAConfirmTemplate($mfaTokenAction, $mfaTokens)
    {
        assert((is_array($mfaTokens) && array_is_list($mfaTokens)) || is_string($mfaTokens));

        $isBatch = is_array($mfaTokens);
        $this->smarty->assign("isBatch", $isBatch);

        $tokenCount = $isBatch ? count($mfaTokens) : 1;

        $this->smarty->assign($isBatch ? "mfaTokenBatchAction" : "mfaTokenAction", $mfaTokenAction);
        switch ($mfaTokenAction) {
            case 'mfaTokenResetCounter':
                $confirmationPrompt = ngettext("Reset the failed login counter for the following token?", "Reset the failed login counter for the following tokens?", $tokenCount);
                break;
            case 'mfaTokenDeactivate':
                $confirmationPrompt = ngettext("Disable the following token?", "Disable the following tokens?", $tokenCount);
                break;
            case 'mfaTokenActivate':
                $confirmationPrompt = ngettext("Enable the following token?", "Enable the following tokens?", $tokenCount);
                break;
            case 'mfaTokenRevoke':
                $confirmationPrompt = ngettext("Revoke the following token?", "Revoke the following tokens?", $tokenCount);
                break;
            case 'mfaTokenRemove':
                $confirmationPrompt = ngettext("Remove the following token?", "Remove the following tokens?", $tokenCount);
                break;
            default:
                $confirmationPrompt = "";
        }
        $this->smarty->assign("confirmationPrompt", $confirmationPrompt);

        $this->smarty->assign($isBatch ? "mfaTokenSerials" : "tokenSerial", $mfaTokens);

        $tokens = $this->utils->getTokensOfUser($this->getUid(), $this->userRealm);
        $serialsTokens = array_reduce($tokens, function ($result, $token) {
            $result[$token["serial"]] = $token;
            return $result;
        }, []);
        $tokensDescriptions = array();
        if ($isBatch) {
            foreach ($mfaTokens as $tokenSerial) {
                $tokensDescriptions[$tokenSerial] = array_key_exists($tokenSerial, $serialsTokens) ?
                        $serialsTokens[$tokenSerial]['description'] . " ($tokenSerial)" : "$tokenSerial";
            }
        } else {
            $tokensDescriptions[$mfaTokens] = array_key_exists($mfaTokens, $serialsTokens) ?
                    $serialsTokens[$mfaTokens]['description'] . " ($mfaTokens)" : "$mfaTokens";
        }
        $this->smarty->assign("tokensDescriptions", $tokensDescriptions);

        return $this->smarty->fetch(get_template_path("mfa_batchConfirm.tpl", true, dirname(__FILE__)));
    }

    /** @return string The uid of the current user. */
    public function getUid()
    {
        if (isset($this->uid) && $this->uid) {
            return $this->uid;
        }

        if (isset($this->attrs["uid"][0])) {
            return $this->attrs["uid"][0];
        }

        // fall back to logged in user
        if (!isset($this->parent)) {
            return $this->ui->uid;
        }

        assert(false, "could not determine UID");
    }

    /**
     * Save data to object
     */
    public function save_object()
    {
        parent::save_object();

        // If user clicked save settings button.
        if (isset($_POST['edit_apply']) || isset($_POST['edit_finish'])) {
            $mfaRequiredByRule = $this->acl_is_writeable("mfaRequiredByRule") ? isset($_POST['mfaRequiredByRule']) : null;
            $mfaRequiredByUser = $this->acl_is_writeable("mfaRequiredByUser") ? isset($_POST['mfaRequiredByUser']) : null;
            $this->updateAttrsMfaRequired($mfaRequiredByRule, $mfaRequiredByUser);

            if ($this->acl_is_writeable("allowedTokenTypes")) {
                $allowedTokenTypes = @is_array($_POST['allowedTokenTypes']) ? $_POST['allowedTokenTypes'] : array();
                $this->updateAttrsAllowedTokenTypes($allowedTokenTypes);
            }

            new log(
                "modify",
                "users/" . get_class($this),
                $this->dn,
                array(),
                sprintf("mfaService LDAP attributes of user '%s' have been updated.", $this->getUid()),
            );
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
        parent::remove_from_parent();

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
                msgPool::ldaperror($ldap->get_error(), $this->dn, LDAP_MOD, get_class()),
                ERROR_DIALOG
            );
        }

        /* Optionally execute a command after we are done */
        $this->handle_post_events("remove", array("uid" => $this->getUid()));
    }

    /**
     * Save the mfaAccount settings to the ldap database.
     */
    public function save()
    {
        $ldap = $this->config->get_ldap_link();

        // if ($this->initially_was_account) {}

        // Call parents save to prepare $this->attrs
        parent::save();

        // Clean up $this->allowedTokenTypes and $this->mfaRequired
        $this->updateAttrsAllowedTokenTypes();
        $this->updateAttrsMfaRequired();

        // Fill $this->attrs with $this's fields.
        $this->attrs['allowedTokenTypes'] = $this->allowedTokenTypes;
        $this->attrs['mfaRequired'] = $this->mfaRequired;

        // Save data to LDAP
        $ldap->cd($this->dn);
        $this->cleanup();
        $ldap->modify($this->attrs);

        if (!$ldap->success()) {
            msg_dialog::display(
                _("LDAP error"),
                msgPool::ldaperror($ldap->get_error(), $this->dn, LDAP_MOD, get_class()),
                ERROR_DIALOG
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
            return (array());
        }

        $ldap = $this->config->get_ldap_link();

        // Call common method to give check the hook
        $message = parent::check();

        // TODO: If user looks at it's own page AND mfa method must be activated
        // then don't let user leave!
        // $message[] = msgPool::required(_("A MFA method must be initialized or else you can't login anymore!"));

        // $message[] = msgPool::required(_("Email address"));
        // $message[] = "check()";

        return ($message);
    }

    /**
     * Adapt from template
     *
     * @param string $dn   DN of the template to adapt from.
     * @param array  $skip
     */
    public function adapt_from_template($dn, $skip = array())
    {
        // TODO: Adapt allowed token types.
        parent::adapt_from_template($dn, $skip);
    }

    /**
     * Provide plugin information
     * @return array Plugin settings.
     */
    public static function plInfo()
    {
        $properties = mfaAccount::getProperties();

        // PI read only means that it can't be set/resetted or changed in any way via privacyIDEA API.
        // PI write only means that one can't get it via privacyIDEA API (Token actions for example).
        $piReadOnly   = "&nbsp;(" . _("PI read-only") . ") ";
        $piWriteOnly  = "&nbsp;(" . _("PI write-only") . ") ";
        $writeOnly    = "&nbsp;(" . _("write-only") . ") ";
        $readOnly     = "&nbsp;(" . _("read-only") . ") ";

        return (array(
            "plShortName"    => _("MFA"),
            "plDescription"  => _("Multifactor Authentification"),
            "plSelfModify"   => true,
            "plDepends"      => array("user"),           // This plugin depends on
            "plPriority"     => 4,                        // Position in tabs
            "plSection"      => array("personal"   => _("My account")),
            "plCategory"     => array("users"),
            "plOptions"      => array(),
            "plProperties"   => $properties,
            "plRequirements" => array('onFailureDisablePlugin' => array(get_class())),
            "plProvidedAcls" => array(
                # GOsa R+W
                "allowedTokenTypes"     => _("MFA token types"),

                # GOsa R+W
                "mfaRequiredByRule"     => _("Require MFA due to organizational policy"),
                "mfaRequiredByUser"     => _("Require MFA due to user preference"),

                # GOsa R+W
                "manageTokens"          => _("Manage tokens"),

                # GOsa W
                // TODO: Outsource addToken* ACLs into the specific MFAToken class (Create getACLInformation() func?)
                "addTokenAny"           => _("Add any type of token")           . $writeOnly,
                "addTokenPaper"         => _("Add TAN list")                    . $writeOnly,
                "addTokenRegistration"  => _("Add Recovery key")                . $writeOnly,
                "addTokenWebAuthn"      => _("Add Security key")                . $writeOnly,
                "addTokenTotp"          => _("Add Authentication app (TOTP)")   . $writeOnly,

                # GOsa R+W
                "tokenDescription"      => _("Description"),                    # R + W (editing description)
                "tokenFailCount"        => _("Failed login counter")            . $piReadOnly,
                "tokenLastUsed"         => _("Last use")                        . $piReadOnly,
                "tokenStatus"           => _("Status")                          . $piReadOnly,
                "tokenCountAuth"        => _("Authentication count")            . $piReadOnly,
                "tokenCountAuthSuccess" => _("Successful authentication count") . $piReadOnly,

                # GOsa R+W
                "tokenFailCountReset" => _("Resetting of fail counter allowed") . $piWriteOnly,
                "tokenRevocation"     => _("Revocation of tokens allowed")      . $piWriteOnly,
                "tokenRemoval"        => _("Removal of tokens allowed")         . $piWriteOnly,
                "tokenActivation"     => _("Activation of tokens allowed")      . $piWriteOnly,
                "tokenDeactivation"   => _("Deactivation of tokens allowed")    . $piWriteOnly,
            ),
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
                "name"        => "piServer",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "core",
                "default"     => "http://localhost:5000/",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("privacyIDEA server URL.")
            ),
            array(
                "name"        => "piTokenOrigin",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "core",
                "default"     => "https://gosa.example.org/",
                "migrate"     => "",
                "mandatory"   => false,
                "description" => _("Protocol + FQDN. Used to set Origin: header for privacyIDEA where needed.")
            ),
            array(
                "name"        => "piServiceAccount",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "core",
                "default"     => "username",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("privacyIDEA service account username. Used to authenticate GOsa² against privacyIDEA.")
            ),
            array(
                "name"        => "piServicePass",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "core",
                "default"     => "password",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("privacyIDEA service account password. Used to authenticate GOsa² against privacyIDEA.")
            ),
            array(
                "name"        => "piServiceRealm",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "core",
                "default"     => "defrealm",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("privacyIDEA service account realm. Used to authenticate GOsa² against privacyIDEA.")
            ),
            array(
                "name"        => "piUserRealm",
                "type"        => "string",
                "check"       => "gosaProperty::isString",
                "group"       => "core",
                "default"     => "defrealm",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("privacyIDEA user realm. Used to find users in privacyIDEA.")
            ),
        );

        $token_limits = array(
            array(
                "name"        => "piTokenLimitAll",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "core",
                "default"     => "10",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed to link a total of X tokens from the available MFA methods.")
            ),
            array(
                "name"        => "piTokenLimitPaper",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "core",
                "default"     => "6",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed have a total of X paper tokens.")
            ),
            array(
                "name"        => "piTokenLimitTotp",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "core",
                "default"     => "6",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed have a total of X TOTP tokens.")
            ),
            array(
                "name"        => "piTokenLimitWebAuthn",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "core",
                "default"     => "4",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Users are only allowed have a total of X WebAuthn tokens.")
            ),
            array(
                "name"        => "piTokenLimitRegistration",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "core",
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
                "name"        => "piAmountOfPaperTokenOTPs",
                "type"        => "integer",
                "check"       => "gosaProperty::isInteger",
                "group"       => "core",
                "default"     => "20",
                "migrate"     => "",
                "mandatory"   => true,
                "description" => _("Amount of OTPs which will be printed on a single TAN list."),
            ),
        );

        return array_merge($privacyidea_settings, $token_limits, $misc);
    }
}
