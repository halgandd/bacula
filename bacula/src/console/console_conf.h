/*
 * Bacula User Agent specific configuration and defines
 *
 *     Kern Sibbald, Sep MM
 */

/*
 * Resource codes -- they must be sequential for indexing   
 */
#define R_FIRST                       1001

#define R_DIRECTOR                    1001

#define R_LAST                        R_DIRECTOR

/*
 * Some resource attributes
 */
#define R_NAME                        1020
#define R_ADDRESS                     1021
#define R_PASSWORD                    1022
#define R_TYPE                        1023
#define R_BACKUP                      1024


/* Definition of the contents of each Resource */
struct s_res_dir {
   RES   hdr;
   int   DIRport;                     /* UA server port */
   char *address;                     /* UA server address */
   char *password;                    /* UA server password */
};
typedef struct s_res_dir DIRRES;


/* Define the Union of all the above
 * resource structure definitions.
 */
union u_res {
   struct s_res_dir     res_dir;
   RES hdr;
};

typedef union u_res URES;
