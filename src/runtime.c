#include <stddef.h>
void *memmove(void *p,const void *s,size_t n){unsigned char *d=p;const unsigned char *b=s;if(d<b)while(n--)*d++=*b++;else{d+=n;b+=n;while(n--)*--d=*--b;}return p;}
size_t strlen(const char *s){size_t n=0;while(s[n])++n;return n;}
int strcmp(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return (unsigned char)*a-(unsigned char)*b;}
int strncmp(const char *a,const char *b,size_t n){while(n&&*a&&*a==*b){++a;++b;--n;}return n?(unsigned char)*a-(unsigned char)*b:0;}
char *strcpy(char *d,const char *s){char *p=d;while((*p++=*s++));return d;}
int memcmp(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;++x;++y;}return 0;}
