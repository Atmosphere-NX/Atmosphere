/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string.h>
#include <stddef.h>
#include <limits.h>

/* Note: copied from newlib */
#ifdef __cplusplus
extern "C" {
#endif

/*
FUNCTION
	<<memmove>>---move possibly overlapping memory
INDEX
	memmove
SYNOPSIS
	#include <string.h>
	void *memmove(void *<[dst]>, const void *<[src]>, size_t <[length]>);
DESCRIPTION
	This function moves <[length]> characters from the block of
	memory starting at <<*<[src]>>> to the memory starting at
	<<*<[dst]>>>. <<memmove>> reproduces the characters correctly
	at <<*<[dst]>>> even if the two areas overlap.
RETURNS
	The function returns <[dst]> as passed.
PORTABILITY
<<memmove>> is ANSI C.
<<memmove>> requires no supporting OS subroutines.
QUICKREF
	memmove ansi pure
*/

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (long) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the byte copier.  */
#undef TOO_SMALL
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

/*SUPPRESS 20*/
void *
//__inhibit_loop_to_libcall
__attribute__((weak))
memmove (void *dst_void,
	const void *src_void,
	size_t length)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  char *dst = dst_void;
  const char *src = src_void;

  if (src < dst && dst < src + length)
    {
      /* Have to copy backwards */
      src += length;
      dst += length;
      while (length--)
	{
	  *--dst = *--src;
	}
    }
  else
    {
      while (length--)
	{
	  *dst++ = *src++;
	}
    }

  return dst_void;
#else
  char *dst = dst_void;
  const char *src = src_void;
  long *aligned_dst;
  const long *aligned_src;

  if (src < dst && dst < src + length)
    {
      /* Destructive overlap...have to copy backwards */
      src += length;
      dst += length;
      while (length--)
	{
	  *--dst = *--src;
	}
    }
  else
    {
      /* Use optimizing algorithm for a non-destructive copy to closely
         match memcpy. If the size is small or either SRC or DST is unaligned,
         then punt into the byte copy loop.  This should be rare.  */
      if (!TOO_SMALL(length) && !UNALIGNED (src, dst))
        {
          aligned_dst = (long*)dst;
          aligned_src = (long*)src;

          /* Copy 4X long words at a time if possible.  */
          while (length >= BIGBLOCKSIZE)
            {
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              *aligned_dst++ = *aligned_src++;
              length -= BIGBLOCKSIZE;
            }

          /* Copy one long word at a time if possible.  */
          while (length >= LITTLEBLOCKSIZE)
            {
              *aligned_dst++ = *aligned_src++;
              length -= LITTLEBLOCKSIZE;
            }

          /* Pick up any residual with a byte copier.  */
          dst = (char*)aligned_dst;
          src = (char*)aligned_src;
        }

      while (length--)
        {
          *dst++ = *src++;
        }
    }

  return dst_void;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

/*
FUNCTION
        <<memcpy>>---copy memory regions
SYNOPSIS
        #include <string.h>
        void* memcpy(void *restrict <[out]>, const void *restrict <[in]>,
                     size_t <[n]>);
DESCRIPTION
        This function copies <[n]> bytes from the memory region
        pointed to by <[in]> to the memory region pointed to by
        <[out]>.
        If the regions overlap, the behavior is undefined.
RETURNS
        <<memcpy>> returns a pointer to the first byte of the <[out]>
        region.
PORTABILITY
<<memcpy>> is ANSI C.
<<memcpy>> requires no supporting OS subroutines.
QUICKREF
        memcpy ansi pure
	*/

void *
__attribute__((weak))
memcpy (void * dst0,
	const void * __restrict src0,
	size_t len0)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  char *dst = (char *) dst0;
  char *src = (char *) src0;

  void *save = dst0;

  while (len0--)
    {
      *dst++ = *src++;
    }

  return save;
#else
  char *dst = dst0;
  const char *src = src0;
  long *aligned_dst;
  const long *aligned_src;

  /* If the size is small, or either SRC or DST is unaligned,
     then punt into the byte copy loop.  This should be rare.  */
  if (!TOO_SMALL(len0) && !UNALIGNED (src, dst))
    {
      aligned_dst = (long*)dst;
      aligned_src = (long*)src;

      /* Copy 4X long words at a time if possible.  */
      while (len0 >= BIGBLOCKSIZE)
        {
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
          *aligned_dst++ = *aligned_src++;
          len0 -= BIGBLOCKSIZE;
        }

      /* Copy one long word at a time if possible.  */
      while (len0 >= LITTLEBLOCKSIZE)
        {
          *aligned_dst++ = *aligned_src++;
          len0 -= LITTLEBLOCKSIZE;
        }

       /* Pick up any residual with a byte copier.  */
      dst = (char*)aligned_dst;
      src = (char*)aligned_src;
    }

  while (len0--)
    *dst++ = *src++;

  return dst0;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

/*
FUNCTION
	<<memset>>---set an area of memory
INDEX
	memset
SYNOPSIS
	#include <string.h>
	void *memset(void *<[dst]>, int <[c]>, size_t <[length]>);
DESCRIPTION
	This function converts the argument <[c]> into an unsigned
	char and fills the first <[length]> characters of the array
	pointed to by <[dst]> to the value.
RETURNS
	<<memset>> returns the value of <[dst]>.
PORTABILITY
<<memset>> is ANSI C.
    <<memset>> requires no supporting OS subroutines.
QUICKREF
	memset ansi pure
*/

#include <string.h>

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL

#define LBLOCKSIZE (sizeof(long))
#define UNALIGNED(X)   ((long)X & (LBLOCKSIZE - 1))
#define TOO_SMALL(LEN) ((LEN) < LBLOCKSIZE)

void *
__attribute__((weak))
memset (void *m,
	int c,
	size_t n)
{
  char *s = (char *) m;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
  unsigned int i;
  unsigned long buffer;
  unsigned long *aligned_addr;
  unsigned int d = c & 0xff;	/* To avoid sign extension, copy C to an
				   unsigned variable.  */

  while (UNALIGNED (s))
    {
      if (n--)
        *s++ = (char) c;
      else
        return m;
    }

  if (!TOO_SMALL (n))
    {
      /* If we get this far, we know that n is large and s is word-aligned. */
      aligned_addr = (unsigned long *) s;

      /* Store D into each char sized location in BUFFER so that
         we can set large blocks quickly.  */
      buffer = (d << 8) | d;
      buffer |= (buffer << 16);
      for (i = 32; i < LBLOCKSIZE * 8; i <<= 1)
        buffer = (buffer << i) | buffer;

      /* Unroll the loop.  */
      while (n >= LBLOCKSIZE*4)
        {
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          *aligned_addr++ = buffer;
          n -= 4*LBLOCKSIZE;
        }

      while (n >= LBLOCKSIZE)
        {
          *aligned_addr++ = buffer;
          n -= LBLOCKSIZE;
        }
      /* Pick up the remainder with a bytewise loop.  */
      s = (char*)aligned_addr;
    }

#endif /* not PREFER_SIZE_OVER_SPEED */

  while (n--)
    *s++ = (char) c;

  return m;
}

/*
FUNCTION
	<<memcmp>>---compare two memory areas
INDEX
	memcmp
SYNOPSIS
	#include <string.h>
	int memcmp(const void *<[s1]>, const void *<[s2]>, size_t <[n]>);
DESCRIPTION
	This function compares not more than <[n]> characters of the
	object pointed to by <[s1]> with the object pointed to by <[s2]>.
RETURNS
	The function returns an integer greater than, equal to or
	less than zero 	according to whether the object pointed to by
	<[s1]> is greater than, equal to or less than the object
	pointed to by <[s2]>.
PORTABILITY
<<memcmp>> is ANSI C.
<<memcmp>> requires no supporting OS subroutines.
QUICKREF
	memcmp ansi pure
*/

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* How many bytes are copied each iteration of the word copy loop.  */
#define LBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < LBLOCKSIZE)

int
__attribute__((weak))
memcmp (const void *m1,
	const void *m2,
	size_t n)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  unsigned char *s1 = (unsigned char *) m1;
  unsigned char *s2 = (unsigned char *) m2;

  while (n--)
    {
      if (*s1 != *s2)
	{
	  return *s1 - *s2;
	}
      s1++;
      s2++;
    }
  return 0;
#else
  unsigned char *s1 = (unsigned char *) m1;
  unsigned char *s2 = (unsigned char *) m2;
  unsigned long *a1;
  unsigned long *a2;

  /* If the size is too small, or either pointer is unaligned,
     then we punt to the byte compare loop.  Hopefully this will
     not turn up in inner loops.  */
  if (!TOO_SMALL(n) && !UNALIGNED(s1,s2))
    {
      /* Otherwise, load and compare the blocks of memory one
         word at a time.  */
      a1 = (unsigned long*) s1;
      a2 = (unsigned long*) s2;
      while (n >= LBLOCKSIZE)
        {
          if (*a1 != *a2)
   	    break;
          a1++;
          a2++;
          n -= LBLOCKSIZE;
        }

      /* check m mod LBLOCKSIZE remaining characters */

      s1 = (unsigned char*)a1;
      s2 = (unsigned char*)a2;
    }

  while (n--)
    {
      if (*s1 != *s2)
	return *s1 - *s2;
      s1++;
      s2++;
    }

  return 0;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

#ifdef __cplusplus
} /* extern "C" */
#endif
