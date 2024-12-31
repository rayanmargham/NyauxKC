#pragma once
#include <uacpi/acpi.h>
#include <uacpi/event.h>
#include <uacpi/io.h>
#include <uacpi/kernel_api.h>
#include <uacpi/namespace.h>
#include <uacpi/opregion.h>
#include <uacpi/resources.h>
#include <uacpi/tables.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>

#include "term/term.h"
void initecfromecdt();
void ec_init();
