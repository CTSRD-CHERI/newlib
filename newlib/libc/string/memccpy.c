/*
FUNCTION
        <<memccpy>>---copy memory regions with end-token check

SYNOPSIS
        #include <string.h>
        void* memccpy(void *restrict <[out]>, const void *restrict <[in]>,
                      int <[endchar]>, size_t <[n]>);

DESCRIPTION
        This function copies up to <[n]> bytes from the memory region
        pointed to by <[in]> to the memory region pointed to by
        <[out]>.  If a byte matching the <[endchar]> is encountered,
	the byte is copied and copying stops.

        If the regions overlap, the behavior is undefined.

RETURNS
        <<memccpy>> returns a pointer to the first byte following the
	<[endchar]> in the <[out]> region.  If no byte matching
	<[endchar]> was copied, then <<NULL>> is returned.

PORTABILITY
<<memccpy>> is a GNU extension.

<<memccpy>> requires no supporting OS subroutines.

	*/

#include <_ansi.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#ifdef __CHERI__
typedef __intcap_t BLOCK_TYPE;
typedef __uintcap_t UBLOCK_TYPE;
#else
typedef long BLOCK_TYPE;
typedef unsigned long UBLOCK_TYPE;
#endif

/* Nonzero if either X or Y is not aligned on a "BLOCK_TYPE" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (BLOCK_TYPE) - 1)) | ((long)Y & (sizeof (BLOCK_TYPE) - 1)))

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (BLOCK_TYPE))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < LITTLEBLOCKSIZE)

/* Macros for detecting endchar */
#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
/* Nonzero if X (a BLOCK_TYPE int) contains a NULL byte. */
#define DETECTNULL(X) (((X) - 0x0101010101010101) & ~(X) & 0x8080808080808080)
#else
#error BLOCK_TYPE int is not a 32bit or 64bit type.
#endif
#endif


void *
memccpy (void *__restrict dst0,
	const void *__restrict src0,
	int endchar0,
	size_t len0)
{

#if (defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)) && !defined(__CHERI__)
  void *ptr = NULL;
  char *dst = (char *) dst0;
  char *src = (char *) src0;
  char endchar = endchar0 & 0xff;

  while (len0--)
    {
      if ((*dst++ = *src++) == endchar)
        {
          ptr = dst;
          break;
        }
    }

  return ptr;
#else
  void *ptr = NULL;
  unsigned char *dst = dst0;
  const unsigned char *src = src0;
  BLOCK_TYPE *aligned_dst;
  const BLOCK_TYPE *aligned_src;
  unsigned char endchar = endchar0 & 0xff;

  /* If the size is small, or either SRC or DST is unaligned,
     then punt into the byte copy loop.  This should be rare.  */
  if (!TOO_SMALL(len0) && !UNALIGNED (src, dst))
    {
      unsigned int i;
#ifndef __CHERI__
      UBLOCK_TYPE mask = 0;
#endif

      aligned_dst = (BLOCK_TYPE*)dst;
      aligned_src = (BLOCK_TYPE*)src;

      /* The fast code reads the ASCII one word at a time and only
         performs the bytewise search on word-sized segments if they
         contain the search character, which is detected by XORing
         the word-sized segment with a word-sized block of the search
         character and then detecting for the presence of NULL in the
         result.  */
#ifndef __CHERI__
      for (i = 0; i < LITTLEBLOCKSIZE; i++)
        mask = (mask << 8) + endchar;
#endif


      /* Copy one BLOCK_TYPE word at a time if possible.  */
      while (len0 >= LITTLEBLOCKSIZE)
        {
#ifdef __CHERI__
          /* Can't read long's due to strict aliasing between long and
           * __intcap_t for aligned_src.  */
          for (i = 0; i < LITTLEBLOCKSIZE; i++)
            if (((char *)aligned_src)[i] == endchar)
              break;
          if (i != LITTLEBLOCKSIZE)
            break; /* endchar is found, go byte by byte from here */
          *aligned_dst++ = *aligned_src++;
          len0 -= LITTLEBLOCKSIZE;
#else
          UBLOCK_TYPE buffer = (UBLOCK_TYPE)(*aligned_src);
          buffer ^=  mask;
          if (DETECTNULL (buffer))
            break; /* endchar is found, go byte by byte from here */
          *aligned_dst++ = *aligned_src++;
          len0 -= LITTLEBLOCKSIZE;
#endif
        }

       /* Pick up any residual with a byte copier.  */
      dst = (unsigned char*)aligned_dst;
      src = (unsigned char*)aligned_src;
    }

  while (len0--)
    {
      if ((*dst++ = *src++) == endchar)
        {
          ptr = dst;
          break;
        }
    }

  return ptr;
#endif /* not PREFER_SIZE_OVER_SPEED */
}
