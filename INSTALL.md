# Installation

After installation of the .deb package gosa-plugin-privacyidea, you
need to add the following lines to /etc/gosa/gosa.conf (e.g. inside of
the `<usertabs>` entry):

```xml
     <!-- PrivacyIDEA Plugin -->
     <tab class="mfaAccount" name="Multifactor Auth"/>
```

also add the following inside of the `<MyAccountTabs>` entry:

```xml
     <!-- PrivacyIDEA Plugin -->
     <tab class="mfaAccount" name="Multifactor Auth"/>
```

also please add these lines to `/etc/ldap/slapd.conf` under the `## gosa:`
section at the beginning of the file:
```bash
# PrivacyIDEA Plugin
include /etc/ldap/schema/gosa/mfa.schema
```

For the changes to take effect, restart your *http daemon* (e.g. the Apache web server)
and your *LDAP server* (e.g. slapd). **Thank you** for using gosa-plugin-privacyidea.
You can contribute to this project at https://github.com/gosa-project/gosa-plugins-privacyidea/
