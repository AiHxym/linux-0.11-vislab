/*
 *  linux/fs/inode.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <string.h> 
#include <sys/stat.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/system.h>

struct m_inode inode_table[NR_INODE]={{0,},};

static void read_inode(struct m_inode * inode);
static void write_inode(struct m_inode * inode);

static inline void wait_on_inode(struct m_inode * inode)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"wait_on_inode\",\"data\":{\"inode\":%d}}\n",CURRENT_TIME,inode->i_num);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"cli\",\"data\":{}}\n",CURRENT_TIME);
	cli();
	while (inode->i_lock)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sleep_on\",\"data\":{\"inode\":%d}}\n",CURRENT_TIME,inode->i_num);
		sleep_on(&inode->i_wait);
	}
	sti();
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sti\",\"data\":{}}\n",CURRENT_TIME);
}

static inline void lock_inode(struct m_inode * inode)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"lock_inode\",\"data\":{\"inode\":%d}}\n",CURRENT_TIME,inode->i_num);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"cli\",\"data\":{}}\n",CURRENT_TIME);
	cli();
	while (inode->i_lock)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sleep_on\",\"data\":{\"inode\":%d}}\n",CURRENT_TIME,inode->i_num);
		sleep_on(&inode->i_wait);
	}
	inode->i_lock=1;
	sti();
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sti\",\"data\":{}}\n",CURRENT_TIME);
}

static inline void unlock_inode(struct m_inode * inode)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"unlock_inode\",\"data\":{\"inode\":%d}}\n",CURRENT_TIME,inode->i_num);
	inode->i_lock=0;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"wake_up\",\"data\":{\"inode\":%d}}\n",CURRENT_TIME,inode->i_num);
	wake_up(&inode->i_wait);
}

void invalidate_inodes(int dev)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"invalidata_inodes\",\"data\":{\"dev\":%d}}\n",CURRENT_TIME,dev);
	int i;
	struct m_inode * inode;

	inode = 0+inode_table;
	for(i=0 ; i<NR_INODE ; i++,inode++) {
		wait_on_inode(inode);
		if (inode->i_dev == dev) {
			if (inode->i_count)
				printk("inode in use on removed disk\n\r");
			inode->i_dev = inode->i_dirt = 0;
		}
	}
}

void sync_inodes(void)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sync_inodes\",\"data\":{}}\n",CURRENT_TIME);
	
	int i;
	struct m_inode * inode;

	inode = 0+inode_table;
	for(i=0 ; i<NR_INODE ; i++,inode++) {
		wait_on_inode(inode);
		if (inode->i_dirt && !inode->i_pipe)
			write_inode(inode);
	}
}

static int _bmap(struct m_inode * inode,int block,int create)
{
	
	
	struct buffer_head * bh;
	int i;

	if (block<0)
		panic("_bmap: block<0");
	if (block >= 7+512+512*512)
		panic("_bmap: block>big");
	if (block<7) {
		if (create && !inode->i_zone[block])
			if ((inode->i_zone[block]=new_block(inode->i_dev))) {
				inode->i_ctime=CURRENT_TIME;
				inode->i_dirt=1;
			}
		return inode->i_zone[block];
	}
	block -= 7;
	if (block<512) {
		if (create && !inode->i_zone[7])
			if ((inode->i_zone[7]=new_block(inode->i_dev))) {
				inode->i_dirt=1;
				inode->i_ctime=CURRENT_TIME;
			}
		if (!inode->i_zone[7])
			return 0;
		if (!(bh = bread(inode->i_dev,inode->i_zone[7])))
			return 0;
		i = ((unsigned short *) (bh->b_data))[block];
		if (create && !i)
			if ((i=new_block(inode->i_dev))) {
				((unsigned short *) (bh->b_data))[block]=i;
				bh->b_dirt=1;
			}
		brelse(bh);
		return i;
	}
	block -= 512;
	if (create && !inode->i_zone[8])
		if ((inode->i_zone[8]=new_block(inode->i_dev))) {
			inode->i_dirt=1;
			inode->i_ctime=CURRENT_TIME;
		}
	if (!inode->i_zone[8])
	{
		return 0;
	}
	if (!(bh=bread(inode->i_dev,inode->i_zone[8])))
	{
		return 0;
	}
	i = ((unsigned short *)bh->b_data)[block>>9];
	if (create && !i)
		if ((i=new_block(inode->i_dev))) {
			((unsigned short *) (bh->b_data))[block>>9]=i;
			bh->b_dirt=1;
		}
	brelse(bh);
	if (!i)
	{
		return 0;
	}
	if (!(bh=bread(inode->i_dev,i)))
	{
		return 0;
	}
	i = ((unsigned short *)bh->b_data)[block&511];
	if (create && !i)
		if ((i=new_block(inode->i_dev))) {
			((unsigned short *) (bh->b_data))[block&511]=i;
			bh->b_dirt=1;
		}
	brelse(bh);
	return i;
}

int bmap(struct m_inode * inode,int block)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"bmap\",\"data\":{\"inode\":%d,\"block\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,inode->i_num,block);

	int a= _bmap(inode,block,0);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"bmap\",\"data\":{\"inode\":%d,\"block\":%d,\"zone_block\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,inode->i_num,block,a);
	return a;
}

int create_block(struct m_inode * inode, int block)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"creat_block\",\"data\":{\"inode\":%d,\"block\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,inode->i_num,block);

	 int a =_bmap(inode,block,1);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"creat_block\",\"data\":{\"inode\":%d,\"block\":%d,\"zone_block\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,inode->i_num,block,a);
	return a;
}
		
void iput(struct m_inode * inode)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iput\",\"data\":{\"inode\":%d,}}\n",CURRENT_TIME,inode->i_num);
	
	if (!inode)
		return;
	wait_on_inode(inode);
	if (!inode->i_count)
		panic("iput: trying to free free inode");
	if (inode->i_pipe) {
		wake_up(&inode->i_wait);
		if (--inode->i_count)
			return;
		free_page(inode->i_size);
		inode->i_count=0;
		inode->i_dirt=0;
		inode->i_pipe=0;
		return;
	}
	if (!inode->i_dev) {
		inode->i_count--;
		return;
	}
	if (S_ISBLK(inode->i_mode)) {
		sync_dev(inode->i_zone[0]);
		wait_on_inode(inode);
	}
repeat:
	if (inode->i_count>1) {
		inode->i_count--;
		return;
	}
	if (!inode->i_nlinks) {
		truncate(inode);
		free_inode(inode);
		return;
	}
	if (inode->i_dirt) {
		write_inode(inode);	/* we can sleep - so do again */
		wait_on_inode(inode);
		goto repeat;
	}
	inode->i_count--;
	return;
}

