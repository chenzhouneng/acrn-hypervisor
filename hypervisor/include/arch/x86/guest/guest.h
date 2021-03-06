/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GUEST_H
#define GUEST_H

/* Defines for VM Launch and Resume */
#define VM_RESUME               0
#define VM_LAUNCH               1

#define ACRN_DBG_PTIRQ		6U
#define ACRN_DBG_IRQ		6U

#ifndef ASSEMBLER

#include <mmu.h>

#define foreach_vcpu(idx, vm, vcpu)				\
	for ((idx) = 0U, vcpu = &(vm->hw.vcpu_array[(idx)]);	\
		(idx) < vm->hw.created_vcpus;			\
		(idx)++, vcpu = &(vm->hw.vcpu_array[(idx)])) \
		if (vcpu->state != VCPU_OFFLINE)

/* the index is matched with emulated msrs array*/
#define IDX_TSC_DEADLINE		0U
#define IDX_BIOS_UPDT_TRIG		(IDX_TSC_DEADLINE + 1U)
#define IDX_BIOS_SIGN_ID		(IDX_BIOS_UPDT_TRIG + 1U)
#define IDX_TSC		(IDX_BIOS_SIGN_ID + 1U)
#define IDX_PAT		(IDX_TSC + 1U)
#define IDX_APIC_BASE	(IDX_PAT + 1U)
#define IDX_MAX_MSR	(IDX_APIC_BASE + 1U)

/*
 * VCPU related APIs
 */
#define ACRN_REQUEST_EXCP      0U
#define ACRN_REQUEST_EVENT    1U
#define ACRN_REQUEST_EXTINT   2U
#define ACRN_REQUEST_NMI        3U
#define ACRN_REQUEST_TMR_UPDATE  4U
#define ACRN_REQUEST_EPT_FLUSH      5U
#define ACRN_REQUEST_TRP_FAULT      6U
#define ACRN_REQUEST_VPID_FLUSH    7U /* flush vpid tlb */

#define E820_MAX_ENTRIES    32U

#define save_segment(seg, SEG_NAME)				\
{								\
	(seg).selector = exec_vmread16(SEG_NAME##_SEL);		\
	(seg).base = exec_vmread(SEG_NAME##_BASE);		\
	(seg).limit = exec_vmread32(SEG_NAME##_LIMIT);		\
	(seg).attr = exec_vmread32(SEG_NAME##_ATTR);		\
}

#define load_segment(seg, SEG_NAME)				\
{								\
	exec_vmwrite16(SEG_NAME##_SEL, (seg).selector);		\
	exec_vmwrite(SEG_NAME##_BASE, (seg).base);		\
	exec_vmwrite32(SEG_NAME##_LIMIT, (seg).limit);		\
	exec_vmwrite32(SEG_NAME##_ATTR, (seg).attr);		\
}

/* Define segments constants for guest */
#define REAL_MODE_BSP_INIT_CODE_SEL     (0xf000U)
#define REAL_MODE_DATA_SEG_AR           (0x0093U)
#define REAL_MODE_CODE_SEG_AR           (0x009fU)
#define PROTECTED_MODE_DATA_SEG_AR      (0xc093U)
#define PROTECTED_MODE_CODE_SEG_AR      (0xc09bU)
#define DR7_INIT_VALUE                  (0x400UL)
#define LDTR_AR                         (0x0082U) /* LDT, type must be 2, refer to SDM Vol3 26.3.1.2 */
#define TR_AR                           (0x008bU) /* TSS (busy), refer to SDM Vol3 26.3.1.2 */

struct e820_mem_params {
	uint64_t mem_bottom;
	uint64_t mem_top;
	uint64_t total_mem_size;
	uint64_t max_ram_blk_base; /* used for the start address of UOS */
	uint64_t max_ram_blk_size;
};

int prepare_vm0_memmap_and_e820(struct vm *vm);
uint64_t e820_alloc_low_memory(uint32_t size_arg);

/* Definition for a mem map lookup */
struct vm_lu_mem_map {
	struct list_head list;                 /* EPT mem map lookup list*/
	void *hpa;	/* Host physical start address of the map*/
	void *gpa;	/* Guest physical start address of the map */
	uint64_t size;	/* Size of map */
};

/* Use # of paging level to identify paging mode */
enum vm_paging_mode {
	PAGING_MODE_0_LEVEL = 0U,	/* Flat */
	PAGING_MODE_2_LEVEL = 2U,	/* 32bit paging, 2-level */
	PAGING_MODE_3_LEVEL = 3U,	/* PAE paging, 3-level */
	PAGING_MODE_4_LEVEL = 4U,	/* 64bit paging, 4-level */
	PAGING_MODE_NUM,
};

/*
 * VM related APIs
 */
uint64_t vcpumask2pcpumask(struct vm *vm, uint64_t vdmask);

int gva2gpa(struct vcpu *vcpu, uint64_t gva, uint64_t *gpa, uint32_t *err_code);

enum vm_paging_mode get_vcpu_paging_mode(struct vcpu *vcpu);

void init_e820(void);
void obtain_e820_mem_info(void);
extern uint32_t e820_entries;
extern struct e820_entry e820[E820_MAX_ENTRIES];

#ifdef CONFIG_PARTITION_MODE
/*
 * Default e820 mem map:
 *
 * Assumption is every VM launched by ACRN in partition mode uses 2G of RAM.
 * there is reserved memory of 64K for MPtable and PCI hole of 512MB
 */
#define NUM_E820_ENTRIES        5U
extern const struct e820_entry e820_default_entries[NUM_E820_ENTRIES];
#endif

extern uint32_t boot_regs[2];
extern struct e820_mem_params e820_mem;

int rdmsr_vmexit_handler(struct vcpu *vcpu);
int wrmsr_vmexit_handler(struct vcpu *vcpu);
void init_msr_emulation(struct vcpu *vcpu);

struct run_context;
int vmx_vmrun(struct run_context *context, int ops, int ibrs);

int general_sw_loader(struct vm *vm);

typedef int (*vm_sw_loader_t)(struct vm *vm);
extern vm_sw_loader_t vm_sw_loader;

/* @pre Caller(Guest) should make sure gpa is continuous.
 * - gpa from hypercall input which from kernel stack is gpa continuous, not
 *   support kernel stack from vmap
 * - some other gpa from hypercall parameters, VHM should make sure it's
 *   continuous
 * @pre Pointer vm is non-NULL
 */
int copy_from_gpa(struct vm *vm, void *h_ptr, uint64_t gpa, uint32_t size);
/* @pre Caller(Guest) should make sure gpa is continuous.
 * - gpa from hypercall input which from kernel stack is gpa continuous, not
 *   support kernel stack from vmap
 * - some other gpa from hypercall parameters, VHM should make sure it's
 *   continuous
 * @pre Pointer vm is non-NULL
 */
int copy_to_gpa(struct vm *vm, void *h_ptr, uint64_t gpa, uint32_t size);
int copy_from_gva(struct vcpu *vcpu, void *h_ptr, uint64_t gva,
	uint32_t size, uint32_t *err_code, uint64_t *fault_addr);
int copy_to_gva(struct vcpu *vcpu, void *h_ptr, uint64_t gva,
	uint32_t size, uint32_t *err_code, uint64_t *fault_addr);
extern struct acrn_vcpu_regs vm0_boot_context;

#endif	/* !ASSEMBLER */

#endif /* GUEST_H*/
