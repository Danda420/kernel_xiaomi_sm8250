kernelsu-objs := ksu.o
kernelsu-objs += allowlist.o
kernelsu-objs += apk_sign.o
kernelsu-objs += sucompat.o
kernelsu-objs += throne_tracker.o
kernelsu-objs += core_hook.o
kernelsu-objs += ksud.o
kernelsu-objs += embed_ksud.o
kernelsu-objs += kernel_compat.o

kernelsu-objs += selinux/selinux.o
kernelsu-objs += selinux/sepolicy.o
kernelsu-objs += selinux/rules.o
ccflags-y += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include
ccflags-y += -I$(objtree)/security/selinux -include $(srctree)/include/uapi/asm-generic/errno.h

obj-$(CONFIG_KSU) += kernelsu.o

# .git is a text file while the module is imported by 'git submodule add'.
ifeq ($(shell test -e $(srctree)/$(src)/../.git; echo $$?),0)
# We must use the absolute path to git, otherwise the build will fail if git is not in the PATH
KSU_GIT_VERSION := $(shell cd $(srctree)/$(src); /usr/bin/env PATH="$$PATH":/usr/bin:/usr/local/bin git rev-list --count HEAD)
ccflags-y += -DKSU_GIT_VERSION=$(KSU_GIT_VERSION)
endif

ifeq ($(shell grep -q " current_sid(void)" $(srctree)/security/selinux/include/objsec.h; echo $$?),0)
ccflags-y += -DKSU_COMPAT_HAS_CURRENT_SID
endif

ifndef KSU_EXPECTED_SIZE
KSU_EXPECTED_SIZE := 0x033b
endif

ifndef KSU_EXPECTED_HASH
KSU_EXPECTED_HASH := c371061b19d8c7d7d6133c6a9bafe198fa944e50c1b31c9d8daa8d7f1fc2d2d6
endif

ifndef KSU_NEXT_EXPECTED_SIZE
KSU_NEXT_EXPECTED_SIZE := 0x3e6
endif

ifndef KSU_NEXT_EXPECTED_HASH
KSU_NEXT_EXPECTED_HASH := 79e590113c4c4c0c222978e413a5faa801666957b1212a328e46c00c69821bf7
endif

ifndef KSU_LEGACY_EXPECTED_SIZE
KSU_LEGACY_EXPECTED_SIZE := 0x363
endif

ifndef KSU_LEGACY_EXPECTED_HASH
KSU_LEGACY_EXPECTED_HASH := 4359c171f32543394cbc23ef908c4bb94cad7c8087002ba164c8230948c21549
endif

ifndef KSU_N3X7G3N_EXPECTED_SIZE
KSU_N3X7G3N_EXPECTED_SIZE := 0x29c
endif

ifndef KSU_N3X7G3N_EXPECTED_HASH
KSU_N3X7G3N_EXPECTED_HASH := bfddf83a559355b053187177775c39c639d2d2695163baa77253746dbf18098d
endif


ifndef KSU_DKM_EXPECTED_SIZE
KSU_DKM_EXPECTED_SIZE := 0x387
endif

ifndef KSU_DKM_EXPECTED_HASH
KSU_DKM_EXPECTED_HASH := 97075159a623c0c072da0918f61656fb1c70500638790926020be2a10820869b
endif

ifdef KSU_MANAGER_PACKAGE
ccflags-y += -DKSU_MANAGER_PACKAGE=\"$(KSU_MANAGER_PACKAGE)\"
$(info -- KernelSU Manager package name: $(KSU_MANAGER_PACKAGE))
endif

$(info -- KernelSU Manager signature size: $(KSU_EXPECTED_SIZE))
$(info -- KernelSU Manager signature hash: $(KSU_EXPECTED_HASH))
$(info -- KernelSU next Manager signature size: $(KSU_NEXT_EXPECTED_SIZE))
$(info -- KernelSU next Manager signature hash: $(KSU_NEXT_EXPECTED_HASH))
$(info -- KernelSU legacy Manager signature size: $(KSU_LEGACY_EXPECTED_SIZE))
$(info -- KernelSU legacy Manager signature hash: $(KSU_LEGACY_EXPECTED_HASH))
$(info -- KernelSU n3x7g3n Manager signature size: $(KSU_N3X7G3N_EXPECTED_SIZE))
$(info -- KernelSU n3x7g3n Manager signature hash: $(KSU_N3X7G3N_EXPECTED_HASH))
$(info -- DKM SU Manager signature size: $(KSU_DKM_EXPECTED_SIZE))
$(info -- DKM SU Manager signature hash: $(KSU_DKM_EXPECTED_HASH))

ccflags-y += -DEXPECTED_SIZE=$(KSU_EXPECTED_SIZE)
ccflags-y += -DEXPECTED_HASH=\"$(KSU_EXPECTED_HASH)\"
ccflags-y += -DEXPECTED_NEXT_SIZE=$(KSU_NEXT_EXPECTED_SIZE)
ccflags-y += -DEXPECTED_NEXT_HASH=\"$(KSU_NEXT_EXPECTED_HASH)\"
ccflags-y += -DEXPECTED_LEGACY_SIZE=$(KSU_LEGACY_EXPECTED_SIZE)
ccflags-y += -DEXPECTED_LEGACY_HASH=\"$(KSU_LEGACY_EXPECTED_HASH)\"
ccflags-y += -DEXPECTED_N3X7G3N_SIZE=$(KSU_N3X7G3N_EXPECTED_SIZE)
ccflags-y += -DEXPECTED_N3X7G3N_HASH=\"$(KSU_N3X7G3N_EXPECTED_HASH)\"
ccflags-y += -DEXPECTED_DKM_SIZE=$(KSU_DKM_EXPECTED_SIZE)
ccflags-y += -DEXPECTED_DKM_HASH=\"$(KSU_DKM_EXPECTED_HASH)\"

ccflags-y += -Wno-implicit-function-declaration -Wno-strict-prototypes -Wno-int-conversion -Wno-gcc-compat
ccflags-y += -Wno-declaration-after-statement -Wno-unused-function

## For susfs stuff ##
ifeq ($(shell test -e $(srctree)/fs/susfs.c; echo $$?),0)
$(eval SUSFS_VERSION=$(shell cat $(srctree)/include/linux/susfs.h | grep -E '^#define SUSFS_VERSION' | cut -d' ' -f3 | sed 's/"//g'))
$(info )
$(info -- SUSFS_VERSION: $(SUSFS_VERSION))
else
$(info -- You have not integrate susfs in your kernel.)
$(info -- Read: https://gitlab.com/simonpunk/susfs4ksu)
endif
