/* Note: copied from newlib */
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stddef.h>
#include <limits.h>

/*
 * Copyright (C) 2004 CodeSourcery, LLC
 *
 * Permission to use, copy, modify, and distribute this file
 * for any purpose is hereby granted without fee, provided that
 * the above copyright notice and this notice appears in all
 * copies.
 *
 * This file is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

/* Handle ELF .{pre_init,init,fini}_array sections.  */
#include <sys/types.h>

#ifndef HAVE_INITFINI_ARRAY
#define HAVE_INITFINI_ARRAY
#endif

#undef HAVE_INIT_FINI

#ifdef HAVE_INITFINI_ARRAY

/* These magic symbols are provided by the linker.  */
extern void (*__preinit_array_start []) (void) __attribute__((weak));
extern void (*__preinit_array_end []) (void) __attribute__((weak));
extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));

#ifdef HAVE_INIT_FINI
extern void _init (void);
#endif

/* Iterate over all the init routines.  */
void
__libc_init_array (void)
{
  size_t count;
  size_t i;

  count = __preinit_array_end - __preinit_array_start;
  for (i = 0; i < count; i++)
    __preinit_array_start[i] ();

#ifdef HAVE_INIT_FINI
  _init ();
#endif

  count = __init_array_end - __init_array_start;
  for (i = 0; i < count; i++)
    __init_array_start[i] ();
}
#endif

#ifdef HAVE_INITFINI_ARRAY
extern void (*__fini_array_start []) (void) __attribute__((weak));
extern void (*__fini_array_end []) (void) __attribute__((weak));

#ifdef HAVE_INIT_FINI
extern void _fini (void);
#endif

/* Run all the cleanup routines.  */
void
__libc_fini_array (void)
{
  size_t count;
  size_t i;
  
  count = __fini_array_end - __fini_array_start;
  for (i = count; i > 0; i--)
    __fini_array_start[i-1] ();

#ifdef HAVE_INIT_FINI
  _fini ();
#endif
}
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
	<<memchr>>---find character in memory
INDEX
	memchr
SYNOPSIS
	#include <string.h>
	void *memchr(const void *<[src]>, int <[c]>, size_t <[length]>);
DESCRIPTION
	This function searches memory starting at <<*<[src]>>> for the
	character <[c]>.  The search only ends with the first
	occurrence of <[c]>, or after <[length]> characters; in
	particular, <<NUL>> does not terminate the search.
RETURNS
	If the character <[c]> is found within <[length]> characters
	of <<*<[src]>>>, a pointer to the character is returned. If
	<[c]> is not found, then <<NULL>> is returned.
PORTABILITY
<<memchr>> is ANSI C.
<<memchr>> requires no supporting OS subroutines.
QUICKREF
	memchr ansi pure
*/

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL


/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X) ((long)X & (sizeof (long) - 1))

/* How many bytes are loaded each iteration of the word copy loop.  */
#define LBLOCKSIZE (sizeof (long))

/* Threshhold for punting to the bytewise iterator.  */
#define TOO_SMALL(LEN)  ((LEN) < LBLOCKSIZE)

#if LONG_MAX == 2147483647L
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)
#else
#if LONG_MAX == 9223372036854775807L
/* Nonzero if X (a long int) contains a NULL byte. */
#define DETECTNULL(X) (((X) - 0x0101010101010101) & ~(X) & 0x8080808080808080)
#else
#error long int is not a 32bit or 64bit type.
#endif
#endif

#ifndef DETECTNULL
#error long int is not a 32bit or 64bit byte
#endif

/* DETECTCHAR returns nonzero if (long)X contains the byte used
   to fill (long)MASK. */
#define DETECTCHAR(X,MASK) (DETECTNULL(X ^ MASK))

