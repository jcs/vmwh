/*
 * Copyright (c) 2007 David Crawshaw <david@zentus.com>
 * Copyright (c) 2008 David Gwynne <dlg@openbsd.org>
 * Copyright (c) 2010 joshua stein <jcs@jcs.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <vis.h>

#include "vmwh.h"

int mouse_grabbed = 0;

/* "The" magic number, always occupies the EAX register. */
#define VM_MAGIC			0x564D5868

/* Port numbers, passed on EDX.LOW . */
#define VM_PORT_CMD			0x5658
#define VM_PORT_RPC			0x5659

/* Commands, passed on ECX.LOW. */
#define VM_CMD_GET_MOUSEPOS		0x04
#define	 VM_MOUSE_UNGRABBED_POS		-100
#define VM_CMD_SET_MOUSEPOS		0x05
#define VM_CMD_GET_CLIPBOARD_LEN	0x06
#define VM_CMD_GET_CLIPBOARD		0x07
#define VM_CMD_SET_CLIPBOARD_LEN	0x08
#define VM_CMD_SET_CLIPBOARD		0x09
#define VM_CMD_GET_VERSION		0x0a
#define  VM_VERSION_UNMANAGED			0x7fffffff

/* A register. */
union vm_reg {
	struct {
		uint16_t low;
		uint16_t high;
	} part;
	uint32_t word;
#ifdef __amd64__
	struct {
		uint32_t low;
		uint32_t high;
	} words;
	uint64_t quad;
#endif
} __packed;

/* A register frame. */
struct vm_backdoor {
	union vm_reg eax;
	union vm_reg ebx;
	union vm_reg ecx;
	union vm_reg edx;
	union vm_reg esi;
	union vm_reg edi;
	union vm_reg ebp;
} __packed;

void vm_cmd(struct vm_backdoor *);
void vm_ins(struct vm_backdoor *);
void vm_outs(struct vm_backdoor *);

#define BACKDOOR_OP_I386(op, frame)		\
	__asm__ __volatile__ (			\
		"pushal;"			\
		"pushl %%eax;"			\
		"movl 0x18(%%eax), %%ebp;"	\
		"movl 0x14(%%eax), %%edi;"	\
		"movl 0x10(%%eax), %%esi;"	\
		"movl 0x0c(%%eax), %%edx;"	\
		"movl 0x08(%%eax), %%ecx;"	\
		"movl 0x04(%%eax), %%ebx;"	\
		"movl 0x00(%%eax), %%eax;"	\
		op				\
		"xchgl %%eax, 0x00(%%esp);"	\
		"movl %%ebp, 0x18(%%eax);"	\
		"movl %%edi, 0x14(%%eax);"	\
		"movl %%esi, 0x10(%%eax);"	\
		"movl %%edx, 0x0c(%%eax);"	\
		"movl %%ecx, 0x08(%%eax);"	\
		"movl %%ebx, 0x04(%%eax);"	\
		"popl 0x00(%%eax);"		\
		"popal;"			\
		::"a"(frame)			\
	)

#define BACKDOOR_OP_AMD64(op, frame)		\
	__asm__ __volatile__ (			\
		"pushq %%rbp;			\n\t" \
		"pushq %%rax;			\n\t" \
		"movq 0x30(%%rax), %%rbp;	\n\t" \
		"movq 0x28(%%rax), %%rdi;	\n\t" \
		"movq 0x20(%%rax), %%rsi;	\n\t" \
		"movq 0x18(%%rax), %%rdx;	\n\t" \
		"movq 0x10(%%rax), %%rcx;	\n\t" \
		"movq 0x08(%%rax), %%rbx;	\n\t" \
		"movq 0x00(%%rax), %%rax;	\n\t" \
		op				"\n\t" \
		"xchgq %%rax, 0x00(%%rsp);	\n\t" \
		"movq %%rbp, 0x30(%%rax);	\n\t" \
		"movq %%rdi, 0x28(%%rax);	\n\t" \
		"movq %%rsi, 0x20(%%rax);	\n\t" \
		"movq %%rdx, 0x18(%%rax);	\n\t" \
		"movq %%rcx, 0x10(%%rax);	\n\t" \
		"movq %%rbx, 0x08(%%rax);	\n\t" \
		"popq 0x00(%%rax);		\n\t" \
		"popq %%rbp;			\n\t" \
		: /* No outputs. */ : "a" (frame) \
		  /* No pushal on amd64 so warn gcc about the clobbered registers. */ \
		: "rbx", "rcx", "rdx", "rdi", "rsi", "cc", "memory" \
	)


#ifdef __i386__
#define BACKDOOR_OP(op, frame) BACKDOOR_OP_I386(op, frame)
#else
#define BACKDOOR_OP(op, frame) BACKDOOR_OP_AMD64(op, frame)
#endif

void
vm_cmd(struct vm_backdoor *frame)
{
	BACKDOOR_OP("inl %%dx, %%eax;", frame);
}

