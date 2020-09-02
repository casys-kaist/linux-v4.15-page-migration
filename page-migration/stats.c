#include <linux/mm.h>
#include <linux/huge_mm.h>
#include <linux/memcontrol.h>

void add_num_total_pages(struct mem_cgroup *memcg,
		unsigned long num_pages, bool is_huge)
{
	if (!is_huge) {
		memcg->stats.num_total_pages += num_pages;
		memcg->stats.num_total_base_pages += num_pages;
	} else {
		memcg->stats.num_total_pages += num_pages * HPAGE_PMD_NR;
		memcg->stats.num_total_huge_pages += num_pages;
	}
}

void inc_num_page_migrations_slow_to_fast(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.num_page_migrations += 1;
		memcg->stats.num_page_migrations_slow_to_fast += 1;
	} else {
		memcg->stats.num_page_migrations += HPAGE_PMD_NR;
		memcg->stats.num_page_migrations_slow_to_fast += HPAGE_PMD_NR;
	}
}

void inc_num_page_migrations_fast_to_slow(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.num_page_migrations += 1;
		memcg->stats.num_page_migrations_fast_to_slow += 1;
	} else {
		memcg->stats.num_page_migrations += HPAGE_PMD_NR;
		memcg->stats.num_page_migrations_fast_to_slow += HPAGE_PMD_NR;
	}
}

void inc_num_accessed_pages(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.num_accessed_pages += 1;
		memcg->stats.num_accessed_base_pages += 1;
	} else {
		memcg->stats.num_accessed_pages += HPAGE_PMD_NR;
		memcg->stats.num_accessed_huge_pages += 1;
	}
}

void inc_num_fast_memory_hit_pages(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.num_fast_memory_hit_pages += 1;
		memcg->stats.num_fast_memory_hit_base_pages += 1;
		memcg->stats.numa.num_fast_memory_hit_pages += 1;
	} else {
		memcg->stats.num_fast_memory_hit_pages += HPAGE_PMD_NR;
		memcg->stats.num_fast_memory_hit_huge_pages += 1;
		memcg->stats.numa.num_fast_memory_hit_pages += HPAGE_PMD_NR;
	}
}

void inc_num_fast_memory_miss_pages(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.numa.num_fast_memory_miss_pages += 1;
	} else {
		memcg->stats.numa.num_fast_memory_miss_pages += HPAGE_PMD_NR;
	}
}

void inc_num_slow_memory_hit_pages(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.numa.num_slow_memory_hit_pages += 1;
	} else {
		memcg->stats.numa.num_slow_memory_hit_pages += HPAGE_PMD_NR;
	}
}

void inc_num_slow_memory_miss_pages(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.numa.num_slow_memory_miss_pages += 1;
	} else {
		memcg->stats.numa.num_slow_memory_miss_pages += HPAGE_PMD_NR;
	}
}

void inc_lru_num_fast_memory_hit_pages(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.lru.num_fast_memory_hit_pages += 1;
		memcg->stats.lru.num_fast_memory_hit_base_pages += 1;
	} else {
		memcg->stats.lru.num_fast_memory_hit_pages += HPAGE_PMD_NR;
		memcg->stats.lru.num_fast_memory_hit_huge_pages += 1;
	}
}

void inc_lfu_num_fast_memory_hit_pages(struct mem_cgroup *memcg,
		bool is_huge)
{
	if (!is_huge) {
		memcg->stats.lfu.num_fast_memory_hit_pages += 1;
		memcg->stats.lfu.num_fast_memory_hit_base_pages += 1;
	} else {
		memcg->stats.lfu.num_fast_memory_hit_pages += HPAGE_PMD_NR;
		memcg->stats.lfu.num_fast_memory_hit_huge_pages += 1;
	}
}
