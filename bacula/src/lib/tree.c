/*
 * Directory tree build/traverse routines
 * 
 *    Kern Sibbald, June MMII
 *
*/
/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */


#include "bacula.h"
#include "findlib/find.h"
	     

/* Forward referenced subroutines */
static TREE_NODE *search_and_insert_tree_node(char *fname, int type, 
	       TREE_ROOT *root, TREE_NODE *parent);
static char *tree_alloc(TREE_ROOT *root, int size);

/*
 * NOTE !!!!! we turn off Debug messages for performance reasons.
 */
#undef Dmsg0
#undef Dmsg1
#undef Dmsg2
#undef Dmsg3
#define Dmsg0(n,f)
#define Dmsg1(n,f,a1)
#define Dmsg2(n,f,a1,a2)
#define Dmsg3(n,f,a1,a2,a3)

/*
 * This subroutine gets a big buffer.
 */
static void malloc_buf(TREE_ROOT *root, int size)
{
   struct s_mem *mem;

   mem = (struct s_mem *)malloc(size);
   root->total_size += size;
   root->blocks++;
   mem->next = root->mem;
   root->mem = mem;
   mem->mem = mem->first;
   mem->rem = (char *)mem + size - mem->mem;
   Dmsg2(200, "malloc buf size=%d rem=%d\n", size, mem->rem);
}


/*
 * Note, we allocate a big buffer in the tree root
 *  from which we allocate nodes. This runs more
 *  than 100 times as fast as directly using malloc()
 *  for each of the nodes.
 */
TREE_ROOT *new_tree(int count)
{
   TREE_ROOT *root;
   uint32_t size;

   if (count < 1000) {		      /* minimum tree size */
      count = 1000;
   }
   root = (TREE_ROOT *)malloc(sizeof(TREE_ROOT));
   memset(root, 0, sizeof(TREE_ROOT));
   /* Assume filename + node  = 40 characters average length */
   size = count * (BALIGN(sizeof(TREE_NODE)) + 40);
   if (count > 1000000 || size > 10000000) {
      size = 10000000;
   }
   Dmsg2(400, "count=%d size=%d\n", count, size);
   malloc_buf(root, size);
   root->cached_path_len = -1;
   root->cached_path = get_pool_memory(PM_FNAME);
   root->type = TN_ROOT;
   root->fname = "";
   return root;
}

/* 
 * Create a new tree node.
 */
static TREE_NODE *new_tree_node(TREE_ROOT *root)
{
   TREE_NODE *node;
   int size = sizeof(TREE_NODE);
   node = (TREE_NODE *)tree_alloc(root, size);
   memset(node, 0, size);
   return node;
}

#ifdef USE_DLIST
/*
 * This routine can be called to release the 
 *  previously allocated tree node.
 */
static void free_tree_node(TREE_ROOT *root)
{
   int asize = BALIGN(sizeof(TREE_NODE));
   root->mem->rem += asize;
   root->mem->mem -= asize;
}
#endif



/*
 * Allocate bytes for filename in tree structure.
 *  Keep the pointers properly aligned by allocating
 *  sizes that are aligned.
 */
static char *tree_alloc(TREE_ROOT *root, int size)
{
   char *buf;
   int asize = BALIGN(size);

   if (root->mem->rem < asize) {
      uint32_t mb_size;
      if (root->total_size >= 1000000) {
	 mb_size = 1000000;
      } else {
	 mb_size = 100000;
      }
      malloc_buf(root, mb_size);
   }
   root->mem->rem -= asize;
   buf = root->mem->mem;
   root->mem->mem += asize;
   return buf;
}


/* This routine frees the whole tree */
void free_tree(TREE_ROOT *root)
{
   struct s_mem *mem, *rel;

   for (mem=root->mem; mem; ) {
      rel = mem;
      mem = mem->next;
      free(rel);
   }
   if (root->cached_path) {
      free_pool_memory(root->cached_path);
      root->cached_path = NULL;
   }
   Dmsg2(400, "Total size=%u blocks=%d\n", root->total_size, root->blocks);
   free(root);
   return;
}


/* 
 * Insert a node in the tree. This is the main subroutine
 *   called when building a tree.
 *
 */
