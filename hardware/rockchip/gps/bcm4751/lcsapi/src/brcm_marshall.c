#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "brcm_marshall.h"


typedef struct {
   enum brcm_marshall_types type;
   uint32_t value;
   size_t len;
} marshall_arg_t;


#define CHECK_SIZE_OK(n,payload_sz) ((payload_sz+sizeof(marshall_arg_t))<=n)
#define CHECK_MIN_SIZE_OK(n) CHECK_SIZE_OK(n,0)
#define PAYLOAD_ADDR(data) (((uint8_t *)data)+sizeof(marshall_arg_t))
//#define ELEM_SIZE(data)    (((marshall_arg_t *)data)->len+sizeof(marshall_arg_t))

#define BRCM_MARSHAL_FUNC_UNDEF_ID (UINT32_MAX)

enum brcm_marshall_types brcm_marshall_get_type(uint8_t *rawdata, size_t n)
{
	marshall_arg_t marg_v;
    memcpy(&marg_v, rawdata, sizeof(marg_v));
	
    if (!CHECK_MIN_SIZE_OK(n))
        return BRCM_MARSHALL_ARG_UNDEF;
    return marg_v.type;
}

uint32_t brcm_marshall_get_value(uint8_t *rawdata, size_t n)
{
	marshall_arg_t marg_v;
    memcpy(&marg_v, rawdata, sizeof(marg_v));
	
    if (!CHECK_MIN_SIZE_OK(n))
        return 0;
    return marg_v.value;
}

ssize_t brcm_marshall_init(uint8_t *rawdata, size_t n, enum brcm_marshall_types type, uint32_t value, uint8_t *payload, size_t psz)
{
	marshall_arg_t marg_v;

    if (!CHECK_SIZE_OK(n,psz))
        return -1;
    if (psz && !payload)
        return -1;
    marg_v.type=type;
    marg_v.value=value;
    marg_v.len=psz;
	memcpy(rawdata, &marg_v, sizeof(marg_v));

    if (payload)
        memcpy(PAYLOAD_ADDR(rawdata),payload,psz);
    return (marg_v.len+sizeof(marshall_arg_t));//ELEM_SIZE(rawdata);
}

ssize_t brcm_marshall_get_payload_len(uint8_t *rawdata, size_t n)
{
    //marshall_arg_t *marg=(marshall_arg_t *)rawdata;
	marshall_arg_t marg_v;
    memcpy(&marg_v, rawdata, sizeof(marg_v));

    if (!CHECK_MIN_SIZE_OK(n))
        return -1;
    if (!CHECK_SIZE_OK(n,marg_v.len))
        return -1;
    return marg_v.len;
}

ssize_t brcm_marshall_get_len(uint8_t *rawdata, size_t n)
{
	marshall_arg_t marg_v;
    memcpy(&marg_v, rawdata, sizeof(marg_v));
	
    if (!CHECK_MIN_SIZE_OK(n))
        return -1;
    if (!CHECK_SIZE_OK(n,marg_v.len))
        return -1;
    return (marg_v.len+sizeof(marshall_arg_t));
}

ssize_t brcm_marshall_get_payload(uint8_t *rawdata, size_t n, uint8_t *payload, size_t psz)
{
    ssize_t p_len;
    if (!CHECK_MIN_SIZE_OK(n))
        return -1;
    if ((p_len=brcm_marshall_get_payload_len(rawdata,n))<0)
        return -1;
    if (psz>p_len)
        return -1;
    memcpy(payload,PAYLOAD_ADDR(rawdata),psz);
    return p_len;
}

ssize_t brcm_marshall_func_init(uint8_t *rawdata, size_t n, int func_id)
{
    return brcm_marshall_init(rawdata,n,BRCM_MARSHALL_ARG_FUNC,func_id,NULL,0);
}

ssize_t brcm_marshall_func_add_arg(uint8_t *rawdata, size_t n,
        enum brcm_marshall_types argt, uint32_t value, uint8_t *payload, size_t psz)
{
    ssize_t old_parent_payload_len;
    ssize_t len;
    uint8_t *enddata;
    uint8_t *parent_payload;
    uint8_t *end_parent_payload;
    uint32_t fid;
    if ((old_parent_payload_len=brcm_marshall_get_payload_len(rawdata,n))<0)
        return -1;
    fid=brcm_marshall_func_id(rawdata,n);
    enddata=rawdata+n;
    parent_payload=PAYLOAD_ADDR(rawdata);
    end_parent_payload=parent_payload+old_parent_payload_len;
    if (end_parent_payload >= enddata)
        return -1;
    if ((len=brcm_marshall_init(end_parent_payload,enddata-end_parent_payload,
                argt, value, payload,psz)) < 0) {
        return -1;
    }
    return brcm_marshall_init(rawdata,n,BRCM_MARSHALL_ARG_FUNC,fid,
            parent_payload,old_parent_payload_len+len);

}

