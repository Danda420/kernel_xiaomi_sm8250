/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2016-2020, The Linux Foundation. All rights reserved.
 */

#ifndef __WALT_H
#define __WALT_H

static inline bool prefer_spread_on_idle(int cpu)
{
	return false;
}

static inline void walt_sched_init_rq(struct rq *rq) { }

static inline void walt_rotate_work_init(void) { }
static inline void walt_rotation_checkpoint(int nr_big) { }
static inline void walt_update_last_enqueue(struct task_struct *p) { }

static inline void update_task_ravg(struct task_struct *p, struct rq *rq,
				int event, u64 wallclock, u64 irqtime) { }
static inline void walt_inc_cumulative_runnable_avg(struct rq *rq,
		struct task_struct *p)
{
}

static inline unsigned int walt_big_tasks(int cpu)
{
	return 0;
}

static inline void walt_adjust_nr_big_tasks(struct rq *rq,
		int delta, bool inc)
{
}

static inline void inc_nr_big_task(struct walt_sched_stats *stats,
		struct task_struct *p)
{
}

static inline void dec_nr_big_task(struct walt_sched_stats *stats,
		struct task_struct *p)
{
}
static inline void walt_dec_cumulative_runnable_avg(struct rq *rq,
		 struct task_struct *p)
{
}

static inline void fixup_busy_time(struct task_struct *p, int new_cpu) { }
static inline void init_new_task_load(struct task_struct *p)
{
}

static inline void mark_task_starting(struct task_struct *p) { }
static inline void set_window_start(struct rq *rq) { }
static inline int sched_cpu_high_irqload(int cpu) { return 0; }

static inline void sched_account_irqstart(int cpu, struct task_struct *curr,
					  u64 wallclock)
{
}

static inline void update_cluster_topology(void) { }
static inline void init_clusters(void) {}
static inline void sched_account_irqtime(int cpu, struct task_struct *curr,
				 u64 delta, u64 wallclock)
{
}

static inline int same_cluster(int src_cpu, int dst_cpu) { return 1; }
static inline bool do_pl_notif(struct rq *rq) { return false; }

static inline void
inc_rq_walt_stats(struct rq *rq, struct task_struct *p) { }

static inline void
dec_rq_walt_stats(struct rq *rq, struct task_struct *p) { }

static inline void
fixup_walt_sched_stats_common(struct rq *rq, struct task_struct *p,
			      u16 updated_demand_scaled,
			      u16 updated_pred_demand_scaled)
{
}

static inline u64 sched_irqload(int cpu)
{
	return 0;
}

static inline bool walt_should_kick_upmigrate(struct task_struct *p, int cpu)
{
	return false;
}

static inline u64 get_rtgb_active_time(void)
{
	return 0;
}

#endif
