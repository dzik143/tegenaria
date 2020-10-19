rem
rem Private key.
rem

openssl genrsa -des3 -out server.key 2048              

rem
rem Public key from private.
rem

rem openssl rsa -in server.key -out client.key -outform PEM

rem
rem Generate a certificate signing request (CSR)
rem

openssl req -new -key server.key -out server.csr

rem
rem Generate self signed certificate
rem

openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt