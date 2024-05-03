#include <stdint.h>
#include <stdbool.h>
#include "prg_api.h"

void prg_otp_read(uint16_t offset, uint16_t dlen, uint8_t *buff)
{

}

bool prg_otp_verify(uint16_t offset, uint16_t dlen, uint8_t *data)
{
    return false;
}

bool prg_otp_write(uint16_t offset, uint16_t dlen, uint8_t *data)
{
    return false;
}
