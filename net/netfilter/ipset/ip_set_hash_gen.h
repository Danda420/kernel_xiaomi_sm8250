/* Copyright (C) 2013 Jozsef Kadlecsik <kadlec@blackhole.kfki.hu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _IP_SET_HASH_GEN_H
#define _IP_SET_HASH_GEN_H

#include <linux/rcupdate.h>
#include <linux/jhash.h>
#include <linux/types.h>
#include <linux/netfilter/ipset/ip_set_timeout.h>

#define __ipset_dereference_protected(p, c)	rcu_dereference_protected(p, c)
#define ipset_dereference_protected(p, set) \
	__ipset_dereference_protected(p, spin_is_locked(&(set)->lock))

#define rcu_dereference_bh_nfnl(p)	rcu_dereference_bh_check(p, 1)

/* Hashing which uses arrays to resolve clashing. The hash table is resized
 * (doubled) when searching becomes too long.
 * Internally jhash is used with the assumption that the size of the
 * stored data is a multiple of sizeof(u32).
 *
 * Readers and resizing
 *
 * Resizing can be triggered by userspace command only, and those
 * are serialized by the nfnl mutex. During resizing the set is
 * read-locked, so the only possible concurrent operations are
 * the kernel side readers. Those must be protected by proper RCU locking.
 */

/* Number of elements to store in an initial array block */
#define AHASH_INIT_SIZE			4
/* Max number of elements to store in an array block */
#define AHASH_MAX_SIZE			(3 * AHASH_INIT_SIZE)
/* Max muber of elements in the array block when tuned */
#define AHASH_MAX_TUNED			64

/* Max number of elements can be tuned */
#ifdef IP_SET_HASH_WITH_MULTI
#define AHASH_MAX(h)			((h)->ahash_max)

static inline u8
tune_ahash_max(u8 curr, u32 multi)
{
	u32 n;

	if (multi < curr)
		return curr;

	n = curr + AHASH_INIT_SIZE;
	/* Currently, at listing one hash bucket must fit into a message.
	 * Therefore we have a hard limit here.
	 */
	return n > curr && n <= AHASH_MAX_TUNED ? n : curr;
}

#define TUNE_AHASH_MAX(h, multi)	\
	((h)->ahash_max = tune_ahash_max((h)->ahash_max, multi))
#else
#define AHASH_MAX(h)			AHASH_MAX_SIZE
#define TUNE_AHASH_MAX(h, multi)
#endif

/* A hash bucket */
struct hbucket {
	struct rcu_head rcu;	/* for call_rcu_bh */
	/* Which positions are used in the array */
	DECLARE_BITMAP(used, AHASH_MAX_TUNED);
	u8 size;		/* size of the array */
	u8 pos;			/* position of the first free entry */
	unsigned char value[]	/* the array of the values */
		__aligned(__alignof__(u64));
};

/* The hash table: the table size stored here in order to make resizing easy */
struct htable {
	atomic_t ref;		/* References for resizing */
	atomic_t uref;		/* References for dumping */
	u8 htable_bits;		/* size of hash table == 2^htable_bits */
	struct hbucket __rcu *bucket[]; /* hashtable buckets */
};

#define hbucket(h, i)		((h)->bucket[i])
#define ext_size(n, dsize)	\
	(sizeof(struct hbucket) + (n) * (dsize))

#ifndef IPSET_NET_COUNT
#define IPSET_NET_COUNT		1
#endif

/* Book-keeping of the prefixes added to the set */
struct net_prefixes {
	u32 nets[IPSET_NET_COUNT]; /* number of elements for this cidr */
	u8 cidr[IPSET_NET_COUNT];  /* the cidr value */
};

/* Compute the hash table size */
static size_t
htable_size(u8 hbits)
{
	size_t hsize;

	/* We must fit both into u32 in jhash and INT_MAX in kvmalloc_node() */
	if (hbits > 31)
		return 0;
	hsize = jhash_size(hbits);
	if ((INT_MAX - sizeof(struct htable)) / sizeof(struct hbucket *)
	    < hsize)
		return 0;

	return hsize * sizeof(struct hbucket *) + sizeof(struct htable);
}

#ifdef IP_SET_HASH_WITH_NETS
#if IPSET_NET_COUNT > 1
#define __CIDR(cidr, i)		(cidr[i])
#else
#define __CIDR(cidr, i)		(cidr)
#endif

/* cidr + 1 is stored in net_prefixes to support /0 */
#define NCIDR_PUT(cidr)		((cidr) + 1)
#define NCIDR_GET(cidr)		((cidr) - 1)

#ifdef IP_SET_HASH_WITH_NETS_PACKED
/* When cidr is packed with nomatch, cidr - 1 is stored in the data entry */
#define DCIDR_PUT(cidr)		((cidr) - 1)
#define DCIDR_GET(cidr, i)	(__CIDR(cidr, i) + 1)
#else
#define DCIDR_PUT(cidr)		(cidr)
#define DCIDR_GET(cidr, i)	__CIDR(cidr, i)
#endif

#define INIT_CIDR(cidr, host_mask)	\
	DCIDR_PUT(((cidr) ? NCIDR_GET(cidr) : host_mask))

#ifdef IP_SET_HASH_WITH_NET0
/* cidr from 0 to HOST_MASK value and c = cidr + 1 */
#define NLEN			(HOST_MASK + 1)
#define CIDR_POS(c)		((c) - 1)
#else
/* cidr from 1 to HOST_MASK value and c = cidr + 1 */
#define NLEN			HOST_MASK
#define CIDR_POS(c)		((c) - 2)
#endif

#else
#define NLEN			0
#endif /* IP_SET_HASH_WITH_NETS */

#endif /* _IP_SET_HASH_GEN_H */

#ifndef MTYPE
#error "MTYPE is not defined!"
#endif

#ifndef HTYPE
#error "HTYPE is not defined!"
#endif

#ifndef HOST_MASK
#error "HOST_MASK is not defined!"
#endif

/* Family dependent templates */

