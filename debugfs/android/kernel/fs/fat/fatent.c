/*
 * Copyright (C) 2004, OGAWA Hirofumi
 * Released under GPL v2.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/msdos_fs.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/rbtree.h>
#include "fat.h"


struct fatent_operations {
	void (*ent_blocknr)(struct super_block *, int, int *, sector_t *);
	void (*ent_set_ptr)(struct fat_entry *, int);
	int (*ent_bread)(struct super_block *, struct fat_entry *,
			 int, sector_t);
	int (*ent_get)(struct fat_entry *);
	void (*ent_put)(struct fat_entry *, int);
	int (*ent_next)(struct fat_entry *);
};



static DEFINE_SPINLOCK(fat12_entry_lock);


#ifdef FAT_USE_BLOCK_MANAGER_WGZ
//wanggongzhen.wt add,reason:sdcard write speed is low when more than one file write simultaneously
#define rb_to_bm(node) rb_entry(node,struct blocks_mamager,start_node)
#define debug_print(fmt, args...) pr_debug(fmt, ##args)
struct blocks_mamager
{
	int start;
	int length;
	struct msdos_inode_info *inode;
	struct rb_node start_node;
	struct blocks_mamager *self_for_debug;
//	struct mutex i_lock;	
};


struct rb_root start_root = RB_ROOT;
struct rb_node *last_merged = NULL;

/*
for reduce complexity,use a big lock
current the performance is ok , if need higher performance
please modify this,open i_lock ,use small lock instead
*/
static DEFINE_MUTEX(root_lock);	

static int rb_count = 0;

//return a - b;
typedef int (*add_compare)(struct rb_node *a,struct rb_node *b);
//private function 
void blocks_mamager_rb_add(struct rb_root *root, struct rb_node *node,add_compare c)
{
	struct rb_node **p = &root->rb_node;
	struct rb_node *parent = NULL;
	struct blocks_mamager *fb = rb_to_bm(node);
	while (*p) {
		parent = *p;
		if (c(node,*p) < 0)
			p = &(*p)->rb_left;
		else
			p = &(*p)->rb_right;
	}
//	mutex_lock(&root_lock);
	rb_link_node(node, parent, p);
	rb_insert_color(node, root);
	rb_count++;
//	mutex_unlock(&root_lock);
	debug_print("wgz_fat32 add block rb_count:%d,start:%d,length:%d\n",rb_count,fb->start,fb->length);
}
//private function 
void blocks_mamager_rb_del(struct rb_root *root, struct rb_node *node)
{
	struct blocks_mamager *fb = rb_to_bm(node);
	BUG_ON(RB_EMPTY_NODE(node));
//	mutex_lock(&root_lock);
	rb_erase(node, root);
	RB_CLEAR_NODE(node);
	rb_count--;
//	mutex_unlock(&root_lock);
	debug_print("wgz_fat32 remove block rb_count:%d,start:%d,length:%d\n",rb_count,fb->start,fb->length);
}


typedef int (*try_merge)(struct rb_node *node,int start,int count);
//private function 
struct rb_node *blocks_mamager_merge(struct rb_root *root,int start,int count,try_merge c)
{
	struct rb_node *n = root->rb_node;
	int cr = 0;
	while (n) {
		cr = c(n,start,count);
		if (cr < 0)
			n = n->rb_left;
		else if (cr > 0)
			n = n->rb_right;
		else
			return n;
	}
	return NULL;
}

//private function 
int try_back_merge(struct rb_node *node)
{
	struct blocks_mamager *next_bf = NULL;
	struct rb_node *next_node;
	struct blocks_mamager *current_bf;
	if(node == NULL)
	{
		return -1;
	}
	next_node = rb_next(node);
	if(next_node == NULL)
	{
		return -1;
	}
	current_bf = rb_to_bm(node);
	next_bf = rb_to_bm(next_node);

	debug_print("wgz_fat32 try before s_pos:%d,length:%d,e_pos:%d,length:%d\n"
		,current_bf->start,current_bf->length,next_bf->start,next_bf->length);
	if(current_bf->start+current_bf->length != next_bf->start)
	{
//		mutex_unlock(&next_bf->i_lock);
		return -1;
	}
	//	mutex_lock(&next_bf->i_lock);
	if(next_bf->inode != NULL)
	{
//		mutex_unlock(&next_bf->i_lock);
		if(current_bf->inode == NULL)
		{
			next_bf->inode->blocks_mamager_node = &current_bf->start_node;
			next_bf->inode = NULL;
			current_bf->inode = next_bf->inode;
		}
		else
		{
			debug_print("wgz_fat32 both inode is not null ,can not merge\n");
			return -1;
		}
	}
//	mutex_lock(&current_bf->i_lock);
	current_bf->length += next_bf->length;
//	mutex_unlock(&current_bf->i_lock);
	last_merged = &current_bf->start_node;
	blocks_mamager_rb_del(&start_root , &next_bf->start_node);
	debug_print("wgz_fat32 merge after s_pos:%d,length:%d\n",current_bf->start,current_bf->length);
//	mutex_unlock(&next_bf->i_lock);
	kfree(next_bf);
	return 0;
}
//private function 
int try_front_merge(struct rb_node *node)
{
	last_merged = node;
	if(rb_prev(node) == NULL)
	{
		return -1;
	}
	if(try_back_merge(rb_prev(node)))
	{
		return -1;
	}
	return 0;
}
//private function 
int blocks_mamager_add_compare(struct rb_node *a,struct rb_node *b)
{
	struct blocks_mamager *first = rb_to_bm(a);
	struct blocks_mamager *second = rb_to_bm(b);
	return first->start - second->start;
}


