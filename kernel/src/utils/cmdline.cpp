#include "cmdline.hpp"
#include "frg/string.hpp"
#include "term/term.h"

extern "C" void parse_cmdline() {
    frg::string_view cmdline = frg::basic_string_view(limine_cmdline.response->cmdline);
    kprintf_log(STATUSOK, "Nyaux Kernel Command Line: %s, size: %lu\r\n", cmdline.data(), cmdline.size());
    if (cmdline.size() == 0) {
        return; // lit nothing to do lol
    }
    size_t step = cmdline.find_first('=');
    kprintf_log(TRACE, "substring: %s\r\n", cmdline.sub_string(step + 1, cmdline.size() - step - 1).data());
}
