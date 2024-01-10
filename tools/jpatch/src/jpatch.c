#include "jpatch.h"
#ifdef _WIN32
#define LOGD printf
#define LOGE printf
#else
#include "log.h"
#endif


enum {
    JP_ESC = 0xa7,
    JP_MOD = 0xa6,
    JP_INS = 0xa5,
    JP_DEL = 0xa4,
    JP_EQL = 0xa3,
    JP_BKT = 0xa2
};


/**
 * @brief 从jp_data_t缓冲区读取数据
 * @param jd  jp_data_t指针
 * @param buf 指向目标区域的指针
 * @param len 要读取的数据长度
 * @return long 实际上读取到的数据单元的数量
 */
static long jp_read(jp_data_t *jd, void *buf, long len)
{
    long readlen = 0, remain = 0;

    if (!jd || !buf || !len) {
        return 0;
    }

    remain = jd->size - jd->pos;
    readlen = (len <= remain) ? len : remain;

    memcpy(buf, jd->buf+jd->pos, readlen);
    jd->pos += readlen;

    return readlen;
}

/**
 * @brief 从jp_data_t缓冲区写入数据
 * @param jd  jp_data_t指针
 * @param buf 指向目标区域的指针
 * @param len 要写入的数据长度
 * @return long 实际写入的数据长度
 */
static long jp_write(jp_data_t *jd, void *buf, long len)
{
    long writelen = 0, remain = 0;

    if (!jd || !buf || !len) {
        return 0;
    }

    remain = jd->size - jd->pos;
    writelen = (len <= remain) ? len : remain;

    memcpy(jd->buf+jd->pos, buf, writelen);
    jd->pos += writelen;

    return writelen;
}

/**
 * @brief 重新设置jp_data_t指针的偏移量
 * @param jd jp_data_t指针
 * @param offset 指定的偏移量
 * @param origin 开始添加偏移量 offset 的位置（SEEK_SET：流的开始；SEEK_CUR：流当前的位置；SEEK_END：流的结束）
 * @return int 成功则返回 0，否则返回 -1。
 */
static int jp_seek(jp_data_t *jd, long offset, int origin)
{
    int ret = -1;
    long require = 0;

    if (jd == NULL) {
        return ret;
    }

    switch (origin) {
        case SEEK_SET:
        {
            /* 开头 */
            if ((offset >= 0) && (offset <= jd->size)) {
                jd->pos = offset;
                ret = 0;
            }
            else {
                ret = -1;
            }
        }
        break;

        case SEEK_CUR:
        {
            /* 当前位置 */
            require = offset + jd->pos;
            if ((require >= 0) && (require <= jd->size)) {
                jd->pos = require;
                ret = 0;
            }
            else {
                ret = -1;
            }
        }
        break;

        case SEEK_END:
        {
            /* 结尾 */
            require = offset + (long)jd->size;
            if ((offset <= 0) && (require <= jd->size)) {
                jd->pos = require;
                ret = 0;
            }
            else {
                ret = -1;
            }
        }
        break;

        default:
        ret = -1;
        break;
    }

    return ret;
}

static int jp_copy(jp_data_t* dst, jp_data_t* src, int len)
{
    if (dst->pos+len>dst->size-1 || src->pos+len>src->size-1) {
        return -1;
    }

    memcpy(dst->buf+dst->pos, src->buf+src->pos, len);
    dst->pos += len;
    src->pos += len;

    return 0;
}


static int jp_getc(jp_data_t* jd)
{
    if (jd->pos < 0) 
        return -1;

    if (jd->pos >= jd->size) {
        return EOF;
    }

    int c = jd->buf[jd->pos++];
    return c;
}
static int jp_putc(jp_data_t* jd, int c)
{
    if (jd->pos < 0) {
        return -1;
    }


    if (jd->pos >= jd->size) {
        return EOF;
    }
    
    jd->buf[jd->pos++] = c;
    return 0;
}



static void process_mod(jp_data_t *source, jp_data_t*patch, jp_data_t*target, bool up_source_stream)
{
    // it can be that ESC character is actually in the data, but then it's prefixed with another ESC
    // so... we're looking for a lone ESC character
    size_t cnt = 0;
    while (1) {
        cnt++;
        int c = jp_getc(patch);
        if (c == -1) {
            // End of file stream... rewind 1 character and return, this will yield back to janpatch main function, which will exit
            jp_seek(source, -1, SEEK_CUR);
            return;
        }
        // LOGD("%02x ", m);
        // so... if it's *NOT* an ESC character, just write it to the target stream
        if (c != JP_ESC) {
            // LOGD("NOT ESC\n");
            jp_putc(target, c);
            if (up_source_stream) {
                jp_seek(source, 1, SEEK_CUR); // and up source
            }
            continue;
        }

        // read the next character to see what we should do
        c = jp_getc(patch);
        // LOGD("%02x ", m);

        if (c == -1) {
            // End of file stream... rewind 1 character and return, this will yield back to janpatch main function, which will exit
            jp_seek(source, -1, SEEK_CUR);
            return;
        }

        // if the character after this is *not* an operator (except ESC)
        if (c == JP_ESC) {
            // LOGD("ESC, NEXT CHAR ALSO ESC\n");
            jp_putc(target, c);
            if (up_source_stream) {
                jp_seek(source, 1, SEEK_CUR);
            }
        }
        else if (c >= 0xA2 && c <= 0xA6) { // character after this is an operator? Then roll back two characters and exit
            // LOGD("ESC, THEN OPERATOR\n");
            //LOGD("%lld bytes\n", cnt);
            jp_seek(patch, -2, SEEK_CUR);
            break;
        }
        else { // else... write both the ESC and m
            // LOGD("ESC, BUT NO OPERATOR\n");
            jp_putc(target, JP_ESC);
            jp_putc(target, c);
            if (up_source_stream) {
                jp_seek(source, 2, SEEK_CUR); // up source by 2
            }
        }
    }
}

