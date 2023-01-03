# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2020 Oplus. All rights reserved.
obj-$(CONFIG_OPLUS_FEATURE_DUMP_DEVICE_INFO)	+=  dump_device_info.o
obj-$(CONFIG_OPLUS_FEATURE_OPROJECT) 	+= oplus_project.o
obj-$(CONFIG_OPLUS_FEATURE_SELINUX_CONTROL_LOG) += proc.o
obj-$(CONFIG_OPLUS_FEATURE_PMIC_MONITOR)     += oplus_pmic_monitor/
obj-$(CONFIG_PROCESS_RECLAIM_ENHANCE)     += oplus_performance/oplus_mm/process_reclaim/
obj-$(CONFIG_OPLUS_FEATURE_ZRAM_OPT)     += oplus_performance/oplus_mm/zram_opt/
obj-$(CONFIG_ZWB_HANDLE) += oplus_performance/zwb_handle/
obj-$(CONFIG_OPLUS_FEATURE_PMIC_MONITOR)     += oplus_pmic_monitor/
obj-$(CONFIG_OPLUS_FEATURE_FDLEAK_CHECK) += oplus_fdleak_check.o
obj-$(CONFIG_COLOR_CTRL_V2) += colorctrl.o
obj-$(CONFIG_OPLUS_ION_BOOSTPOOL) += oplus_performance/ion_boost_pool/oplus_ion_boost_pool.o
obj-$(CONFIG_OPLUS_FEATURE_GAME_OPT) += game_opt/
obj-y += power/midas/

#obj-y += oplus_performance/foreground_io_opt/
obj-y += network/
obj-y += wakelock_profiler/
obj-y += oplus_performance/healthinfo/fs/
obj-y += oplus_performance/healthinfo/mm/

#Add for NandSwap
obj-$(CONFIG_NANDSWAP)		+= oplus_performance/oplus_nandswap/
