/*
*  linux/kernel/fork.c
*
*  (C) 1991  Linus Torvalds
*/

/*
*  'fork.c' contains the help-routines for the 'fork' system call
* (see also system_call.s), and some misc functions ('verify_area').
* Fork is rather simple, once you get the hang of it, but the memory
* management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
*/
#include <string.h>
#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

extern void write_verify(unsigned long address);

long last_pid = 0;
int count = 0;

void verify_area(void * addr, int size)
{
	unsigned long start;

	start = (unsigned long)addr;
	size += start & 0xfff;
	start &= 0xfffff000;
	start += get_base(current->ldt[2]);
	
	while (size>0) {
		size -= 4096;
		if (count == 0){
			log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"verify_area\"}\n");
			count++;
		}
		write_verify(start);
		start += 4096;
	}
}

int copy_mem(int nr, struct task_struct * p)
{
	unsigned long old_data_base, new_data_base, data_limit;
	unsigned long old_code_base, new_code_base, code_limit;

	code_limit = get_limit(0x0f);
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"get_code_limit\"}\n");
	data_limit = get_limit(0x17);
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"get_data_limit\"}\n");
	old_code_base = get_base(current->ldt[1]);
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"get_code_base\"}\n");
	old_data_base = get_base(current->ldt[2]);
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"get_data_base\"}\n");
	if (old_data_base != old_code_base)
		panic("We don't support separate I&D");
	if (data_limit < code_limit)
		panic("Bad data_limit");
	new_data_base = new_code_base = nr * 0x4000000;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"new_base\"}\n");
	p->start_code = new_code_base;
	set_base(p->ldt[1], new_code_base);
	set_base(p->ldt[2], new_data_base);
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"set_new_process_page_directory_and_page_table\"}\n");
	if (copy_page_tables(old_data_base, new_data_base, data_limit)) {
		log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"free_page_tables\"}\n");
		
		free_page_tables(new_data_base, data_limit);
		return -ENOMEM;
	}
	return 0;
}

/*
*  Ok, this is the main fork-routine. It copies the system process
* information (task[nr]) and sets up the necessary registers. It
* also copies the data segment in it's entirety.
*/
int copy_process(int nr, long ebp, long edi, long esi, long gs, long none,
	long ebx, long ecx, long edx,
	long fs, long es, long ds,
	long eip, long cs, long eflags, long esp, long ss)
{
	struct task_struct *p;
	int i;
	struct file *f;

	p = (struct task_struct *) get_free_page();
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"type\":\"get_free_page\"}\n");

	if (!p)
		return -EAGAIN;
	log("{\"module\":\"memory\", \"event\":\"copy_process_get\",\"provider\":\"gqz\",\"current_proc\":\"%d\",\
\"data\":{",current->pid);print_task(); log(",\"page\":0x%lx}}\n",p);
	task[nr] = p;

	// NOTE!: the following statement now work with gcc 4.3.2 now, and you
	// must compile _THIS_ memcpy without no -O of gcc.#ifndef GCC4_3
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
	p->state = TASK_UNINTERRUPTIBLE;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"structure\":\"change_new_process_state\",\"type\":\"case_copy_structure\"}\n");
	p->pid = last_pid;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"structure\":\"set_new_process_id\",\"type\":\"case_copy_structure\"}\n");
	p->father = current->pid;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"structure\":\"set_father_process_of_new_process\",\"type\":\"case_copy_structure\"}\n");
	p->counter = p->priority;
	p->signal = 0;
	p->alarm = 0;
	p->leader = 0;		/* process leadership doesn't inherit */
	p->utime = p->stime = 0;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"structure\":\"initialize_usertime_and_system_time\",\"type\":\"case_copy_structure\"}\n");
	p->cutime = p->cstime = 0;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"structure\":\"initialize_new_process_usertime_and_system_time\",\"type\":\"case_copy_structure\"}\n");
	p->start_time = jiffies;
	p->tss.back_link = 0;
	p->tss.esp0 = PAGE_SIZE + (long)p;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"structure\":\"allocate_new_page\",\"type\":\"case_copy_structure\"}\n");
	p->tss.ss0 = 0x10;
	p->tss.eip = eip;
	p->tss.eflags = eflags;
	p->tss.eax = 0;
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"structure\":\"set_local_descriptor_table\",\"type\":\"case_copy_structure\"}\n");
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork\",\"structure\":\"copy_process\",\"type\":\"case_copy_structure\"}\n");
	p->tss.es = es & 0xffff;
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);
	p->tss.trace_bitmap = 0x80000000;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"RRY\",\"event\":\"fork_process\", \"type\": \"\", \"process_id\":%d}\n", p->pid);
	if (last_task_used_math == current)
		__asm__("clts ; fnsave %0"::"m" (p->tss.i387));
	if (copy_mem(nr, p)) {
		task[nr] = NULL;

		free_page((long) p);
		log("{\"module\":\"memory\", \"event\":\"copy_process_free\",\"provider\":\"gqz\",\"current_proc\":\"%d\",\
\"data\":{",current->pid);print_task(); log(",\"page\":0x%lx}}\n",p);
		return -EAGAIN;
	}
	for (i = 0; i<NR_OPEN; i++)
		if ((f = p->filp[i]))
			f->f_count++;
	if (current->pwd)
		current->pwd->i_count++;
	if (current->root)
		current->root->i_count++;
	if (current->executable)
		current->executable->i_count++;
	set_tss_desc(gdt + (nr << 1) + FIRST_TSS_ENTRY, &(p->tss));
	set_ldt_desc(gdt + (nr << 1) + FIRST_LDT_ENTRY, &(p->ldt));
	p->state = TASK_RUNNING;	/* do this last, just in case */

	return last_pid;
}

int find_empty_process(void)
{
	int i;

repeat:
	if ((++last_pid)<0) last_pid = 1;
	for (i = 0; i<NR_TASKS; i++)
		if (task[i] && task[i]->pid == last_pid) goto repeat;
	for (i = 1; i<NR_TASKS; i++)
		if (!task[i])
			return i;
	return -EAGAIN;
}


// void print_task(void)
// {
// 	log("\"task\":{\"start_code\":0x%lx,\"end_code\":0x%lx,\"end_data\":0x%lx,\"brk\":0x%lx,\"start_stack\":0x%lx,\"esp\":0x%lx}",current->start_code,current->end_code,current->end_data,current->brk,current->start_stack,(current->tss).esp);
// }