TREE_NODE *insert_tree_node(char *path, char *fname, int type,
			    TREE_ROOT *root, TREE_NODE *parent)
{
   char *p, *q;
   int path_len = strlen(path);
   TREE_NODE *node;

   Dmsg1(100, "insert_tree_node: %s\n", path);
   /*
    * If trailing slash on path, strip it
    */
   if (path_len > 0) {
      q = path + path_len - 1;
      if (*q == '/') {
	 *q = 0;		      /* strip trailing slash */
      } else {
	 q = NULL;		      /* no trailing slash */
      }
   } else {
      q = NULL; 		      /* no trailing slash */
   }
   /* If no filename, strip last component of path as "filename" */
   if (*fname == 0) {
      p = strrchr(path, '/');         /* separate path and filename */
      if (p) {
	 fname = p + 1; 	      /* set new filename */
	 *p = 0;		      /* terminate new path */
      }
   } else {
      p = NULL;
   }
   if (*fname) {
      if (!parent) {		      /* if no parent, we need to make one */
         Dmsg1(100, "make_tree_path for %s\n", path);
	 path_len = strlen(path);     /* get new length */
	 if (path_len == root->cached_path_len &&
	     strcmp(path, root->cached_path) == 0) {
	    parent = root->cached_parent;
	 } else {
	    root->cached_path_len = path_len;
	    pm_strcpy(&root->cached_path, path);
	    parent = make_tree_path(path, root);
	    root->cached_parent = parent; 
	 }
         Dmsg1(100, "parent=%s\n", parent->fname);
      }
   } else {
      fname = path;
      if (!parent) {
	 parent = (TREE_NODE *)root;
	 type = TN_DIR_NLS;
      }
      Dmsg1(100, "No / found: %s\n", path);
   }

   node = search_and_insert_tree_node(fname, 0, root, parent);
   if (q) {			      /* if trailing slash on entry */
      *q = '/';                       /*  restore it */
   }
   if (p) {			      /* if slash in path trashed */
      *p = '/';                       /* restore full path */
   }
   return node;
}

/*
 * Ensure that all appropriate nodes for a full path exist in
 *  the tree.
 */
TREE_NODE *make_tree_path(char *path, TREE_ROOT *root)
{
   TREE_NODE *parent, *node;
   char *fname, *p;
   int type = TN_NEWDIR;

   Dmsg1(100, "make_tree_path: %s\n", path);
   if (*path == 0) {
      Dmsg0(100, "make_tree_path: parent=*root*\n");
      return (TREE_NODE *)root;
   }
   p = strrchr(path, '/');           /* get last dir component of path */
   if (p) {
      fname = p + 1;
      *p = 0;			      /* terminate path */
      parent = make_tree_path(path, root);
      *p = '/';                       /* restore full name */
   } else {
      fname = path;
      parent = (TREE_NODE *)root;
      type = TN_DIR_NLS;
   }
   node = search_and_insert_tree_node(fname, type, root, parent);
   return node;
}  

#ifdef USE_DLIST
static int node_compare(void *item1, void *item2)
{
   TREE_NODE *tn1 = (TREE_NODE *)item1;
   TREE_NODE *tn2 = (TREE_NODE *)item2;
   if (tn1->fname[0] > tn2->fname[0]) {
      return 1;
   } else if (tn1->fname[0] < tn2->fname[0]) {
      return -1;
   }
   return strcmp(tn1->fname, tn2->fname);
}
#endif

/*
 *  See if the fname already exists. If not insert a new node for it.
 */
static TREE_NODE *search_and_insert_tree_node(char *fname, int type, 
	       TREE_ROOT *root, TREE_NODE *parent)
{
#ifdef USE_DLIST
   TREE_NODE *node, *found_node;
   node = new_tree_node(root);
   node->fname = fname;
   found_node = (TREE_NODE *)parent->child.unique_binary_insert(node, node_compare);
   if (found_node != node) {	      /* already in list */
      free_tree_node(root);	      /* free node allocated above */
      found_node->inserted = false;
      return found_node;
   }
   /* It was not found, but is now inserted */
   node->fname_len = strlen(fname);
   node->fname = tree_alloc(root, node->fname_len + 1);
   strcpy(node->fname, fname);
   node->parent = parent;
   node->type = type;

   /* Maintain a linear chain of nodes */
   if (!root->first) {
      root->first = node;
      root->last = node;
   } else {
      root->last->next = node;
      root->last = node;
   }
   node->inserted = true;	      /* inserted into tree */
   return node;

#else
   TREE_NODE *sibling, *last_sibling = NULL;
   uint16_t fname_len = strlen(fname);
   int cmp;

   /* Is it already a sibling? */
   foreach_child(sibling, parent) {    
      Dmsg2(000, "sibling->fname=%s fname=%s\n", sibling->fname, fname);
      if (fname[0] > sibling->fname[0] || (cmp=strcmp(fname, sibling->fname)) > 0) {
	 last_sibling = sibling;
	 continue;
      }
      if (cmp < 0) {
	 /* Insert before current sibling */
	 if (!node) {
	    node = new_tree_node(root);
	 }
	 node->sibling_ = sibling;
	 if (sibling == first_child(parent)) { /* if sibling was at head of list */
	    parent->child_ = NULL;	  /* force parent to be updated below */
	 }
         Dmsg2(000, "insert before sibling->fname=%s fname=%s\n", sibling->fname, fname);
	 break;
      }
      /* Found it */
      sibling->inserted = false;      /* already in tree */
      return sibling;
   }


   /* 
    * At this point, the fname is not found. We must add it 
    */
   if (!node) {
      node = new_tree_node(root);
   }
   Dmsg1(000, "append_tree_node: %s\n", fname);
   node->fname_len = fname_len;
   node->fname = tree_alloc(root, node->fname_len + 1);
   strcpy(node->fname, fname);
   node->parent = parent;
   node->type = type;
   if (!tree_node_has_child(parent)) {
      parent->child_ = node;
   } else {
      last_sibling->sibling_ = node;
   }

   /* Maintain a linear chain of nodes */
   if (!root->first) {
      root->first = node;
      root->last = node;
   } else {
      root->last->next = node;
      root->last = node;
   }
   node->inserted = true;	      /* inserted into tree */
   return node;
#endif
}