void
vm_ins(struct vm_backdoor *frame)
{
	BACKDOOR_OP("cld;\n\trep insb;", frame);
}

void
vm_outs(struct vm_backdoor *frame)
{
	BACKDOOR_OP("cld;\n\trep outsb;", frame);
}

void
vmware_check_version(void)
{
	struct vm_backdoor frame;

	bzero(&frame, sizeof(frame));

	frame.eax.word = VM_MAGIC;
	frame.ebx.word = ~VM_MAGIC;
	frame.ecx.part.low = VM_CMD_GET_VERSION;
	frame.ecx.part.high = 0xffff;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = 0;

	vm_cmd(&frame);

	if (debug)
		printf("vmware_check_version: received version 0x%x\n",
			frame.eax.word);

	if (frame.eax.word == 0xffffffff ||
	    frame.ebx.word != VM_MAGIC)
	    	errx(1, "not running under vmware\n");
}

void
vmware_get_mouse_position(void)
{
	struct vm_backdoor frame;
	int was_grabbed = mouse_grabbed;

	bzero(&frame, sizeof(frame));

	frame.eax.word      = VM_MAGIC;
	frame.ecx.part.low  = VM_CMD_GET_MOUSEPOS;
	frame.ecx.part.high = 0xffff;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = 0;

	vm_cmd(&frame);

	if ((int16_t)(frame.eax.word >> 16) == VM_MOUSE_UNGRABBED_POS)
		mouse_grabbed = 0;
	else
		mouse_grabbed = 1;

	if (debug && (was_grabbed != mouse_grabbed))
		printf("vmware_get_mouse_position: host cursor is %s\n",
			(mouse_grabbed ? "grabbed" : "ungrabbed"));
}

int
vmware_get_clipboard(char **buf)
{
	struct vm_backdoor frame;
	uint32_t total, left;
	char *tbuf;

	bzero(&frame, sizeof(frame));

	frame.eax.word      = VM_MAGIC;
	frame.ecx.part.low  = VM_CMD_GET_CLIPBOARD_LEN;
	frame.ecx.part.high = 0xffff;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = 0;

	vm_cmd(&frame);

	total = left = frame.eax.word;

	if (total == 0 || total > 0xffff) {
		if (debug)
			printf("vmware_get_clipboard: nothing there\n");

		return (0);
	}

	if (debug)
		printf("vmware_get_clipboard: have %d byte%s to read\n",
			total, (total == 1 ? "" : "s"));

	if ((tbuf = malloc(total + 1)) == NULL)
		err(1, "malloc");

	for (;;) {
		bzero(&frame, sizeof(frame));

		frame.eax.word      = VM_MAGIC;
		frame.ecx.part.low  = VM_CMD_GET_CLIPBOARD;
		frame.ecx.part.high = 0xffff;
		frame.edx.part.low  = VM_PORT_CMD;
		frame.edx.part.high = 0;

		vm_cmd(&frame);

		memcpy(tbuf + (total - left), &frame.eax.word,
			left > 4 ? 4 : left);

		if (left <= 4) {
			tbuf[total] = '\0';
			break;
		} else
			left -= 4;
	}

	*buf = tbuf;

	if (debug) {
		char visbuf[strlen(*buf) * 4];
		strnvis(visbuf, *buf, sizeof(visbuf), VIS_TAB | VIS_NL | VIS_CSTYLE);
		printf("vmware_get_clipboard: \"%s\"\n", visbuf);
	}

	return (1);
}

void
vmware_set_clipboard(char *buf)
{
	struct vm_backdoor frame;
	uint32_t total, left;

	if (debug) {
		char visbuf[strlen(buf) * 4];
		strnvis(visbuf, buf, sizeof(visbuf), VIS_TAB | VIS_NL | VIS_CSTYLE);
		printf("vmware_set_clipboard: \"%s\"\n", visbuf);
	}

	if (!strlen(buf))
		return;

	bzero(&frame, sizeof(frame));

	frame.eax.word      = VM_MAGIC;
	frame.ecx.part.low  = VM_CMD_SET_CLIPBOARD_LEN;
	frame.ecx.part.high = 0xffff;
	frame.edx.part.low  = VM_PORT_CMD;
	frame.edx.part.high = 0;
	frame.ebx.word      = (uint32_t)strlen(buf);

	vm_cmd(&frame);

	total = left = strlen(buf);

	for (;;) {
		bzero(&frame, sizeof(frame));

		frame.eax.word      = VM_MAGIC;
		frame.ecx.part.low  = VM_CMD_SET_CLIPBOARD;
		frame.ecx.part.high = 0xffff;
		frame.edx.part.low  = VM_PORT_CMD;
		frame.edx.part.high = 0;

		memcpy(&frame.ebx.word, buf + (total - left),
			left > 4 ? 4 : left);

		vm_cmd(&frame);

		if (left <= 4)
			break;
		else
			left -= 4;
	}

	return;
}