//private function 
struct blocks_mamager* new_blocks_mamager(int start , int length)
{
	struct blocks_mamager *new_bf = kzalloc(sizeof(struct blocks_mamager),GFP_KERNEL);
	if(new_bf == NULL)
	{
		pr_err("wgz no memory ,%s:%d",__FUNCTION__,__LINE__);
		return NULL;
	}
//	mutex_init(&new_bf->i_lock);
	RB_CLEAR_NODE(&new_bf->start_node);
	new_bf->self_for_debug = new_bf;
	new_bf->start = start;
	new_bf->length = length;
	new_bf->inode = NULL;
//	mutex_lock(&root_lock);
	blocks_mamager_rb_add(&start_root,&new_bf->start_node,blocks_mamager_add_compare);
//	mutex_unlock(&root_lock);
	return new_bf;
}
//private function 
static int blocks_mamager_try_merge(struct rb_node *node,int block_start , int count)
{
	struct blocks_mamager *bf = NULL;
	
	bf = rb_to_bm(node);
//	mutex_lock(&bf->i_lock);
	//back_merge
	if(block_start == bf->start+bf->length)
	{
		bf->length += count;
//		mutex_unlock(&bf->i_lock);
		return 0;
	}
	//front merge
	else if(block_start + count == bf->start)
	{
		bf->start -= count;
		bf->length += count;
//		mutex_unlock(&bf->i_lock);
		return 0;
	}
//	mutex_unlock(&bf->i_lock);
	return block_start - bf->start;
}

//publib function
struct rb_node *blocks_mamager_free_blocks(int block_start , int count)
{
	struct rb_node *merged_node = NULL;
	struct blocks_mamager *bm = NULL;
	debug_print("wgz_fat32 free block_start:%d,count=%d\n",block_start,count);
	if(count <= 0)
	{
		return NULL;
	}
	mutex_lock(&root_lock);
	if(last_merged != NULL)
	{
		if(!blocks_mamager_try_merge(last_merged,block_start,count))
		{
			merged_node = last_merged;
		}
	}
	if(merged_node == NULL)
	{
		merged_node = blocks_mamager_merge(&start_root,block_start,count,blocks_mamager_try_merge);
	}
	if(merged_node != NULL)
	{
		bm = rb_to_bm(merged_node);
		debug_print("wgz_fat32 after first merge pos:%d,length:%d,free_s:%d,free_c:%d\n"
			,bm->start,bm->length,block_start,count);
		if(bm->start == block_start)
		{
			try_front_merge(merged_node);
		}
		else
		{
			try_back_merge(merged_node);
		}
	}
	else
	{
		bm = new_blocks_mamager(block_start,count);
		if(bm == NULL)
		{
			merged_node = NULL;
		}
		else
		{
			merged_node = &bm->start_node;
		}
	}
	mutex_unlock(&root_lock);
	return merged_node;
}

//private
int blocks_mamager_alloc_blocks_priv(struct rb_node *node,struct msdos_inode_info *inode , int count)
{
	struct blocks_mamager *bm = rb_to_bm(node);
	int block_start = 0;
//	mutex_lock(&bm->i_lock);
	block_start = bm->start;
	bm->start+=count;
	BUG_ON(bm->length<count);
	bm->length-=count;
	if(bm->length <= 0)
	{
//		mutex_lock(&root_lock);
		inode->blocks_mamager_node = NULL;
		blocks_mamager_rb_del(&start_root,&bm->start_node);
//		mutex_unlock(&root_lock);
		
//		mutex_unlock(&bm->i_lock);
		kfree(bm);
		
	}
	else
	{
//		mutex_unlock(&bm->i_lock);
	}
	debug_print("wgz_fat32 alloc inode:%p,count:%d,block_start:%d,s_start:%d,s_length:%d\n"
		,inode,count,block_start,bm->start,bm->length);
	return block_start;
}
//private 
struct blocks_mamager* blocks_mamager_split(struct blocks_mamager *node)
{
		int start = 0;
		int length = 0;
		struct blocks_mamager *split_node = 0;
		
