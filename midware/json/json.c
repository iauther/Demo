#include <ctype.h>
#include "json.h"
#include "cjson.h"
#include "log.h"

#define RANGE_CHECK(x,s,e,fmt, ...)  if(((x)<(s)) || ((x)>(e))) LOGE(fmt, ##__VA_ARGS__); \
                                     else 

int json_init(void)
{
    cJSON_InitHooks(NULL);
    return 0;
}

int str_cut(char* src, const char* tok)
{
    char* p = strstr(src, tok);
    if (!p) {
        return - 1;
    }

    int tlen = strlen(src);
    int len = strlen(tok);
    memmove(p, p+len, (src+tlen)-(p+len)+1);

    return 0;
}
int get_hms(char* src, int *h, int *m, int *s)
{
    char t;
    int n = 0;
    char *p,*p1;
    
    p = strchr(src, 'h');
    if (p) {
        t = *p;
        *p = 0;
        p1 = p-1; while (p1 > src && isdigit(*p1)) p1--;
        *s = atoi(p1);
        *p = t;
        n++;
    }

    p = strchr(src, 'm');
    if (p) {
        t = *p;
        *p = 0;
        p1 = p-1; while (p1 > src && isdigit(*p1)) p1--;
        *m = atoi(p1);
        *p = t;
        n++;
    }

    p = strchr(src, 's');
    if (p) {
        t = *p;
        *p = 0;
        p1 = p-1; while (p1 > src && isdigit(*p1)) p1--;
        *s = atoi(p1);
        *p = t;
        n++;
    }

    return n;
}


