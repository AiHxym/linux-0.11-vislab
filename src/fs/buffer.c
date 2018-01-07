/*
 *  linux/fs/buffer.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'buffer.c' implements the buffer-cache functions. Race-conditions have
 * been avoided by NEVER letting a interrupt change a buffer (except for the
 * data, of course), but instead letting the caller do it. NOTE! As interrupts
 * can wake up a caller, some cli-sti sequences are needed to check for
 * sleep-on-calls. These should be extremely quick, though (I hope).
 */

/*
 * NOTE! There is one discordant note here: checking floppies for
 * disk change. This is where it fits best, I think, as it should
 * invalidate changed floppy-disk-caches.
 */

#include <stdarg.h>
 
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/system.h>
#include <asm/io.h>

extern int end;
extern void put_super(int);
extern void invalidate_inodes(int);

struct buffer_head * start_buffer = (struct buffer_head *) &end;
struct buffer_head * hash_table[NR_HASH];
static struct buffer_head * free_list;
static struct task_struct * buffer_wait = NULL;
int NR_BUFFERS = 0;

static inline void wait_on_buffer(struct buffer_head * bh)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"wait_on_buffer\","
		"\"data\":{"
		"\"buffer\":%d}}\n",CURRENT_TIME,bh->b_blocknr);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"cli\","
		"\"data\":{}}\n",CURRENT_TIME);
	cli();
	
	while (bh->b_lock)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sleep_on\","
		"\"data\":{"
		"\"buffer\":%d}}\n",CURRENT_TIME,bh->b_blocknr);
		sleep_on(&bh->b_wait);
	}
	sti();
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sti\","
		"\"data\":{}}\n",CURRENT_TIME);
}

int sys_sync(void)
{
	int i;
	struct buffer_head * bh;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sys_sync\","
			"\"data\":{}}\n",CURRENT_TIME);
	sync_inodes();		/* write out inodes into buffers */
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		wait_on_buffer(bh);
		if (bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	return 0;
}

int sync_dev(int dev)
{
	int i;
	struct buffer_head * bh;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sync_dev\","
			"\"data\":{\"dev\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev);
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	sync_inodes();
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_dirt)
			ll_rw_block(WRITE,bh);
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sync_dev\","
			"\"data\":{\"dev\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev);
	return 0;
}

static inline void invalidate_buffers(int dev)
{
	int i;
	struct buffer_head * bh;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"invalidate_buffers\","
			"\"data\":{\"dev\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev);
	bh = start_buffer;
	for (i=0 ; i<NR_BUFFERS ; i++,bh++) {
		if (bh->b_dev != dev)
			continue;
		wait_on_buffer(bh);
		if (bh->b_dev == dev)
			bh->b_uptodate = bh->b_dirt = 0;
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"invalidate_buffers\","
			"\"data\":{\"dev\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev);
}

/*
 * This routine checks whether a floppy has been changed, and
 * invalidates all buffer-cache-entries in that case. This
 * is a relatively slow routine, so we have to try to minimize using
 * it. Thus it is called only upon a 'mount' or 'open'. This
 * is the best way of combining speed and utility, I think.
 * People changing diskettes in the middle of an operation deserve
 * to loose :-)
 *
 * NOTE! Although currently this is only for floppies, the idea is
 * that any additional removable block-device will use this routine,
 * and that mount/open needn't know that floppies/whatever are
 * special.
 */
void check_disk_change(int dev)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_disk_change\","
			"\"data\":{\"dev\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev);
	int i;

	if (MAJOR(dev) != 2)
		return;
	if (!floppy_change(dev & 0x03))
		return;
	for (i=0 ; i<NR_SUPER ; i++)
		if (super_block[i].s_dev == dev)
			put_super(super_block[i].s_dev);
	invalidate_inodes(dev);
	invalidate_buffers(dev); 
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_disk_change\","
			"\"data\":{\"dev\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev);
}

#define _hashfn(dev,block) (((unsigned)(dev^block))%NR_HASH)
#define hash(dev,block) hash_table[_hashfn(dev,block)]

