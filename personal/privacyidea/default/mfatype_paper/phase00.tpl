<h1>Phase 0: Paper</h1>
<h1>Theme: default</h1>

TAN-Liste hinzufügen

Wählen Sie eine aussagekräftige Beschreibung, mit der Sie die TAN-Liste wiedererkennen können.
Sie können die Beschreibung später anpassen.

{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
<input type="hidden" id="add_token" name="add_token" value="yes">
<input type="hidden" id="token_type" name="token_type" value="paper">
<input type="hidden" id="current_phase" name="current_phase" value="0">

{* Use a little hack, remove 'add_token' input, so that mfaAccount doesn't think we are in a token setup. *}
<button class="btn primary" onclick="document.getElementById('add_token').remove();" type="submit">{t}Cancel{/t}</button>
<button class="btn primary" type="submit">{t}Next step{/t}</a>