#ifdef SLOW_WAY
/* Moved to tree.h to eliminate subroutine call */
TREE_NODE *first_tree_node(TREE_ROOT *root)
{
   return root->first;
}

TREE_NODE *next_tree_node(TREE_NODE *node)
{
   return node->next;
}
#endif



int tree_getpath(TREE_NODE *node, char *buf, int buf_size)
{
   if (!node) {
      buf[0] = 0;
      return 1;
   }
   tree_getpath(node->parent, buf, buf_size);
   /* 
    * Fixup for Win32. If we have a Win32 directory and 
    *	 there is only a / in the buffer, remove it since
    *    win32 names don't generally start with /
    */
   if (node->type == TN_DIR_NLS && buf[0] == '/' && buf[1] == 0) {
      buf[0] = 0;   
   }
   bstrncat(buf, node->fname, buf_size);
   /* Add a slash for all directories unless we are at the root,
    *  also add a slash to a soft linked file if it has children
    *  i.e. it is linked to a directory.
    */
   if ((node->type != TN_FILE && !(buf[0] == '/' && buf[1] == 0)) ||
       (node->soft_link && tree_node_has_child(node))) {
      bstrncat(buf, "/", buf_size);
   }
   return 1;
}

/* 
 * Change to specified directory
 */
TREE_NODE *tree_cwd(char *path, TREE_ROOT *root, TREE_NODE *node)
{
   if (strcmp(path, ".") == 0) {
      return node;
   }
   if (strcmp(path, "..") == 0) {
      if (node->parent) {
	 return node->parent;
      } else {
	 return node;
      }
   }
   if (path[0] == '/') {
      Dmsg0(100, "Doing absolute lookup.\n");
      return tree_relcwd(path+1, root, (TREE_NODE *)root);
   }
   Dmsg0(100, "Doing relative lookup.\n");
   return tree_relcwd(path, root, node);
}


/*
 * Do a relative cwd -- i.e. relative to current node rather than root node
 */
TREE_NODE *tree_relcwd(char *path, TREE_ROOT *root, TREE_NODE *node)
{
   char *p;
   int len;
   TREE_NODE *cd;

   if (*path == 0) {
      return node;
   }
   /* Check the current segment only */
   p = strchr(path, '/');
   if (p) {
      len = p - path;
   } else {
      len = strlen(path);
   }
   Dmsg2(100, "tree_relcwd: len=%d path=%s\n", len, path);
   foreach_child(cd, node) {
      Dmsg1(100, "tree_relcwd: test cd=%s\n", cd->fname);
      if (cd->fname[0] == path[0] && len == (int)strlen(cd->fname)    
	  && strncmp(cd->fname, path, len) == 0) {
	 break;
      }
   }
   if (!cd || (cd->type == TN_FILE && !tree_node_has_child(cd))) {
      return NULL;
   }
   if (!p) {
      Dmsg0(100, "tree_relcwd: no more to lookup. found.\n");
      return cd;
   }
   Dmsg2(100, "recurse tree_relcwd with path=%s, cd=%s\n", p+1, cd->fname);
   /* Check the next segment if any */
   return tree_relcwd(p+1, root, cd);
}



#ifdef BUILD_TEST_PROGRAM

void FillDirectoryTree(char *path, TREE_ROOT *root, TREE_NODE *parent);

static uint32_t FileIndex = 0;
/*
 * Simple test program for tree routines
 */
