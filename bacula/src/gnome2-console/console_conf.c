/*
 *   Main configuration file parser for Bacula User Agent
 *    some parts may be split into separate files such as
 *    the schedule configuration (sch_config.c).
 *
 *   Note, the configuration file parser consists of three parts
 *
 *   1. The generic lexical scanner in lib/lex.c and lib/lex.h
 *
 *   2. The generic config  scanner in lib/parse_config.c and 
 *	lib/parse_config.h.
 *	These files contain the parser code, some utility
 *	routines, and the common store routines (name, int,
 *	string).
 *
 *   3. The daemon specific file, which contains the Resource
 *	definitions as well as any specific store routines
 *	for the resource records.
 *
 *     Kern Sibbald, January MM, September MM
 *
 *     Version $Id$
 */

/*
   Copyright (C) 2000, 2001 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "bacula.h"
#include "console_conf.h"

/* Define the first and last resource ID record
 * types. Note, these should be unique for each
 * daemon though not a requirement.
 */
int r_first = R_FIRST;
int r_last  = R_LAST;

/* Forward referenced subroutines */


/* We build the current resource here as we are
 * scanning the resource configuration definition,
 * then move it to allocated memory when the resource
 * scan is complete.
 */
URES res_all;
int  res_all_size = sizeof(res_all);

/* Definition of records permitted within each
 * resource with the routine to process the record 
 * information.
 */ 
