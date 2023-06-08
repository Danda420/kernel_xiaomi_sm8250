#include "ksu.h"
#include "linux/delay.h"
#include "linux/fs.h"
#include "linux/gfp.h"
#include "linux/kernel.h"
#include "linux/list.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/version.h"
#include "klog.h" // IWYU pragma: keep
#include "selinux/selinux.h"
#include "kernel_compat.h"
#include "allowlist.h"

#define FILE_MAGIC 0x7f4b5355 // ' KSU', u32
#define FILE_FORMAT_VERSION 2 // u32

static DEFINE_MUTEX(allowlist_mutex);

// default root identify
static struct root_identity default_root_identity;
static bool default_umount_modules = true;

static void init_root_identity()
{
	default_root_identity.uid = 0;
	default_root_identity.gid = 0;
	default_root_identity.groups_count = 1;
	default_root_identity.groups[0] = 0;
	memset(&default_root_identity.capabilities, 0xff,
	       sizeof(default_root_identity.capabilities));
	default_root_identity.namespaces = 0;
	strcpy(default_root_identity.selinux_domain, "su");
}

struct perm_data {
	struct list_head list;
	struct app_profile profile;
};

static struct list_head allow_list;

#define KERNEL_SU_ALLOWLIST "/data/adb/ksu/.allowlist"

static struct work_struct ksu_save_work;
static struct work_struct ksu_load_work;

bool persistent_allow_list(void);

void ksu_show_allow_list(void)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	pr_info("ksu_show_allow_list");
	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		pr_info("uid :%d, allow: %d\n", p->profile.current_uid,
			p->profile.allow_su);
	}
}

static void ksu_grant_root_to_shell()
{
	struct app_profile profile = {
		.allow_su = true,
		.current_uid = 2000,
	};
	strcpy(profile.key, "com.android.shell");
	ksu_set_app_profile(&profile, false);
}

bool ksu_get_app_profile(struct app_profile *profile)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	bool found = false;

	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		if (!strcmp(profile->key, p->profile.key)) {
			// found it, override it with ours
			memcpy(profile, &p->profile, sizeof(*profile));
			found = true;
			goto exit;
		}
	}

	if (!found) {
		// don't found, fill it with default profile
		if (profile->allow_su) {
			profile->root_profile.use_default = true;
			memcpy(&profile->root_profile.identity,
			       &default_root_identity,
			       sizeof(default_root_identity));
		} else {
			profile->non_root_profile.use_default = true;
			profile->non_root_profile.umount_modules =
				default_umount_modules;
		}
	}
exit:
	return found;
}

bool ksu_set_app_profile(struct app_profile *profile, bool persist)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	bool result = false;

	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		if (!strcmp(profile->key, p->profile.key)) {
			// found it, just override it all!
			memcpy(&p->profile, profile, sizeof(*profile));
			result = true;
			goto exit;
		}
	}

	// not found, alloc a new node!
	p = (struct perm_data *)kmalloc(sizeof(struct perm_data), GFP_KERNEL);
	if (!p) {
		pr_err("ksu_set_app_profile alloc failed\n");
		return false;
	}

	memcpy(&p->profile, profile, sizeof(*profile));
	pr_info("set app profile, key: %s, uid: %d\n", profile->key,
		profile->current_uid);
	list_add_tail(&p->list, &allow_list);
	result = true;

exit:
	if (persist)
		persistent_allow_list();

	return result;
}

bool ksu_is_allow_uid(uid_t uid)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;

	if (uid == 0) {
		// already root, but only allow our domain.
		return is_ksu_domain();
	}

	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		// pr_info("is_allow_uid uid :%d, allow: %d\n", p->uid, p->allow);
		if (uid == p->profile.current_uid) {
			return p->profile.allow_su;
		}
	}

	return false;
}

bool ksu_get_allow_list(int *array, int *length, bool allow)
{
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	int i = 0;
	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		// pr_info("get_allow_list uid: %d allow: %d\n", p->uid, p->allow);
		if (p->profile.allow_su == allow) {
			array[i++] = p->profile.allow_su;
		}
	}
	*length = i;

	return true;
}

