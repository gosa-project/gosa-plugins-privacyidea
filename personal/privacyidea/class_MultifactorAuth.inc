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

class MultifactorAuth extends plugin
{
    // Plugin definitions
    public $plHeadline    = "Multifactor authentication";
    public $plDescription = "Manage Multifactor authentication";
    public $access        = "";

    // Attribute list for save action
    public $attributes    = [];
    public $objectclasses = [];
    private $view_logged = false;

    public $config    = null;
    public $initTime  = null;
    private $ldap     = null;
    private $ldapinfo = null;
    private $utils    = null;

    public function __construct(&$config, $dn = null)
    {
        $this->initTime = microtime(true);

        /* Include config object */
        $this->config = &$config;

        // Initialize utils class
        $this->utils = new PrivacyIdeaUtils($config);

        $this->ui = get_userinfo();
        stats::log(
            'plugin',
            $class = get_class($this),
            $category = array($this-> acl_category),
            $action = 'open',
            $amount = 1,
            $duration = (microtime(true) - $this->initTime),
        );
    }

    public function execute()
    {
        /* Call parent execute */
        plugin::execute();

        /* Log view */
        if ($this->is_account && !$this->view_logged) {
            $this->view_logged = true;
            new log("view", "users/" . get_class($this), $this->dn);
        }

        // Initialize smarty
        $smarty = get_smarty();

        // Initialize LDAP
        $this->ldap = $this->config->get_ldap_link();
        if (!is_array($this->ldapinfo)) {
            $this->ldapinfo = [];
        }

        // Showing this template next…
        $return_template = null;

        // Which Phase are we loading?
        $tmpl_path = "phase0.tpl";
        #$tmpl_path = "phase1.tpl";

        // Test:
        $this->utils->validateCheck("testuser", "000000", "testrealm");

        // Show introduction page…
        $return_template = $smarty->fetch(get_template_path($tmpl_path, true, dirname(__FILE__)));

        return $return_template;
    }

    /*!
     * \brief  Save data to object
     */
    public function save_object()
    {
        plugin::save_object();
    }

    /*!
     * \brief  Removes the mailAccount extension from ldap
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
            msg_dialog::display(_("LDAP error"), msgPool::ldaperror($ldap->get_error(), $this->dn, LDAP_MOD, get_class()));
        }

        /* Optionally execute a command after we are done */
        $this->handle_post_events("remove", array("uid" => $this->uid));
    }

    /*!
     * \brief  Save the MultifactorAuth settings to the ldap database.
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
            msg_dialog::display(_("LDAP error"), msgPool::ldaperror($ldap->get_error(), $this->dn, LDAP_MOD, get_class()));
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

    /*!
     * \brief  Check given values
     */
    public function check()
    {
        if (!$this->is_account) {
            return (array ());
        }

        $ldap = $this->config->get_ldap_link();

        // Call common method to give check the hook
        $message = plugin::check();

        // $message[] = msgPool::required(_("Mail address"));
        $message[] = "check()";

        return ($message);
    }

    /*!
     * \brief  Adapt from template, using 'dn'
     */
    public function adapt_from_template($dn, $skip = array ())
    {
        plugin::adapt_from_template($dn, $skip);
    }
}