static int get_length(jp_data_t *jd)
{
    int len;
    /* So... the EQL/BKT length thing works like this:
    *
    * If byte[0] is between 1..251 => use byte[0] + 1
    * If byte[0] is 252 => use ???
    * If byte[0] is 253 => use (byte[1] << 8) + byte[2]
    * If byte[0] is 254 => use (byte[1] << 16) + (byte[2] << 8) + byte[3] (NOT VERIFIED)
    */
    len = jp_getc(jd);
    if (len < 252) {
        len = len + 1;
    }
    else if (len == 252) {
        len = len + jp_getc(jd) + 1;
    }
    else if (len == 253) {
        len = jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
    }
    else if (len == 254) {
        len = jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
    }
    else {
        len = jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
        len = (len << 8) + jp_getc(jd);
    }
    return len;
}


int jpatch(jp_data_t *source, jp_data_t *patch, jp_data_t *target)
{
    int c,length;

    while ((c = jp_getc(patch)) != EOF) {

        if (c == JP_ESC) {
            c = jp_getc(patch);
        }
        else if(c != EOF) {
            jp_seek(patch, -1, SEEK_CUR);
            c = JP_MOD;
        }

        switch (c) {
            case JP_EQL: {
                length = get_length(patch);
                if (length == -1) {
                    LOGE("JP_EQL invalid length, source=%ld patch=%ld target=%ld\n", source->pos, patch->pos, target->pos);
                    return 1;
                }

                //LOGD("EQL: %d bytes\n", length);
                jp_copy(target, source, length); 
            }
            break;

            case JP_MOD: {
                // MOD means to modify the next series of bytes
                // so just write everything (until the next ESC sequence) to the target jp_data_t
                // but also up the position in the source jp_data_t every time
                process_mod(source, patch, target, true);
            }
            break;

            case JP_INS: {
                //LOGD("INS: ");
                // INS inserts the sequence in the new jp_data_t, but does not up the position of the source jp_data_t
                // so just write everything (until the next ESC sequence) to the target jp_data_t
                process_mod(source, patch, target, false);
            }
            break;

            case JP_BKT: {
                // BKT = backtrace, seek back in source jp_data_t with X bytes...
                length = get_length(patch);
                if (length == -1) {
                    LOGE("BKT length invalid, source=%ld patch=%ld target=%ld\n", source->pos, patch->pos, target->pos);
                    return 1;
                }

                //LOGD("BKT: %d bytes\n", length);
                jp_seek(source, -length, SEEK_CUR);
            }
            break;

            case JP_DEL: {
                // DEL deletes bytes, so up the source stream with X bytes
                length = get_length(patch);
                if (length == -1) {
                    LOGE("DEL length invalid, source=%ld patch=%ld target=%ld\n", source->pos, patch->pos, target->pos);
                    return 1;
                }

                //LOGD("DEL: %d bytes\n", length);
                jp_seek(source, length, SEEK_CUR);
            }
            break;

            case -1: {
                // End of file stream... rewind 1 character and break, this will yield back to main loop
                jp_seek(source, -1, SEEK_CUR);
            }
            break;

            default: {
                LOGD("Unsupported operator %02x\n", c);
                LOGD("Positions are, source=%ld patch=%ld target=%ld\n", source->pos, patch->pos, target->pos);
                return 1;
            }
        }
    }
    
    return 0;
}

//////////////////////////////////////////////////////
#ifdef _WIN32
#include <sys/stat.h>
jp_data_t src, pat, dst;
static FILE* data_init(jp_data_t* jd, char* path, int len)
{
    struct stat st;
    char* mod = (len==0) ? "rb" : "wb";
    FILE* fp = fopen(path, mod);

    stat(path, &st);
    if (!fp) {
        LOGE("___ %s open failed\n", path);
        return NULL;
    }
    if (len==0) {
        jd->size = st.st_size;
    }
    else {
        jd->size = len;
    }
        
    jd->buf = malloc(jd->size);
    jd->pos = 0;

    if (!jd->buf) {
        fclose(fp);
        LOGE("___ %s malloc %ld failed\n", path, jd->size);
        return NULL;
    }

    if (len == 0) {
        fread(jd->buf, 1, jd->size, fp);
    }

    return fp;
}
static int data_save(jp_data_t* jd, FILE* fp)
{
    printf("___ save data len: %ld\n", jd->pos);
    fwrite(jd->buf, 1, jd->pos, fp);

    return 0;
}


int main(int argc, char** argv)
{
    if (argc != 4) {
        LOGE("Usage: jpatch [old-file] [patch-file] ([new-file])\n");
        return -1;
    }
    
    FILE* fs = data_init(&src, argv[1], 0);
    FILE* fp = data_init(&pat, argv[2], 0);
    FILE* ft = data_init(&dst, argv[3], 1024*1024);

    jpatch(&src, &pat, &dst);

    data_save(&dst, ft);

    fclose(fs); fclose(fp); fclose(ft);

    return 0;
}
#endif

