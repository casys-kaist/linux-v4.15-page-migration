#include <linux/memcontrol.h>
#include <linux/page_idle.h>
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

static int page_age_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct page *page_a = container_of(a, struct page, lru);
	struct page *page_b = container_of(b, struct page, lru);
	if (page_a->age != page_b->age)
		return page_a->age > page_b->age;
	else
		return page_a->access_frequency > page_b->access_frequency;
}

static int page_access_frequency_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct page *page_a = container_of(a, struct page, lru);
	struct page *page_b = container_of(b, struct page, lru);
	if (page_a->access_frequency != page_b->access_frequency)
		return page_a->access_frequency < page_b->access_frequency;
	else
		return page_a->age < page_b->age;
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

void do_migrate_lru(struct mem_cgroup *memcg)
{
	do_migrate_with_metric(memcg, page_age_cmp);
}

void do_migrate_lfu(struct mem_cgroup *memcg)
{
	do_migrate_with_metric(memcg, page_access_frequency_cmp);
}

void do_migrate_amp(struct mem_cgroup *memcg)
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

	// sort and migrate pages
	if (memcg->migration_policy == MIG_POLICY_PSEUDO_RANDOM) {
		list_sort(NULL, &base_page_list, page_age_cmp);
		list_sort(NULL, &huge_page_list, page_age_cmp);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_base_pages, &base_page_list, false,
				MIG_POLICY_LRU, false);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_huge_pages, &huge_page_list, true,
				MIG_POLICY_LRU, false);

		list_sort(NULL, &base_page_list, page_access_frequency_cmp);
		list_sort(NULL, &huge_page_list, page_access_frequency_cmp);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_base_pages, &base_page_list, false,
				MIG_POLICY_LFU, false);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_huge_pages, &huge_page_list, true,
				MIG_POLICY_LFU, false);

		list_sort(NULL, &base_page_list, page_pseudo_random_cmp);
		list_sort(NULL, &huge_page_list, page_pseudo_random_cmp);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_base_pages, &base_page_list, false,
				MIG_POLICY_PSEUDO_RANDOM, true);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_huge_pages, &huge_page_list, true,
				MIG_POLICY_PSEUDO_RANDOM, true);
	} else if (memcg->migration_policy == MIG_POLICY_LRU) {
		list_sort(NULL, &base_page_list, page_access_frequency_cmp);
		list_sort(NULL, &huge_page_list, page_access_frequency_cmp);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_base_pages, &base_page_list, false,
				MIG_POLICY_LFU, false);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_huge_pages, &huge_page_list, true,
				MIG_POLICY_LFU, false);

		list_sort(NULL, &base_page_list, page_age_cmp);
		list_sort(NULL, &huge_page_list, page_age_cmp);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_base_pages, &base_page_list, false,
				MIG_POLICY_LRU, true);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_huge_pages, &huge_page_list, true,
				MIG_POLICY_LRU, true);
	} else if (memcg->migration_policy == MIG_POLICY_LFU) {
		list_sort(NULL, &base_page_list, page_age_cmp);
		list_sort(NULL, &huge_page_list, page_age_cmp);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_base_pages, &base_page_list, false,
				MIG_POLICY_LRU, false);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_huge_pages, &huge_page_list, true,
				MIG_POLICY_LRU, false);

		list_sort(NULL, &base_page_list, page_access_frequency_cmp);
		list_sort(NULL, &huge_page_list, page_access_frequency_cmp);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_base_pages, &base_page_list, false,
				MIG_POLICY_LFU, true);
		meet_fast_memory_ratio(memcg,
				num_total_isolated_huge_pages, &huge_page_list, true,
				MIG_POLICY_LFU, true);
	}

	// putback pages
	putback_movable_pages(&base_page_list);
	putback_movable_pages(&huge_page_list);
}
