<h1>Phase 3: Paper</h1>
<h1>Theme: default</h1>

TAN-Liste hinzugefügt

Die TAN-Liste $BESCHREIBUNG wurde erfolgreich zu Ihrer Uni-ID hinzugefügt und kann ab sofort genutzt werden.

{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
<input type="hidden" id="add_token" name="add_token" value="yes">
<input type="hidden" id="token_type" name="token_type" value="paper">
<input type="hidden" id="current_phase" name="current_phase" value="3">

{* Use a little hack, remove 'add_token' input, so that mfaAccount doesn't think we are in a token setup. *}
<button class="btn primary" onclick="document.getElementById('add_token').remove();" type="submit">{t}Cancel{/t}</button>
<button class="btn primary" type="submit">{t}Next step{/t}</a>