int main(int argc, char *argv[])
{
    TREE_ROOT *root;
    TREE_NODE *node;
    char buf[MAXPATHLEN];

    root = new_tree();
    root->fname = tree_alloc(root, 1);
    *root->fname = 0;
    root->fname_len = 0;

    FillDirectoryTree("/home/kern/bacula/k", root, NULL);

    for (node = first_tree_node(root); node; node=next_tree_node(node)) {
       tree_getpath(node, buf, sizeof(buf));
       Dmsg2(100, "%d: %s\n", node->FileIndex, buf);
    }

    node = (TREE_NODE *)root;
    Pmsg0(000, "doing cd /home/kern/bacula/k/techlogs\n");
    node = tree_cwd("/home/kern/bacula/k/techlogs", root, node);
    if (node) {
       tree_getpath(node, buf, sizeof(buf));
       Dmsg2(100, "findex=%d: cwd=%s\n", node->FileIndex, buf);
    }

    Pmsg0(000, "doing cd /home/kern/bacula/k/src/testprogs\n");
    node = tree_cwd("/home/kern/bacula/k/src/testprogs", root, node);
    if (node) {
       tree_getpath(node, buf, sizeof(buf));
       Dmsg2(100, "findex=%d: cwd=%s\n", node->FileIndex, buf);
    } else {
       Dmsg0(100, "testprogs not found.\n");
    }

    free_tree((TREE_NODE *)root);

    return 0;
}

void FillDirectoryTree(char *path, TREE_ROOT *root, TREE_NODE *parent)
{
   TREE_NODE *newparent = NULL;
   TREE_NODE *node;
   struct stat statbuf;
   DIR *dp;
   struct dirent *dir;
   char pathbuf[MAXPATHLEN];
   char file[MAXPATHLEN];
   int type;
   int i;
   
   Dmsg1(100, "FillDirectoryTree: %s\n", path);
   dp = opendir(path);
   if (!dp) {
      return;
   }
   while ((dir = readdir(dp))) {
      if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
	 continue;
      }
      bstrncpy(file, dir->d_name, sizeof(file));
      snprintf(pathbuf, MAXPATHLEN-1, "%s/%s", path, file);
      if (lstat(pathbuf, &statbuf) < 0) {
         printf("lstat() failed. ERR=%s\n", strerror(errno));
	 continue;
      }
//      printf("got file=%s, pathbuf=%s\n", file, pathbuf);
      type = TN_FILE;
      if (S_ISLNK(statbuf.st_mode))
	 type =  TN_FILE;  /* link */
      else if (S_ISREG(statbuf.st_mode))
	 type = TN_FILE;
      else if (S_ISDIR(statbuf.st_mode)) {
	 type = TN_DIR;
      } else if (S_ISCHR(statbuf.st_mode))
	 type = TN_FILE; /* char dev */
      else if (S_ISBLK(statbuf.st_mode))
	 type = TN_FILE; /* block dev */
      else if (S_ISFIFO(statbuf.st_mode))
	 type = TN_FILE; /* fifo */
      else if (S_ISSOCK(statbuf.st_mode))
	 type = TN_FILE; /* sock */
      else {
	 type = TN_FILE;
         printf("Unknown file type: 0x%x\n", statbuf.st_mode);
      }

      Dmsg2(100, "Doing: %d %s\n", type, pathbuf);
      node = new_tree_node(root);
      node->FileIndex = ++FileIndex;
      parent = insert_tree_node(pathbuf, node, root, parent);
      if (S_ISDIR(statbuf.st_mode) && !S_ISLNK(statbuf.st_mode)) {
         Dmsg2(100, "calling fill. pathbuf=%s, file=%s\n", pathbuf, file);
	 FillDirectoryTree(pathbuf, root, node);
      }
   }
   closedir(dp);
}

#ifndef MAXPATHLEN
#define MAXPATHLEN 2000
#endif

void print_tree(char *path, TREE_NODE *tree)
{
   char buf[MAXPATHLEN];
   char *termchr;

   if (!tree) {
      return;
   }
   switch (tree->type) {
   case TN_DIR_NLS:
   case TN_DIR:
   case TN_NEWDIR:  
      termchr = "/";
      break;
   case TN_ROOT:
   case TN_FILE:
   default:
      termchr = "";
      break;
   }
   Dmsg3(-1, "%s/%s%s\n", path, tree->fname, termchr);
   switch (tree->type) {
   case TN_FILE:
   case TN_NEWDIR:
   case TN_DIR:
   case TN_DIR_NLS:
      bsnprintf(buf, sizeof(buf), "%s/%s", path, tree->fname);
      print_tree(buf, first_child(tree));
      break;
   case TN_ROOT:
      print_tree(path, first_child(tree));
      break;
   default:
      Pmsg1(000, "Unknown node type %d\n", tree->type);
   }
   print_tree(path, tree->sibling_);
   return;
}

#endif