int json_from(char *js, int js_len, usr_para_t *usr)
{
    int i,j,jlen;
    U8  chID;
    char *json;
    cJSON *root = NULL;
    cJSON *item,*array;
    ch_para_t *pch;

    if (!js || !js_len || !usr) {
        return -1; 
    }
    
    root = cJSON_CreateObject();
    if(!root) {
        return -1;
    }
    
    //add usr->net
    item = cJSON_CreateObject();
    if(item) {
        cJSON *tmp=NULL;
        
        cJSON_AddNumberToObject(item, "mode",          usr->net.mode);
        tmp = cJSON_CreateObject();
        if(tmp) {
            if(usr->net.mode==0) {  //tcp
                cJSON_AddStringToObject(tmp, "ip", usr->net.para.tcp.ip);
                cJSON_AddNumberToObject(item, "ch",usr->net.para.tcp.port);
                
                cJSON_AddItemToObject(item, "tcp", tmp);
            }
            else {                  //plat
                cJSON_AddStringToObject(tmp, "host",        usr->net.para.plat.host);
                cJSON_AddNumberToObject(tmp, "port",        usr->net.para.plat.port);
                cJSON_AddStringToObject(tmp, "prdKey",      usr->net.para.plat.prdKey);
                cJSON_AddStringToObject(tmp, "prdSecret",   usr->net.para.plat.prdSecret);
                cJSON_AddStringToObject(tmp, "devKey",      usr->net.para.plat.devKey);
                cJSON_AddStringToObject(tmp, "devSecret",   usr->net.para.plat.devSecret);
                
                cJSON_AddItemToObject(item, "plat", tmp);
            }
        }
        cJSON_AddItemToObject(root, "netPara", item);
    }
    
    //add usr->dac
    item = cJSON_CreateObject();
    if(item) {
        cJSON_AddNumberToObject(item, "enable",          usr->dac.enable);
        cJSON_AddNumberToObject(item, "fdiv",            usr->dac.fdiv);
        
        cJSON_AddItemToObject(root, "dacPara", item);
    }
    
    //add usr->smp
    item = cJSON_CreateObject();
    if(item) {
        int h, m, s;
        char tmp[100];
        cJSON_AddNumberToObject(item, "pwr_mode", usr->smp.pwr_mode);

        h = usr->smp.pwr_period / 3600;
        m = usr->smp.pwr_period / 60;
        s = usr->smp.pwr_period % 60;

        snprintf(tmp, sizeof(tmp), "%dh%dm%ds", h,m,s);
        str_cut(tmp, "0h"); str_cut(tmp, "0m"); str_cut(tmp, "0s");
        cJSON_AddStringToObject(item, "pwr_period", tmp);
        
        cJSON_AddItemToObject(root, "smpPara", item);
    }
    
    //add usr->dbg
    item = cJSON_CreateObject();
    if(item) {
        cJSON_AddNumberToObject(item, "enable",         usr->dbg.enable);
        cJSON_AddNumberToObject(item, "level",          usr->dbg.level);
        cJSON_AddNumberToObject(item, "to",             usr->dbg.to);
        
        cJSON_AddItemToObject(root, "dbgPara", item);
    }

    //add usr->mbus
    item = cJSON_CreateObject();
    if(item) {
        cJSON_AddNumberToObject(item, "addr",         usr->mbus.addr);
        
        cJSON_AddItemToObject(root, "mbPara", item);
    }
    
    //add usr->card
    item = cJSON_CreateObject();
    if(item) {
        cJSON_AddStringToObject(item, "apn",            usr->card.apn);
        cJSON_AddNumberToObject(item, "auth",           usr->card.auth);
        cJSON_AddNumberToObject(item, "type",           usr->card.type);
        
        cJSON_AddItemToObject(root, "cardPara", item);
    }
    
    pch = usr->ch;
    array = cJSON_CreateArray();
    if(array) {
        for(i=0; i<CH_MAX; i++) {
            item = cJSON_CreateObject();
            if(item) {
                cJSON_AddNumberToObject(item, "ch",                 i);
                cJSON_AddNumberToObject(item, "smpMode",            pch[i].smpMode);
                cJSON_AddNumberToObject(item, "smpFreq",            pch[i].smpFreq/1000);
                cJSON_AddNumberToObject(item, "smpPoints",          pch[i].smpPoints);
                cJSON_AddNumberToObject(item, "smpInterval",        pch[i].smpInterval);
                cJSON_AddNumberToObject(item, "smpTimes",           pch[i].smpTimes);
                cJSON_AddNumberToObject(item, "ampThreshold",       pch[i].ampThreshold);
                cJSON_AddNumberToObject(item, "messDuration",       pch[i].messDuration/1000);
                cJSON_AddNumberToObject(item, "trigDelay",          pch[i].trigDelay/1000);
                cJSON_AddNumberToObject(item, "n_ev",               pch[i].n_ev);
                cJSON_AddNumberToObject(item, "upway",              pch[i].upway);
                cJSON_AddNumberToObject(item, "upwav",              pch[i].upwav);
                cJSON_AddNumberToObject(item, "savwav",             pch[i].savwav);
                cJSON_AddNumberToObject(item, "evCalcCnt",          pch[i].evCalcCnt);
                
                cJSON *tmp = cJSON_CreateArray();
                if(tmp) {
                    for(j=0; j<pch[i].n_ev; j++) {
                        cJSON_AddItemToArray(tmp, cJSON_CreateNumber(pch[i].ev[j]));
                    }
                    cJSON_AddItemToObject(item, "ev", tmp);
                }
                
                tmp = cJSON_CreateObject();
                if(tmp) {
                    cJSON_AddNumberToObject(tmp, "a",               pch[i].coef.a);
                    cJSON_AddNumberToObject(tmp, "b",               pch[i].coef.b);
                    
                    cJSON_AddItemToObject(item, "coef", tmp);
                }
                
                cJSON_AddItemToArray(array, item);
            }  
        }
        cJSON_AddItemToObject(root, "chPara", array);
    }   
    
    json = cJSON_Print(root);
    if(!json) {
        cJSON_Delete(root);
        return -1;
    }
    
    jlen = strlen(json);
    if(js_len<jlen) {
        return -1;
    }
    
    strcpy(js, json);
    cJSON_Delete(root);
    
    return jlen;
}



