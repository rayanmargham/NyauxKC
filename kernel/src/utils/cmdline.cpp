#include "cmdline.hpp"
#include "cppglue/glue.hpp"
#include "frg/vector.hpp"
#include <frg/span.hpp>
#include "utils/basic.h"
#include "utils/libc.h"
#include <span>

void parse_cmd(frg::string_view cmdline) {
		if (cmdline.size() == 0) {
				return; // lit nothing to do lol
			}
			// fuck=test blah=blah
			// no
			// let me break it down for you mark
			// breaks it down
			// ok so basically get our first shit
			const char *path = cmdline.data();
			;
			frg::vector<cmdlineobj, frgalloc<cmdlineobj>> yo(frgalloc<cmdlineobj>{});
			while (*path) {
				while (*path && *path == ' ') {
					path += 1;
				}
				const char *start = path;
				while (*path && *path != ' ') {
					path += 1;
				}
				size_t len = path - start;
				if (len == 0) {
					continue;
				}
				char *component = static_cast<char *>(kmalloc(len + 1));
				memcpy(component, start, len);
				component[len] = '\0';
				size_t him = frg::string_view(component).find_first('=');
				bool right = true;
				if (him == 0) {
					kfree(component, len + 1);
					continue; // not valid option, =shit is not valid
				} else if (static_cast<int>(him) == -1) {
					// valid but fuck you
					right = false;
				}
				char *left = component;
				if (right == true) {
					left = static_cast<char *>(kmalloc(him + 1));
					left[him] = '\0';

					memcpy(left, component, him);
				}

				if (right && *(component + him + 1)) {
					size_t right_len = len - him - 1;
					char *right_str = static_cast<char *>(kmalloc(right_len + 1));
					right_str[right_len] = '\0';
					memcpy(right_str, component + him + 1, right_len);
					auto attemptnumber = frg::string_view(right_str).to_number<size_t>();

					cmdlineobj tobepushed = {
							.key = strdup((const char*)left),
						 .type = None
					};
					if (strcmp(right_str, "true") == 0 || strcmp(right_str, "false") == 0) {
							tobepushed.type = Bool;
												if (strcmp(right_str, "true") == 0) {
														tobepushed.condition = true;

												} else if (strcmp(right_str, "false") == 0) {
														tobepushed.condition = false;
												}
					} else if (attemptnumber.has_value()) {
							tobepushed.num = attemptnumber.value();
							tobepushed.type = Number;
					} else {
							tobepushed.type = String;
							tobepushed.str = strdup(static_cast<const char*>(right_str));
					}


					yo.push(tobepushed);

					kfree(left, him + 1);
					kfree(right_str, right_len + 1);
				} else {
						cmdlineobj push = {
								.key = strdup(static_cast<const char*>(left)),
								.type = None,
						};
						yo.push(push);
				}

				kfree(component, len + 1);
			}

			for (cmdlineobj &obj : yo) {
					// Access fields of object

					kprintf("key %s -> ", obj.key);
					if (obj.type == cmdType::Bool) {
							const char *choose = obj.condition ? "true" : "false";
							kprintf("bool: %s\r\n", choose);
					} else if (obj.type == cmdType::Number) {
							kprintf("number: %lu\r\n", obj.num);
					} else if (obj.type == cmdType::String) {
							kprintf("string: %s\r\n", obj.str);
					} else {
							kprintf("None");
					}
			}
			info.size = yo.size();
			cmdlineobj *funny = static_cast<cmdlineobj*>(kmalloc(yo.size() * sizeof(cmdlineobj)));
			memcpy(funny, yo.data(), yo.size() * sizeof(cmdlineobj));
			info.cmdarray = funny;
}
// Look for cmdline Option
// returns null if not found
// returns a ptr to a struct cmdlineobj if otherwise
extern "C" void *look_for_option(const char *key) {
	auto spam = std::span(info.cmdarray, info.size);
	for (cmdlineobj &obj : spam) {
		if (strcmp(key, obj.key) == 0) {
			return static_cast<void*>(&obj);
		}
	}
	return NULL;
} 
extern "C" void parse_cmdline() {
	frg::string_view cmdline =
			frg::basic_string_view(limine_cmdline.response->cmdline);
	kprintf_log(STATUSOK, "Nyaux Kernel Command Line: [%s] size: [%lu]\r\n",
							cmdline.data(), cmdline.size());
	parse_cmd(cmdline);
}
