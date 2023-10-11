# Installation

After installation of the .deb package gosa-plugin-privacyidea, you
need to add the following lines to /etc/gosa/gosa.conf (e.g. inside of
the `<usertabs>` entry):

```xml
     <!-- PrivacyIDEA Plugin -->
     <tab class="MultifactorAuth" name="Multifactor Auth"/>
```

also add the following inside of the `<MyAccountTabs>` entry:

```xml
     <!-- PrivacyIDEA Plugin -->
     <tab class="MultifactorAuth" name="Multifactor Auth"/>
```

For the changes to take effect, restart your http daemon (e.g. the Apache web server).
**Thank you** for using gosa-plugin-privacyidea. You can contribute to this project at
https://github.com/gosa-project/gosa-plugins-privacyidea/
