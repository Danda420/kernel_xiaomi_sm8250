/* Copyright (c) 2011-2012, The Linux Foundation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Memory fragmentize simulate test
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <linux/gfp.h>
#include <linux/memory.h>
#include <linux/kobject.h>
#include <linux/version.h>

#include <asm/page.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>

#define DEFAULT_BLOCK_ORDER 2
#define DEFAULT_ALLOC_TIMES 16384
#define DEFAULT_ACCUPY_SIZE 0

/* Module params */
static unsigned long alloc_times = DEFAULT_ALLOC_TIMES;
static unsigned long real_alloc_times = DEFAULT_ALLOC_TIMES;
static unsigned long block_order = DEFAULT_BLOCK_ORDER;
static unsigned long accupy;
static unsigned long real_accupy;

static unsigned long *pfn_occupy;

static int filled;

static struct kobject *fragmentize_kobj;

static ssize_t alloc_times_total_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	if (block_order == 0)
		return sprintf(buf, "%u\n", filled);
	else
		return sprintf(buf, "%u\n", filled / ((1 << block_order) / 2));
}

static ssize_t alloc_size_total_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u MBytes\n", (filled << PAGE_SHIFT) / SZ_1M);
}

static DEVICE_ATTR_RO(alloc_times_total);
static DEVICE_ATTR_RO(alloc_size_total);

static struct attribute *fragmentize_attrs[] = {
	&dev_attr_alloc_size_total.attr,
	&dev_attr_alloc_times_total.attr,
	NULL,
};

struct attribute_group fragmentize_attr_group = {
	.attrs = fragmentize_attrs,
};

static int __init fragmentize_init(void)
{
	struct page *contigpages = NULL;
	unsigned long contigpfn = 0;
	unsigned long page_records;
	int i, j, m;
	int block_idx = 0;
	int ret = 0;

	fragmentize_kobj = kobject_create_and_add("fragmentize", kernel_kobj);
	if (!fragmentize_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(fragmentize_kobj, &fragmentize_attr_group);
	if (ret < 0) {
		kobject_put(fragmentize_kobj);
		pr_warn("Error creating sysfs group");
		return -EPERM;
	}

	if (block_order >= MAX_ORDER)
		block_order = MAX_ORDER - 1;

	// if block_order=zero mean only accupy not to fragmention
	if (block_order == 0) {
		if (accupy != DEFAULT_ACCUPY_SIZE)
			alloc_times = (accupy * SZ_1M) >> PAGE_SHIFT;
		page_records = alloc_times;
	} else {
		if (accupy != DEFAULT_ACCUPY_SIZE)
			alloc_times = ((accupy * SZ_1M) >> PAGE_SHIFT) /
				      ((1 << block_order) / 2);
		page_records = ((1 << block_order) / 2) * alloc_times;
	}
	accupy = (page_records << PAGE_SHIFT) / SZ_1M;

	pfn_occupy =
		(unsigned long *)vmalloc(page_records * sizeof(unsigned long));

	if (pfn_occupy == NULL) {
		pr_err("fail to vmalloc pfn_free or pfn_occupy\n");
		goto skip_alloc;
	}

	for (i = 0; i < alloc_times; i++) {
		contigpages =
			alloc_pages(GFP_KERNEL | __GFP_MOVABLE, block_order);
		if (contigpages != NULL) {
			contigpfn = page_to_pfn(contigpages);
			for (m = 0; m < (1 << block_order); m++)
				atomic_set(&((pfn_to_page(contigpfn + m))
						     ->_refcount),
					   1);

			if (block_order == 0) {
				block_idx = i;
				pfn_occupy[block_idx] = contigpfn;
			} else {
				block_idx = i * (1 << block_order) / 2;
				for (j = 0; j < (1 << block_order) / 2; j++) {
					__free_pages(
						pfn_to_page(contigpfn + 2 * j),
						0);
					pfn_occupy[block_idx + j] =
						contigpfn + 2 * j + 1;
				}
			}
		} else {
			break;
		}
	}

	real_alloc_times = i;

	if (block_order == 0)
		filled = block_idx + (1 << block_order);
	else
		filled = block_idx + (1 << block_order) / 2;

	real_accupy = (filled << PAGE_SHIFT) / SZ_1M;
skip_alloc:
	return ret;
}

static void __exit fragmentize_exit(void)
{
	int k = 0;

	sysfs_remove_group(fragmentize_kobj, &fragmentize_attr_group);
	kobject_put(fragmentize_kobj);

	if (pfn_occupy != NULL) {
		for (; k < filled; k++) {
			//atomic_set(&((pfn_to_page(pfn_free[k]))->_count), 0);
			__free_pages(pfn_to_page(pfn_occupy[k]), 0);
		}
		vfree(pfn_occupy);
	}
}

module_init(fragmentize_init);
module_exit(fragmentize_exit);

module_param(alloc_times, ulong, 0400);
module_param(real_alloc_times, ulong, 0400);
module_param(block_order, ulong, 0400);
module_param(accupy, ulong, 0400);
module_param(real_accupy, ulong, 0400);
MODULE_PARM_DESC(block_order, "block_order should be less than MAX_ORDER");
MODULE_PARM_DESC(
	accupy,
	"The two accupy and alloc_times are used together, accupy is valid");

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("fragmention");