		debug_print("wgz_fat32 splice s_pos:%d,s_length:%d\n",node->start,node->length);
//		mutex_lock(&node->i_lock);
		if(node->length < 2)
		{
			debug_print("wgz_fat32 splice s_length:%d\n",node->length);
//			mutex_unlock(&node->i_lock);
			return NULL;
		}
		start = node->length/2 + node->start;
		length = node->length-node->length/2;
		node->length -= length;

//		mutex_lock(&root_lock);
		split_node = new_blocks_mamager(start , length);
//		mutex_unlock(&root_lock);
		if(split_node == NULL)
		{
			node->length += length;
		}
//		mutex_unlock(&node->i_lock);
		debug_print("wgz_fat32 splice after s_pos:%d,s_length:%d,s_pos:%d,s_length:%d\n"
			,node->start,node->length,split_node->start,split_node->length);

		return split_node;
}


int blocks_mamager_alloc_blocks(struct msdos_inode_info *inode, int count)
{
	struct blocks_mamager *bm = 0;
	int block_start = -1;
	
	mutex_lock(&root_lock);
	if(inode->blocks_mamager_node != NULL && S_ISREG(inode->vfs_inode.i_mode))
	{
		bm = rb_to_bm(inode->blocks_mamager_node);
//		mutex_lock(&bm->i_lock);
		if(bm->length < count)
		{
			debug_print("wgz_fat32 no enough alloced_node->length=%d,count=%d\n",bm->length,count);
			bm->inode = NULL;
			inode->blocks_mamager_node = NULL;
//			mutex_unlock(&bm->i_lock);
		}
		else
		{
			block_start = blocks_mamager_alloc_blocks_priv(&bm->start_node,inode,count);
			goto unlock;
		}
	}
	else
	{
		struct rb_node *node = rb_first(&start_root);
		int max_length = 0;
		struct blocks_mamager *max_node = 0;
//		mutex_lock(&root_lock);
		while(node != NULL)
		{
			bm = rb_to_bm(node);
//			mutex_lock(&bm->i_lock);
			if(bm->inode == NULL && bm->length >= count)
			{
				debug_print("wgz_fat32 find a new node %p:%d:%d\n",inode,bm->start,bm->length);
				if (S_ISREG(inode->vfs_inode.i_mode))
				{
					inode->blocks_mamager_node = &bm->start_node;
					bm->inode = inode;
				}
				//mutex_unlock(&bm->i_lock);
//				mutex_unlock(&root_lock);
				block_start = blocks_mamager_alloc_blocks_priv(&bm->start_node,inode , count);
				goto unlock;
			}
			else
			{
				if(bm->length > max_length)
				{
					max_node = bm;
					max_length = bm->length;
				}
//				mutex_unlock(&bm->i_lock);
			}
			node = rb_next(node);
		}
//		mutex_unlock(&root_lock);
		if(max_node != NULL)
		{
			bm = blocks_mamager_split(max_node);
			if(bm!=NULL&&bm->length >= count*2+1)
			{
				debug_print("wgz_fat32 splice a new node 0x%x:0x%x\n",bm->start,bm->length);
//				mutex_lock(&bm->i_lock);
				if (S_ISREG(inode->vfs_inode.i_mode))
				{
					inode->blocks_mamager_node = &bm->start_node;
					bm->inode = inode;
				}
				//mutex_unlock(&bm->i_lock);
				block_start = blocks_mamager_alloc_blocks_priv(&bm->start_node,inode , count);
				goto unlock;
			}
		}
	}
unlock:
	mutex_unlock(&root_lock);
	return block_start;
}

//public
void blocks_mamager_on_inode_release(struct msdos_inode_info *inode)
{
	struct rb_node *node ;
	struct blocks_mamager *bm;
	mutex_lock(&root_lock);
	node = inode->blocks_mamager_node;
	if(node == NULL || !S_ISREG(inode->vfs_inode.i_mode))
	{
		goto unlock;
	}
	bm = rb_to_bm(node);
	debug_print("wgz_fat32 inode released %p:%d:%d\n",inode,bm->start,bm->length);
//	mutex_lock(&bm->i_lock);
	bm->inode = NULL;
	inode->blocks_mamager_node = NULL;
	try_back_merge(node);
	try_front_merge(node);
//	mutex_unlock(&bm->i_lock);
unlock:
	mutex_unlock(&root_lock);
}

//public
void blocks_mamager_free_all(void)
{
	struct rb_node *node ;
	struct blocks_mamager *bm = 0;
	debug_print("wgz_fat32 free all blocks_manager %d",rb_count);
	mutex_lock(&root_lock);
	node = rb_first(&start_root);
	while(node != NULL)
	{
		bm = rb_to_bm(node);
		//mutex_lock(&bm->i_lock);
		if(bm->inode)
		{
			debug_print("wgz_fat32 bm inode is not null,maybe error\n");
			bm->inode->blocks_mamager_node = NULL;
			bm->inode = NULL;
		}
		blocks_mamager_rb_del(&start_root,&bm->start_node);
		//mutex_unlock(&bm->i_lock);
		kfree(bm);
		node = rb_first(&start_root);
	}
	mutex_unlock(&root_lock);
}
#endif


static void fat12_ent_blocknr(struct super_block *sb, int entry,
			      int *offset, sector_t *blocknr)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	int bytes = entry + (entry >> 1);
	WARN_ON(entry < FAT_START_ENT || sbi->max_cluster <= entry);
	*offset = bytes & (sb->s_blocksize - 1);
	*blocknr = sbi->fat_start + (bytes >> sb->s_blocksize_bits);
}