static inline void remove_from_queues(struct buffer_head * bh)
{
/* remove from hash-queue */
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"remove_from_queues\",");
	log("\"data\":{\"buffer\":%d,\"buffer_prev\":%d,\"buffer_next\":%d,\"buffer_prev_free\":%d,\"buffer_next_free\":%d",bh->b_blocknr,bh->b_prev,bh->b_next,bh->b_prev_free->b_blocknr,bh->b_next_free->b_blocknr);
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	if (hash(bh->b_dev,bh->b_blocknr) == bh)
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
/* remove from free list */
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("Free block list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;
	if (free_list == bh)
		free_list = bh->b_next_free;
	log("}}\n",CURRENT_TIME);
}

static inline void insert_into_queues(struct buffer_head * bh)
{
/* put at end of free list */
log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"insert_into_queues\",\"data\":{\"buffer\":%d,\"buffer_prev\":%d,\"buffer_next\":%d,\"buffer_prev_free\":%d,\"buffer_next_free\":%d}}\n",
Y
CURRENT_TIME,bh->b_blocknr,bh->b_prev,bh->b_next,bh->b_prev_free->b_blocknr,bh->b_next_free->b_blocknr);
	bh->b_next_free = free_list;
	bh->b_prev_free = free_list->b_prev_free;
	free_list->b_prev_free->b_next_free = bh;
	free_list->b_prev_free = bh;
/* put the buffer in new hash-queue if it has a device */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	if (!bh->b_dev)
		return;
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);
	hash(bh->b_dev,bh->b_blocknr) = bh;
	bh->b_next->b_prev = bh;
}

static struct buffer_head * find_buffer(int dev, int block)
{		
	struct buffer_head * tmp;

	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
		if (tmp->b_dev==dev && tmp->b_blocknr==block)
			return tmp;
	return NULL;
}

/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are readint them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
struct buffer_head * get_hash_table(int dev, int block)
{
	struct buffer_head * bh;
//	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"get_hash_table\","
	//	"\"data\":[");
	
	for (;;) {
		//log("{");
		if (!(bh=find_buffer(dev,block)))
		{
		//	log("}]}");
			return NULL;
		}
		bh->b_count++;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_blocknr == block)
		{
			return bh;
		}
		bh->b_count--;
//		log("}");
	}
	
}

/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algoritm is changed: hopefully better, and an elusive bug removed.
 */
#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)
struct buffer_head * getblk(int dev,int block)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"getblk\",\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev,block);
	struct buffer_head * tmp, * bh;
repeat:
	if ((bh = get_hash_table(dev,block)))
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"get_hash_table\","
			"\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"success\",\"buffer\":%d}}\n",CURRENT_TIME,dev,block,bh->b_blocknr);
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"getblk\",\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev,block);
		return bh;
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"get_hash_table\","
			"\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"fail\"}}\n",CURRENT_TIME,dev,block);
	tmp = free_list;
	do {
		if (tmp->b_count)
			continue;
		if (!bh || BADNESS(tmp)<BADNESS(bh)) {
			bh = tmp;	
			if (!BADNESS(tmp))
				break;
		}
/* and repeat until we find something good */
	} while ((tmp = tmp->b_next_free) != free_list);
	if (!bh) {
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_bh\","
		"\"data\":{"
		"\"buffer\":%d, "
		"\"result\":\"fail\"}}\n",CURRENT_TIME,bh->b_blocknr);
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sleep_on\","
		"\"data\":{"
		"\"buffer\":%d}}\n",CURRENT_TIME,bh->b_blocknr);
		sleep_on(&buffer_wait);
		goto repeat;
	}
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_bh\","
		"\"data\":{"
		"\"buffer\":%d,\"result\":\"success\"}}\n",CURRENT_TIME,bh->b_blocknr);
	
	wait_on_buffer(bh);
	if (bh->b_count)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_occupy\","
			"\"data\":{\"buffer\":%d,\"result\":\"success\"}}\n",CURRENT_TIME,bh->b_blocknr);
		goto repeat;
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_occupy\","
			"\"data\":{\"buffer\":%d,\"result\":\"fail\"}}\n",CURRENT_TIME,bh->b_blocknr);
	while (bh->b_dirt) {
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_dirt\","
		"\"data\":{\"buffer\":%d,\"result\":\"success\"}}\n",CURRENT_TIME,bh->b_blocknr);
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"sync_dev\",\"data\":{\"dev\":%d}}\n",CURRENT_TIME,bh->b_dev);
		sync_dev(bh->b_dev);
		wait_on_buffer(bh);
		if (bh->b_count)
		{
			log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_occupy\","
			"\"data\":{\"buffer\":%d,\"result\":\"success\"}}\n",CURRENT_TIME,bh->b_blocknr);
			goto repeat;
		}
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_occupy\","
			"\"data\":{\"buffer\":%d,\"result\":\"fail\"}}\n",CURRENT_TIME,bh->b_blocknr);
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"check_dirt\","
		"\"data\":{\"buffer\":%d,\"result\":\"fail\"}}\n",CURRENT_TIME,bh->b_blocknr);
