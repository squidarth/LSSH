# LSSH - A less secure SSH

## Summary

This is highly WIP library for a C program for executing
commands on a remote computer.

### Build Steps

* Building the server: `make server`
* Running the server: `make run_server`
* Building the client: `make client`

[NOT IMPLEMENTED] Running the client: ./client HOST PORT

## Code Notes

### crypto.h

crypto.h contains a few functions that are wrappers around
OpenSSL functionality for generating keys and performing
encryption for Elliptic Curve Diffie Hellman.

Check out `crypto_test_2.c` for specific details on how to
use these functions.

* `encrypt_data` accepts plaintext, a key, initialization vector, and an allocated buffer for the ciphertext, and writes the ciphertext to the buffer. *Does not allocate any memory*.

* `decrypt` accepts ciphertext, a key, initialization vector, and an allocated buffer for the plaintext, and writes the plaintext to the buffer. *Does not allocate any memory*.

* `gen` accepts no arguments, but returns an allocated EVP_PKEY\*, which needs to be freed later using the function EVP_PKEY_free.

* `derive` is the function that given an EVP_KEY object returns the secret key. The trick here is that it takes the EVP_PKEY, along with the public key of the peer, and returns a secret. **This allocates memory**, and you should be careful to free it using
`OPENSSL_FREE`.

#### Sidenotes

An important note is that the function `i2d_PUBKEY` from openssl is used for extracting public keys fromt the `EVP_PKEY` objects. There is example usage of this in crypto_test_2.c.

## TODO
 
* Implement RSA authentication