#undef ahash_data
#undef mtype_data_equal
#undef mtype_do_data_match
#undef mtype_data_set_flags
#undef mtype_data_reset_elem
#undef mtype_data_reset_flags
#undef mtype_data_netmask
#undef mtype_data_list
#undef mtype_data_next
#undef mtype_elem

#undef mtype_ahash_destroy
#undef mtype_ext_cleanup
#undef mtype_add_cidr
#undef mtype_del_cidr
#undef mtype_ahash_memsize
#undef mtype_flush
#undef mtype_destroy
#undef mtype_same_set
#undef mtype_kadt
#undef mtype_uadt

#undef mtype_add
#undef mtype_del
#undef mtype_test_cidrs
#undef mtype_test
#undef mtype_uref
#undef mtype_expire
#undef mtype_resize
#undef mtype_head
#undef mtype_list
#undef mtype_gc
#undef mtype_gc_init
#undef mtype_variant
#undef mtype_data_match

#undef htype
#undef HKEY

#define mtype_data_equal	IPSET_TOKEN(MTYPE, _data_equal)
#ifdef IP_SET_HASH_WITH_NETS
#define mtype_do_data_match	IPSET_TOKEN(MTYPE, _do_data_match)
#else
#define mtype_do_data_match(d)	1
#endif
#define mtype_data_set_flags	IPSET_TOKEN(MTYPE, _data_set_flags)
#define mtype_data_reset_elem	IPSET_TOKEN(MTYPE, _data_reset_elem)
#define mtype_data_reset_flags	IPSET_TOKEN(MTYPE, _data_reset_flags)
#define mtype_data_netmask	IPSET_TOKEN(MTYPE, _data_netmask)
#define mtype_data_list		IPSET_TOKEN(MTYPE, _data_list)
#define mtype_data_next		IPSET_TOKEN(MTYPE, _data_next)
#define mtype_elem		IPSET_TOKEN(MTYPE, _elem)

#define mtype_ahash_destroy	IPSET_TOKEN(MTYPE, _ahash_destroy)
#define mtype_ext_cleanup	IPSET_TOKEN(MTYPE, _ext_cleanup)
#define mtype_add_cidr		IPSET_TOKEN(MTYPE, _add_cidr)
#define mtype_del_cidr		IPSET_TOKEN(MTYPE, _del_cidr)
#define mtype_ahash_memsize	IPSET_TOKEN(MTYPE, _ahash_memsize)
#define mtype_flush		IPSET_TOKEN(MTYPE, _flush)
#define mtype_destroy		IPSET_TOKEN(MTYPE, _destroy)
#define mtype_same_set		IPSET_TOKEN(MTYPE, _same_set)
#define mtype_kadt		IPSET_TOKEN(MTYPE, _kadt)
#define mtype_uadt		IPSET_TOKEN(MTYPE, _uadt)

#define mtype_add		IPSET_TOKEN(MTYPE, _add)
#define mtype_del		IPSET_TOKEN(MTYPE, _del)
#define mtype_test_cidrs	IPSET_TOKEN(MTYPE, _test_cidrs)
#define mtype_test		IPSET_TOKEN(MTYPE, _test)
#define mtype_uref		IPSET_TOKEN(MTYPE, _uref)
#define mtype_expire		IPSET_TOKEN(MTYPE, _expire)
#define mtype_resize		IPSET_TOKEN(MTYPE, _resize)
#define mtype_head		IPSET_TOKEN(MTYPE, _head)
#define mtype_list		IPSET_TOKEN(MTYPE, _list)
#define mtype_gc		IPSET_TOKEN(MTYPE, _gc)
#define mtype_gc_init		IPSET_TOKEN(MTYPE, _gc_init)
#define mtype_variant		IPSET_TOKEN(MTYPE, _variant)
#define mtype_data_match	IPSET_TOKEN(MTYPE, _data_match)

#ifndef HKEY_DATALEN
#define HKEY_DATALEN		sizeof(struct mtype_elem)
#endif

#define htype			MTYPE

#define HKEY(data, initval, htable_bits)			\
({								\
	const u32 *__k = (const u32 *)data;			\
	u32 __l = HKEY_DATALEN / sizeof(u32);			\
								\
	BUILD_BUG_ON(HKEY_DATALEN % sizeof(u32) != 0);		\
								\
	jhash2(__k, __l, initval) & jhash_mask(htable_bits);	\
})

/* The generic hash structure */
struct htype {
	struct htable __rcu *table; /* the hash table */
	struct timer_list gc;	/* garbage collection when timeout enabled */
	struct ip_set *set;	/* attached to this ip_set */
	u32 maxelem;		/* max elements in the hash */
	u32 initval;		/* random jhash init value */
#ifdef IP_SET_HASH_WITH_MARKMASK
	u32 markmask;		/* markmask value for mark mask to store */
#endif
#ifdef IP_SET_HASH_WITH_MULTI
	u8 ahash_max;		/* max elements in an array block */
#endif
#ifdef IP_SET_HASH_WITH_NETMASK
	u8 netmask;		/* netmask value for subnets to store */
#endif
	struct mtype_elem next; /* temporary storage for uadd */
#ifdef IP_SET_HASH_WITH_NETS
	struct net_prefixes nets[NLEN]; /* book-keeping of prefixes */
#endif
};

#ifdef IP_SET_HASH_WITH_NETS
/* Network cidr size book keeping when the hash stores different
 * sized networks. cidr == real cidr + 1 to support /0.
 */
static void
mtype_add_cidr(struct htype *h, u8 cidr, u8 n)
{
	int i, j;

	/* Add in increasing prefix order, so larger cidr first */
	for (i = 0, j = -1; i < NLEN && h->nets[i].cidr[n]; i++) {
		if (j != -1) {
			continue;
		} else if (h->nets[i].cidr[n] < cidr) {
			j = i;
		} else if (h->nets[i].cidr[n] == cidr) {
			h->nets[CIDR_POS(cidr)].nets[n]++;
			return;
		}
	}
	if (j != -1) {
		for (; i > j; i--)
			h->nets[i].cidr[n] = h->nets[i - 1].cidr[n];
	}
	h->nets[i].cidr[n] = cidr;
	h->nets[CIDR_POS(cidr)].nets[n] = 1;
}