static void fat_ent_blocknr(struct super_block *sb, int entry,
			    int *offset, sector_t *blocknr)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	int bytes = (entry << sbi->fatent_shift);
	WARN_ON(entry < FAT_START_ENT || sbi->max_cluster <= entry);
	*offset = bytes & (sb->s_blocksize - 1);
	*blocknr = sbi->fat_start + (bytes >> sb->s_blocksize_bits);
}

static void fat12_ent_set_ptr(struct fat_entry *fatent, int offset)
{
	struct buffer_head **bhs = fatent->bhs;
	if (fatent->nr_bhs == 1) {
		WARN_ON(offset >= (bhs[0]->b_size - 1));
		fatent->u.ent12_p[0] = bhs[0]->b_data + offset;
		fatent->u.ent12_p[1] = bhs[0]->b_data + (offset + 1);
	} else {
		WARN_ON(offset != (bhs[0]->b_size - 1));
		fatent->u.ent12_p[0] = bhs[0]->b_data + offset;
		fatent->u.ent12_p[1] = bhs[1]->b_data;
	}
}

static void fat16_ent_set_ptr(struct fat_entry *fatent, int offset)
{
	WARN_ON(offset & (2 - 1));
	fatent->u.ent16_p = (__le16 *)(fatent->bhs[0]->b_data + offset);
}

static void fat32_ent_set_ptr(struct fat_entry *fatent, int offset)
{
	WARN_ON(offset & (4 - 1));
	fatent->u.ent32_p = (__le32 *)(fatent->bhs[0]->b_data + offset);
}

static int fat12_ent_bread(struct super_block *sb, struct fat_entry *fatent,
			   int offset, sector_t blocknr)
{
	struct buffer_head **bhs = fatent->bhs;

	WARN_ON(blocknr < MSDOS_SB(sb)->fat_start);
	fatent->fat_inode = MSDOS_SB(sb)->fat_inode;

	bhs[0] = sb_bread(sb, blocknr);
	if (!bhs[0])
		goto err;

	if ((offset + 1) < sb->s_blocksize)
		fatent->nr_bhs = 1;
	else {
		/* This entry is block boundary, it needs the next block */
		blocknr++;
		bhs[1] = sb_bread(sb, blocknr);
		if (!bhs[1])
			goto err_brelse;
		fatent->nr_bhs = 2;
	}
	fat12_ent_set_ptr(fatent, offset);
	return 0;

err_brelse:
	brelse(bhs[0]);
err:
	fat_msg(sb, KERN_ERR, "FAT read failed (blocknr %llu)", (llu)blocknr);
	return -EIO;
}

static int fat_ent_bread(struct super_block *sb, struct fat_entry *fatent,
			 int offset, sector_t blocknr)
{
	struct fatent_operations *ops = MSDOS_SB(sb)->fatent_ops;

	WARN_ON(blocknr < MSDOS_SB(sb)->fat_start);
	fatent->fat_inode = MSDOS_SB(sb)->fat_inode;
	fatent->bhs[0] = sb_bread(sb, blocknr);
	if (!fatent->bhs[0]) {
		fat_msg(sb, KERN_ERR, "FAT read failed (blocknr %llu)",
		       (llu)blocknr);
		return -EIO;
	}
	fatent->nr_bhs = 1;
	ops->ent_set_ptr(fatent, offset);
	return 0;
}

static int fat12_ent_get(struct fat_entry *fatent)
{
	u8 **ent12_p = fatent->u.ent12_p;
	int next;

	spin_lock(&fat12_entry_lock);
	if (fatent->entry & 1)
		next = (*ent12_p[0] >> 4) | (*ent12_p[1] << 4);
	else
		next = (*ent12_p[1] << 8) | *ent12_p[0];
	spin_unlock(&fat12_entry_lock);

	next &= 0x0fff;
	if (next >= BAD_FAT12)
		next = FAT_ENT_EOF;
	return next;
}

static int fat16_ent_get(struct fat_entry *fatent)
{
	int next = le16_to_cpu(*fatent->u.ent16_p);
	WARN_ON((unsigned long)fatent->u.ent16_p & (2 - 1));
	if (next >= BAD_FAT16)
		next = FAT_ENT_EOF;
	return next;
}

static int fat32_ent_get(struct fat_entry *fatent)
{
	int next = le32_to_cpu(*fatent->u.ent32_p) & 0x0fffffff;
	WARN_ON((unsigned long)fatent->u.ent32_p & (4 - 1));
	if (next >= BAD_FAT32)
		next = FAT_ENT_EOF;
	return next;
}