/* NOTE!! While we slept waiting for this block, somebody else might */
/* already have added "this" block to the cache. check it */
	if (find_buffer(dev,block))
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"find_buffer\","
			"\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"success\"}}\n",CURRENT_TIME,dev,block);
		goto repeat;
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"find_buffer\","
			"\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"fail\"}}\n",CURRENT_TIME,dev,block);
/* OK, FINALLY we know that this buffer is the only one of it's kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
		
	bh->b_count=1;
	bh->b_dirt=0;
	bh->b_uptodate=0;
	remove_from_queues(bh);
	bh->b_dev=dev;
	bh->b_blocknr=block;
	insert_into_queues(bh);
	log(
	"{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"final_buffer\","
	" \"data\":{\"buffer\":{\"b_data\":%d,  \"b_blocknr\":%d, \"b_dev\":%d, \"b_uptodate\":%d, \"b_dirt\":%d, \"b_count\":%d, \"b_lock\":%d, \"b_wait\":%d, \"b_prev\":%d, \"b_next\":%d, \"b_prev_free\":%d, \"b_next_free\":%d}}}\n",CURRENT_TIME,
	 bh->b_data,bh->b_blocknr,bh->b_dev,bh->b_uptodate,bh->b_dirt,bh->b_count,bh->b_lock,bh->b_wait,bh->b_prev,bh->b_next,bh->b_prev_free->b_blocknr,bh->b_next_free->b_blocknr);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"getblk\",\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,dev,block);
	return bh;
}

void brelse(struct buffer_head * buf)
{
	if (!buf)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"brelse\",\"data\":{\"buffer\":null,\"result\":\"start\"}}\n",CURRENT_TIME);
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"brelse\",\"data\":{\"buffer\":null,\"result\":\"end\"}}\n",CURRENT_TIME);
		return;
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"brelse\",\"data\":{\"buffer\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,buf->b_blocknr);
	wait_on_buffer(buf);
	if (!(buf->b_count--))
		panic("Trying to free free buffer");
	wake_up(&buffer_wait);
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"brelse\",\"data\":{\"buffer\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,buf->b_blocknr);
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
struct buffer_head * bread(int dev,int block)
{
	struct buffer_head * bh;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"bread\",\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev,block);
	if (!(bh=getblk(dev,block)))
	{
		panic("bread: getblk returned NULL ");
	}
	if (bh->b_uptodate)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"bread\",\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"end\",\"buffer\":%d}}\n",CURRENT_TIME,dev,block,bh->b_blocknr);
		return bh;
	}
//	log("\"uptodate1\":0 ,");
	ll_rw_block(READ,bh);
	wait_on_buffer(bh);
	if (bh->b_uptodate)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"bread\",\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"end\",\"buffer\":%d}}\n",CURRENT_TIME,dev,block,bh->b_blocknr);
		return bh;
	}
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"bread\",\"data\":{\"dev\":%d,\"block\":%d,\"result\":\"end\",\"buffer\":null}}\n",CURRENT_TIME,dev,block);
	return NULL;
}

#define COPYBLK(from,to) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"movsl\n\t" \
	::"c" (BLOCK_SIZE/4),"S" (from),"D" (to) \
	)

/*
 * bread_page reads four buffers into memory at the desired address. It's
 * a function of its own, as there is some speed to be got by reading them
 * all at the same time, not waiting for one to be read, and then another
 * etc.
 */
