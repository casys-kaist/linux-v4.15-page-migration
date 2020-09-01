#ifndef __PAGE_MIGRATION_H__
#define __PAGE_MIGRATION_H__

#define FAST_NODE_ID 0
#define SLOW_NODE_ID 1

enum migration_policy {
	MIG_POLICY_NOP = 0,
	MIG_POLICY_PURE_RANDOM,
	MIG_POLICY_PSEUDO_RANDOM,
	MIG_POLICY_MODIFIED_LRU_LISTS,
	NUM_MIG_POLICIES
};

// common
unsigned long isolate_lru_pages(unsigned long nr_to_scan,
		struct lruvec *lruvec,
		struct list_head *dst_base_page,
		struct list_head *dst_huge_page,
		unsigned long *nr_scanned,
		unsigned long *nr_taken_base_page,
		unsigned long *nr_taken_huge_page,
		isolate_mode_t mode, enum lru_list lru);
void isolate_all_lru_pages(pg_data_t *pgdat, struct mem_cgroup *memcg,
		struct list_head *base_page_list, struct list_head *huge_page_list,
		unsigned long *num_isolated_base_pages, unsigned long *num_isolated_huge_pages);

// page migration policies
void do_migrate_with_metric(struct mem_cgroup *memcg,
		int (*sort) (void *, struct list_head *, struct list_head *));
void do_migrate_pure_random(struct mem_cgroup *memcg);
void do_migrate_pseudo_random(struct mem_cgroup *memcg);
void do_migrate_modified_lru_lists(struct mem_cgroup *memcg);

// hotness tracking
void shrink_lists(struct mem_cgroup *memcg);

// page migration
struct page *new_node_page(struct page *page, unsigned long node, int **x);
void meet_fast_memory_ratio(struct mem_cgroup *memcg,
		unsigned long num_pages, struct list_head *page_list, bool is_huge,
		unsigned int migration_policy, bool migrate);

#endif	/* __PAGE_MIGRATION_H__ */