static void
mtype_del_cidr(struct htype *h, u8 cidr, u8 n)
{
	u8 i, j, net_end = NLEN - 1;

	for (i = 0; i < NLEN; i++) {
		if (h->nets[i].cidr[n] != cidr)
			continue;
		h->nets[CIDR_POS(cidr)].nets[n]--;
		if (h->nets[CIDR_POS(cidr)].nets[n] > 0)
			return;
		for (j = i; j < net_end && h->nets[j].cidr[n]; j++)
			h->nets[j].cidr[n] = h->nets[j + 1].cidr[n];
		h->nets[j].cidr[n] = 0;
		return;
	}
}
#endif

/* Calculate the actual memory size of the set data */
static size_t
mtype_ahash_memsize(const struct htype *h, const struct htable *t)
{
	return sizeof(*h) + sizeof(*t);
}

/* Get the ith element from the array block n */
#define ahash_data(n, i, dsize)	\
	((struct mtype_elem *)((n)->value + ((i) * (dsize))))

static void
mtype_ext_cleanup(struct ip_set *set, struct hbucket *n)
{
	int i;

	for (i = 0; i < n->pos; i++)
		if (test_bit(i, n->used))
			ip_set_ext_destroy(set, ahash_data(n, i, set->dsize));
}

/* Flush a hash type of set: destroy all elements */
static void
mtype_flush(struct ip_set *set)
{
	struct htype *h = set->data;
	struct htable *t;
	struct hbucket *n;
	u32 i;

	t = ipset_dereference_protected(h->table, set);
	for (i = 0; i < jhash_size(t->htable_bits); i++) {
		n = __ipset_dereference_protected(hbucket(t, i), 1);
		if (!n)
			continue;
		if (set->extensions & IPSET_EXT_DESTROY)
			mtype_ext_cleanup(set, n);
		/* FIXME: use slab cache */
		rcu_assign_pointer(hbucket(t, i), NULL);
		kfree_rcu(n, rcu);
	}
#ifdef IP_SET_HASH_WITH_NETS
	memset(h->nets, 0, sizeof(h->nets));
#endif
	set->elements = 0;
	set->ext_size = 0;
}

/* Destroy the hashtable part of the set */
static void
mtype_ahash_destroy(struct ip_set *set, struct htable *t, bool ext_destroy)
{
	struct hbucket *n;
	u32 i;

	for (i = 0; i < jhash_size(t->htable_bits); i++) {
		n = __ipset_dereference_protected(hbucket(t, i), 1);
		if (!n)
			continue;
		if (set->extensions & IPSET_EXT_DESTROY && ext_destroy)
			mtype_ext_cleanup(set, n);
		/* FIXME: use slab cache */
		kfree(n);
	}

	ip_set_free(t);
}

/* Destroy a hash type of set */
static void
mtype_destroy(struct ip_set *set)
{
	struct htype *h = set->data;

	if (SET_WITH_TIMEOUT(set))
		del_timer_sync(&h->gc);

	mtype_ahash_destroy(set,
			    __ipset_dereference_protected(h->table, 1), true);
	kfree(h);

	set->data = NULL;
}

static void
mtype_gc_init(struct ip_set *set, void (*gc)(struct timer_list *t))
{
	struct htype *h = set->data;

	timer_setup(&h->gc, gc, 0);
	mod_timer(&h->gc, jiffies + IPSET_GC_PERIOD(set->timeout) * HZ);
	pr_debug("gc initialized, run in every %u\n",
		 IPSET_GC_PERIOD(set->timeout));
}

static bool
mtype_same_set(const struct ip_set *a, const struct ip_set *b)
{
	const struct htype *x = a->data;
	const struct htype *y = b->data;

	/* Resizing changes htable_bits, so we ignore it */
	return x->maxelem == y->maxelem &&
	       a->timeout == b->timeout &&
#ifdef IP_SET_HASH_WITH_NETMASK
	       x->netmask == y->netmask &&
#endif
#ifdef IP_SET_HASH_WITH_MARKMASK
	       x->markmask == y->markmask &&
#endif
	       a->extensions == b->extensions;
}

/* Delete expired elements from the hashtable */
static void
mtype_expire(struct ip_set *set, struct htype *h)
{
	struct htable *t;
	struct hbucket *n, *tmp;
	struct mtype_elem *data;
	u32 i, j, d;
	size_t dsize = set->dsize;
#ifdef IP_SET_HASH_WITH_NETS
	u8 k;
#endif

	t = ipset_dereference_protected(h->table, set);
	for (i = 0; i < jhash_size(t->htable_bits); i++) {
		n = __ipset_dereference_protected(hbucket(t, i), 1);
		if (!n)
			continue;
		for (j = 0, d = 0; j < n->pos; j++) {
			if (!test_bit(j, n->used)) {
				d++;
				continue;
			}
			data = ahash_data(n, j, dsize);
			if (!ip_set_timeout_expired(ext_timeout(data, set)))
				continue;
			pr_debug("expired %u/%u\n", i, j);
			clear_bit(j, n->used);
			smp_mb__after_atomic();
#ifdef IP_SET_HASH_WITH_NETS
			for (k = 0; k < IPSET_NET_COUNT; k++)
				mtype_del_cidr(h,
					NCIDR_PUT(DCIDR_GET(data->cidr, k)),
					k);
#endif
			ip_set_ext_destroy(set, data);
			set->elements--;
			d++;
		}
		if (d >= AHASH_INIT_SIZE) {
			if (d >= n->size) {
				rcu_assign_pointer(hbucket(t, i), NULL);
				kfree_rcu(n, rcu);
				continue;
			}
			tmp = kzalloc(sizeof(*tmp) +
				      (n->size - AHASH_INIT_SIZE) * dsize,
				      GFP_ATOMIC);
			if (!tmp)
				/* Still try to delete expired elements */
				continue;
			tmp->size = n->size - AHASH_INIT_SIZE;
			for (j = 0, d = 0; j < n->pos; j++) {
				if (!test_bit(j, n->used))
					continue;
				data = ahash_data(n, j, dsize);
				memcpy(tmp->value + d * dsize, data, dsize);
				set_bit(d, tmp->used);
				d++;
			}
			tmp->pos = d;
			set->ext_size -= ext_size(AHASH_INIT_SIZE, dsize);
			rcu_assign_pointer(hbucket(t, i), tmp);
			kfree_rcu(n, rcu);
		}
	}
}

