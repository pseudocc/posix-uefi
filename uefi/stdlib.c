/*
 * stdlib.c
 *
 * Copyright (C) 2021 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of the POSIX-UEFI package.
 * @brief Implementing functions which are defined in stdlib.h
 *
 */

#include <uefi.h>

int errno = 0;
static uint64_t __srand_seed = 6364136223846793005ULL;

int atoi(const wchar_t *s)
{
    return (int)atol(s);
}

int64_t atol(const wchar_t *s)
{
    int64_t sign = 1;
    if(!s || !*s) return 0;
    if(*s == L'-') { sign = -1; s++; }
    if(s[0] == L'0') {
        if(s[1] == L'x')
            return strtol(s + 2, NULL, 16);
        if(s[1] >= L'0' && s[1] <= L'7')
            return strtol(s, NULL, 8);
    }
    return strtol(s, NULL, 10) * sign;
}

int64_t strtol (const wchar_t *s, wchar_t **__endptr, int __base)
{
    int64_t v=0, sign = 1;
    if(!s || !*s) return 0;
    if(*s == L'-') { sign = -1; s++; }
    while(!(*s < L'0' || (__base < 10 && *s >= __base + L'0') || (__base >= 10 && ((*s > L'9' && *s < L'A') ||
            (*s > L'F' && *s < L'a') || *s > L'f')))) {
        v *= __base;
        if(*s >= L'0' && *s <= (__base < 10 ? __base + L'0' : L'9'))
            v += (*s)-L'0';
        else if(__base == 16 && *s >= L'a' && *s <= L'f')
            v += (*s)-L'a'+10;
        else if(__base == 16 && *s >= L'A' && *s <= L'F')
            v += (*s)-L'A'+10;
        s++;
    };
    if(__endptr) *__endptr = (wchar_t*)s;
    return v * sign;
}

void *malloc (size_t __size)
{
    void *ret = NULL;
    efi_status_t status = BS->AllocatePool(LIP ? LIP->ImageDataType : EfiLoaderData, __size, &ret);
    if(EFI_ERROR(status) || !ret) { errno = ENOMEM; ret = NULL; }
    return ret;
}

void *calloc (size_t __nmemb, size_t __size)
{
    void *ret = malloc(__nmemb * __size);
    if(ret) memset(ret, 0, __nmemb * __size);
    return ret;
}

void *realloc (void *__ptr, size_t __size)
{
#if 1
    void *ret = __ptr;
    /* not sure if this works */
    efi_status_t status = BS->AllocatePool(LIP ? LIP->ImageDataType : EfiLoaderData, __size, &ret);
    if(EFI_ERROR(status) || !ret) { errno = ENOMEM; ret = NULL; }
    return ret;
#else
    void *ret = malloc(__size);
    /* this isn't perfect, because we don't know the original size... */
    if(ret && __ptr) memcpy(ret, __ptr, __size);
    if(__ptr) free(__ptr);
    return ret;
#endif
}

void free (void *__ptr)
{
    efi_status_t status = BS->FreePool(__ptr);
    if(EFI_ERROR(status)) errno = ENOMEM;
}

void abort ()
{
    BS->Exit(IM, EFI_ABORTED, 0, NULL);
}

void exit (int __status)
{
    BS->Exit(IM, !__status ? 0 : (__status < 0 ? EFIERR(-__status) : EFIERR(__status)), 0, NULL);
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, __compar_fn_t cmp)
{
    uint64_t s=0, e=nmemb, m;
    int ret;
    while (s < e) {
        m = s + (e-s)/2;
        ret = cmp(key, (uint8_t*)base + m*size);
        if (ret < 0) e = m; else
        if (ret > 0) s = m+1; else
            return (void *)((uint8_t*)base + m*size);
    }
    return NULL;
}

int mblen(const char *s, size_t n)
{
    const char *e = s+n;
    int c = 0;
    if(s) {
        while(s < e && *s) {
            if((*s & 128) != 0) {
                if((*s & 32) == 0 ) s++; else
                if((*s & 16) == 0 ) s+=2; else
                if((*s & 8) == 0 ) s+=3;
            }
            c++;
            s++;
        }
    }
    return c;
}

