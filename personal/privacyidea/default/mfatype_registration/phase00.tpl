<h1>Phase 0: Registration</h1>
<h1>Theme: default</h1>

{* These hidden inputs should always get send via _POST, so that mfaAccount knows which type of token setup we want. *}
<input type="hidden" id="add_token" name="add_token" value="yes">
<input type="hidden" id="token_type" name="token_type" value="registration">

{* Use a little hack, remove 'add_token' input, so that mfaAccount doesn't think we are in a token setup. *}
<button class="btn primary"
    onclick="document.getElementById('add_token').remove();"
    type="submit">{t}Back{/t}
</button>