static RES_ITEM dir_items[] = {
   {"name",        store_name,     ITEM(dir_res.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,      ITEM(dir_res.hdr.desc), 0, 0, 0},
   {"dirport",     store_int,      ITEM(dir_res.DIRport),  0, ITEM_DEFAULT, 9101},
   {"address",     store_str,      ITEM(dir_res.address),  0, ITEM_REQUIRED, 0},
   {"password",    store_password, ITEM(dir_res.password), 0, 0, 0},
   {"enablessl", store_yesno,      ITEM(dir_res.enable_ssl), 1, ITEM_DEFAULT, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

static RES_ITEM con_items[] = {
   {"name",        store_name,     ITEM(con_res.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,      ITEM(con_res.hdr.desc), 0, 0, 0},
   {"password",    store_password, ITEM(con_res.password), 0, ITEM_REQUIRED, 0},
   {"requiressl",  store_yesno,    ITEM(con_res.require_ssl), 1, ITEM_DEFAULT, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};

static RES_ITEM con_font_items[] = {
   {"name",        store_name,     ITEM(con_font.hdr.name), 0, ITEM_REQUIRED, 0},
   {"description", store_str,      ITEM(con_font.hdr.desc), 0, 0, 0},
   {"font",        store_str,      ITEM(con_font.fontface), 0, 0, 0},
   {"requiressl",  store_yesno,    ITEM(con_font.require_ssl), 1, ITEM_DEFAULT, 0},
   {NULL, NULL, NULL, 0, 0, 0} 
};


/* 
 * This is the master resource definition.  
 * It must have one item for each of the resources.
 */
RES_TABLE resources[] = {
   {"director",      dir_items,   R_DIRECTOR,  NULL},
   {"console",       con_items,   R_CONSOLE,   NULL},
   {"consolefont",   con_font_items, R_CONSOLE_FONT,   NULL},
   {NULL,	     NULL,	  0,	       NULL}
};


/* Dump contents of resource */
void dump_resource(int type, RES *reshdr, void sendit(void *sock, char *fmt, ...), void *sock)
{
   URES *res = (URES *)reshdr;
   int recurse = 1;

   if (res == NULL) {
      printf("No record for %d %s\n", type, res_to_str(type));
      return;
   }
   if (type < 0) {		      /* no recursion */
      type = - type;
      recurse = 0;
   }
   switch (type) {
   case R_DIRECTOR:
      printf("Director: name=%s address=%s DIRport=%d\n", reshdr->name, 
	      res->dir_res.address, res->dir_res.DIRport);
      break;
   case R_CONSOLE:
      printf("Console: name=%s\n", reshdr->name);
      break;
   case R_CONSOLE_FONT:
      printf("ConsoleFont: name=%s font face=%s\n", 
	     reshdr->name, NPRT(res->con_font.fontface));
      break;
   default:
      printf("Unknown resource type %d\n", type);
   }
   if (recurse && res->dir_res.hdr.next) {
      dump_resource(type, res->dir_res.hdr.next, sendit, sock);
   }
}

/* 
 * Free memory of resource.  
 * NB, we don't need to worry about freeing any references
 * to other resources as they will be freed when that 
 * resource chain is traversed.  Mainly we worry about freeing
 * allocated strings (names).
 */
void free_resource(RES *sres, int type)
{
   RES *nres;
   URES *res = (URES *)sres;

   if (res == NULL)
      return;

   /* common stuff -- free the resource name */
   nres = (RES *)res->dir_res.hdr.next;
   if (res->dir_res.hdr.name) {
      free(res->dir_res.hdr.name);
   }
   if (res->dir_res.hdr.desc) {
      free(res->dir_res.hdr.desc);
   }

   switch (type) {
   case R_DIRECTOR:
      if (res->dir_res.address) {
	 free(res->dir_res.address);
      }
      break;
   case R_CONSOLE:
      if (res->con_res.password) {
	 free(res->con_res.password);
      }
      break;
   case R_CONSOLE_FONT:
      if (res->con_font.fontface) {
	 free(res->con_font.fontface);
      }
      break;
   default:
      printf("Unknown resource type %d\n", type);
   }
   /* Common stuff again -- free the resource, recurse to next one */
   free(res);
   if (nres) {
      free_resource(nres, type);
   }
}

/* Save the new resource by chaining it into the head list for
 * the resource. If this is pass 2, we update any resource
 * pointers (currently only in the Job resource).
 */
void save_resource(int type, RES_ITEM *items, int pass)
{
   URES *res;
   int rindex = type - r_first;
   int i, size = 0;
   int error = 0;

   /* 
    * Ensure that all required items are present
    */
   for (i=0; items[i].name; i++) {
      if (items[i].flags & ITEM_REQUIRED) {
	    if (!bit_is_set(i, res_all.dir_res.hdr.item_present)) {  
               Emsg2(M_ABORT, 0, "%s item is required in %s resource, but not found.\n",
		 items[i].name, resources[rindex]);
	     }
      }
   }

   /* During pass 2, we looked up pointers to all the resources
    * referrenced in the current resource, , now we
    * must copy their address from the static record to the allocated
    * record.
    */
   if (pass == 2) {
      switch (type) {
      /* Resources not containing a resource */
      case R_DIRECTOR:
	 break;

      case R_CONSOLE:
      case R_CONSOLE_FONT:
	 break;

      default:
         Emsg1(M_ERROR, 0, "Unknown resource type %d\n", type);
	 error = 1;
	 break;
      }
      /* Note, the resoure name was already saved during pass 1,
       * so here, we can just release it.
       */
      if (res_all.dir_res.hdr.name) {
	 free(res_all.dir_res.hdr.name);
	 res_all.dir_res.hdr.name = NULL;
      }
      if (res_all.dir_res.hdr.desc) {
	 free(res_all.dir_res.hdr.desc);
	 res_all.dir_res.hdr.desc = NULL;
      }
      return;
   }

   /* The following code is only executed during pass 1 */
   switch (type) {
   case R_DIRECTOR:
      size = sizeof(DIRRES);
      break;
   case R_CONSOLE_FONT:
      size = sizeof(CONFONTRES);
      break;
   case R_CONSOLE:
      size = sizeof(CONRES);
      break;
   default:
      printf("Unknown resource type %d\n", type);
      error = 1;
      break;
   }
   /* Common */
   if (!error) {
      res = (URES *)malloc(size);
      memcpy(res, &res_all, size);
      if (!resources[rindex].res_head) {
	 resources[rindex].res_head = (RES *)res; /* store first entry */
      } else {
	 RES *next;
	 /* Add new res to end of chain */
	 for (next=resources[rindex].res_head; next->next; next=next->next) {
	    if (strcmp(next->name, res->dir_res.hdr.name) == 0) {
	       Emsg2(M_ERROR_TERM, 0,
                  _("Attempt to define second %s resource named \"%s\" is not permitted.\n"),
		  resources[rindex].name, res->dir_res.hdr.name);
	    }
	 }
	 next->next = (RES *)res;
         Dmsg2(90, "Inserting %s res: %s\n", res_to_str(type),
	       res->dir_res.hdr.name);
      }
   }
}