static void fat12_ent_put(struct fat_entry *fatent, int new)
{
	u8 **ent12_p = fatent->u.ent12_p;

	if (new == FAT_ENT_EOF)
		new = EOF_FAT12;

	spin_lock(&fat12_entry_lock);
	if (fatent->entry & 1) {
		*ent12_p[0] = (new << 4) | (*ent12_p[0] & 0x0f);
		*ent12_p[1] = new >> 4;
	} else {
		*ent12_p[0] = new & 0xff;
		*ent12_p[1] = (*ent12_p[1] & 0xf0) | (new >> 8);
	}
	spin_unlock(&fat12_entry_lock);

	mark_buffer_dirty_inode(fatent->bhs[0], fatent->fat_inode);
	if (fatent->nr_bhs == 2)
		mark_buffer_dirty_inode(fatent->bhs[1], fatent->fat_inode);
}

static void fat16_ent_put(struct fat_entry *fatent, int new)
{
	if (new == FAT_ENT_EOF)
		new = EOF_FAT16;

	*fatent->u.ent16_p = cpu_to_le16(new);
	mark_buffer_dirty_inode(fatent->bhs[0], fatent->fat_inode);
}

static void fat32_ent_put(struct fat_entry *fatent, int new)
{
	WARN_ON(new & 0xf0000000);
	new |= le32_to_cpu(*fatent->u.ent32_p) & ~0x0fffffff;
	*fatent->u.ent32_p = cpu_to_le32(new);
	mark_buffer_dirty_inode(fatent->bhs[0], fatent->fat_inode);
}

static int fat12_ent_next(struct fat_entry *fatent)
{
	u8 **ent12_p = fatent->u.ent12_p;
	struct buffer_head **bhs = fatent->bhs;
	u8 *nextp = ent12_p[1] + 1 + (fatent->entry & 1);

	fatent->entry++;
	if (fatent->nr_bhs == 1) {
		WARN_ON(ent12_p[0] > (u8 *)(bhs[0]->b_data +
							(bhs[0]->b_size - 2)));
		WARN_ON(ent12_p[1] > (u8 *)(bhs[0]->b_data +
							(bhs[0]->b_size - 1)));
		if (nextp < (u8 *)(bhs[0]->b_data + (bhs[0]->b_size - 1))) {
			ent12_p[0] = nextp - 1;
			ent12_p[1] = nextp;
			return 1;
		}
	} else {
		WARN_ON(ent12_p[0] != (u8 *)(bhs[0]->b_data +
							(bhs[0]->b_size - 1)));
		WARN_ON(ent12_p[1] != (u8 *)bhs[1]->b_data);
		ent12_p[0] = nextp - 1;
		ent12_p[1] = nextp;
		brelse(bhs[0]);
		bhs[0] = bhs[1];
		fatent->nr_bhs = 1;
		return 1;
	}
	ent12_p[0] = NULL;
	ent12_p[1] = NULL;
	return 0;
}

static int fat16_ent_next(struct fat_entry *fatent)
{
	const struct buffer_head *bh = fatent->bhs[0];
	fatent->entry++;
	if (fatent->u.ent16_p < (__le16 *)(bh->b_data + (bh->b_size - 2))) {
		fatent->u.ent16_p++;
		return 1;
	}
	fatent->u.ent16_p = NULL;
	return 0;
}

static int fat32_ent_next(struct fat_entry *fatent)
{
	const struct buffer_head *bh = fatent->bhs[0];
	fatent->entry++;
	if (fatent->u.ent32_p < (__le32 *)(bh->b_data + (bh->b_size - 4))) {
		fatent->u.ent32_p++;
		return 1;
	}
	fatent->u.ent32_p = NULL;
	return 0;
}

static struct fatent_operations fat12_ops = {
	.ent_blocknr	= fat12_ent_blocknr,
	.ent_set_ptr	= fat12_ent_set_ptr,
	.ent_bread	= fat12_ent_bread,
	.ent_get	= fat12_ent_get,
	.ent_put	= fat12_ent_put,
	.ent_next	= fat12_ent_next,
};

static struct fatent_operations fat16_ops = {
	.ent_blocknr	= fat_ent_blocknr,
	.ent_set_ptr	= fat16_ent_set_ptr,
	.ent_bread	= fat_ent_bread,
	.ent_get	= fat16_ent_get,
	.ent_put	= fat16_ent_put,
	.ent_next	= fat16_ent_next,
};

static struct fatent_operations fat32_ops = {
	.ent_blocknr	= fat_ent_blocknr,
	.ent_set_ptr	= fat32_ent_set_ptr,
	.ent_bread	= fat_ent_bread,
	.ent_get	= fat32_ent_get,
	.ent_put	= fat32_ent_put,
	.ent_next	= fat32_ent_next,
};

static inline void lock_fat(struct msdos_sb_info *sbi)
{
	mutex_lock(&sbi->fat_lock);
}

static inline void unlock_fat(struct msdos_sb_info *sbi)
{
	mutex_unlock(&sbi->fat_lock);
}

void fat_ent_access_init(struct super_block *sb)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);

	mutex_init(&sbi->fat_lock);

	switch (sbi->fat_bits) {
	case 32:
		sbi->fatent_shift = 2;
		sbi->fatent_ops = &fat32_ops;
		break;
	case 16:
		sbi->fatent_shift = 1;
		sbi->fatent_ops = &fat16_ops;
		break;
	case 12:
		sbi->fatent_shift = -1;
		sbi->fatent_ops = &fat12_ops;
		break;
	}
}

