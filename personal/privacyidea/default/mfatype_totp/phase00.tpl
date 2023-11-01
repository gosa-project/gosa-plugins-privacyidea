<h1>Phase 0: TOTP</h1>
<h1>Theme: default</h1>

Authentifizierungsapp hinzufügen

Eine  Authentifizierungsapp auf Ihrem Smartphone generiert zeitbasierte Codes (TOTP), die bei jeder Anmeldung abgelesen und eingegeben werden müssen.

{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
<input type="hidden" id="add_token" name="add_token" value="yes">
<input type="hidden" id="token_type" name="token_type" value="totp">
<input type="hidden" id="current_phase" name="current_phase" value="0">

{* Use a little hack, remove 'add_token' input, so that mfaAccount doesn't think we are in a token setup. *}
<button class="btn primary" onclick="document.getElementById('add_token').remove();" type="submit">{t}Cancel{/t}</button>
<button class="btn primary" type="submit">{t}Next step{/t}</a>
