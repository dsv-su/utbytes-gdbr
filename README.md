## Usage
Request to:
```bash
https://$webroot/gdpr/export/email=%username%&pn=%pn&ticket=%ticket%
```
This will create a .zip archive with .csv data and all attachments. You need to supply either **pn** or **email**. You must supply a **ticket**, alternatively it can be sent via header.
