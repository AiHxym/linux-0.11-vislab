/*
 *  linux/fs/open.c
 *
 *  (C) 1991  Linus Torvalds
 */

/* #include <string.h> */
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <asm/segment.h>

int sys_ustat(int dev, struct ustat * ubuf)
{
	return -ENOSYS;
}

int sys_utime(char * filename, struct utimbuf * times)
{
	struct m_inode * inode;
	long actime,modtime;

	if (!(inode=namei(filename)))
		return -ENOENT;
	if (times) {
		actime = get_fs_long((unsigned long *) &times->actime);
		modtime = get_fs_long((unsigned long *) &times->modtime);
	} else
		actime = modtime = CURRENT_TIME;
	inode->i_atime = actime;
	inode->i_mtime = modtime;
	inode->i_dirt = 1;
	iput(inode);
	return 0;
}

/*
 * XXX should we use the real or effective uid?  BSD uses the real uid,
 * so as to make this call useful to setuid programs.
 */
int sys_access(const char * filename,int mode)
{
	/*//log("{\n \"module\":\"fs\",\"time\":%d,\n"
	"\"event\":\"sys_access\",\n"
	"\"provide\":\"jsq\",\n"
	"\"time\":-1,\n"
	"\"data\":{\n");*/
	struct m_inode * inode;
	int res, i_mode;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_access\",\"data\":{\"mode\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,mode);

	mode &= 0007;
	if (!(inode=namei(filename)))
	{
		////log("},\n},\n");
		return -EACCES;
	}
	i_mode = res = inode->i_mode & 0777;
	iput(inode);
	if (current->uid == inode->i_uid)
		res >>= 6;
	else if (current->gid == inode->i_gid)
		res >>= 6;
	if ((res & 0007 & mode) == mode)
	{
		////log("},\n},\n");
		return 0;
	}
	/*
	 * XXX we are doing this test last because we really should be
	 * swapping the effective with the real user id (temporarily),
	 * and then calling suser() routine.  If we do call the
	 * suser() routine, it needs to be called last. 
	 */
	if ((!current->uid) &&
	    (!(mode & 1) || (i_mode & 0111)))
	{
		////log("},\n},\n");
		return 0;
	}
	////log("}\n}\n",CURRENT_TIME);
	return -EACCES;
}

int sys_chdir(const char * filename)
{
	/*//log("{\n \"module\":\"fs\",\"time\":%d,\n"
	"\"event\":\"sys_chdir\",\n"
	"\"provide\":\"jsq\",\n"
	"\"time\":-1,\n"
	"\"data\":{\n");
	*/struct m_inode * inode;
log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_chdir\",\"data\":{\"result\":\"start\"}}\n",CURRENT_TIME);

	if (!(inode = namei(filename)))
	{
		////log("},\n},\n");
		return -ENOENT;
	}
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		////log("},\n},\n");
		return -ENOTDIR;
	}
	iput(current->pwd);
	current->pwd = inode;
	////log("},\n},\n");
	return (0);
}

int sys_chroot(const char * filename)
{
	/*//log("{\n \"module\":\"fs\",\"time\":%d,\n"
	"\"event\":\"sys_chroot\",\n"
	"\"provide\":\"jsq\",\n"
	"\"time\":-1,\n"
	"\"data\":{\n");
	*/struct m_inode * inode;
log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_chroot\",\"data\":{\"result\":\"start\"}}\n",CURRENT_TIME);

	if (!(inode=namei(filename)))
	{
		////log("},\n},\n");
		return -ENOENT;
	}
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		////log("},\n},\n");
		return -ENOTDIR;
	}
	iput(current->root);
	current->root = inode;
	////log("},\n},\n");
	return (0);
}

int sys_chmod(const char * filename,int mode)
{
	/*//log("{\n \"module\":\"fs\",\"time\":%d,\n"
	"\"event\":\"sys_chmod\",\n"
	"\"provide\":\"jsq\",\n"
	"\"time\":-1,\n"
	"\"data\":{\n");
	*/struct m_inode * inode;
log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_chmod\",\"data\":{\"mode\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,mode);

	if (!(inode=namei(filename)))
	{
		////log("E");
		////log("},\n},\n");
		return -ENOENT;
	}
	if ((current->euid != inode->i_uid) && !suser()) {
		iput(inode);
		////log("},\n},\n");
		return -EACCES;
	}
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	inode->i_dirt = 1;
	iput(inode);
	////log("},\n},\n");
	return 0;
}

int sys_chown(const char * filename,int uid,int gid)
{
//	//log("{\n \"module\":\"fs\",\"time\":%d,\n"
	//"\"event\":\"sys_chown\",\n"
	//"\"provide\":\"jsq\",\n"
	//"\"time\":-1\n,"
	//"\"data\":{\n");
	struct m_inode * inode;
log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_chown\",\"data\":{\"uid\":%d,\"gid\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,uid,gid);

	if (!(inode=namei(filename)))
	{
		////log("},\n},\n");
		return -ENOENT;
	}
	if (!suser()) {
		iput(inode);
	//	//log("},\n},\n");
		return -EACCES;
	}
	inode->i_uid=uid;
	inode->i_gid=gid;
	inode->i_dirt=1;
	iput(inode);
//	//log("},\n},\n");
	return 0;
}

