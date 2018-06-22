
server: ssh_server.c
	gcc ssh_server.c -o server -lssl -lcrypto -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include\n

crypto_test: crypto_test_2.c
	gcc crypto_test_2.c -o test_libssl -lssl -lcrypto -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include\n

client: ssh_client.c
	gcc ssh_client.c -o client -lssl -lcrypto -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include\n

all: client server
