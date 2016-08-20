/*-
 * Copyright (c)1999 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/include/wchar.h 247411 2013-02-27 19:50:46Z jhb $
 */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *	$NetBSD: wchar.h,v 1.8 2000/12/22 05:31:42 itojun Exp $
 */

#ifndef _WCHAR_H_
#define _WCHAR_H_

#include <stdio.h> /* for FILE* */
#include <sys/_null.h>
#include <sys/types.h>
#include <machine/limits.h>
#include <machine/stdarg.h> /* for __va_list */
#include <ctype.h>

#if __POSIX_VISIBLE >= 200809 || __XSI_VISIBLE
#ifndef _VA_LIST_DECLARED
#define	_VA_LIST_DECLARED
#ifdef va_start
/* prevent gcc from fixing this header */
#endif
typedef	__va_list	va_list;
#endif
#endif

#ifndef __cplusplus
#ifndef _WCHAR_T_DECLARED
#define	_WCHAR_T_DECLARED
typedef	__wchar_t	wchar_t;
#endif
#endif

#ifndef WCHAR_MIN
#define	WCHAR_MIN	INT_MIN
#endif

#ifndef WCHAR_MAX
#define	WCHAR_MAX	INT_MAX
#endif

#ifndef _WINT_T_DECLARED
#define	_WINT_T_DECLARED
typedef __wint_t	wint_t;
#endif

#ifndef _MBSTATE_T_DECLARED
#define	_MBSTATE_T_DECLARED
typedef __mbstate_t	mbstate_t;
#endif

#ifndef _SIZE_T_DECLARED
#define _SIZE_T_DECLARED
typedef __size_t	size_t;
#endif

#ifndef WEOF
#define	WEOF 	((wint_t)(-1))
#endif

#if __BSD_VISIBLE
#define WCSBIN_EOF		0x01	/* indicate input buffer EOF */
#define WCSBIN_SURRO		0x02	/* allow surrogates */
#define WCSBIN_LONGCODES	0x04	/* allow up to 31-bit WCs */
#define WCSBIN_STRICT		0x08	/* no escaping, else escapes happen */
#endif

struct tm;

__BEGIN_DECLS
wint_t	btowc(int);
wint_t	fgetwc(FILE *);
wchar_t	*
	fgetws(wchar_t * __restrict, int, FILE * __restrict);
wint_t	fputwc(wchar_t, FILE *);
int	fputws(const wchar_t * __restrict, FILE * __restrict);
int	fwide(FILE *, int);
int	fwprintf(FILE * __restrict, const wchar_t * __restrict, ...);
int	fwscanf(FILE * __restrict, const wchar_t * __restrict, ...);
wint_t	getwc(FILE *);
wint_t	getwchar(void);
size_t	mbrlen(const char * __restrict, size_t, mbstate_t * __restrict);
size_t	mbrtowc(wchar_t * __restrict, const char * __restrict, size_t,
	    mbstate_t * __restrict);
int	mbsinit(const mbstate_t *);
size_t	mbsrtowcs(wchar_t * __restrict, const char ** __restrict, size_t,
	    mbstate_t * __restrict);
#if __BSD_VISIBLE
size_t	mbintowcr(wchar_t * __restrict dst, const char * __restrict src,
	    size_t dlen, size_t *slen, int flags);
#endif
wint_t	putwc(wchar_t, FILE *);
wint_t	putwchar(wchar_t);
int	swprintf(wchar_t * __restrict, size_t n, const wchar_t * __restrict,
	    ...);
int	swscanf(const wchar_t * __restrict, const wchar_t * __restrict, ...);
wint_t	ungetwc(wint_t, FILE *);
int	vfwprintf(FILE * __restrict, const wchar_t * __restrict,
	    __va_list);
int	vswprintf(wchar_t * __restrict, size_t n, const wchar_t * __restrict,
	    __va_list);
int	vwprintf(const wchar_t * __restrict, __va_list);
size_t	wcrtomb(char * __restrict, wchar_t, mbstate_t * __restrict);
#if __BSD_VISIBLE
size_t	wcrtombin(char * __restrict dst, const wchar_t * __restrict src,
	    size_t dlen, size_t *slen, int flags);
#endif
wchar_t	*wcscat(wchar_t * __restrict, const wchar_t * __restrict);
wchar_t	*wcschr(const wchar_t *, wchar_t) __pure;
int	wcscmp(const wchar_t *, const wchar_t *) __pure;
int	wcscoll(const wchar_t *, const wchar_t *);
wchar_t	*wcscpy(wchar_t * __restrict, const wchar_t * __restrict);
size_t	wcscspn(const wchar_t *, const wchar_t *) __pure;
size_t	wcsftime(wchar_t * __restrict, size_t, const wchar_t * __restrict,
	    const struct tm * __restrict);
size_t	wcslen(const wchar_t *) __pure;
wchar_t	*wcsncat(wchar_t * __restrict, const wchar_t * __restrict,
	    size_t);
