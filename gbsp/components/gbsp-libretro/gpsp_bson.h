#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* gpSP's state subset of BSON. Validate the complete enclosing buffer before
 * passing a document pointer to lookup/read helpers. No live state is touched
 * during this structural pass. The nesting limit also bounds task stack use. */
typedef struct {
    uint8_t type;
    const uint8_t *key, *value, *next;
    size_t key_len, size;
} gp_bson_field_t;

static inline uint32_t gp_bson_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}

static inline bool gp_bson_field(const uint8_t *p, const uint8_t *end, gp_bson_field_t *f)
{
    if (p >= end || !*p) return false;
    f->type = *p++;
    const uint8_t *zero = (const uint8_t *)memchr(p, 0, (size_t)(end-p));
    if (!zero) return false;
    f->key = p; f->key_len = (size_t)(zero-p);
    p = zero+1; f->value = p;
    size_t left = (size_t)(end-p), n;
    switch (f->type) {
    case 0x10: n=4; break;
    case 0x03: case 0x04:
        if (left<4) return false;
        n=gp_bson_u32(p); if(n<5) return false; break;
    case 0x05:
        if (left<5 || gp_bson_u32(p)>left-5 || p[4]!=0) return false;
        n=(size_t)gp_bson_u32(p)+5; break;
    default: return false;
    }
    if (n>left) return false;
    f->size=n; f->next=p+n;
    return true;
}

static inline bool gp_bson_document(const uint8_t *doc, size_t size, unsigned depth)
{
    if (!doc || size<5 || depth>8 || gp_bson_u32(doc)!=size || doc[size-1]) return false;
    const uint8_t *p=doc+4, *end=doc+size-1;
    while(p<end) {
        gp_bson_field_t f;
        if (!gp_bson_field(p,end,&f)) return false;
        if ((f.type==3 || f.type==4) && !gp_bson_document(f.value,f.size,depth+1)) return false;
        p=f.next;
    }
    return p==end;
}
static inline bool gp_bson_validate(const void *data, size_t size)
{
    return gp_bson_document((const uint8_t *)data,size,0);
}
static inline bool gp_bson_find(const uint8_t *doc, const char *key, gp_bson_field_t *found)
{
    if(!doc) return false;
    size_t size=gp_bson_u32(doc), len=strlen(key);
    if(size<5 || size>416u*1024u) return false;
    const uint8_t *p=doc+4,*end=doc+size-1;
    while(p<end) {
        gp_bson_field_t f;
        if(!gp_bson_field(p,end,&f)) return false;
        if(f.key_len==len && !memcmp(f.key,key,len)){*found=f;return true;}
        p=f.next;
    }
    return false;
}
static inline bool gp_bson_i32(const uint8_t *doc,const char *key,void *out)
{
    gp_bson_field_t f;
    if(!gp_bson_find(doc,key,&f) || f.type!=0x10) return false;
    if(out) { uint32_t value=gp_bson_u32(f.value); memcpy(out,&value,4); }
    return true;
}
static inline bool gp_bson_bytes(const uint8_t *doc,const char *key,void *out,size_t count)
{
    gp_bson_field_t f;
    if(!gp_bson_find(doc,key,&f) || f.type!=5 || gp_bson_u32(f.value)!=count) return false;
    if(out) memcpy(out,f.value+5,count);
    return true;
}
static inline bool gp_bson_array(const uint8_t *doc,const char *key,void *out,size_t count)
{
    gp_bson_field_t f;
    if(!gp_bson_find(doc,key,&f) || f.type!=4) return false;
    const uint8_t *p=f.value+4,*end=f.value+f.size-1;
    size_t got=0;
    while(p<end){gp_bson_field_t e;if(!gp_bson_field(p,end,&e)||e.type!=0x10)return false;got++;p=e.next;}
    if(p!=end || got!=count) return false;
    p=f.value+4;
    for(size_t i=0;out && i<count;i++){gp_bson_field_t e;if(!gp_bson_field(p,end,&e))return false;uint32_t value=gp_bson_u32(e.value);memcpy((uint8_t *)out+i*4,&value,4);p=e.next;}
    return true;
}
