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

/**
 * This class is a collection of useful/common functions and variables
 * used by every MFAToken subclass. Every MFAToken subclass should extend this
 * class and implement the interface MFAToken.
 */
class MFATokenHelper
{
    /** @var string */
    public $renderResult = "";
    /** @var Smarty */
    public $smarty;
    /** @var string */
    public $TEMPLATE_PREFIX = "/";
    /** @var int Indicates how many phases the token setup has. */
    public $MAX_SETUP_PHASES = 0;

    public function renderSetup()
    {
        // Do not try to catch any of the exceptions call_user_func throws.
        // If it throws smth, then that must mean the programmer f'ed smth up while developing.

        $currentPhase = $this->getPOSTPhaseNumber();

        if ($currentPhase === false) {
            // This happens if data got corrupted/altered and if people click on the setup button in the MFA intro.
            $nextPhaseNumber = 0;
            $currentPhase = 0;
            $evaluationSuccess = true;
        } else {
            $nextPhaseNumber = $currentPhase + 1;
            $evaluationSuccess = call_user_func(array($this, "evaluatePhase" . $currentPhase));
        }

        if (!$evaluationSuccess) {
            // Oh, something did go wrong. Do not show next phase.
            $nextPhaseNumber = $currentPhase;
            call_user_func(array($this, "preparePhase" . $currentPhase));
        } else {
            $prepareSuccess = call_user_func(array($this, "preparePhase" . $nextPhaseNumber));

            if (!$prepareSuccess) {
                $nextPhaseNumber = $currentPhase;
                if (!call_user_func(array($this, "preparePhase" . $currentPhase))) {
                    return false; // Ok, let's just bail out of here.
                }
            }
        }

        $templateFileName = "phase" . str_pad($nextPhaseNumber, 2, "0", STR_PAD_LEFT) . ".tpl";
        $this->renderResult = $this->smarty->fetch(
            get_template_path($this->TEMPLATE_PREFIX . $templateFileName, true, dirname(__FILE__) . "/..")
        );

        return true;
    }

    public function getRenderSetupResult()
    {
        return $this->renderResult;
    }

    public function getPOSTPhaseNumber()
    {
        if (!isset($_POST["current_phase"])) {
            return false;
        }

        if (!is_numeric($_POST["current_phase"])) {
            return false;
        }

        $currentPhase = intval($_POST["current_phase"]);

        if ($currentPhase < 0 || $currentPhase >= $this->MAX_SETUP_PHASES - 1) {
            return false;
        }

        return $currentPhase;
    }
}