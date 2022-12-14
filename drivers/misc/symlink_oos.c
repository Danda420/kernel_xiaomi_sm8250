// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * OxygenOS DT2W interfaces
 *
 * Copyright (C) 2022, Rudi Setiyawan <diphons@gmail.com>
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#ifdef CONFIG_ARCH_SDM845
#include <linux/slab.h>
#include <linux/input/tp_common.h>
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("diphons");
MODULE_DESCRIPTION("oxygen os touch gesture");
MODULE_VERSION("0.0.9");

#define tpdir "touchpanel"

#define d_tap "touchpanel/double_tap_enable"
#define tp_dt "/touchpanel/double_tap"
#define tp_g "touchpanel/gesture_enable"
#define tpg "tp_gesture"
#define tpdir_fts "devices/platform/soc/a98000.i2c/i2c-3/3-0038/fts_gesture_mode"

#ifdef CONFIG_ARCH_SDM845
bool capacitive_keys_enabled;
struct kobject *touchpanel_kobj;

#define TS_ENABLE_FOPS(type)                                                   \
	int tp_common_set_##type##_ops(struct tp_common_ops *ops)              \
	{                                                                      \
		static struct kobj_attribute kattr =                           \
			__ATTR(type, (S_IWUSR | S_IRUGO), NULL, NULL);         \
		kattr.show = ops->show;                                        \
		kattr.store = ops->store;                                      \
		return sysfs_create_file(touchpanel_kobj, &kattr.attr);        \
	}

TS_ENABLE_FOPS(capacitive_keys)
TS_ENABLE_FOPS(double_tap)
TS_ENABLE_FOPS(reversed_keys)
#endif

static int __init touch_oos_init(void) {
	char *driver_path;
	static struct proc_dir_entry *tp_dir;
	static struct proc_dir_entry *tp_oos;
	int ret = 0;

	printk(KERN_INFO "oxygen os touch gesture initial");

#if defined(CONFIG_BOARD_XIAOMI_SM8250) || defined(CONFIG_MACH_XIAOMI_SM8250) || defined(CONFIG_ARCH_SM8150)
	tp_dir = proc_mkdir(tpdir, NULL);
	driver_path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (!driver_path) {
		pr_err("%s: failed to allocate memory\n", __func__);
		ret = -ENOMEM;
	}

	sprintf(driver_path, "/sys%s", tp_dt);

	pr_err("%s: driver_path=%s\n", __func__, driver_path);

	tp_oos = proc_symlink(d_tap, NULL, driver_path);

	if (!tp_oos) {
		ret = -ENOMEM;
	}
	kfree(driver_path);
	tp_oos = proc_symlink(tp_g, NULL, "double_tap_enable");
	if (!tp_oos)
		ret = -ENOMEM;
#else
	tp_dir = proc_mkdir(tpdir, NULL);
	touchpanel_kobj = kobject_create_and_add("touchpanel", NULL);
	if (!touchpanel_kobj)
		ret = -ENOMEM;

	sprintf(driver_path, "/sys%s", tp_dt);
	pr_err("%s: driver_path=%s\n", __func__, driver_path);
	tp_oos = proc_symlink(tp_g, NULL, driver_path);
	if (!tp_oos) {
		ret = -ENOMEM;
	}
	kfree(driver_path);
	tp_oos = proc_symlink(d_tap, NULL, "gesture_enable");
	if (!tp_oos)
		ret = -ENOMEM;
#endif
	tp_oos = proc_symlink(tpg, NULL, tp_g);
	if (!tp_oos) {
		ret = -ENOMEM;
		printk(KERN_INFO "oxygen os touch gesture initial failed");
	} else
		printk(KERN_INFO "oxygen os touch gesture initialized");

	return ret;
}

static void __exit touch_oos_exit(void) {
	printk(KERN_INFO "oxygen os touch gesture exit");
}

module_init(touch_oos_init);
module_exit(touch_oos_exit);

