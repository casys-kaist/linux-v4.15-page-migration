#include <linux/memcontrol.h>
#include <linux/list_sort.h>
#include <linux/migrate.h>
#include <linux/page_migration.h>

static int page_pure_random_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct page *page_a = container_of(a, struct page, lru);
	struct page *page_b = container_of(b, struct page, lru);
	unsigned long page_a_rand_val
		= (unsigned long) page_a ^ (unsigned long) page_to_pfn(page_a);
	unsigned long page_b_rand_val
		= (unsigned long) page_b ^ (unsigned long) page_to_pfn(page_b);
	return page_a_rand_val > page_b_rand_val;
}

static int page_pseudo_random_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct page *page_b = container_of(b, struct page, lru);
	int nid_b = pfn_to_nid(page_to_pfn(page_b));
	return nid_b == FAST_NODE_ID;
}

void do_migrate_with_metric(struct mem_cgroup *memcg,
		int (*sort) (void *, struct list_head *, struct list_head *))
{
	int nid;
	LIST_HEAD(base_page_list);
	LIST_HEAD(huge_page_list);
	unsigned long num_total_isolated_base_pages = 0, num_total_isolated_huge_pages = 0;

	// isolate all pages
	for (nid = 0; nid < num_online_nodes(); nid++) {
		pg_data_t *pgdat = NODE_DATA(nid);
		unsigned long num_isolated_base_pages = 0, num_isolated_huge_pages = 0;
		isolate_all_lru_pages(pgdat, memcg,
				&base_page_list, &huge_page_list,
				&num_isolated_base_pages, &num_isolated_huge_pages);
		num_total_isolated_base_pages += num_isolated_base_pages;
		num_total_isolated_huge_pages += num_isolated_huge_pages;
	}

	// sort pages
	list_sort(NULL, &base_page_list, sort);
	list_sort(NULL, &huge_page_list, sort);

	// migrate pages
	meet_fast_memory_ratio(memcg,
			num_total_isolated_base_pages, &base_page_list, false,
			memcg->migration_policy, true);
	meet_fast_memory_ratio(memcg,
			num_total_isolated_huge_pages, &huge_page_list, true,
			memcg->migration_policy, true);

	// putback pages
	putback_movable_pages(&base_page_list);
	putback_movable_pages(&huge_page_list);
}

void do_migrate_pure_random(struct mem_cgroup *memcg)
{
	do_migrate_with_metric(memcg, page_pure_random_cmp);
}

void do_migrate_pseudo_random(struct mem_cgroup *memcg)
{
	do_migrate_with_metric(memcg, page_pseudo_random_cmp);
}
