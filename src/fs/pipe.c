/*
 *  linux/fs/pipe.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <signal.h>

#include <linux/sched.h>
#include <linux/mm.h>	/* for get_free_page */
#include <asm/segment.h>

int read_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, read = 0;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"read_pipe\",\"process_id\": %d}\n", current->pid);
	int wait_write = 1;
	while (count>0) {
		while (!(size=PIPE_SIZE(*inode))) {
			wake_up(&inode->i_wait);
			if (inode->i_count != 2) /* are there any writers? */
				return read;
			
			if(wait_write){
				wait_write = 0;
				log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"wait_write\",\"process_id\": %d}\n",current->pid);
			}
			sleep_on(&inode->i_wait);
			
		}
		chars = PAGE_SIZE-PIPE_TAIL(*inode);
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;
		count -= chars;
		read += chars;
		size = PIPE_TAIL(*inode);
		log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"read_pointer_set\",\"process_id\": %d,\"num\":%d}\n",current->pid, size);
		PIPE_TAIL(*inode) += chars;
		PIPE_TAIL(*inode) &= (PAGE_SIZE-1);
		log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"read_byte\",\"process_id\": %d,\"num\":%d}\n",current->pid,chars);
		while (chars-->0)
			put_fs_byte(((char *)inode->i_size)[size++],buf++);
	}
	wake_up(&inode->i_wait);
	return read;
}
	
int write_pipe(struct m_inode * inode, char * buf, int count)
{
	int chars, size, written = 0;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"write_pipe\",\"process_id\": %d}\n", current->pid);
	int wait_read = 1;
	while (count>0) {
		while (!(size=(PAGE_SIZE-1)-PIPE_SIZE(*inode))) {
			wake_up(&inode->i_wait);
			if (inode->i_count != 2) { /* no readers */
				current->signal |= (1<<(SIGPIPE-1));
				return written?written:-1;
			}
			sleep_on(&inode->i_wait);
			if(wait_read){
				wait_read = 0;
				log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"wait_read\",\"process_id\": %d}\n",current->pid);
			}
		}
		chars = PAGE_SIZE-PIPE_HEAD(*inode);
		if (chars > count)
			chars = count;
		if (chars > size)
			chars = size;
		count -= chars;
		written += chars;
		size = PIPE_HEAD(*inode);
		log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"write_pointer_set\",\"process_id\": %d,\"num\":%d}\n",current->pid, size);
		PIPE_HEAD(*inode) += chars;
		PIPE_HEAD(*inode) &= (PAGE_SIZE-1);
		log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"write_byte\",\"process_id\": %d,\"num\":%d}\n",current->pid,chars);
		while (chars-->0)
			((char *)inode->i_size)[size++]=get_fs_byte(buf++);
	}
	wake_up(&inode->i_wait);
	return written;
}

int sys_pipe(unsigned long * fildes)
{
	struct m_inode * inode;
	struct file * f[2];
	int fd[2];
	int i,j;
	
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"find_item_in_file_table\"}\n");
	j=0;
	for(i=0;j<2 && i<NR_FILE;i++)
		if (!file_table[i].f_count)
			(f[j++]=i+file_table)->f_count++;
	if (j==1)
		f[0]->f_count=0;
	if (j<2){
		log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"file_item_not_found\"}\n");
		return -1;
	}
	j=0;
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"find_item_in_process_table\"}\n");
	for(i=0;j<2 && i<NR_OPEN;i++)
		if (!current->filp[i]) {
			current->filp[ fd[j]=i ] = f[j];
			j++;
		}
	if (j==1)
		current->filp[fd[0]]=NULL;
	if (j<2) {
		log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"process_item_not_found\"}\n");
		f[0]->f_count=f[1]->f_count=0;
		return -1;
	}
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"get_inode\"}\n");
	if (!(inode=get_pipe_inode())) {
		current->filp[fd[0]] =
			current->filp[fd[1]] = NULL;
		f[0]->f_count = f[1]->f_count = 0;
		return -1;
	}
	log("{\"module\":\"process\",\"time\":1234,\"provider\":\"lzk\",\"event\":\"pipe\",\"type\":\"file_init\"}\n");
	f[0]->f_inode = f[1]->f_inode = inode;
	f[0]->f_pos = f[1]->f_pos = 0;
	f[0]->f_mode = 1;		/* read */
	f[1]->f_mode = 2;		/* write */
	put_fs_long(fd[0],0+fildes);
	put_fs_long(fd[1],1+fildes);
	return 0;
}