static void
mtype_gc(struct timer_list *t)
{
	struct htype *h = from_timer(h, t, gc);
	struct ip_set *set = h->set;

	pr_debug("called\n");
	spin_lock_bh(&set->lock);
	mtype_expire(set, h);
	spin_unlock_bh(&set->lock);

	h->gc.expires = jiffies + IPSET_GC_PERIOD(set->timeout) * HZ;
	add_timer(&h->gc);
}

/* Resize a hash: create a new hash table with doubling the hashsize
 * and inserting the elements to it. Repeat until we succeed or
 * fail due to memory pressures.
 */
static int
mtype_resize(struct ip_set *set, bool retried)
{
	struct htype *h = set->data;
	struct htable *t, *orig;
	u8 htable_bits;
	size_t extsize, dsize = set->dsize;
#ifdef IP_SET_HASH_WITH_NETS
	u8 flags;
	struct mtype_elem *tmp;
#endif
	struct mtype_elem *data;
	struct mtype_elem *d;
	struct hbucket *n, *m;
	u32 i, j, key;
	int ret;

#ifdef IP_SET_HASH_WITH_NETS
	tmp = kmalloc(dsize, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
#endif
	rcu_read_lock_bh();
	orig = rcu_dereference_bh_nfnl(h->table);
	htable_bits = orig->htable_bits;
	rcu_read_unlock_bh();

retry:
	ret = 0;
	htable_bits++;
	if (!htable_bits) {
		/* In case we have plenty of memory :-) */
		pr_warn("Cannot increase the hashsize of set %s further\n",
			set->name);
		ret = -IPSET_ERR_HASH_FULL;
		goto out;
	}
	t = ip_set_alloc(htable_size(htable_bits));
	if (!t) {
		ret = -ENOMEM;
		goto out;
	}
	t->htable_bits = htable_bits;

	spin_lock_bh(&set->lock);
	orig = __ipset_dereference_protected(h->table, 1);
	/* There can't be another parallel resizing, but dumping is possible */
	atomic_set(&orig->ref, 1);
	atomic_inc(&orig->uref);
	extsize = 0;
	pr_debug("attempt to resize set %s from %u to %u, t %p\n",
		 set->name, orig->htable_bits, htable_bits, orig);
	for (i = 0; i < jhash_size(orig->htable_bits); i++) {
		n = __ipset_dereference_protected(hbucket(orig, i), 1);
		if (!n)
			continue;
		for (j = 0; j < n->pos; j++) {
			if (!test_bit(j, n->used))
				continue;
			data = ahash_data(n, j, dsize);
#ifdef IP_SET_HASH_WITH_NETS
			/* We have readers running parallel with us,
			 * so the live data cannot be modified.
			 */
			flags = 0;
			memcpy(tmp, data, dsize);
			data = tmp;
			mtype_data_reset_flags(data, &flags);
#endif
			key = HKEY(data, h->initval, htable_bits);
			m = __ipset_dereference_protected(hbucket(t, key), 1);
			if (!m) {
				m = kzalloc(sizeof(*m) +
					    AHASH_INIT_SIZE * dsize,
					    GFP_ATOMIC);
				if (!m) {
					ret = -ENOMEM;
					goto cleanup;
				}
				m->size = AHASH_INIT_SIZE;
				extsize += ext_size(AHASH_INIT_SIZE, dsize);
				RCU_INIT_POINTER(hbucket(t, key), m);
			} else if (m->pos >= m->size) {
				struct hbucket *ht;

				if (m->size >= AHASH_MAX(h)) {
					ret = -EAGAIN;
				} else {
					ht = kzalloc(sizeof(*ht) +
						(m->size + AHASH_INIT_SIZE)
						* dsize,
						GFP_ATOMIC);
					if (!ht)
						ret = -ENOMEM;
				}
				if (ret < 0)
					goto cleanup;
				memcpy(ht, m, sizeof(struct hbucket) +
					      m->size * dsize);
				ht->size = m->size + AHASH_INIT_SIZE;
				extsize += ext_size(AHASH_INIT_SIZE, dsize);
				kfree(m);
				m = ht;
				RCU_INIT_POINTER(hbucket(t, key), ht);
			}
			d = ahash_data(m, m->pos, dsize);
			memcpy(d, data, dsize);
			set_bit(m->pos++, m->used);
#ifdef IP_SET_HASH_WITH_NETS
			mtype_data_reset_flags(d, &flags);
#endif
		}
	}
	rcu_assign_pointer(h->table, t);
	set->ext_size = extsize;

	spin_unlock_bh(&set->lock);

	/* Give time to other readers of the set */
	synchronize_rcu_bh();

	pr_debug("set %s resized from %u (%p) to %u (%p)\n", set->name,
		 orig->htable_bits, orig, t->htable_bits, t);
	/* If there's nobody else dumping the table, destroy it */
	if (atomic_dec_and_test(&orig->uref)) {
		pr_debug("Table destroy by resize %p\n", orig);
		mtype_ahash_destroy(set, orig, false);
	}

out:
#ifdef IP_SET_HASH_WITH_NETS
	kfree(tmp);
#endif
	return ret;

cleanup:
	atomic_set(&orig->ref, 0);
	atomic_dec(&orig->uref);
	spin_unlock_bh(&set->lock);
	mtype_ahash_destroy(set, t, false);
	if (ret == -EAGAIN)
		goto retry;
	goto out;
}

/* Add an element to a hash and update the internal counters when succeeded,
 * otherwise report the proper error code.
 */
static int
mtype_add(struct ip_set *set, void *value, const struct ip_set_ext *ext,
	  struct ip_set_ext *mext, u32 flags)
{
	struct htype *h = set->data;
	struct htable *t;
	const struct mtype_elem *d = value;
	struct mtype_elem *data;
	struct hbucket *n, *old = ERR_PTR(-ENOENT);
	int i, j = -1;
	bool flag_exist = flags & IPSET_FLAG_EXIST;
	bool deleted = false, forceadd = false, reuse = false;
	u32 key, multi = 0;

	if (set->elements >= h->maxelem) {
		if (SET_WITH_TIMEOUT(set))
			/* FIXME: when set is full, we slow down here */
			mtype_expire(set, h);
		if (set->elements >= h->maxelem && SET_WITH_FORCEADD(set))
			forceadd = true;
	}

	t = ipset_dereference_protected(h->table, set);
	key = HKEY(value, h->initval, t->htable_bits);
	n = __ipset_dereference_protected(hbucket(t, key), 1);
	if (!n) {
		if (forceadd || set->elements >= h->maxelem)
			goto set_full;
		old = NULL;
		n = kzalloc(sizeof(*n) + AHASH_INIT_SIZE * set->dsize,
			    GFP_ATOMIC);
		if (!n)
			return -ENOMEM;
		n->size = AHASH_INIT_SIZE;
		set->ext_size += ext_size(AHASH_INIT_SIZE, set->dsize);
		goto copy_elem;
	}
	for (i = 0; i < n->pos; i++) {
		if (!test_bit(i, n->used)) {
			/* Reuse first deleted entry */
			if (j == -1) {
				deleted = reuse = true;
				j = i;
			}
			continue;
		}
		data = ahash_data(n, i, set->dsize);
		if (mtype_data_equal(data, d, &multi)) {
			if (flag_exist ||
			    (SET_WITH_TIMEOUT(set) &&
			     ip_set_timeout_expired(ext_timeout(data, set)))) {
				/* Just the extensions could be overwritten */
				j = i;
				goto overwrite_extensions;
			}
			return -IPSET_ERR_EXIST;
		}
		/* Reuse first timed out entry */
		if (SET_WITH_TIMEOUT(set) &&
		    ip_set_timeout_expired(ext_timeout(data, set)) &&
		    j == -1) {
			j = i;
			reuse = true;
		}
	}
	if (reuse || forceadd) {
		data = ahash_data(n, j, set->dsize);
		if (!deleted) {
#ifdef IP_SET_HASH_WITH_NETS
			for (i = 0; i < IPSET_NET_COUNT; i++)
				mtype_del_cidr(h,
					NCIDR_PUT(DCIDR_GET(data->cidr, i)),
					i);
#endif
			ip_set_ext_destroy(set, data);
			set->elements--;
		}
		goto copy_data;
	}
	if (set->elements >= h->maxelem)
		goto set_full;
	/* Create a new slot */
	if (n->pos >= n->size) {
		TUNE_AHASH_MAX(h, multi);
		if (n->size >= AHASH_MAX(h)) {
			/* Trigger rehashing */
			mtype_data_next(&h->next, d);
			return -EAGAIN;
		}
		old = n;
		n = kzalloc(sizeof(*n) +
			    (old->size + AHASH_INIT_SIZE) * set->dsize,
			    GFP_ATOMIC);
		if (!n)
			return -ENOMEM;
		memcpy(n, old, sizeof(struct hbucket) +
		       old->size * set->dsize);
		n->size = old->size + AHASH_INIT_SIZE;
		set->ext_size += ext_size(AHASH_INIT_SIZE, set->dsize);
	}

copy_elem:
	j = n->pos++;
	data = ahash_data(n, j, set->dsize);
copy_data:
	set->elements++;
#ifdef IP_SET_HASH_WITH_NETS
	for (i = 0; i < IPSET_NET_COUNT; i++)
		mtype_add_cidr(h, NCIDR_PUT(DCIDR_GET(d->cidr, i)), i);
#endif
	memcpy(data, d, sizeof(struct mtype_elem));
overwrite_extensions:
#ifdef IP_SET_HASH_WITH_NETS
	mtype_data_set_flags(data, flags);
#endif
	if (SET_WITH_COUNTER(set))
		ip_set_init_counter(ext_counter(data, set), ext);
	if (SET_WITH_COMMENT(set))
		ip_set_init_comment(set, ext_comment(data, set), ext);
	if (SET_WITH_SKBINFO(set))
		ip_set_init_skbinfo(ext_skbinfo(data, set), ext);
	/* Must come last for the case when timed out entry is reused */
	if (SET_WITH_TIMEOUT(set))
		ip_set_timeout_set(ext_timeout(data, set), ext->timeout);
	smp_mb__before_atomic();
	set_bit(j, n->used);
	if (old != ERR_PTR(-ENOENT)) {
		rcu_assign_pointer(hbucket(t, key), n);
		if (old)
			kfree_rcu(old, rcu);
	}

	return 0;
set_full:
	if (net_ratelimit())
		pr_warn("Set %s is full, maxelem %u reached\n",
			set->name, h->maxelem);
	return -IPSET_ERR_HASH_FULL;
}

/* Delete an element from the hash and free up space if possible.
 */
static int
mtype_del(struct ip_set *set, void *value, const struct ip_set_ext *ext,
	  struct ip_set_ext *mext, u32 flags)
{
	struct htype *h = set->data;
	struct htable *t;
	const struct mtype_elem *d = value;
	struct mtype_elem *data;
	struct hbucket *n;
	int i, j, k, ret = -IPSET_ERR_EXIST;
	u32 key, multi = 0;
	size_t dsize = set->dsize;

	t = ipset_dereference_protected(h->table, set);
	key = HKEY(value, h->initval, t->htable_bits);
	n = __ipset_dereference_protected(hbucket(t, key), 1);
	if (!n)
		goto out;
	for (i = 0, k = 0; i < n->pos; i++) {
		if (!test_bit(i, n->used)) {
			k++;
			continue;
		}
		data = ahash_data(n, i, dsize);
		if (!mtype_data_equal(data, d, &multi))
			continue;
		if (SET_WITH_TIMEOUT(set) &&
		    ip_set_timeout_expired(ext_timeout(data, set)))
			goto out;

		ret = 0;
		clear_bit(i, n->used);
		smp_mb__after_atomic();
		if (i + 1 == n->pos)
			n->pos--;
		set->elements--;
#ifdef IP_SET_HASH_WITH_NETS
		for (j = 0; j < IPSET_NET_COUNT; j++)
			mtype_del_cidr(h, NCIDR_PUT(DCIDR_GET(d->cidr, j)),
				       j);
#endif
		ip_set_ext_destroy(set, data);

		for (; i < n->pos; i++) {
			if (!test_bit(i, n->used))
				k++;
		}
		if (n->pos == 0 && k == 0) {
			set->ext_size -= ext_size(n->size, dsize);
			rcu_assign_pointer(hbucket(t, key), NULL);
			kfree_rcu(n, rcu);
		} else if (k >= AHASH_INIT_SIZE) {
			struct hbucket *tmp = kzalloc(sizeof(*tmp) +
					(n->size - AHASH_INIT_SIZE) * dsize,
					GFP_ATOMIC);
			if (!tmp)
				goto out;
			tmp->size = n->size - AHASH_INIT_SIZE;
			for (j = 0, k = 0; j < n->pos; j++) {
				if (!test_bit(j, n->used))
					continue;
				data = ahash_data(n, j, dsize);
				memcpy(tmp->value + k * dsize, data, dsize);
				set_bit(k, tmp->used);
				k++;
			}
			tmp->pos = k;
			set->ext_size -= ext_size(AHASH_INIT_SIZE, dsize);
			rcu_assign_pointer(hbucket(t, key), tmp);
			kfree_rcu(n, rcu);
		}
		goto out;
	}

out:
	return ret;
}

static inline int
mtype_data_match(struct mtype_elem *data, const struct ip_set_ext *ext,
		 struct ip_set_ext *mext, struct ip_set *set, u32 flags)
{
	if (!ip_set_match_extensions(set, ext, mext, flags, data))
		return 0;
	/* nomatch entries return -ENOTEMPTY */
	return mtype_do_data_match(data);
}

#ifdef IP_SET_HASH_WITH_NETS
/* Special test function which takes into account the different network
 * sizes added to the set
 */
static int
mtype_test_cidrs(struct ip_set *set, struct mtype_elem *d,
		 const struct ip_set_ext *ext,
		 struct ip_set_ext *mext, u32 flags)
{
	struct htype *h = set->data;
	struct htable *t = rcu_dereference_bh(h->table);
	struct hbucket *n;
	struct mtype_elem *data;
#if IPSET_NET_COUNT == 2
	struct mtype_elem orig = *d;
	int ret, i, j = 0, k;
#else
	int ret, i, j = 0;
#endif
	u32 key, multi = 0;

	pr_debug("test by nets\n");
	for (; j < NLEN && h->nets[j].cidr[0] && !multi; j++) {
#if IPSET_NET_COUNT == 2
		mtype_data_reset_elem(d, &orig);
		mtype_data_netmask(d, NCIDR_GET(h->nets[j].cidr[0]), false);
		for (k = 0; k < NLEN && h->nets[k].cidr[1] && !multi;
		     k++) {
			mtype_data_netmask(d, NCIDR_GET(h->nets[k].cidr[1]),
					   true);
#else
		mtype_data_netmask(d, NCIDR_GET(h->nets[j].cidr[0]));
#endif
		key = HKEY(d, h->initval, t->htable_bits);
		n =  rcu_dereference_bh(hbucket(t, key));
		if (!n)
			continue;
		for (i = 0; i < n->pos; i++) {
			if (!test_bit(i, n->used))
				continue;
			data = ahash_data(n, i, set->dsize);
			if (!mtype_data_equal(data, d, &multi))
				continue;
			ret = mtype_data_match(data, ext, mext, set, flags);
			if (ret != 0)
				return ret;
#ifdef IP_SET_HASH_WITH_MULTI
			/* No match, reset multiple match flag */
			multi = 0;
#endif
		}
#if IPSET_NET_COUNT == 2
		}
#endif
	}
	return 0;
}
#endif

/* Test whether the element is added to the set */
static int
mtype_test(struct ip_set *set, void *value, const struct ip_set_ext *ext,
	   struct ip_set_ext *mext, u32 flags)
{
	struct htype *h = set->data;
	struct htable *t;
	struct mtype_elem *d = value;
	struct hbucket *n;
	struct mtype_elem *data;
	int i, ret = 0;
	u32 key, multi = 0;

	t = rcu_dereference_bh(h->table);
#ifdef IP_SET_HASH_WITH_NETS
	/* If we test an IP address and not a network address,
	 * try all possible network sizes
	 */
	for (i = 0; i < IPSET_NET_COUNT; i++)
		if (DCIDR_GET(d->cidr, i) != HOST_MASK)
			break;
	if (i == IPSET_NET_COUNT) {
		ret = mtype_test_cidrs(set, d, ext, mext, flags);
		goto out;
	}
#endif

	key = HKEY(d, h->initval, t->htable_bits);
	n = rcu_dereference_bh(hbucket(t, key));
	if (!n) {
		ret = 0;
		goto out;
	}
	for (i = 0; i < n->pos; i++) {
		if (!test_bit(i, n->used))
			continue;
		data = ahash_data(n, i, set->dsize);
		if (!mtype_data_equal(data, d, &multi))
			continue;
		ret = mtype_data_match(data, ext, mext, set, flags);
		if (ret != 0)
			goto out;
	}
out:
	return ret;
}

/* Reply a HEADER request: fill out the header part of the set */
static int
mtype_head(struct ip_set *set, struct sk_buff *skb)
{
	struct htype *h = set->data;
	const struct htable *t;
	struct nlattr *nested;
	size_t memsize;
	u8 htable_bits;

	/* If any members have expired, set->elements will be wrong
	 * mytype_expire function will update it with the right count.
	 * we do not hold set->lock here, so grab it first.
	 * set->elements can still be incorrect in the case of a huge set,
	 * because elements might time out during the listing.
	 */
	if (SET_WITH_TIMEOUT(set)) {
		spin_lock_bh(&set->lock);
		mtype_expire(set, h);
		spin_unlock_bh(&set->lock);
	}

	rcu_read_lock_bh();
	t = rcu_dereference_bh_nfnl(h->table);
	memsize = mtype_ahash_memsize(h, t) + set->ext_size;
	htable_bits = t->htable_bits;
	rcu_read_unlock_bh();

	nested = ipset_nest_start(skb, IPSET_ATTR_DATA);
	if (!nested)
		goto nla_put_failure;
	if (nla_put_net32(skb, IPSET_ATTR_HASHSIZE,
			  htonl(jhash_size(htable_bits))) ||
	    nla_put_net32(skb, IPSET_ATTR_MAXELEM, htonl(h->maxelem)))
		goto nla_put_failure;
#ifdef IP_SET_HASH_WITH_NETMASK
	if (h->netmask != HOST_MASK &&
	    nla_put_u8(skb, IPSET_ATTR_NETMASK, h->netmask))
		goto nla_put_failure;
#endif
#ifdef IP_SET_HASH_WITH_MARKMASK
	if (nla_put_u32(skb, IPSET_ATTR_MARKMASK, h->markmask))
		goto nla_put_failure;
#endif
	if (nla_put_net32(skb, IPSET_ATTR_REFERENCES, htonl(set->ref)) ||
	    nla_put_net32(skb, IPSET_ATTR_MEMSIZE, htonl(memsize)) ||
	    nla_put_net32(skb, IPSET_ATTR_ELEMENTS, htonl(set->elements)))
		goto nla_put_failure;
	if (unlikely(ip_set_put_flags(skb, set)))
		goto nla_put_failure;
	ipset_nest_end(skb, nested);

	return 0;
nla_put_failure:
	return -EMSGSIZE;
}

/* Make possible to run dumping parallel with resizing */
static void
mtype_uref(struct ip_set *set, struct netlink_callback *cb, bool start)
{
	struct htype *h = set->data;
	struct htable *t;

	if (start) {
		rcu_read_lock_bh();
		t = rcu_dereference_bh_nfnl(h->table);
		atomic_inc(&t->uref);
		cb->args[IPSET_CB_PRIVATE] = (unsigned long)t;
		rcu_read_unlock_bh();
	} else if (cb->args[IPSET_CB_PRIVATE]) {
		t = (struct htable *)cb->args[IPSET_CB_PRIVATE];
		if (atomic_dec_and_test(&t->uref) && atomic_read(&t->ref)) {
			/* Resizing didn't destroy the hash table */
			pr_debug("Table destroy by dump: %p\n", t);
			mtype_ahash_destroy(set, t, false);
		}
		cb->args[IPSET_CB_PRIVATE] = 0;
	}
}

/* Reply a LIST/SAVE request: dump the elements of the specified set */
static int
mtype_list(const struct ip_set *set,
	   struct sk_buff *skb, struct netlink_callback *cb)
{
	const struct htable *t;
	struct nlattr *atd, *nested;
	const struct hbucket *n;
	const struct mtype_elem *e;
	u32 first = cb->args[IPSET_CB_ARG0];
	/* We assume that one hash bucket fills into one page */
	void *incomplete;
	int i, ret = 0;

	atd = ipset_nest_start(skb, IPSET_ATTR_ADT);
	if (!atd)
		return -EMSGSIZE;

	pr_debug("list hash set %s\n", set->name);
	t = (const struct htable *)cb->args[IPSET_CB_PRIVATE];
	/* Expire may replace a hbucket with another one */
	rcu_read_lock();
	for (; cb->args[IPSET_CB_ARG0] < jhash_size(t->htable_bits);
	     cb->args[IPSET_CB_ARG0]++) {
		cond_resched_rcu();
		incomplete = skb_tail_pointer(skb);
		n = rcu_dereference(hbucket(t, cb->args[IPSET_CB_ARG0]));
		pr_debug("cb->arg bucket: %lu, t %p n %p\n",
			 cb->args[IPSET_CB_ARG0], t, n);
		if (!n)
			continue;
		for (i = 0; i < n->pos; i++) {
			if (!test_bit(i, n->used))
				continue;
			e = ahash_data(n, i, set->dsize);
			if (SET_WITH_TIMEOUT(set) &&
			    ip_set_timeout_expired(ext_timeout(e, set)))
				continue;
			pr_debug("list hash %lu hbucket %p i %u, data %p\n",
				 cb->args[IPSET_CB_ARG0], n, i, e);
			nested = ipset_nest_start(skb, IPSET_ATTR_DATA);
			if (!nested) {
				if (cb->args[IPSET_CB_ARG0] == first) {
					nla_nest_cancel(skb, atd);
					ret = -EMSGSIZE;
					goto out;
				}
				goto nla_put_failure;
			}
			if (mtype_data_list(skb, e))
				goto nla_put_failure;
			if (ip_set_put_extensions(skb, set, e, true))
				goto nla_put_failure;
			ipset_nest_end(skb, nested);
		}
	}
	ipset_nest_end(skb, atd);
	/* Set listing finished */
	cb->args[IPSET_CB_ARG0] = 0;

	goto out;

nla_put_failure:
	nlmsg_trim(skb, incomplete);
	if (unlikely(first == cb->args[IPSET_CB_ARG0])) {
		pr_warn("Can't list set %s: one bucket does not fit into a message. Please report it!\n",
			set->name);
		cb->args[IPSET_CB_ARG0] = 0;
		ret = -EMSGSIZE;
	} else {
		ipset_nest_end(skb, atd);
	}
out:
	rcu_read_unlock();
	return ret;
}

static int
IPSET_TOKEN(MTYPE, _kadt)(struct ip_set *set, const struct sk_buff *skb,
			  const struct xt_action_param *par,
			  enum ipset_adt adt, struct ip_set_adt_opt *opt);

static int
IPSET_TOKEN(MTYPE, _uadt)(struct ip_set *set, struct nlattr *tb[],
			  enum ipset_adt adt, u32 *lineno, u32 flags,
			  bool retried);

static const struct ip_set_type_variant mtype_variant = {
	.kadt	= mtype_kadt,
	.uadt	= mtype_uadt,
	.adt	= {
		[IPSET_ADD] = mtype_add,
		[IPSET_DEL] = mtype_del,
		[IPSET_TEST] = mtype_test,
	},
	.destroy = mtype_destroy,
	.flush	= mtype_flush,
	.head	= mtype_head,
	.list	= mtype_list,
	.uref	= mtype_uref,
	.resize	= mtype_resize,
	.same_set = mtype_same_set,
};

#ifdef IP_SET_EMIT_CREATE
static int
IPSET_TOKEN(HTYPE, _create)(struct net *net, struct ip_set *set,
			    struct nlattr *tb[], u32 flags)
{
	u32 hashsize = IPSET_DEFAULT_HASHSIZE, maxelem = IPSET_DEFAULT_MAXELEM;
#ifdef IP_SET_HASH_WITH_MARKMASK
	u32 markmask;
#endif
	u8 hbits;
#ifdef IP_SET_HASH_WITH_NETMASK
	u8 netmask;
#endif
	size_t hsize;
	struct htype *h;
	struct htable *t;

	pr_debug("Create set %s with family %s\n",
		 set->name, set->family == NFPROTO_IPV4 ? "inet" : "inet6");

#ifdef IP_SET_PROTO_UNDEF
	if (set->family != NFPROTO_UNSPEC)
		return -IPSET_ERR_INVALID_FAMILY;
#else
	if (!(set->family == NFPROTO_IPV4 || set->family == NFPROTO_IPV6))
		return -IPSET_ERR_INVALID_FAMILY;
#endif

	if (unlikely(!ip_set_optattr_netorder(tb, IPSET_ATTR_HASHSIZE) ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_MAXELEM) ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_TIMEOUT) ||
		     !ip_set_optattr_netorder(tb, IPSET_ATTR_CADT_FLAGS)))
		return -IPSET_ERR_PROTOCOL;

