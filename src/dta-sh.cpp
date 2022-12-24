// Intel Pin.
#include "pin.H"

// libdft64
#include "branch_pred.h"
#include "libdft_api.h"

int main(int argc, char** argv) {
    PIN_InitSymbols();

    if (unlikely(PIN_Init(argc, argv))) {
        return 1;
    }

    if (unlikely(libdft_init()) != 0) {
        libdft_die();
        return 1;
    }

    PIN_StartProgram();

    return 0;
}
