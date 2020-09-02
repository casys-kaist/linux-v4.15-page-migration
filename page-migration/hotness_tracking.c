#include <linux/memcontrol.h>
#include <linux/page_idle.h>
#include <linux/migrate.h>
#include <linux/page_migration.h>
#include "../mm/internal.h"

static void __update_age(struct mem_cgroup *memcg,
        struct list_head *page_list, bool is_huge)
{
	struct page *page;
	list_for_each_entry(page, page_list, lru) {
		page_idle_clear_pte_refs(page);
		if (page_is_idle(page)) {
			// miss
			if (page->age < S32_MAX)
				page->age++;
		} else {
			// hit
			page->age = 0;
			set_page_idle(page);
		}
	}
}

void update_age(struct mem_cgroup *memcg)
{
	int nid;
	LIST_HEAD(base_page_list);
	LIST_HEAD(huge_page_list);

	// isolate all pages
	for (nid = 0; nid < num_online_nodes(); nid++) {
		pg_data_t *pgdat = NODE_DATA(nid);
		unsigned long num_isolated_base_pages = 0, num_isolated_huge_pages = 0;
		isolate_all_lru_pages(pgdat, memcg,
				&base_page_list, &huge_page_list,
				&num_isolated_base_pages, &num_isolated_huge_pages);
	}

	// check access bits
	__update_age(memcg, &base_page_list, false);
	__update_age(memcg, &huge_page_list, true);

	// putback pages
	putback_movable_pages(&base_page_list);
	putback_movable_pages(&huge_page_list);
}