int json_to(char *js, usr_para_t *usr)
{
    int r=0;
    int i,j,cnt,ch,sz;
    cJSON *root,*l1,*l2,*l3;
    cJSON* array;
    ch_para_t *pch;

    if (!js || !usr) {
        return -1;
    }

    root = cJSON_Parse(js);
    if(!root) {
        LOGE("json file is wrong, %s\n", cJSON_GetErrorPtr());
        return -1;
    }
    
    l1 = cJSON_GetObjectItem(root, "netPara");
    if (l1){
        
        l2 = cJSON_GetObjectItem(l1, "mode");
        if(l2) {
            RANGE_CHECK(l2->valueint, 0, 1, "net.mode is wrong,  range: 0,1\n");
            usr->net.mode = l2->valueint;
        }
        
        if(usr->net.mode==0) {
            l2 = cJSON_GetObjectItem(l1, "tcp");
            if (l2) {
                l3 = cJSON_GetObjectItem(l2, "ip");
                if (l3) {
                    sz = sizeof(usr->net.para.tcp.ip) - 1;
                    RANGE_CHECK(strlen(l3->valuestring), 1, sz, "tcp.ip is too long, range: 1~%d\n", sz)
                    strcpy(usr->net.para.tcp.ip, l3->valuestring);
                }

                l3 = cJSON_GetObjectItem(l2, "port");
                if (l3) {
                    RANGE_CHECK(l3->valueint, 1, 65535, "tcp.port is wrong,  range: 1~65535\n")
                    usr->net.para.tcp.port = l3->valueint;
                }
            }
        }
        else {
            l2 = cJSON_GetObjectItem(l1, "plat");
            if (l2) {
                l3 = cJSON_GetObjectItem(l2, "host");
                if (l3) {
                    sz = sizeof(usr->net.para.plat.host) - 1;
                    RANGE_CHECK(strlen(l3->valuestring), 1, sz, "plat.host is too long, range: 1~%d\n", sz)
                    strcpy(usr->net.para.plat.host, l3->valuestring);
                }

                l3 = cJSON_GetObjectItem(l2, "port");
                if (l3) {
                    RANGE_CHECK(l3->valueint, 1, 65535, "tcp.port is wrong,  range: 1~65535\n")
                    usr->net.para.plat.port = l3->valueint;
                }

                l3 = cJSON_GetObjectItem(l2, "prdKey");
                if (l3) {
                    sz = sizeof(usr->net.para.plat.prdKey) - 1;
                    RANGE_CHECK(strlen(l3->valuestring), 1, sz, "plat.prdKey is too long, range: 1~%d\n", sz)
                    strcpy(usr->net.para.plat.prdKey, l3->valuestring);
                }

                l3 = cJSON_GetObjectItem(l2, "prdSecret");
                if (l3){
                    sz = sizeof(usr->net.para.plat.prdSecret) - 1;
                    RANGE_CHECK(strlen(l3->valuestring), 1, sz, "plat.prdSecret is too long, range: 1~%d\n", sz)
                    strcpy(usr->net.para.plat.prdSecret, l3->valuestring);
                }

                l3 = cJSON_GetObjectItem(l2, "devKey");
                if (l3) {
                    sz = sizeof(usr->net.para.plat.devKey) - 1;
                    RANGE_CHECK(strlen(l3->valuestring), 1, sz, "plat.prdSecret is too long, range: 1~%d\n", sz)
                    strcpy(usr->net.para.plat.devKey, l3->valuestring);
                }

                l3 = cJSON_GetObjectItem(l2, "devSecret");
                if (l3) {
                    sz = sizeof(usr->net.para.plat.devSecret) - 1;
                    RANGE_CHECK(strlen(l3->valuestring), 1, sz, "plat.prdSecret is too long, range: 1~%d\n", sz)
                    strcpy(usr->net.para.plat.devSecret, l3->valuestring);
                }
            }
        }
    }
    
    l1 = cJSON_GetObjectItem(root, "mbPara");
    if (l1){
        l2 = cJSON_GetObjectItem(l1, "addr");
        if(l2) {
            RANGE_CHECK(l2->valueint, 1, 255, "mbus.addr is wrong, range: 1~255\n")
            usr->mbus.addr = l2->valueint;
        }
    }
    
    l1 = cJSON_GetObjectItem(root, "cardPara");
    if (l1){
        l2 = cJSON_GetObjectItem(l1, "type");
        if(l2) {
            //RANGE_CHECK(l2->valueint, 0, 2, "card.type is wrong, range: 0,1,2\n")
            usr->card.type = l2->valueint;
        }
        
        l2 = cJSON_GetObjectItem(l1, "auth");
        if(l2) {
            RANGE_CHECK(l2->valueint, 0, 2, "card.auth is wrong, range: 0,1,2\n")
            usr->card.auth = l2->valueint;
        }
        
        l2 = cJSON_GetObjectItem(l1, "apn");
        if(l2) {
            sz = sizeof(usr->card.apn) - 1;
            RANGE_CHECK(strlen(l2->valuestring), 1, sz, "card.apn is too long, range: 1~%d\n", sz)
            strcpy(usr->card.apn, l2->valuestring);
        }
    }
    
    l1 = cJSON_GetObjectItem(root, "smpPara");
    if (l1){
        l2 = cJSON_GetObjectItem(l1, "pwr_mode");
        if(l2) {
            RANGE_CHECK(l2->valueint, 0, 1, "card.auth is wrong, range: 0,1\n")
            usr->smp.pwr_mode = l2->valueint;
        }
        
        l2 = cJSON_GetObjectItem(l1, "pwr_period");
        if(l2) {
            
            int h,m,s,n;
            h = m = s = 0;
            n = get_hms(l2->valuestring, &h, &m, &s);
            if(n>0) {
                usr->smp.pwr_period = h*3600+m*60+s;
            }
        }
    }
    
    l1 = cJSON_GetObjectItem(root, "dbgPara");
    if (l1) {
        l2 = cJSON_GetObjectItem(l1, "to");
        if (l2) {
            //RANGE_CHECK(l2->valueint, 0, 1, "dbg.to is wrong, range: 0,1,2,3\n")
            usr->dbg.to = l2->valueint;
        }

        l2 = cJSON_GetObjectItem(l1, "enable");
        if (l2) {
            RANGE_CHECK(l2->valueint, 0, 1, "dbg.enable is wrong, range: 0,1\n")
            usr->dbg.enable = l2->valueint;
        }

        l2 = cJSON_GetObjectItem(l1, "level");
        if (l2) {
            RANGE_CHECK(l2->valueint, 0, LV_MAX-1, "dbg.level is wrong, range: 0~%d\n", LV_MAX-1)
            usr->dbg.level = l2->valueint;
        }
    }

    l1 = cJSON_GetObjectItem(root, "dacPara");
    if (l1){
        l2 = cJSON_GetObjectItem(l1, "enable");
        if(l2) {
            RANGE_CHECK(l2->valueint, 0, 1, "dac.enable is wrong, range: 0,1\n")
            usr->dac.enable = l2->valueint;
        }
        
        l2 = cJSON_GetObjectItem(l1, "fdiv");
        if(l2) {
            RANGE_CHECK(l2->valueint, 0, 0xffffffff, "dac.fdiv is wrong, range: 0,0xffffffff\n")
            usr->dac.fdiv = l2->valueint;
        }
    }
    
    l1 = cJSON_GetObjectItem(root, "chPara");
    if (l1 && cJSON_IsArray(l1) ){
        cnt = cJSON_GetArraySize(l1);
        if(cnt>CH_MAX) {
            cJSON_Delete(root);
            LOGE(" json array size too large!\n");
            return -1;
        }
        
        for(i=0; i<CH_MAX; i++) {
            usr->ch[i].ch = 0xff;
        }
        
        for(i=0; i<cnt; i++) {
            pch = NULL;
            
            array = cJSON_GetArrayItem(l1, i);
            if(!array) {
                r = -1; LOGE("json array %d not found!\n", i); continue;
            }
            
            l2 = cJSON_GetObjectItem(array, "ch");
            if(!l2) {
                r = -1; LOGE("ch not found!\n"); continue; 
            }
            
            ch = l2->valueint;
            if(ch>=CH_MAX) {
                r = -1; LOGE(" ch: %d is error\n", ch); continue; 
            }
            pch = &usr->ch[ch];
            pch->ch = ch;
            
            l2 = cJSON_GetObjectItem(array, "smpMode");
            if(l2) {
                RANGE_CHECK(l2->valueint, 0, 2, "ch[%d].smpMode is wrong, range: 0,1,2\n", ch)
                pch->smpMode = l2->valueint;
            }
            
            l2 = cJSON_GetObjectItem(array, "smpFreq");
            if(l2) {
                RANGE_CHECK(l2->valueint, 1, 250, "ch[%d].smpFreq is wrong, range: 1~250(kps)\n", ch)
                pch->smpFreq = l2->valueint*1000;
            }
            
            l2 = cJSON_GetObjectItem(array, "smpPoints");
            if(l2) {
                RANGE_CHECK(l2->valueint, 1000, 10000, "ch[%d].smpPoints is smpPoints, range: 1000~10000\n", ch)
                pch->smpPoints = l2->valueint;
            }
            
            l2 = cJSON_GetObjectItem(array, "smpInterval");
            if(l2) {
                RANGE_CHECK(l2->valueint, 0, 10000, "ch[%d].smpInterval is wrong, range: 0~10000(ms)\n", ch)
                pch->smpInterval = l2->valueint;
            }
            
            l2 = cJSON_GetObjectItem(array, "smpTimes");
            if(l2) {
                RANGE_CHECK(l2->valueint, 1, 100, "ch[%d].smpTimes is wrong, range: 1~100\n", ch)
                pch->smpTimes = l2->valueint;
            }
            
            l2 = cJSON_GetObjectItem(array, "ampThreshold");
            if(l2) {
                RANGE_CHECK(l2->valueint, 1, 10, "ch[%d].ampThreshold is wrong, range: 1~10(mv)\n", ch)
                pch->ampThreshold = l2->valuedouble;
            }

            l2 = cJSON_GetObjectItem(array, "messDuration");
            if (l2) {
                RANGE_CHECK(l2->valueint, 1, 10, "ch[%d].messDuration is wrong, range: 1~10(s)\n", ch)
                    pch->messDuration = l2->valueint*1000;
            }
            
            l2 = cJSON_GetObjectItem(array, "trigDelay");
            if(l2) {
                RANGE_CHECK(l2->valueint, 10, 60, "ch[%d].trigDelay is wrong, range: 10~60(s)\n", ch)
                pch->trigDelay = l2->valueint*1000;
            }
            
            l2 = cJSON_GetObjectItem(array, "evCalcCnt");
            if(l2) {
                RANGE_CHECK(l2->valueint, 1000, 10000, "ch[%d].evCalcCnt is wrong, range: 1000~10000\n", ch)
                pch->evCalcCnt = l2->valueint;
            } 
            
            l2 = cJSON_GetObjectItem(array, "upway");
            if(l2) {
                RANGE_CHECK(l2->valueint, 0, 1, "ch[%d].upway is wrong, range: 0,1\n", ch)
                pch->upway = l2->valueint;
            }
            
            l2 = cJSON_GetObjectItem(array, "upwav");
            if(l2) {
                RANGE_CHECK(l2->valueint, 0, 1, "ch[%d].upwav is wrong, range: 0,1\n", ch)
                pch->upwav = l2->valueint;
            }

            l2 = cJSON_GetObjectItem(array, "savwav");
            if (l2) {
                RANGE_CHECK(l2->valueint, 0, 1, "ch[%d].savwav is wrong, range: 0,1\n", ch)
                    pch->savwav = l2->valueint;
            }
            
            l2 = cJSON_GetObjectItem(array, "ev");
            if (l2 && cJSON_IsArray(l2)){
                memset(pch->ev, 0xff, EV_NUM);
                
                int n_ev = cJSON_GetArraySize(l2);
                RANGE_CHECK(n_ev, 1, EV_NUM, "ch[%d].ev array size %d is wrong, range: 1~%d\n", ch, n_ev, EV_NUM)
                {
                    pch->n_ev = n_ev;

                    for (j = 0; j < pch->n_ev; j++) {
                        cJSON* it = cJSON_GetArrayItem(l2, j);
                        if (it && cJSON_IsNumber(it)) {
                            RANGE_CHECK(it->valueint, 0, EV_NUM-1, "ch[%d].ev[%d] value %d is wrong, range: 1~%d\n", ch, j, it->valueint, EV_NUM-1)
                            pch->ev[j] = it->valueint;
                        }
                    }
                }
            }

            l2 = cJSON_GetObjectItem(array, "coef");
            if (l2) {
                l3 = cJSON_GetObjectItem(l2, "a");
                if (l3) {
                    pch->coef.a = l3->valuedouble;
                }

                l3 = cJSON_GetObjectItem(l2, "b");
                if (l3) {
                    pch->coef.b = l3->valuedouble;
                }
            }
        }
    }
    
    cJSON_Delete(root);
    
    return r;
}