struct m_inode * get_empty_inode(void)
{
	struct m_inode * inode;
	static struct m_inode * last_inode = inode_table;
	int i;

	do {
		inode = NULL;
		for (i = NR_INODE; i ; i--) {
			if (++last_inode >= inode_table + NR_INODE)
				last_inode = inode_table;
			if (!last_inode->i_count) {
				inode = last_inode;
				if (!inode->i_dirt && !inode->i_lock)
					break;
			}
		}
		if (!inode) {
			for (i=0 ; i<NR_INODE ; i++)
				printk("%04x: %6d\t",inode_table[i].i_dev,
					inode_table[i].i_num);
			panic("No free inodes in mem");
		}
		wait_on_inode(inode);
		while (inode->i_dirt) {
			write_inode(inode);
			wait_on_inode(inode);
		}
	} while (inode->i_count);
	memset(inode,0,sizeof(*inode));
	inode->i_count = 1;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"get_empty_inode\",\"data\":{\"inode\":%d,}}\n",CURRENT_TIME,inode->i_num);

	return inode;
}

struct m_inode * get_pipe_inode(void)
{
	struct m_inode * inode;

	if (!(inode = get_empty_inode()))
		return NULL;
	if (!(inode->i_size=get_free_page())) {
		inode->i_count = 0;
		return NULL;
	}
	inode->i_count = 2;	/* sum of readers/writers */
	PIPE_HEAD(*inode) = PIPE_TAIL(*inode) = 0;
	inode->i_pipe = 1;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"get_pipe_inode\",\"data\":{\"inode\":%d,}}\n",CURRENT_TIME,inode->i_num);

	return inode;
}

