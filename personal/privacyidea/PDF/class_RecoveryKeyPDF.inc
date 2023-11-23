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

/* Don't load this class automatically */ class RecoveryKeyPDF extends tFPDF
{
    public $HREF;
    public $I;
    public $U;
    public $B;

    public function RecoveryKeyPDF($sn, $givenname, $academictitle, $uid, $mfaRecoveryKey)
    {
        global $config;
        $this->__construct('P', 'mm', 'A4');

        // Add all used variants of Unicode font (uses UTF-8).
        $this->AddFont('DejaVu', '', 'DejaVuSans.ttf', true);
        $this->AddFont('DejaVu', 'B', 'DejaVuSans-Bold.ttf', true);

        // Instanciation of inherited class
        $this->SetTitle(_("Multifactor authentication recovery key"));
        $this->SetAuthor(_("Automatically generated"));
        $this->SetCompression(true);
        $this->SetCreator("GOsa");
        /* FIXME: Describe better what account this is */
        $this->SetSubject(sprintf(_("This document contains information about " .
            "a multifactor authentication recovery key for your account %s."), $uid));
        $this->AliasNbPages();

        // Add first page
        $this->AddPage();

        // Fold marks
        $this->SetLineWidth(0.2);
        $this->Line(4, 106.5, 7, 106.5);
        $this->Line(4, 212.0, 7, 212.0);

        $this->SetX(20);
        $this->SetY(60);

        $this->Ln(15);

        $this->SetFont('DejaVu', 'B', 10);
        $this->Cell(12, 10, _("Subject: Recovery key"));
        $this->Ln(12);
        $this->SetFont('DejaVu', '', 10);
        $this->WriteHTML($this->Salutation($sn, $givenname, $academictitle));
        $this->Ln(12);
        $this->WriteHTML(sprintf(_("A recovery key has been created for your account '%s'."), $uid), 10);
        $this->Ln(6);
        $this->WriteHTML(_("You can use this key one time only as a second factor when logging into our system."), 10);
        $this->Ln(12);
        $this->WriteHTML(sprintf(_("Your recovery key is: <b>%s</b>"), $mfaRecoveryKey), 10);
        $this->Ln(24);
        $this->WriteHTML(_("Best regards"), 10);
    }

    public function PutLink($URL, $txt)
    {
        // Put a hyperlink
        $this->SetTextColor(0, 0, 255);
        $this->SetStyle('U', true);
        $this->Write(5, $txt, $URL);
        $this->SetStyle('U', false);
        $this->SetTextColor(0);
    }

    public function WriteHTML($html, $lh= 5)
    {
        // HTML parser
        $html=str_replace("\n", ' ', $html);
        $a=preg_split('/<(.*)>/U', $html, -1, PREG_SPLIT_DELIM_CAPTURE);
        foreach ($a as $i=>$e) {
            if ($i%2==0) {
                // Text
                if ($this->HREF) {
                    $this->PutLink($this->HREF, $e);
                } else {
                    $this->Write($lh, $e);
                }
            } else {
                // Tag
                if ($e[0] == '/') {
                    $this->CloseTag(strtoupper(substr($e, 1)));
                } else {
                    // Extract attributes
                    $a2=explode(' ', $e);
                    $tag=strtoupper(array_shift($a2));
                    $attr=array();
                    foreach ($a2 as $v) {
                        if (preg_match('/^([^=]*)=["\']?([^"\']*)["\']?$/', $v, $a3)) {
                            $attr[strtoupper($a3[1])]=$a3[2];
                        }
                    }
                    $this->OpenTag($tag, $attr);
                }
            }
        }
    }

    public function SetStyle($tag, $enable)
    {
        // Modify style and select corresponding font
        $this->$tag+=($enable ? 1 : -1);
        $style='';
        foreach (array('B','I','U') as $s) {
            if ($this->$s>0) {
                $style.=$s;
            }
        }
        $this->SetFont('', $style);
    }

    public function OpenTag($tag, $attr)
    {
        // Opening tag
        if ($tag=='B' or $tag=='I' or $tag=='U') {
            $this->SetStyle($tag, true);
        }
        if ($tag=='A') {
            $this->HREF=$attr['HREF'];
        }
        if ($tag=='BR') {
            $this->Ln(5);
        }
    }

    public function CloseTag($tag)
    {
        // Closing tag
        if ($tag=='B' or $tag=='I' or $tag=='U') {
            $this->SetStyle($tag, false);
        }
        if ($tag=='A') {
            $this->HREF='';
        }
    }

    public function PersonNameAndTitle($sn, $givenname, $academictitle)
    {
        $greeter = "";

        if (!empty($academictitle)) {
            $greeter .= $academictitle . " ";
        }

        $greeter .= "$givenname $sn";
        return $greeter;
    }


    public function Salutation($sn, $givenname, $academictitle)
    {
        $greeter = $this->PersonNameAndTitle($sn, $givenname, $academictitle);
        return sprintf(_("Dear %s"), $greeter);
    }

}