void *
memchr (const void *src_void,
	int c,
	size_t length)
{
  const unsigned char *src = (const unsigned char *) src_void;
  unsigned char d = c;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
  unsigned long *asrc;
  unsigned long  mask;
  unsigned int i;

  while (UNALIGNED (src))
    {
      if (!length--)
        return NULL;
      if (*src == d)
        return (void *) src;
      src++;
    }

  if (!TOO_SMALL (length))
    {
      /* If we get this far, we know that length is large and src is
         word-aligned. */
      /* The fast code reads the source one word at a time and only
         performs the bytewise search on word-sized segments if they
         contain the search character, which is detected by XORing
         the word-sized segment with a word-sized block of the search
         character and then detecting for the presence of NUL in the
         result.  */
      asrc = (unsigned long *) src;
      mask = d << 8 | d;
      mask = mask << 16 | mask;
      for (i = 32; i < LBLOCKSIZE * 8; i <<= 1)
        mask = (mask << i) | mask;

      while (length >= LBLOCKSIZE)
        {
          if (DETECTCHAR (*asrc, mask))
            break;
          length -= LBLOCKSIZE;
          asrc++;
        }

      /* If there are fewer than LBLOCKSIZE characters left,
         then we resort to the bytewise loop.  */

      src = (unsigned char *) asrc;
    }

#endif /* not PREFER_SIZE_OVER_SPEED */

  while (length--)
    {
      if (*src == d)
        return (void *) src;
      src++;
    }

  return NULL;
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

/*
FUNCTION
	<<strchr>>---search for character in string
INDEX
	strchr
SYNOPSIS
	#include <string.h>
	char * strchr(const char *<[string]>, int <[c]>);
DESCRIPTION
	This function finds the first occurence of <[c]> (converted to
	a char) in the string pointed to by <[string]> (including the
	terminating null character).
RETURNS
	Returns a pointer to the located character, or a null pointer
	if <[c]> does not occur in <[string]>.
PORTABILITY
<<strchr>> is ANSI C.
<<strchr>> requires no supporting OS subroutines.
QUICKREF
	strchr ansi pure
*/

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL


/* Nonzero if X is not aligned on a "long" boundary.  */
#define UNALIGNED(X) ((long)X & (sizeof (long) - 1))

/* How many bytes are loaded each iteration of the word copy loop.  */
#define LBLOCKSIZE (sizeof (long))

char *
strchr (const char *s1,
	int i)
{
  const unsigned char *s = (const unsigned char *)s1;
  unsigned char c = i;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
  unsigned long mask,j;
  unsigned long *aligned_addr;

  /* Special case for finding 0.  */
  if (!c)
    {
      while (UNALIGNED (s))
        {
          if (!*s)
            return (char *) s;
          s++;
        }
      /* Operate a word at a time.  */
      aligned_addr = (unsigned long *) s;
      while (!DETECTNULL (*aligned_addr))
        aligned_addr++;
      /* Found the end of string.  */
      s = (const unsigned char *) aligned_addr;
      while (*s)
        s++;
      return (char *) s;
    }

  /* All other bytes.  Align the pointer, then search a long at a time.  */
  while (UNALIGNED (s))
    {
      if (!*s)
        return NULL;
      if (*s == c)
        return (char *) s;
      s++;
    }

  mask = c;
  for (j = 8; j < LBLOCKSIZE * 8; j <<= 1)
    mask = (mask << j) | mask;

  aligned_addr = (unsigned long *) s;
  while (!DETECTNULL (*aligned_addr) && !DETECTCHAR (*aligned_addr, mask))
    aligned_addr++;

  /* The block of bytes currently pointed to by aligned_addr
     contains either a null or the target char, or both.  We
     catch it using the bytewise search.  */

  s = (unsigned char *) aligned_addr;

#endif /* not PREFER_SIZE_OVER_SPEED */

  while (*s && *s != c)
    s++;
  if (*s == c)
    return (char *)s;
  return NULL;
}

/*
FUNCTION
	<<strcmp>>---character string compare
	
INDEX
	strcmp
SYNOPSIS
	#include <string.h>
	int strcmp(const char *<[a]>, const char *<[b]>);
DESCRIPTION
	<<strcmp>> compares the string at <[a]> to
	the string at <[b]>.
RETURNS
	If <<*<[a]>>> sorts lexicographically after <<*<[b]>>>,
	<<strcmp>> returns a number greater than zero.  If the two
	strings match, <<strcmp>> returns zero.  If <<*<[a]>>>
	sorts lexicographically before <<*<[b]>>>, <<strcmp>> returns a
	number less than zero.
PORTABILITY
<<strcmp>> is ANSI C.
<<strcmp>> requires no supporting OS subroutines.
QUICKREF
	strcmp ansi pure
*/

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

int
strcmp (const char *s1,
	const char *s2)
{ 
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  while (*s1 != '\0' && *s1 == *s2)
    {
      s1++;
      s2++;
    }

  return (*(unsigned char *) s1) - (*(unsigned char *) s2);
#else
  unsigned long *a1;
  unsigned long *a2;

  /* If s1 or s2 are unaligned, then compare bytes. */
  if (!UNALIGNED (s1, s2))
    {  
      /* If s1 and s2 are word-aligned, compare them a word at a time. */
      a1 = (unsigned long*)s1;
      a2 = (unsigned long*)s2;
      while (*a1 == *a2)
        {
          /* To get here, *a1 == *a2, thus if we find a null in *a1,
	     then the strings must be equal, so return zero.  */
          if (DETECTNULL (*a1))
	    return 0;

          a1++;
          a2++;
        }

      /* A difference was detected in last few bytes of s1, so search bytewise */
      s1 = (char*)a1;
      s2 = (char*)a2;
    }

  while (*s1 != '\0' && *s1 == *s2)
    {
      s1++;
      s2++;
    }
  return (*(unsigned char *) s1) - (*(unsigned char *) s2);
#endif /* not PREFER_SIZE_OVER_SPEED */
}

/*
FUNCTION
	<<strcpy>>---copy string
INDEX
	strcpy
SYNOPSIS
	#include <string.h>
	char *strcpy(char *<[dst]>, const char *<[src]>);
DESCRIPTION
	<<strcpy>> copies the string pointed to by <[src]>
	(including the terminating null character) to the array
	pointed to by <[dst]>.
RETURNS
	This function returns the initial value of <[dst]>.
PORTABILITY
<<strcpy>> is ANSI C.
<<strcpy>> requires no supporting OS subroutines.
QUICKREF
	strcpy ansi pure
*/

/*SUPPRESS 560*/
/*SUPPRESS 530*/

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

char*
strcpy (char *dst0,
	const char *src0)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  char *s = dst0;

  while ((*dst0++ = *src0++))
    ;

  return s;
#else
  char *dst = dst0;
  const char *src = src0;
  long *aligned_dst;
  const long *aligned_src;

  /* If SRC or DEST is unaligned, then copy bytes.  */
  if (!UNALIGNED (src, dst))
    {
      aligned_dst = (long*)dst;
      aligned_src = (long*)src;

      /* SRC and DEST are both "long int" aligned, try to do "long int"
         sized copies.  */
      while (!DETECTNULL(*aligned_src))
        {
          *aligned_dst++ = *aligned_src++;
        }

      dst = (char*)aligned_dst;
      src = (char*)aligned_src;
    }

  while ((*dst++ = *src++))
    ;
  return dst0;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

/*
FUNCTION
	<<strlen>>---character string length
INDEX
	strlen
SYNOPSIS
	#include <string.h>
	size_t strlen(const char *<[str]>);
DESCRIPTION
	The <<strlen>> function works out the length of the string
	starting at <<*<[str]>>> by counting chararacters until it
	reaches a <<NULL>> character.
RETURNS
	<<strlen>> returns the character count.
PORTABILITY
<<strlen>> is ANSI C.
<<strlen>> requires no supporting OS subroutines.
QUICKREF
	strlen ansi pure
*/

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL

#define LBLOCKSIZE   (sizeof (long))
#define UNALIGNED(X) ((long)X & (LBLOCKSIZE - 1))
size_t
strlen (const char *str)
{
  const char *start = str;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
  unsigned long *aligned_addr;

  /* Align the pointer, so we can search a word at a time.  */
  while (UNALIGNED (str))
    {
      if (!*str)
	return str - start;
      str++;
    }

  /* If the string is word-aligned, we can check for the presence of
     a null in each word-sized block.  */
  aligned_addr = (unsigned long *)str;
  while (!DETECTNULL (*aligned_addr))
    aligned_addr++;

  /* Once a null is detected, we check each byte in that block for a
     precise position of the null.  */
  str = (char *) aligned_addr;

#endif /* not PREFER_SIZE_OVER_SPEED */

  while (*str)
    str++;
  return str - start;
}

/*
FUNCTION
	<<strncmp>>---character string compare
	
INDEX
	strncmp
SYNOPSIS
	#include <string.h>
	int strncmp(const char *<[a]>, const char * <[b]>, size_t <[length]>);
DESCRIPTION
	<<strncmp>> compares up to <[length]> characters
	from the string at <[a]> to the string at <[b]>.
RETURNS
	If <<*<[a]>>> sorts lexicographically after <<*<[b]>>>,
	<<strncmp>> returns a number greater than zero.  If the two
	strings are equivalent, <<strncmp>> returns zero.  If <<*<[a]>>>
	sorts lexicographically before <<*<[b]>>>, <<strncmp>> returns a
	number less than zero.
PORTABILITY
<<strncmp>> is ANSI C.
<<strncmp>> requires no supporting OS subroutines.
QUICKREF
	strncmp ansi pure
*/

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL

#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

int 
strncmp (const char *s1,
	const char *s2,
	size_t n)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  if (n == 0)
    return 0;

  while (n-- != 0 && *s1 == *s2)
    {
      if (n == 0 || *s1 == '\0')
	break;
      s1++;
      s2++;
    }

  return (*(unsigned char *) s1) - (*(unsigned char *) s2);
#else
  unsigned long *a1;
  unsigned long *a2;

  if (n == 0)
    return 0;

  /* If s1 or s2 are unaligned, then compare bytes. */
  if (!UNALIGNED (s1, s2))
    {
      /* If s1 and s2 are word-aligned, compare them a word at a time. */
      a1 = (unsigned long*)s1;
      a2 = (unsigned long*)s2;
      while (n >= sizeof (long) && *a1 == *a2)
        {
          n -= sizeof (long);

          /* If we've run out of bytes or hit a null, return zero
	     since we already know *a1 == *a2.  */
          if (n == 0 || DETECTNULL (*a1))
	    return 0;

          a1++;
          a2++;
        }

      /* A difference was detected in last few bytes of s1, so search bytewise */
      s1 = (char*)a1;
      s2 = (char*)a2;
    }

  while (n-- > 0 && *s1 == *s2)
    {
      /* If we've run out of bytes or hit a null, return zero
	 since we already know *s1 == *s2.  */
      if (n == 0 || *s1 == '\0')
	return 0;
      s1++;
      s2++;
    }
  return (*(unsigned char *) s1) - (*(unsigned char *) s2);
#endif /* not PREFER_SIZE_OVER_SPEED */
}