static void mark_fsinfo_dirty(struct super_block *sb)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);

	if (sb->s_flags & MS_RDONLY || sbi->fat_bits != 32)
		return;

	__mark_inode_dirty(sbi->fsinfo_inode, I_DIRTY_SYNC);
}

static inline int fat_ent_update_ptr(struct super_block *sb,
				     struct fat_entry *fatent,
				     int offset, sector_t blocknr)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	struct fatent_operations *ops = sbi->fatent_ops;
	struct buffer_head **bhs = fatent->bhs;

	/* Is this fatent's blocks including this entry? */
	if (!fatent->nr_bhs || bhs[0]->b_blocknr != blocknr)
		return 0;
	if (sbi->fat_bits == 12) {
		if ((offset + 1) < sb->s_blocksize) {
			/* This entry is on bhs[0]. */
			if (fatent->nr_bhs == 2) {
				brelse(bhs[1]);
				fatent->nr_bhs = 1;
			}
		} else {
			/* This entry needs the next block. */
			if (fatent->nr_bhs != 2)
				return 0;
			if (bhs[1]->b_blocknr != (blocknr + 1))
				return 0;
		}
	}
	ops->ent_set_ptr(fatent, offset);
	return 1;
}

int fat_ent_read(struct inode *inode, struct fat_entry *fatent, int entry)
{
	struct super_block *sb = inode->i_sb;
	struct msdos_sb_info *sbi = MSDOS_SB(inode->i_sb);
	struct fatent_operations *ops = sbi->fatent_ops;
	int err, offset;
	sector_t blocknr;

	if (entry < FAT_START_ENT || sbi->max_cluster <= entry) {
		fatent_brelse(fatent);
		fat_fs_error(sb, "invalid access to FAT (entry 0x%08x)", entry);
		return -EIO;
	}

	fatent_set_entry(fatent, entry);
	ops->ent_blocknr(sb, entry, &offset, &blocknr);

	if (!fat_ent_update_ptr(sb, fatent, offset, blocknr)) {
		fatent_brelse(fatent);
		err = ops->ent_bread(sb, fatent, offset, blocknr);
		if (err)
			return err;
	}
	return ops->ent_get(fatent);
}

/* FIXME: We can write the blocks as more big chunk. */
static int fat_mirror_bhs(struct super_block *sb, struct buffer_head **bhs,
			  int nr_bhs)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	struct buffer_head *c_bh;
	int err, n, copy;

	err = 0;
	for (copy = 1; copy < sbi->fats; copy++) {
		sector_t backup_fat = sbi->fat_length * copy;

		for (n = 0; n < nr_bhs; n++) {
			c_bh = sb_getblk(sb, backup_fat + bhs[n]->b_blocknr);
			if (!c_bh) {
				err = -ENOMEM;
				goto error;
			}
			memcpy(c_bh->b_data, bhs[n]->b_data, sb->s_blocksize);
			set_buffer_uptodate(c_bh);
			mark_buffer_dirty_inode(c_bh, sbi->fat_inode);
			if (sb->s_flags & MS_SYNCHRONOUS)
				err = sync_dirty_buffer(c_bh);
			brelse(c_bh);
			if (err)
				goto error;
		}
	}
error:
	return err;
}

int fat_ent_write(struct inode *inode, struct fat_entry *fatent,
		  int new, int wait)
{
	struct super_block *sb = inode->i_sb;
	struct fatent_operations *ops = MSDOS_SB(sb)->fatent_ops;
	int err;

	ops->ent_put(fatent, new);
	if (wait) {
		err = fat_sync_bhs(fatent->bhs, fatent->nr_bhs);
		if (err)
			return err;
	}
	return fat_mirror_bhs(sb, fatent->bhs, fatent->nr_bhs);
}

static inline int fat_ent_next(struct msdos_sb_info *sbi,
			       struct fat_entry *fatent)
{
	if (sbi->fatent_ops->ent_next(fatent)) {
		if (fatent->entry < sbi->max_cluster)
			return 1;
	}
	return 0;
}

static inline int fat_ent_read_block(struct super_block *sb,
				     struct fat_entry *fatent)
{
	struct fatent_operations *ops = MSDOS_SB(sb)->fatent_ops;
	sector_t blocknr;
	int offset;

	fatent_brelse(fatent);
	ops->ent_blocknr(sb, fatent->entry, &offset, &blocknr);
	return ops->ent_bread(sb, fatent, offset, blocknr);
}

static void fat_collect_bhs(struct buffer_head **bhs, int *nr_bhs,
			    struct fat_entry *fatent)
{
	int n, i;

	for (n = 0; n < fatent->nr_bhs; n++) {
		for (i = 0; i < *nr_bhs; i++) {
			if (fatent->bhs[n] == bhs[i])
				break;
		}
		if (i == *nr_bhs) {
			get_bh(fatent->bhs[n]);
			bhs[i] = fatent->bhs[n];
			(*nr_bhs)++;
		}
	}
}

