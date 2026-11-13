#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
bool ConvertStringToUI64(const char *str, uint64_t *val);

#endif
