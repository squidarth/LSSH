CC=gcc
server: ssh_server.c
	$(CC) ssh_server.c crypto.c -o server -lssl -lcrypto -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include\n

crypto_test: crypto_test_2.c
	$(CC) crypto_test_2.c crypto.c -o test_libssl -lssl -lcrypto -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include\n

client: ssh_client.c
	$(CC) ssh_client.c crypto.c -o client -lssl -lcrypto -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include\n

all: client server
