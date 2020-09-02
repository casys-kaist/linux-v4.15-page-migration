#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/mm_inline.h>
#include <linux/page_migration.h>

unsigned long isolate_lru_pages(unsigned long nr_to_scan,
		struct lruvec *lruvec,
		struct list_head *dst_base_page,
		struct list_head *dst_huge_page,
		unsigned long *nr_scanned,
		unsigned long *nr_taken_base_page,
		unsigned long *nr_taken_huge_page,
		isolate_mode_t mode, enum lru_list lru)
{
	struct list_head *src = &lruvec->lists[lru];
	unsigned long nr_taken = 0;
	unsigned long nr_zone_taken[MAX_NR_ZONES] = { 0 };
	unsigned long scan, total_scan, nr_pages;
	LIST_HEAD(busy_list);
	LIST_HEAD(odd_list);

	scan = 0;
	for (total_scan = 0;
	     scan < nr_to_scan && nr_taken < nr_to_scan && !list_empty(src);
	     total_scan++) {
		struct page *page;

		page = lru_to_page(src);

		VM_BUG_ON_PAGE(!PageLRU(page), page);

		/*
		 * Do not count skipped pages because that makes the function
		 * return with no isolated pages if the LRU mostly contains
		 * ineligible pages.  This causes the VM to not reclaim any
		 * pages, triggering a premature OOM.
		 */
		scan++;
		switch (__isolate_lru_page(page, mode)) {
		case 0:
			nr_pages = hpage_nr_pages(page);
			nr_taken += nr_pages;
			nr_zone_taken[page_zonenum(page)] += nr_pages;
			if (nr_pages == 1) {
				list_move(&page->lru, dst_base_page);
				*nr_taken_base_page += nr_pages;
			} else if (nr_pages == HPAGE_PMD_NR){
				list_move(&page->lru, dst_huge_page);
				*nr_taken_huge_page += nr_pages;
			} else {
				list_move(&page->lru, &odd_list);
				*nr_taken_base_page += nr_pages;
			}
			break;

		case -EBUSY:
			/* else it is being freed elsewhere */
			list_move(&page->lru, &busy_list);
			continue;

		default:
			BUG();
		}
	}
	if (!list_empty(&busy_list))
		list_splice(&busy_list, src);

	list_splice_tail(&odd_list, dst_huge_page);

	*nr_scanned = total_scan;
	update_lru_sizes(lruvec, lru, nr_zone_taken);
	return nr_taken;
}

void isolate_all_lru_pages(pg_data_t *pgdat, struct mem_cgroup *memcg,
		struct list_head *base_page_list, struct list_head *huge_page_list,
		unsigned long *num_total_isolated_base_pages,
		unsigned long *num_total_isolated_huge_pages)
{
	enum lru_list lru;
	unsigned long num_pages_to_scan, num_scanned;
	struct lruvec *lruvec = mem_cgroup_lruvec(pgdat, memcg);
	for_each_evictable_lru(lru) {
		unsigned long num_isolated_base_pages = 0, num_isolated_huge_pages = 0;

		spin_lock_irq(&pgdat->lru_lock);
		num_pages_to_scan = lruvec_size_memcg_node(lru, memcg, pgdat->node_id);
		isolate_lru_pages(num_pages_to_scan, lruvec,
				base_page_list, huge_page_list,
				&num_scanned,
				&num_isolated_base_pages, &num_isolated_huge_pages,
				0, lru);
		spin_unlock_irq(&pgdat->lru_lock);

		*num_total_isolated_base_pages += num_isolated_base_pages;
		*num_total_isolated_huge_pages += num_isolated_huge_pages / HPAGE_PMD_NR;
	}
}
