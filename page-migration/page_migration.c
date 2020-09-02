#include <linux/memcontrol.h>
#include <linux/exchange.h>
#include <linux/page_migration.h>

/* allocate a new page from the requested node. */
struct page *new_node_page(struct page *page, unsigned long node, int **x)
{
	if (PageHuge(page))
		return alloc_huge_page_node(page_hstate(compound_head(page)),
					node);
	else if (thp_migration_supported() && PageTransHuge(page)) {
		struct page *thp;

		thp = alloc_pages_node(node,
			(GFP_TRANSHUGE | __GFP_THISNODE),
			HPAGE_PMD_ORDER);
		if (!thp)
			return NULL;
		prep_transhuge_page(thp);
		return thp;
	} else
		return __alloc_pages_node(node, GFP_HIGHUSER_MOVABLE |
						    __GFP_THISNODE, 0);
}

void meet_fast_memory_ratio(struct mem_cgroup *memcg,
        unsigned long num_pages, struct list_head *page_list, bool is_huge,
		unsigned int migration_policy, bool migrate)
{
	LIST_HEAD(fast_page_list);
	LIST_HEAD(slow_page_list);
	LIST_HEAD(fast_to_slow_page_list);
	LIST_HEAD(slow_to_fast_page_list);
	unsigned long num_fast_pages,
				  num_target_fast_pages, num_target_slow_pages;
	struct page *page, *next;

	if (migrate)
		add_num_total_pages(memcg, num_pages, is_huge);

	// find the number of pages in slow memory
	num_target_fast_pages = (num_pages * memcg->fast_memory_ratio) / (100 * 10);
	num_target_slow_pages = num_pages - num_target_fast_pages;

	// find correct page locations
	num_fast_pages = 0;
	list_for_each_entry_safe(page, next, page_list, lru) {
		if (num_fast_pages < num_target_fast_pages) {
			list_move(&page->lru, &fast_page_list);
		} else {
			list_move(&page->lru, &slow_page_list);
		}
		num_fast_pages++;
	}

	// remove pages that are in the correct location already
	list_for_each_entry_safe(page, next, &fast_page_list, lru) {
		if (migration_policy == MIG_POLICY_LRU)
			page->lru_nid = FAST_NODE_ID;
		if (migration_policy == MIG_POLICY_LFU)
			page->lfu_nid = FAST_NODE_ID;

		if (pfn_to_nid(page_to_pfn(page)) == FAST_NODE_ID) {
			list_move(&page->lru, page_list);
		} else {
			if (migrate)
				inc_num_page_migrations_slow_to_fast(memcg, is_huge);
		}
	}
	list_for_each_entry_safe(page, next, &slow_page_list, lru) {
		if (migration_policy == MIG_POLICY_LRU)
			page->lru_nid = SLOW_NODE_ID;
		if (migration_policy == MIG_POLICY_LFU)
			page->lfu_nid = SLOW_NODE_ID;

		if (pfn_to_nid(page_to_pfn(page)) == SLOW_NODE_ID) {
			list_move(&page->lru, page_list);
		} else {
			if (migrate)
				inc_num_page_migrations_fast_to_slow(memcg, is_huge);
		}
	}

	// migrate pages
	if (migrate) {
		if (!list_empty(&fast_page_list) && !list_empty(&slow_page_list))
			do_exchange_page_list_no_putback(&fast_page_list, &slow_page_list);
		if (!list_empty(&fast_page_list))
			migrate_pages(&fast_page_list, new_node_page, NULL, FAST_NODE_ID,
					MIGRATE_ASYNC, MR_PAGE_MIGRATION_SLOW_TO_FAST);
		if (!list_empty(&slow_page_list))
			migrate_pages(&slow_page_list, new_node_page, NULL, SLOW_NODE_ID,
					MIGRATE_ASYNC, MR_PAGE_MIGRATION_FAST_TO_SLOW);
	}
	list_splice(&fast_page_list, page_list);
	list_splice(&slow_page_list, page_list);
}