struct m_inode * iget(int dev,int nr)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev,nr);
	
	struct m_inode * inode, * empty;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget1\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev,nr);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget2\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev,nr);

	if (!dev)
	{
		//log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget_dev\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"NO\"}}\n",CURRENT_TIME,dev,nr);
		panic("iget with dev==0");
	}
			//log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget_dev\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"YSE\"}}\n",CURRENT_TIME,dev,nr);

	empty = get_empty_inode();
	inode = inode_table;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget3\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,dev,nr);

	while (inode < NR_INODE+inode_table) {
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget4\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,dev,nr);

		if (inode->i_dev != dev || inode->i_num != nr) {
			if(inode->i_dev!=dev)
			{
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget5\",\"data\":{\"inode\":%d,\"result\":\"NO\"}}\n",CURRENT_TIME,inode);
			}
			else
			{
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget6\",\"data\":{\"inode\":%d,\"result\":\"NO\"}}\n",CURRENT_TIME,inode);
			    log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget5\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,dev,nr);
			}
			inode++;
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget3\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,dev,nr);

			continue;
		}
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget7\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget8\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
		wait_on_inode(inode);
		if (inode->i_dev != dev || inode->i_num != nr) {
			if(inode->i_dev!=dev)
			{
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget9\",\"data\":{\"inode\":%d,\"result\":\"NO\"}}\n",CURRENT_TIME,inode);
			}
			else
			{
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget10\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,dev,nr);
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget9\",\"data\":{\"inode\":%d,\"result\":\"NO\"}}\n",CURRENT_TIME,inode);
			}
			inode = inode_table;
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget3\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,dev,nr);

			continue;
		}
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget11\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
		
		inode->i_count++;
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget12\",\"data\":{\"inode\":%d,\"inode->i_count\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode,inode->i_count);
		if (inode->i_mount) {
			int i;
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget13\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget14\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
			
			for (i = 0 ; i<NR_SUPER ; i++)
				if (super_block[i].s_imount==inode)
				{
					log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget15\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
					break;
				}
			if (i >= NR_SUPER) {
				printk("Mounted inode hasn't got sb\n");
				if (empty)
					iput(empty);
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget17\",\"data\":{\"inode\":%d,\"result\":\"NO\"}}\n",CURRENT_TIME,inode);
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget18\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget\",\"data\":{\"dev\":%d,\"nr\":%d,\"inode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev,nr,inode->i_num);
				return inode;
			}
			iput(inode);
			dev = super_block[i].s_dev;
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget15\",\"data\":{\"dev\":%d,\"nr\":%d,\"inode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev,nr,inode->i_num);

			nr = ROOT_INO;
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget16\",\"data\":{\"inode\":%d,\"dev\"%d,\"nr\"%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode,dev,nr);
			inode = inode_table;
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget3\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
			
			continue;
		}
		if (empty)
			iput(empty);
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget19\",\"data\":{\"inode\":%d,\"result\":\"NO\"}}\n",CURRENT_TIME,inode);
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget20\",\"data\":{\"inode\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode);
			
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget\",\"data\":{\"dev\":%d,\"nr\":%d,\"inode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev,nr,inode->i_num);
		return inode;
	}
	if (!empty)
	{
				log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget\",\"data\":{\"dev\":%d,\"nr\":%d,\"inode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev,nr,inode->i_num);

		return (NULL);
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget21\",\"data\":{\"dev\":%d,\"nr\":%d,\"result\":\"NO\"}}\n",CURRENT_TIME,dev,nr);

	inode=empty;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget22\",\"data\":{\"inode\":%d,\"dev\":%d,\"nr\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode,dev,nr);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget23\",\"data\":{\"inode\":%d,\"dev\":%d,\"nr\":%d,\"result\":\"YES\"}}\n",CURRENT_TIME,inode,dev,nr);

	inode->i_dev = dev;
	inode->i_num = nr;
	read_inode(inode);
	
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"iget\",\"data\":{\"dev\":%d,\"nr\":%d,\"inode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev,nr,inode->i_num);
	return inode;
}

static void read_inode(struct m_inode * inode)
{
	
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"read_inode\",\"data\":{\"inode\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,inode->i_num);
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	lock_inode(inode);
	if (!(sb=get_super(inode->i_dev)))
		panic("trying to read inode without dev");
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num-1)/INODES_PER_BLOCK;
	if (!(bh=bread(inode->i_dev,block)))
		panic("unable to read i-node block");
	*(struct d_inode *)inode =
		((struct d_inode *)bh->b_data)
			[(inode->i_num-1)%INODES_PER_BLOCK];
	brelse(bh);
	unlock_inode(inode);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"read_inode\",\"data\":{\"inode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,inode->i_num);

}

static void write_inode(struct m_inode * inode)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"write_inode\",\"data\":{\"inode\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,inode->i_num);
	struct super_block * sb;
	struct buffer_head * bh;
	int block;

	lock_inode(inode);
	if (!inode->i_dirt || !inode->i_dev) {
		unlock_inode(inode);
		return;
	}
	if (!(sb=get_super(inode->i_dev)))
		panic("trying to write inode without device");
	block = 2 + sb->s_imap_blocks + sb->s_zmap_blocks +
		(inode->i_num-1)/INODES_PER_BLOCK;
	if (!(bh=bread(inode->i_dev,block)))
		panic("unable to read i-node block");
	((struct d_inode *)bh->b_data)
		[(inode->i_num-1)%INODES_PER_BLOCK] =
			*(struct d_inode *)inode;
	bh->b_dirt=1;
	inode->i_dirt=0;
	brelse(bh);
	unlock_inode(inode);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"write_inode\",\"data\":{\"inode\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,inode->i_num);

}