void bread_page(unsigned long address,int dev,int b[4])
{
	struct buffer_head * bh[4];
	int i;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"bread_page\",\"data\":{\"address\":%d,\"dev\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,address,dev);
	for (i=0 ; i<4 ; i++)
	{
		if (b[i])
		{
			if ((bh[i] = getblk(dev,b[i])))
			{
				if (!bh[i]->b_uptodate)
				{
					ll_rw_block(READ,bh[i]);
				}
			}
		} else
			bh[i] = NULL;
	}
//	log("\"bread_page_copy\":[");
	for (i=0 ; i<4 ; i++,address += BLOCK_SIZE)
	{
		if (bh[i])
		{
//			log("{");
			wait_on_buffer(bh[i]);
			if (bh[i]->b_uptodate)
			{
				COPYBLK((unsigned long) bh[i]->b_data,address);
			}
			brelse(bh[i]);
		}
	}
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"bread_page\",\"data\":{\"address\":%d,\"dev\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,address,dev);
}

/*
 * Ok, breada can be used as bread, but additionally to mark other
 * blocks for reading as well. End the argument list with a negative
 * number.
 */
struct buffer_head * breada(int dev,int first, ...)
{
	va_list args;
	struct buffer_head * bh, *tmp;
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"breada\",\"data\":{\"dev\":%d,\"first\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,dev,first);
	va_start(args,first);
	if (!(bh=getblk(dev,first)))
		panic("bread: getblk returned NULL ");
	if (!bh->b_uptodate)
		ll_rw_block(READ,bh);
	while ((first=va_arg(args,int))>=0) {
		tmp=getblk(dev,first);
		if (tmp) {
			if (!tmp->b_uptodate)
				ll_rw_block(READA,tmp);
			tmp->b_count--;
		}
	}
	va_end(args);
	wait_on_buffer(bh);
	if (bh->b_uptodate)
	{
		log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"breada\",\"data\":{\"dev\":%d,\"first\":%d,\"result\":\"end\",\"buffer\":%d}}\n",CURRENT_TIME,dev,first,bh->b_blocknr);
		return bh;
	}
	brelse(bh);
	return (NULL);
}

void buffer_init(long buffer_end)
{
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"buffer_init\",\"data\":{\"buffer_end\":%d,\"result\":\"start\"}}\n",CURRENT_TIME,buffer_end);
	struct buffer_head * h = start_buffer;
	void * b;
	int i;

	if (buffer_end == 1<<20)
		b = (void *) (640*1024);
	else
		b = (void *) buffer_end;
	while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {
		h->b_dev = 0;
		h->b_dirt = 0;
		h->b_count = 0;
		h->b_lock = 0;
		h->b_uptodate = 0;
		h->b_wait = NULL;
		h->b_next = NULL;
		h->b_prev = NULL;
		h->b_data = (char *) b;
		h->b_prev_free = h-1;
		h->b_next_free = h+1;
		h++;
		NR_BUFFERS++;
		if (b == (void *) 0x100000)
			b = (void *) 0xA0000;
			
	}
	h--;
	free_list = start_buffer;
	free_list->b_prev_free = h;
	h->b_next_free = free_list;
	for (i=0;i<NR_HASH;i++)
		hash_table[i]=NULL;
	log("{\"module\":\"fs\",\"time\":%d,\"provider\":\"jsq\",\"event\":\"buffer_init\",\"data\":{\"buffer_end\":%d,\"result\":\"end\"}}\n",CURRENT_TIME,buffer_end);

}	
