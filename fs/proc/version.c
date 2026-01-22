// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>

static int version_proc_show(struct seq_file *m, void *v)
{
    struct new_utsname *uts = utsname();
    const char *spoofed_release = "5.10.248-Oxygen+";
    
    seq_printf(m, linux_proc_banner,
             uts->sysname,
             spoofed_release,
             uts->version);
    
    return 0;
}

static int __init proc_version_init(void)
{
	proc_create_single("version", 0, NULL, version_proc_show);
	return 0;
}
fs_initcall(proc_version_init);