int mbtowc (wchar_t * __pwc, const char *s, size_t n)
{
    wchar_t arg;
    const char *orig = s;
    arg = (wchar_t)*s;
    if((*s & 128) != 0) {
        if((*s & 32) == 0 && n > 0) { arg = ((*s & 0x1F)<<6)|(*(s+1) & 0x3F); } else
        if((*s & 16) == 0 && n > 1) { arg = ((*s & 0xF)<<12)|((*(s+1) & 0x3F)<<6)|(*(s+2) & 0x3F); } else
        if((*s & 8) == 0 && n > 2) { arg = ((*s & 0x7)<<18)|((*(s+1) & 0x3F)<<12)|((*(s+2) & 0x3F)<<6)|(*(s+3) & 0x3F); }
        else return -1;
    }
    if(__pwc) *__pwc = arg;
    return s - orig;
}

int wctomb (char *s, wchar_t u)
{
    int ret = 0;
    if(u<0x80) {
        *s = u;
        ret = 1;
    } else if(u<0x800) {
        *(s+0)=((u>>6)&0x1F)|0xC0;
        *(s+1)=(u&0x3F)|0x80;
        ret = 2;
    } else {
        *(s+0)=((u>>12)&0x0F)|0xE0;
        *(s+1)=((u>>6)&0x3F)|0x80;
        *(s+2)=(u&0x3F)|0x80;
        ret = 3;
    }
    return ret;
}

size_t mbstowcs (wchar_t *__pwcs, const char *__s, size_t __n)
{
    int r;
    wchar_t *orig = __pwcs;
    size_t ret = 0;
    if(!__s || !*__s) return 0;
    while(*__s) {
        r = mbtowc(__pwcs, __s, __n - ret);
        if(r < 0) return (size_t)-1;
        __pwcs++;
        __s += r;
    };
    return __pwcs - orig;
}

size_t wcstombs (char *__s, const wchar_t *__pwcs, size_t __n)
{
    int r;
    char *orig = __s;
    if(!__s || !__pwcs || !*__pwcs) return 0;
    while(*__pwcs && (__s - orig + 3 < __n)) {
        r = wctomb(__s, *__pwcs);
        if(r < 0) return (size_t)-1;
        __pwcs++;
        __s += r;
    };
    return __s - orig;
}

void srand(unsigned int __seed)
{
    __srand_seed = __seed - 1;
}

int rand()
{
    efi_guid_t rngGuid = EFI_RNG_PROTOCOL_GUID;
    efi_rng_protocol_t *rng = NULL;
    efi_status_t status;
    int ret = 0;

    __srand_seed = 6364136223846793005ULL*__srand_seed + 1;
    status = BS->LocateProtocol(&rngGuid, NULL, (void**)&rng);
    if(!EFI_ERROR(status) && rng)
        rng->GetRNG(rng, NULL, (uintn_t)sizeof(int), (uint8_t*)&ret);
    ret ^= (int)(__srand_seed>>33);
    return ret;
}

uint8_t *getenv(wchar_t *name, uintn_t *len)
{
    efi_guid_t globGuid = EFI_GLOBAL_VARIABLE;
    uint8_t tmp[EFI_MAXIMUM_VARIABLE_SIZE], *ret;
    uint32_t attr;
    efi_status_t status = RT->GetVariable(name, &globGuid, &attr, len, &tmp);
    if(EFI_ERROR(status) || *len < 1 || !(ret = malloc((*len) + 1))) {
        *len = 0;
        return NULL;
    }
    memcpy(ret, tmp, *len);
    ret[*len] = 0;
    return ret;
}

int setenv(wchar_t *name, uintn_t len, uint8_t *data)
{
    efi_guid_t globGuid = EFI_GLOBAL_VARIABLE;
    efi_status_t status = RT->SetVariable(name, &globGuid, 0, len, data);
    return !EFI_ERROR(status);
}
