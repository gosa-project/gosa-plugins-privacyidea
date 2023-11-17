# Installation

## Install plugin in GOsa²
After installation of the .deb package gosa-plugin-privacyidea, you
need to adjust /etc/gosa/gosa.conf as shown by this patch:

```
diff --git a/contrib/gosa.conf b/contrib/gosa.conf
index 6e7e908b4..113e8d689 100644
--- a/contrib/gosa.conf
+++ b/contrib/gosa.conf
@@ -105,6 +105,7 @@
   <usertabs>
      <tab class="user" name="Generic" />
      <tab class="posixAccount" name="POSIX" />
+     <tab class="mfaAccount" name="Multifactor Authentication" />
      <tab class="sambaAccount" name="Samba" />
      <tab class="netatalk" name="Netatalk" />
      <tab class="mailAccount" name="Mail" />
@@ -121,6 +122,7 @@
   <!-- User dialog -->
   <MyAccountTabs>
      <tab class="user" name="Generic" />
+     <tab class="mfaAccount" name="Multifactor Authentication" />
      <tab class="posixAccount" name="POSIX" />
      <tab class="sambaAccount" name="Samba" />
      <tab class="netatalk" name="Netatalk" />
@@ -288,6 +290,7 @@
   <!-- Role tabs -->
   <roletabs>
     <tab class="roleGeneric" name="Generic"/>
+    <tab class="mfaAccount" name="Multifactor Authentication" />
     <tab class="DynamicLdapGroup" name="Dynamic object" />
   </roletabs>
 
@@ -387,6 +390,18 @@
         forceSSL="false"
         forceGlobals="true"
         ignoreLdapProperties="false"
+        piServer="https://my.privacyidea.tld"
+        piServiceRealm="<admin-realm>"
+        piServiceAccount="<pi-admin-for-gosa>"
+        piServicePass="<password>"
+        piUserRealm="<user-realm>"
+        piTokenOrigin="https://my.gosasite.tld"
+        piTokenLimitAll="10"
+        piTokenLimitPaper="6"
+        piTokenLimitTotp="6"
+        piTokenLimitWebAuthn="4"
+        piTokenLimitRegistration="0"
+        piAmountOfPaperTokenOTPs="20"
 {if $cv.rfc2307bis}
         rfc2307bis="true"
 {else}
```

also please add these lines to `/etc/ldap/slapd.conf` under the `## gosa:`
section at the beginning of the file:
```bash
# privacyIDEA Plugin
include /etc/ldap/schema/gosa/mfa.schema
```

For the changes to take effect, restart your *http daemon* (e.g. the Apache web server)
and your *LDAP server* (e.g. slapd).

## privacyIDEA policies
This plugin supports the 'verify_enrollment' policy. Please have a look at:
https://privacyidea.readthedocs.io/en/latest/policies/enrollment.html?highlight=verify_enrollment#verify-enrollment

**Thank you** for using gosa-plugin-privacyidea.
You can contribute to this project at https://github.com/gosa-project/gosa-plugins-privacyidea/