int fat_alloc_clusters(struct inode *inode, int *cluster, int nr_cluster)
{
	struct super_block *sb = inode->i_sb;
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	struct fatent_operations *ops = sbi->fatent_ops;
	struct fat_entry fatent, prev_ent;
	struct buffer_head *bhs[MAX_BUF_PER_PAGE];
	int i, count, err, nr_bhs, idx_clus ;
#ifdef FAT_USE_BLOCK_MANAGER_WGZ
//wanggongzhen.wt add,reason:sdcard write speed is low when more than one file write simultaneously
	int saved_cluster = 0;
#endif

	BUG_ON(nr_cluster > (MAX_BUF_PER_PAGE / 2));	/* fixed limit */

	lock_fat(sbi);
	if (sbi->free_clusters != -1 && sbi->free_clus_valid &&
	    sbi->free_clusters < nr_cluster) {
		unlock_fat(sbi);
		return -ENOSPC;
	}

	err = nr_bhs = idx_clus = 0;
	count = FAT_START_ENT;
	fatent_init(&prev_ent);
	fatent_init(&fatent);
#ifdef FAT_USE_BLOCK_MANAGER_WGZ
//wanggongzhen.wt add,reason:sdcard write speed is low when more than one file write simultaneously
	saved_cluster = blocks_mamager_alloc_blocks(MSDOS_I(inode),nr_cluster);
	debug_print("wgz_fat32 inode:%p saved_cluster = %d\n",inode,saved_cluster);
	if(saved_cluster != -1)
	{
		sbi->prev_free = saved_cluster-1;
	}
	else
	{
		pr_err("--------------------FATAL ERROR---------------------\n");
		pr_err("wgz alloc return -1 free=%d\n",sbi->free_clusters);
		//dump_stack();
		sbi->free_clusters = 0;
		sbi->free_clus_valid = 1;
		err = -ENOSPC;
		goto out;
	}
#endif
	fatent_set_entry(&fatent, sbi->prev_free+1);
	while (count < sbi->max_cluster) {
		if (fatent.entry >= sbi->max_cluster)
			fatent.entry = FAT_START_ENT;
		fatent_set_entry(&fatent, fatent.entry);
		err = fat_ent_read_block(sb, &fatent);
		if (err)
			goto out;

		/* Find the free entries in a block */
		do {
			if (ops->ent_get(&fatent) == FAT_ENT_FREE) {
				int entry = fatent.entry;

				/* make the cluster chain */
				ops->ent_put(&fatent, FAT_ENT_EOF);
				if (prev_ent.nr_bhs)
					ops->ent_put(&prev_ent, entry);

				fat_collect_bhs(bhs, &nr_bhs, &fatent);

				sbi->prev_free = entry;
				if (sbi->free_clusters != -1)
					sbi->free_clusters--;

				cluster[idx_clus] = entry;
				idx_clus++;
				if (idx_clus == nr_cluster)
					goto out;

				/*
				 * fat_collect_bhs() gets ref-count of bhs,
				 * so we can still use the prev_ent.
				 */
				prev_ent = fatent;
			}
#ifdef FAT_USE_BLOCK_MANAGER_WGZ
//wanggongzhen.wt add,reason:sdcard write speed is low when more than one file write simultaneously
			else
			{
				pr_err("--------------------FATAL ERROR---------------------\n");
				pr_err("wgz cluster %d is not free\n",fatent.entry);
				BUG();
			}
#endif
			count++;
			if (count == sbi->max_cluster)
				break;
		} while (fat_ent_next(sbi, &fatent));
	}

	/* Couldn't allocate the free entries */
	sbi->free_clusters = 0;
	sbi->free_clus_valid = 1;
	err = -ENOSPC;

out:
	unlock_fat(sbi);
	mark_fsinfo_dirty(sb);
	fatent_brelse(&fatent);
	if (!err) {
		if (inode_needs_sync(inode))
			err = fat_sync_bhs(bhs, nr_bhs);
		if (!err)
			err = fat_mirror_bhs(sb, bhs, nr_bhs);
	}
	for (i = 0; i < nr_bhs; i++)
		brelse(bhs[i]);

	if (err && idx_clus)
		fat_free_clusters(inode, cluster[0]);

	return err;
}