int	wcsncmp(const wchar_t *, const wchar_t *, size_t) __pure;
wchar_t	*wcsncpy(wchar_t * __restrict , const wchar_t * __restrict, size_t);
wchar_t	*wcspbrk(const wchar_t *, const wchar_t *) __pure;
wchar_t	*wcsrchr(const wchar_t *, wchar_t) __pure;
size_t	wcsrtombs(char * __restrict, const wchar_t ** __restrict, size_t,
	    mbstate_t * __restrict);
size_t	wcsspn(const wchar_t *, const wchar_t *) __pure;
wchar_t	*wcsstr(const wchar_t * __restrict, const wchar_t * __restrict)
	    __pure;
size_t	wcsxfrm(wchar_t * __restrict, const wchar_t * __restrict, size_t);
int	wctob(wint_t);
double	wcstod(const wchar_t * __restrict, wchar_t ** __restrict);
wchar_t	*wcstok(wchar_t * __restrict, const wchar_t * __restrict,
	    wchar_t ** __restrict);
long	 wcstol(const wchar_t * __restrict, wchar_t ** __restrict, int);
unsigned long
	 wcstoul(const wchar_t * __restrict, wchar_t ** __restrict, int);
wchar_t	*wmemchr(const wchar_t *, wchar_t, size_t) __pure;
int	wmemcmp(const wchar_t *, const wchar_t *, size_t) __pure;
wchar_t	*wmemcpy(wchar_t * __restrict, const wchar_t * __restrict, size_t);
wchar_t	*wmemmove(wchar_t *, const wchar_t *, size_t);
wchar_t	*wmemset(wchar_t *, wchar_t, size_t);
int	wprintf(const wchar_t * __restrict, ...);
int	wscanf(const wchar_t * __restrict, ...);

#if __BSD_VISIBLE
size_t	utf8towcr(wchar_t * __restrict dst, const char * __restrict src,
	    size_t dlen, size_t *slen, int flags);
size_t	wcrtoutf8(char * __restrict dst, const wchar_t * __restrict src,
	    size_t dlen, size_t *slen, int flags);
#endif

#ifndef _STDSTREAM_DECLARED
extern FILE *__stdinp;
extern FILE *__stdoutp;
extern FILE *__stderrp;
#define	_STDSTREAM_DECLARED
#endif

#define	getwc(fp)	fgetwc(fp)
#define	getwchar()	fgetwc(__stdinp)
#define	putwc(wc, fp)	fputwc(wc, fp)
#define	putwchar(wc)	fputwc(wc, __stdoutp)

#if __ISO_C_VISIBLE >= 1999
int	vfwscanf(FILE * __restrict, const wchar_t * __restrict,
	    __va_list);
int	vswscanf(const wchar_t * __restrict, const wchar_t * __restrict,
	    __va_list);
int	vwscanf(const wchar_t * __restrict, __va_list);
float	wcstof(const wchar_t * __restrict, wchar_t ** __restrict);
long double
	wcstold(const wchar_t * __restrict, wchar_t ** __restrict);
#ifdef __LONG_LONG_SUPPORTED
/* LONGLONG */
long long
	wcstoll(const wchar_t * __restrict, wchar_t ** __restrict, int);
/* LONGLONG */
unsigned long long
	 wcstoull(const wchar_t * __restrict, wchar_t ** __restrict, int);
#endif
#endif	/* __ISO_C_VISIBLE >= 1999 */

#if __XSI_VISIBLE
int	wcswidth(const wchar_t *, size_t);
int	wcwidth(wchar_t);
#define	wcwidth(_c)	__wcwidth(_c)
#endif

#if __POSIX_VISIBLE >= 200809
size_t	mbsnrtowcs(wchar_t * __restrict, const char ** __restrict, size_t,
	    size_t, mbstate_t * __restrict);
FILE	*open_wmemstream(wchar_t **, size_t *);
wchar_t	*wcpcpy(wchar_t * __restrict, const wchar_t * __restrict);
wchar_t	*wcpncpy(wchar_t * __restrict, const wchar_t * __restrict, size_t);
wchar_t	*wcsdup(const wchar_t *);
int	wcscasecmp(const wchar_t *, const wchar_t *);
int	wcsncasecmp(const wchar_t *, const wchar_t *, size_t n);
size_t	wcsnlen(const wchar_t *, size_t) __pure;
size_t	wcsnrtombs(char * __restrict, const wchar_t ** __restrict, size_t,
	    size_t, mbstate_t * __restrict);
#endif

#if __BSD_VISIBLE
wchar_t	*fgetwln(FILE * __restrict, size_t * __restrict);
size_t	wcslcat(wchar_t *, const wchar_t *, size_t);
size_t	wcslcpy(wchar_t *, const wchar_t *, size_t);
#endif

#if __POSIX_VISIBLE >= 200809 || defined(_XLOCALE_H_)
#include <xlocale/_wchar.h>
#endif
__END_DECLS

#endif /* !_WCHAR_H_ */
