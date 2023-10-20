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

require_once(dirname(__FILE__) . "/interfaces/interface_MFASetupToken.inc");

class MFASetupTOTP implements MFASetupToken
{
    private string $renderResult = "";
    private string $TEMPLATE_PREFIX = "mfatype_totp/";

    public function render($smarty, $multifactorAuth)
    {
        $this->renderResult = $smarty->fetch(
            get_template_path($this->TEMPLATE_PREFIX . "phase00.tpl", true, dirname(__FILE__) . "/..")
        );

        return true;
    }

    public function getRenderResult()
    {
        return $this->renderResult;
    }
}