#ifdef IP_SET_HASH_WITH_MARKMASK
	/* Separated condition in order to avoid directive in argument list */
	if (unlikely(!ip_set_optattr_netorder(tb, IPSET_ATTR_MARKMASK)))
		return -IPSET_ERR_PROTOCOL;

	markmask = 0xffffffff;
	if (tb[IPSET_ATTR_MARKMASK]) {
		markmask = ntohl(nla_get_be32(tb[IPSET_ATTR_MARKMASK]));
		if (markmask == 0)
			return -IPSET_ERR_INVALID_MARKMASK;
	}
#endif

#ifdef IP_SET_HASH_WITH_NETMASK
	netmask = set->family == NFPROTO_IPV4 ? 32 : 128;
	if (tb[IPSET_ATTR_NETMASK]) {
		netmask = nla_get_u8(tb[IPSET_ATTR_NETMASK]);

		if ((set->family == NFPROTO_IPV4 && netmask > 32) ||
		    (set->family == NFPROTO_IPV6 && netmask > 128) ||
		    netmask == 0)
			return -IPSET_ERR_INVALID_NETMASK;
	}
#endif

	if (tb[IPSET_ATTR_HASHSIZE]) {
		hashsize = ip_set_get_h32(tb[IPSET_ATTR_HASHSIZE]);
		if (hashsize < IPSET_MIMINAL_HASHSIZE)
			hashsize = IPSET_MIMINAL_HASHSIZE;
	}

	if (tb[IPSET_ATTR_MAXELEM])
		maxelem = ip_set_get_h32(tb[IPSET_ATTR_MAXELEM]);

	hsize = sizeof(*h);
	h = kzalloc(hsize, GFP_KERNEL);
	if (!h)
		return -ENOMEM;

	/* Compute htable_bits from the user input parameter hashsize.
	 * Assume that hashsize == 2^htable_bits,
	 * otherwise round up to the first 2^n value.
	 */
	hbits = fls(hashsize - 1);
	hsize = htable_size(hbits);
	if (hsize == 0) {
		kfree(h);
		return -ENOMEM;
	}
	t = ip_set_alloc(hsize);
	if (!t) {
		kfree(h);
		return -ENOMEM;
	}
	h->maxelem = maxelem;
