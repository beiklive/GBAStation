#include <openssl/ui.h>

UI_METHOD* UI_OpenSSL(void) {
    return (UI_METHOD*)UI_null();
}