void do_save_allow_list(struct work_struct *work)
{
	u32 magic = FILE_MAGIC;
	u32 version = FILE_FORMAT_VERSION;
	struct perm_data *p = NULL;
	struct list_head *pos = NULL;
	loff_t off = 0;
	KWORKER_INSTALL_KEYRING();
	struct file *fp =
		filp_open(KERNEL_SU_ALLOWLIST, O_WRONLY | O_CREAT, 0644);

	if (IS_ERR(fp)) {
		pr_err("save_allow_list creat file failed: %ld\n", PTR_ERR(fp));
		return;
	}

	// store magic and version
	if (ksu_kernel_write_compat(fp, &magic, sizeof(magic), &off) !=
	    sizeof(magic)) {
		pr_err("save_allow_list write magic failed.\n");
		goto exit;
	}

	if (ksu_kernel_write_compat(fp, &version, sizeof(version), &off) !=
	    sizeof(version)) {
		pr_err("save_allow_list write version failed.\n");
		goto exit;
	}

	list_for_each (pos, &allow_list) {
		p = list_entry(pos, struct perm_data, list);
		pr_info("save allow list, name: %s uid :%d, allow: %d\n",
			p->profile.key, p->profile.current_uid,
			p->profile.allow_su);

		ksu_kernel_write_compat(fp, &p->profile, sizeof(p->profile),
					&off);
	}

exit:
	filp_close(fp, 0);
}

void do_load_allow_list(struct work_struct *work)
{
	loff_t off = 0;
	ssize_t ret = 0;
	struct file *fp = NULL;
	u32 magic;
	u32 version;
	KWORKER_INSTALL_KEYRING();
	fp = filp_open("/data/adb", O_RDONLY, 0);
	if (IS_ERR(fp)) {
		int errno = PTR_ERR(fp);
		pr_err("load_allow_list open '/data/adb': %d\n", PTR_ERR(fp));
		if (errno == -ENOENT) {
			msleep(2000);
			ksu_queue_work(&ksu_load_work);
			return;
		} else {
			pr_info("load_allow list dir exist now!");
		}
	} else {
		filp_close(fp, 0);
	}

#ifdef CONFIG_KSU_DEBUG
	// always allow adb shell by default
	ksu_grant_root_to_shell();
#endif
	// load allowlist now!
	fp = filp_open(KERNEL_SU_ALLOWLIST, O_RDONLY, 0);

	if (IS_ERR(fp)) {
		pr_err("load_allow_list open file failed: %ld\n", PTR_ERR(fp));
		return;
	}

	// verify magic
	if (ksu_kernel_read_compat(fp, &magic, sizeof(magic), &off) !=
		    sizeof(magic) ||
	    magic != FILE_MAGIC) {
		pr_err("allowlist file invalid: %d!\n", magic);
		goto exit;
	}

	if (ksu_kernel_read_compat(fp, &version, sizeof(version), &off) !=
	    sizeof(version)) {
		pr_err("allowlist read version: %d failed\n", version);
		goto exit;
	}

	pr_info("allowlist version: %d\n", version);

	while (true) {
		struct app_profile profile;

		ret = ksu_kernel_read_compat(fp, &profile, sizeof(profile),
					     &off);

		if (ret <= 0) {
			pr_info("load_allow_list read err: %zd\n", ret);
			break;
		}

		pr_info("load_allow_uid, name: %s, uid: %d, allow: %d\n",
			profile.key, profile.current_uid, profile.allow_su);
		ksu_set_app_profile(&profile, false);
	}

exit:
	ksu_show_allow_list();
	filp_close(fp, 0);
}

void ksu_prune_allowlist(bool (*is_uid_exist)(uid_t, void *), void *data)
{
	struct perm_data *np = NULL;
	struct perm_data *n = NULL;

	bool modified = false;
	// TODO: use RCU!
	mutex_lock(&allowlist_mutex);
	list_for_each_entry_safe (np, n, &allow_list, list) {
		uid_t uid = np->profile.current_uid;
		if (!is_uid_exist(uid, data)) {
			modified = true;
			pr_info("prune uid: %d\n", uid);
			list_del(&np->list);
			kfree(np);
		}
	}
	mutex_unlock(&allowlist_mutex);

	if (modified) {
		persistent_allow_list();
	}
}

// make sure allow list works cross boot
bool persistent_allow_list(void)
{
	return ksu_queue_work(&ksu_save_work);
}

bool ksu_load_allow_list(void)
{
	return ksu_queue_work(&ksu_load_work);
}

void ksu_allowlist_init(void)
{
	INIT_LIST_HEAD(&allow_list);

	INIT_WORK(&ksu_save_work, do_save_allow_list);
	INIT_WORK(&ksu_load_work, do_load_allow_list);

	// init default_root_identity, which is used for root identity when root profile is not set.
	init_root_identity();
}

void ksu_allowlist_exit(void)
{
	struct perm_data *np = NULL;
	struct perm_data *n = NULL;

	do_save_allow_list(NULL);

	// free allowlist
	mutex_lock(&allowlist_mutex);
	list_for_each_entry_safe (np, n, &allow_list, list) {
		list_del(&np->list);
		kfree(np);
	}
	mutex_unlock(&allowlist_mutex);
}