uint32_t brcm_marshall_func_id(uint8_t *rawdata, size_t n)
{
    if (brcm_marshall_get_type(rawdata,n) != BRCM_MARSHALL_ARG_FUNC)
        return BRCM_MARSHAL_FUNC_UNDEF_ID;
    return brcm_marshall_get_value(rawdata,n);
}

uint8_t *brcm_marshall_func_next_arg(uint8_t *rawdata, size_t n, uint8_t *arg_index, size_t *arg_len)
{
    uint8_t *enddata;
    uint8_t *nextarg;
    ssize_t len;
    if (arg_index<rawdata)
        return NULL;
    if ((len=brcm_marshall_get_len(rawdata,n))<0)
        return NULL;
    enddata=rawdata+len;
    if (arg_index>enddata)
        return NULL;
    if ((len=brcm_marshall_get_len(arg_index,enddata-arg_index))<0)
        return NULL;
    nextarg=arg_index+len;
    if ((len=brcm_marshall_get_len(nextarg,enddata-nextarg))<0)
        return NULL;
    if (arg_len) {
        *arg_len = len;
    }
    return nextarg;
}

uint8_t *brcm_marshall_func_first_arg(uint8_t *rawdata, size_t n, size_t *arg_len)
{
    uint8_t *arg;
    uint8_t *enddata=rawdata+n;
    ssize_t len;
    if ((len=brcm_marshall_get_payload_len(rawdata,n))<=0)
        return NULL;
    arg = PAYLOAD_ADDR(rawdata);
    if (arg_len) {
        if ((len=brcm_marshall_get_len(arg,enddata-arg)) < 0)
            *arg_len = 0;
        else
            *arg_len = len;
    }
    return arg; 
}

ssize_t brcm_marshall_func_nargs(uint8_t *rawdata, size_t n)
{
    ssize_t nargs;
    uint8_t *arg;
    for (arg=brcm_marshall_func_first_arg(rawdata,n,NULL),nargs=0; arg!=NULL;
            arg=brcm_marshall_func_next_arg(rawdata,n,arg,NULL),nargs++);
    return nargs;

}

#if 0 
void print_hex(uint8_t *data, size_t sz)
{
    int row;
    char str[16+1];
    uint8_t *ptr = data;
    uint8_t *enddata  = data+sz;
    fprintf(stdout,"\n");
    while (ptr<enddata) {
        fprintf(stdout,"%8p: ",ptr);
        str[0]='\0';
        for (row=0; row<(sizeof(str)-1);row++,ptr++) {
            if (ptr<enddata) {
                fprintf(stdout,"%02X ",*ptr);
                if (*ptr >= 0x20  && *ptr <= '~')
                    str[row]=*ptr;
                else
                    str[row]='.';
            }
            else {
                fprintf(stdout,"   ");
                str[row]=' ';
            }
        }
        str[row]='\0';
        fprintf(stdout,"%s\n",str);
    }
}

#include <time.h>

int main(int argc, char **argv)
{
    uint8_t buf[1024];
    struct tm tms;
    time_t t;
    uint8_t *arg;
    size_t arg_len;
    t = time(NULL);
    memcpy(&tms,gmtime(&t),sizeof(tms)); 
    
    brcm_marshall_func_init(buf,sizeof(buf),1);
    fprintf(stdout,"Nargs: %d\n", brcm_marshall_func_nargs(buf,sizeof(buf)));
    print_hex(buf,brcm_marshall_get_len(buf,sizeof(buf)));
    brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UINT16,
            12,NULL,0);
    fprintf(stdout,"Nargs: %d\n", brcm_marshall_func_nargs(buf,sizeof(buf)));
    print_hex(buf,brcm_marshall_get_len(buf,sizeof(buf)));
    brcm_marshall_func_add_arg(buf,sizeof(buf),BRCM_MARSHALL_ARG_UNDEF,
            12,(uint8_t *)&tms,sizeof(tms));
    print_hex(buf,brcm_marshall_get_len(buf,sizeof(buf)));
    fprintf(stdout,"Nargs: %d\n", brcm_marshall_func_nargs(buf,sizeof(buf)));
    for (arg=brcm_marshall_func_first_arg(buf,sizeof(buf),&arg_len);
            arg != NULL;
            arg = brcm_marshall_func_next_arg(buf,sizeof(buf),arg,&arg_len)) {
        print_hex(arg,brcm_marshall_get_len(arg,sizeof(buf)-(arg-buf)));
    }
    return 0;
}

#endif