int sys_open(const char * filename,int flag,int mode)
{
	
	if(flag== O_CREAT | O_TRUNC)
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_creat\",\"data\":{\"flag\":%d,\"mode\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,flag,mode);
	else
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_open\",\"data\":{\"flag\":%d,\"mode\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,flag,mode);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_open\",\"data\":{\"filename\":");
	char c;
	char *a=filename;
	while(c=get_fs_byte(a++))
	{
		log("%c");		
	}
	log(",\"mode\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,flag,mode);
	struct m_inode * inode;
	struct file * f;
	int i,fd;
	
	mode &= 0777 & ~current->umask;
	for(fd=0 ; fd<NR_OPEN ; fd++)
	{
		if (!current->filp[fd])
		{
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"find_empty\",\"data\":{\"fd\":%d}}\n",CURRENT_TIME,fd);
			break;
		}
	}
	if (fd>=NR_OPEN)
	{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"error\",\"data\":{\"errorcode\":\"-EINVAL\",\"result\":\"fd>=NR_OPEN\"}}\n",CURRENT_TIME);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_open\",\"data\":{\"flag\":%d,\"mode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,flag,mode);
	
		return -EINVAL;
	}
	current->close_on_exec &= ~(1<<fd);
	f=0+file_table;

	for (i=0 ; i<NR_FILE ; i++,f++)
	{
		//log("{\"f_num\":%d},",i);
		if (!f->f_count)
		{
			//log("{}],\n");
			break;
		}
	}
	if (i>=NR_FILE)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"error\",\"data\":{\"errorcode\":\"-EINVAL\",\"result\":\"i>=NR_FILE\"}}\n",CURRENT_TIME);
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_open\",\"data\":{\"flag\":%d,\"mode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,flag,mode);
	
		return -EINVAL;
	}
	(current->filp[fd]=f)->f_count++;
	if ((i=open_namei(filename,flag,mode,&inode))<0) {
		current->filp[fd]=NULL;
		f->f_count=0;
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"get_inode\",\"data\":{\"flag\":%d,\"mode\":%d,\"result\":\"fail\"}}\n",CURRENT_TIME,flag,mode);
		return i;
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"get_inode\",\"data\":{\"inode\":%d,\"result\":\"success\"}}\n",CURRENT_TIME,inode);
/* ttys are somewhat special (ttyxx major==4, tty major==5) */
	if (S_ISCHR(inode->i_mode)) {
		if (MAJOR(inode->i_zone[0])==4) {
			if (current->leader && current->tty<0) {
				current->tty = MINOR(inode->i_zone[0]);
				tty_table[current->tty].pgrp = current->pgrp;
			}
		} else if (MAJOR(inode->i_zone[0])==5)
			if (current->tty<0) {
				iput(inode);
				current->filp[fd]=NULL;
				f->f_count=0;
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"error\",\"data\":{\"errorcode\":\"-EPERM\",\"result\":\"current->tty<0\"}}\n",CURRENT_TIME);
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_open\",\"data\":{\"flag\":%d,\"mode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,flag,mode);
	
				return -EPERM;
			}
	}
/* Likewise with block-devices: check for floppy_change */
	if (S_ISBLK(inode->i_mode))
		check_disk_change(inode->i_zone[0]);
	f->f_mode = inode->i_mode;
	f->f_flags = flag;
	f->f_count = 1;
	f->f_inode = inode;
	f->f_pos = 0;
	if(flag== O_CREAT | O_TRUNC)
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_creat\",\"data\":{\"flag\":%d,\"mode\":%d,\"result\":\"end\",\"fd\":%d}}\n",CURRENT_TIME,flag,mode,fd);
	else
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_open\",\"data\":{\"flag\":%d,\"mode\":%d,\"result\":\"end\",\"fd\":%d}}\n",CURRENT_TIME,flag,mode,fd);
	return (fd);
}

int sys_creat(const char * pathname, int mode)
{
	return sys_open(pathname, O_CREAT | O_TRUNC, mode);
}

int sys_close(unsigned int fd)
{	/*//log("{\n \"module\":\"fs\",\"time\":%d,\n"
	"\"event\":\"sys_close\",\n"
	"\"provide\":\"jsq\",\n"
	"\"time\":-1,\n"
	"\"data\":{\n");
	*/struct file * filp;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_close\",\"data\":{\"fd\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,fd);

	if (fd >= NR_OPEN)
	{
		return -EINVAL;
	}
	current->close_on_exec &= ~(1<<fd);
	if (!(filp = current->filp[fd]))
	{
		////log("},\n},\n");
		return -EINVAL;
	}
	current->filp[fd] = NULL;
	if (filp->f_count == 0)
		panic("Close: file count is 0");
	if (--filp->f_count)
	{
		////log("},\n},\n");
		return (0);
	}
	iput(filp->f_inode);
	////log("},\n},\n");
	return (0);
}
