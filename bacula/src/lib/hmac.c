/*
 *  Hashed Message Authentication Code using MD5 (HMAC-MD5)
 *
 * hmac_md5 was based on sample code in RFC2104 (thanks guys).
 * 
 * Adapted to Bacula by Kern E. Sibbald, February MMI.
 *
 *   Version $Id$
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald and John Walker

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"

#define PAD_LEN 64		      /* PAD length */
#define SIG_LEN 16		      /* MD5 signature length */

void
hmac_md5(
    uint8_t*  text,	       /* pointer to data stream */
    int   text_len,	       /* length of data stream */
    uint8_t*  key,	       /* pointer to authentication key */
    int   key_len,	       /* length of authentication key */
    uint8_t  *hmac)	       /* returned hmac-md5 */
{
   MD5Context md5c;
   uint8_t k_ipad[PAD_LEN];    /* inner padding - key XORd with ipad */
   uint8_t k_opad[PAD_LEN];    /* outer padding - key XORd with opad */
   uint8_t keysig[SIG_LEN];
   int i;

   /* if key is longer than PAD length, reset it to key=MD5(key) */
   if (key_len > PAD_LEN) {
      MD5Context md5key;

      MD5Init(&md5key);
      MD5Update(&md5key, key, key_len);
      MD5Final(keysig, &md5key);

      key = keysig;
      key_len = SIG_LEN;
   }

   /*
    * the HMAC_MD5 transform looks like:
    *
    * MD5(Key XOR opad, MD5(Key XOR ipad, text))
    *
    * where Key is an n byte key
    * ipad is the byte 0x36 repeated 64 times

    * opad is the byte 0x5c repeated 64 times
    * and text is the data being protected
    */

   /* Zero pads and store key */
   memset(k_ipad, 0, PAD_LEN);
   memcpy(k_ipad, key, key_len);
   memcpy(k_opad, k_ipad, PAD_LEN); 

   /* XOR key with ipad and opad values */
   for (i=0; i<PAD_LEN; i++) {
      k_ipad[i] ^= 0x36;
      k_opad[i] ^= 0x5c;
   }

   /* perform inner MD5 */
   MD5Init(&md5c);		      /* start inner hash */
   MD5Update(&md5c, k_ipad, PAD_LEN); /* hash inner pad */
   MD5Update(&md5c, text, text_len);  /* hash text */
   MD5Final(hmac, &md5c);	      /* store inner hash */

   /* perform outer MD5 */
   MD5Init(&md5c);		      /* start outer hash */
   MD5Update(&md5c, k_opad, PAD_LEN); /* hash outer pad */
   MD5Update(&md5c, hmac, SIG_LEN);   /* hash inner hash */
   MD5Final(hmac, &md5c);	      /* store results */
}
/*
Test Vectors (Trailing '\0' of a character string not included in test):

  key = 	0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b
  key_len =	16 bytes
  data =        "Hi There"
  data_len =	8  bytes
  digest =	0x9294727a3638bb1c13f48ef8158bfc9d

  key =         "Jefe"
  data =        "what do ya want for nothing?"
  data_len =	28 bytes
  digest =	0x750c783e6ab0b503eaa86e310a5db738

  key = 	0xAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

  key_len	16 bytes
  data =	0xDDDDDDDDDDDDDDDDDDDD...
		..DDDDDDDDDDDDDDDDDDDD...
		..DDDDDDDDDDDDDDDDDDDD...
		..DDDDDDDDDDDDDDDDDDDD...
		..DDDDDDDDDDDDDDDDDDDD
  data_len =	50 bytes
  digest =	0x56be34521d144c88dbb8c733f0e8b3f6
*/