#ifdef IP_SET_HASH_WITH_NETMASK
	h->netmask = netmask;
#endif
#ifdef IP_SET_HASH_WITH_MARKMASK
	h->markmask = markmask;
#endif
	get_random_bytes(&h->initval, sizeof(h->initval));

	t->htable_bits = hbits;
	RCU_INIT_POINTER(h->table, t);

	h->set = set;
	set->data = h;
#ifndef IP_SET_PROTO_UNDEF
	if (set->family == NFPROTO_IPV4) {
#endif
		set->variant = &IPSET_TOKEN(HTYPE, 4_variant);
		set->dsize = ip_set_elem_len(set, tb,
			sizeof(struct IPSET_TOKEN(HTYPE, 4_elem)),
			__alignof__(struct IPSET_TOKEN(HTYPE, 4_elem)));
#ifndef IP_SET_PROTO_UNDEF
	} else {
		set->variant = &IPSET_TOKEN(HTYPE, 6_variant);
		set->dsize = ip_set_elem_len(set, tb,
			sizeof(struct IPSET_TOKEN(HTYPE, 6_elem)),
			__alignof__(struct IPSET_TOKEN(HTYPE, 6_elem)));
	}
#endif
	set->timeout = IPSET_NO_TIMEOUT;
	if (tb[IPSET_ATTR_TIMEOUT]) {
		set->timeout = ip_set_timeout_uget(tb[IPSET_ATTR_TIMEOUT]);
#ifndef IP_SET_PROTO_UNDEF
		if (set->family == NFPROTO_IPV4)
#endif
			IPSET_TOKEN(HTYPE, 4_gc_init)(set,
				IPSET_TOKEN(HTYPE, 4_gc));
#ifndef IP_SET_PROTO_UNDEF
		else
			IPSET_TOKEN(HTYPE, 6_gc_init)(set,
				IPSET_TOKEN(HTYPE, 6_gc));
#endif
	}
	pr_debug("create %s hashsize %u (%u) maxelem %u: %p(%p)\n",
		 set->name, jhash_size(t->htable_bits),
		 t->htable_bits, h->maxelem, set->data, t);

	return 0;
}
#endif /* IP_SET_EMIT_CREATE */

#undef HKEY_DATALEN
