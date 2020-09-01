/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_EXCHANGE_H
#define _LINUX_EXCHANGE_H

#include <linux/migrate.h>

struct exchange_page_info {
	struct page *from_page;
	struct page *to_page;

	struct anon_vma *from_anon_vma;
	struct anon_vma *to_anon_vma;

	int from_page_was_mapped;
	int to_page_was_mapped;

	pgoff_t from_index, to_index;

	struct list_head list;
};

int exchange_pages(struct list_head *exchange_list,
        enum migrate_mode mode, int reason);
int do_exchange_page_list_no_putback(
		struct list_head *from_pagelist,
		struct list_head *to_pagelist);
#endif /* _LINUX_EXCHANGE_H */