int fat_free_clusters(struct inode *inode, int cluster)
{
	struct super_block *sb = inode->i_sb;
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	struct fatent_operations *ops = sbi->fatent_ops;
	struct fat_entry fatent;
	struct buffer_head *bhs[MAX_BUF_PER_PAGE];
	int i, err, nr_bhs;
	int first_cl = cluster, dirty_fsinfo = 0;

	nr_bhs = 0;
	fatent_init(&fatent);
	lock_fat(sbi);
	do {
		cluster = fat_ent_read(inode, &fatent, cluster);
		if (cluster < 0) {
			err = cluster;
			goto error;
		} else if (cluster == FAT_ENT_FREE) {
			fat_fs_error(sb, "%s: deleting FAT entry beyond EOF",
				     __func__);
			err = -EIO;
			goto error;
		}

		if (sbi->options.discard) {
			/*
			 * Issue discard for the sectors we no longer
			 * care about, batching contiguous clusters
			 * into one request
			 */
			if (cluster != fatent.entry + 1) {
				int nr_clus = fatent.entry - first_cl + 1;

				sb_issue_discard(sb,
					fat_clus_to_blknr(sbi, first_cl),
					nr_clus * sbi->sec_per_clus,
					GFP_NOFS, 0);

				first_cl = cluster;
			}
		}

		ops->ent_put(&fatent, FAT_ENT_FREE);
		if (sbi->free_clusters != -1) {
			sbi->free_clusters++;
#ifdef FAT_USE_BLOCK_MANAGER_WGZ
//wanggongzhen.wt add,reason:sdcard write speed is low when more than one file write simultaneously
			blocks_mamager_free_blocks(fatent.entry,1);
#endif
			dirty_fsinfo = 1;
		}

		if (nr_bhs + fatent.nr_bhs > MAX_BUF_PER_PAGE) {
			if (sb->s_flags & MS_SYNCHRONOUS) {
				err = fat_sync_bhs(bhs, nr_bhs);
				if (err)
					goto error;
			}
			err = fat_mirror_bhs(sb, bhs, nr_bhs);
			if (err)
				goto error;
			for (i = 0; i < nr_bhs; i++)
				brelse(bhs[i]);
			nr_bhs = 0;
		}
		fat_collect_bhs(bhs, &nr_bhs, &fatent);
	} while (cluster != FAT_ENT_EOF);

	if (sb->s_flags & MS_SYNCHRONOUS) {
		err = fat_sync_bhs(bhs, nr_bhs);
		if (err)
			goto error;
	}
	err = fat_mirror_bhs(sb, bhs, nr_bhs);
error:
	fatent_brelse(&fatent);
	for (i = 0; i < nr_bhs; i++)
		brelse(bhs[i]);
	unlock_fat(sbi);
	if (dirty_fsinfo)
		mark_fsinfo_dirty(sb);

	return err;
}
EXPORT_SYMBOL_GPL(fat_free_clusters);

/* 128kb is the whole sectors for FAT12 and FAT16 */
#define FAT_READA_SIZE		(128 * 1024)

static void fat_ent_reada(struct super_block *sb, struct fat_entry *fatent,
			  unsigned long reada_blocks)
{
	struct fatent_operations *ops = MSDOS_SB(sb)->fatent_ops;
	sector_t blocknr;
	int i, offset;

	ops->ent_blocknr(sb, fatent->entry, &offset, &blocknr);

	for (i = 0; i < reada_blocks; i++)
		sb_breadahead(sb, blocknr + i);
}

int fat_count_free_clusters(struct super_block *sb)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	struct fatent_operations *ops = sbi->fatent_ops;
	struct fat_entry fatent;
	unsigned long reada_blocks, reada_mask, cur_block;
	int err = 0, free;
#ifdef FAT_USE_BLOCK_MANAGER_WGZ
//wanggongzhen.wt add,reason:sdcard write speed is low when more than one file write simultaneously
	int free_start = 0;
	int free_count = 0;
	blocks_mamager_free_all();
#endif

	lock_fat(sbi);
	if (sbi->free_clusters != -1 && sbi->free_clus_valid)
		goto out;

	reada_blocks = FAT_READA_SIZE >> sb->s_blocksize_bits;
	reada_mask = reada_blocks - 1;
	cur_block = 0;

	free = 0;
	fatent_init(&fatent);
	fatent_set_entry(&fatent, FAT_START_ENT);
	while (fatent.entry < sbi->max_cluster) {
		/* readahead of fat blocks */
		if ((cur_block & reada_mask) == 0) {
			unsigned long rest = sbi->fat_length - cur_block;
			fat_ent_reada(sb, &fatent, min(reada_blocks, rest));
		}
		cur_block++;

		err = fat_ent_read_block(sb, &fatent);
		if (err)
			goto out;

		do {
			if (ops->ent_get(&fatent) == FAT_ENT_FREE)
			{
				free++;
#ifdef FAT_USE_BLOCK_MANAGER_WGZ
//wanggongzhen.wt add,reason:sdcard write speed is low when more than one file write simultaneously
				if(free_count == 0)
				{
					free_start = fatent.entry;
				}
				free_count++;
			}
			else
			{
				if(free_count != 0)
				{
					blocks_mamager_free_blocks(free_start,free_count);
				}
				free_count = free_start = 0;
#endif
			}

		} while (fat_ent_next(sbi, &fatent));
	}
#ifdef FAT_USE_BLOCK_MANAGER_WGZ
//wanggongzhen.wt add,reason:sdcard write speed is low when more than one file write simultaneously
	debug_print("wgz_fat32 free:%d\n",free);
	if(free_count != 0)
	{
		blocks_mamager_free_blocks(free_start,free_count);
	}
#endif
	sbi->free_clusters = free;
	sbi->free_clus_valid = 1;
	mark_fsinfo_dirty(sb);
	fatent_brelse(&fatent);
out:
	unlock_fat(sbi);
	return err;
}