/*
FUNCTION
	<<strncpy>>---counted copy string
INDEX
	strncpy
SYNOPSIS
	#include <string.h>
	char *strncpy(char *restrict <[dst]>, const char *restrict <[src]>,
                      size_t <[length]>);
DESCRIPTION
	<<strncpy>> copies not more than <[length]> characters from the
	the string pointed to by <[src]> (including the terminating
	null character) to the array pointed to by <[dst]>.  If the
	string pointed to by <[src]> is shorter than <[length]>
	characters, null characters are appended to the destination
	array until a total of <[length]> characters have been
	written.
RETURNS
	This function returns the initial value of <[dst]>.
PORTABILITY
<<strncpy>> is ANSI C.
<<strncpy>> requires no supporting OS subroutines.
QUICKREF
	strncpy ansi pure
*/

/*SUPPRESS 560*/
/*SUPPRESS 530*/

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL

#define UNALIGNED(X, Y) \
  (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

#define TOO_SMALL(LEN) ((LEN) < sizeof (long))

char *
strncpy (char *__restrict dst0,
	const char *__restrict src0,
	size_t count)
{
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  char *dscan;
  const char *sscan;

  dscan = dst0;
  sscan = src0;
  while (count > 0)
    {
      --count;
      if ((*dscan++ = *sscan++) == '\0')
	break;
    }
  while (count-- > 0)
    *dscan++ = '\0';

  return dst0;
#else
  char *dst = dst0;
  const char *src = src0;
  long *aligned_dst;
  const long *aligned_src;

  /* If SRC and DEST is aligned and count large enough, then copy words.  */
  if (!UNALIGNED (src, dst) && !TOO_SMALL (count))
    {
      aligned_dst = (long*)dst;
      aligned_src = (long*)src;

      /* SRC and DEST are both "long int" aligned, try to do "long int"
	 sized copies.  */
      while (count >= sizeof (long int) && !DETECTNULL(*aligned_src))
	{
	  count -= sizeof (long int);
	  *aligned_dst++ = *aligned_src++;
	}

      dst = (char*)aligned_dst;
      src = (char*)aligned_src;
    }

  while (count > 0)
    {
      --count;
      if ((*dst++ = *src++) == '\0')
	break;
    }

  while (count-- > 0)
    *dst++ = '\0';

  return dst0;
#endif /* not PREFER_SIZE_OVER_SPEED */
}

/* 
FUNCTION
	<<strnlen>>---character string length
	
INDEX
	strnlen
SYNOPSIS
	#include <string.h>
	size_t strnlen(const char *<[str]>, size_t <[n]>);
DESCRIPTION
	The <<strnlen>> function works out the length of the string
	starting at <<*<[str]>>> by counting chararacters until it
	reaches a NUL character or the maximum: <[n]> number of
        characters have been inspected.
RETURNS
	<<strnlen>> returns the character count or <[n]>.
PORTABILITY
<<strnlen>> is a GNU extension.
<<strnlen>> requires no supporting OS subroutines.
*/

size_t
strnlen (const char *str,
	size_t n)
{
  const char *start = str;

  while (n-- > 0 && *str)
    str++;

  return str - start;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
