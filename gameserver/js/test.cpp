#include <iostream>
#include <sodium.h>
using namespace std;
#define MESSAGE ((const unsigned char *) "test")
#define MESSAGE_LEN 4
int main(){
    unsigned char out[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(out, MESSAGE, 4);
    char out2[crypto_hash_sha256_BYTES*2+1];
    sodium_bin2hex(out2, crypto_hash_sha256_BYTES * 2 + 1,
                     out, crypto_hash_sha256_BYTES);
    cout << out2 << endl;
}
