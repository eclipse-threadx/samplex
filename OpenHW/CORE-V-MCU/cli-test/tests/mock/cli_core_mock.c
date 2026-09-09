#include "cli.h"

void CLI_uint32_required(const char *name, uint32_t *puthere)
{
    (void)name;
    *puthere = 0U;
}

void CLI_submenu_handler(const struct cli_cmd_entry *pEntry)
{
    (void)pEntry;
}
