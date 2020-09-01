#include <linux/memcontrol.h>
#include <linux/page_migration.h>
#include "../mm/internal.h"

static void shrink_lists_node_memcg(pg_data_t *pgdat, struct mem_cgroup *memcg,
		unsigned int act_scan_ratio, unsigned int inact_scan_ratio)
{
	enum lru_list lru;
	struct lruvec *lruvec = mem_cgroup_lruvec(pgdat, memcg);
	struct scan_control sc = {
		.reclaim_idx = MAX_NR_ZONES - 1,
		.priority = DEF_PRIORITY,
		.may_unmap = 1
	};

	for_each_evictable_lru(lru) {
		unsigned long num_pages_to_scan = lruvec_size_memcg_node(lru, memcg, pgdat->node_id);
		if (is_active_lru(lru)) {
			num_pages_to_scan = (num_pages_to_scan * act_scan_ratio) / 100;
			shrink_active_list(num_pages_to_scan, lruvec, &sc, lru);
		} else {
			num_pages_to_scan = (num_pages_to_scan * inact_scan_ratio) / 100;
			shrink_inactive_list(num_pages_to_scan, lruvec, &sc, lru);
		}
	}
}

void shrink_lists(struct mem_cgroup *memcg)
{
	int nid;
	for (nid = 0; nid < num_online_nodes(); nid++) {
		shrink_lists_node_memcg(NODE_DATA(nid), memcg,
				memcg->modified_lru_lists.act_scan_ratio,
				memcg->modified_lru_lists.inact_scan_ratio);
	}
}

static int page_modified_lru_lists_cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct page *page_a = container_of(a, struct page, lru);
	struct page *page_b = container_of(b, struct page, lru);
	int nid_b = pfn_to_nid(page_to_pfn(page_b));
	bool active_a = PageActive(page_a);
	bool active_b = PageActive(page_b);

	if (!active_a && active_b)
		return true;

	if (active_a == active_b) {
		if (nid_b == FAST_NODE_ID)
			return true;
	}

	return false;
}

void do_migrate_modified_lru_lists(struct mem_cgroup *memcg)
{
	do_migrate_with_metric(memcg, page_modified_lru_lists_cmp);
}
