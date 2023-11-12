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
    char *json;
    cJSON *root = NULL;
    cJSON *l1,*l2,*l3,*a1,*a2;
    ch_para_t *pch;
    ch_paras_t *pchs;

    if (!js || !js_len || !usr) {
        return -1; 
    }
    
    root = cJSON_CreateObject();
    if(!root) {
        return -1;
    }
    
    //add usr->net
    l1 = cJSON_CreateObject();
    if(l1) {
        cJSON_AddNumberToObject(l1, "mode",          usr->net.mode);
        l2 = cJSON_CreateObject();
        if(l2) {
            if(usr->net.mode==0) {  //tcp
                cJSON_AddStringToObject(l2,  "ip", usr->net.para.tcp.ip);
                cJSON_AddNumberToObject(l1, "ch", usr->net.para.tcp.port);
                
                cJSON_AddItemToObject(l1, "tcp", l2);
            }
            else {                  //plat
                cJSON_AddStringToObject(l2, "host",        usr->net.para.plat.host);
                cJSON_AddNumberToObject(l2, "port",        usr->net.para.plat.port);
                cJSON_AddStringToObject(l2, "prdKey",      usr->net.para.plat.prdKey);
                cJSON_AddStringToObject(l2, "prdSecret",   usr->net.para.plat.prdSecret);
                cJSON_AddStringToObject(l2, "devKey",      usr->net.para.plat.devKey);
                cJSON_AddStringToObject(l2, "devSecret",   usr->net.para.plat.devSecret);
                
                cJSON_AddItemToObject(l1, "plat", l2);
            }
        }
        cJSON_AddItemToObject(root, "netPara", l1);
    }
    
    //add usr->dac
    l1 = cJSON_CreateObject();
    if(l1) {
        cJSON_AddNumberToObject(l1, "enable",          usr->dac.enable);
        cJSON_AddNumberToObject(l1, "fdiv",            usr->dac.fdiv);
        
        cJSON_AddItemToObject(root, "dacPara", l1);
    }
    
    //add usr->dbg
    l1 = cJSON_CreateObject();
    if(l1) {
        cJSON_AddNumberToObject(l1, "enable",         usr->dbg.enable);
        cJSON_AddNumberToObject(l1, "level",          usr->dbg.level);
        cJSON_AddNumberToObject(l1, "to",             usr->dbg.to);
        
        cJSON_AddItemToObject(root, "dbgPara", l1);
    }

    //add usr->mbus
    l1 = cJSON_CreateObject();
    if(l1) {
        cJSON_AddNumberToObject(l1, "addr",         usr->mbus.addr);
        
        cJSON_AddItemToObject(root, "mbPara", l1);
    }
    
    //add usr->card
    l1 = cJSON_CreateObject();
    if(l1) {
        cJSON_AddStringToObject(l1, "apn",            usr->card.apn);
        cJSON_AddNumberToObject(l1, "auth",           usr->card.auth);
        cJSON_AddNumberToObject(l1, "type",           usr->card.type);
        
        cJSON_AddItemToObject(root, "cardPara", l1);
    }
    
    //add usr->smp
    l1 = cJSON_CreateObject();
    if(l1) {
        int h, m, s;
        char tmp[100],ht[20],mt[20],st[20];
        U8  mode=usr->smp.mode;
        
        cJSON_AddNumberToObject(l1, "mode",    usr->smp.mode);
        cJSON_AddNumberToObject(l1, "port",    usr->smp.port);
        cJSON_AddNumberToObject(l1, "pwrmode", usr->smp.pwrmode);

        h = usr->smp.worktime / 3600;
        m = usr->smp.worktime / 60;
        s = usr->smp.worktime % 60;

        ht[0] = mt[0] = st[0] = 0;
        snprintf(tmp, sizeof(tmp), "%dh%dm%ds", h,m,s);
        if (h>0) sprintf(ht, "%dh", h);
        if (m>0) sprintf(mt, "%dm", m);
        if (s>0) sprintf(st, "%ds", s);
        sprintf(tmp, "%s%s%s", ht,mt,st);

        cJSON_AddStringToObject(l1, "worktime", tmp);
        
        
        a1 = cJSON_CreateArray();
        if(a1) {
            for(i=0; i<CH_MAX; i++) {
                pchs = &usr->smp.ch[i];
                pch  = &pchs->para[mode];
                
                l2 = cJSON_CreateObject();
                if(l2) {

                    cJSON_AddNumberToObject(l2, "ch",                 i);
                    cJSON_AddNumberToObject(l2, "enable",             pchs->enable);
                    
                    a2 = cJSON_CreateArray();
                    if(a2) {
                        l3 = cJSON_CreateObject();
                        if(l3) {
                            cJSON_AddNumberToObject(l3, "smpMode",            pch->smpMode);
                            cJSON_AddNumberToObject(l3, "smpFreq",            pch->smpFreq/1000);
                            cJSON_AddNumberToObject(l3, "smpPoints",          pch->smpPoints);
                            cJSON_AddNumberToObject(l3, "smpInterval",        pch->smpInterval);
                            cJSON_AddNumberToObject(l3, "smpTimes",           pch->smpTimes);
                            cJSON_AddNumberToObject(l3, "ampThreshold",       pch->ampThreshold);
                            cJSON_AddNumberToObject(l3, "messDuration",       pch->messDuration/1000);
                            cJSON_AddNumberToObject(l3, "trigDelay",          pch->trigDelay/1000);
                            cJSON_AddNumberToObject(l3, "n_ev",               pch->n_ev);
                            cJSON_AddNumberToObject(l3, "upway",              pch->upway);
                            cJSON_AddNumberToObject(l3, "upwav",              pch->upwav);
                            cJSON_AddNumberToObject(l3, "savwav",             pch->savwav);
                            cJSON_AddNumberToObject(l3, "evCalcCnt",          pch->evCalcCnt);
                            
                            cJSON *a3 = cJSON_CreateArray();
                            if(a3) {
                                for(j=0; j<pch->n_ev; j++) {
                                    cJSON_AddItemToArray(a3, cJSON_CreateNumber(pch->ev[j]));
                                }
                                cJSON_AddItemToObject(l3, "ev", a3);
                            }
                            
                            cJSON_AddItemToArray(a2, l3);
                        }
                        cJSON_AddItemToObject(l2, "coef", a2);
                    }
                    
                    l3 = cJSON_CreateObject();
                    if(l3) {
                        cJSON_AddNumberToObject(l3, "a",               pchs->coef.a);
                        cJSON_AddNumberToObject(l3, "b",               pchs->coef.b);
                        
                        cJSON_AddItemToObject(l2, "coef", l3);
                    }
                    
                    cJSON_AddItemToArray(a1, l2);
                }  
            }
            cJSON_AddItemToObject(root, "chPara", a1);
        }
        
        cJSON_AddItemToObject(root, "smpPara", l1);
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
    int i,j,k,cnt,ch,sz;
    cJSON *root,*l1,*l2,*l3,*l4,*l5;
    cJSON *a1,*a2;
    ch_paras_t *pchs;
    ch_para_t  *pch;

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
    
    l1 = cJSON_GetObjectItem(root, "smpPara");
    if (l1){
        
        l2 = cJSON_GetObjectItem(l1, "mode");
        if(l2) {
            RANGE_CHECK(l2->valueint, 0, 2, "smp.mode is wrong, range: 0,1,2\n")
            usr->smp.mode = l2->valueint;
        }
        
        l2 = cJSON_GetObjectItem(l1, "port");
        if(l2) {
            RANGE_CHECK(l2->valueint, 0, PORT_MAX-1, "smp.port is wrong, range: 0~%d\n", PORT_MAX-1)
            usr->smp.port = l2->valueint;
        }        
        
        l2 = cJSON_GetObjectItem(l1, "pwrmode");
        if(l2) {
            RANGE_CHECK(l2->valueint, 0, 1, "smp.pwrmode is wrong, range: 0,1\n")
            usr->smp.pwrmode = l2->valueint;
        }
        
        l2 = cJSON_GetObjectItem(l1, "worktime");
        if(l2) {
            
            int h,m,s,n;
            h = m = s = 0;
            n = get_hms(l2->valuestring, &h, &m, &s);
            if(n>0) {
                usr->smp.worktime = h*3600+m*60+s;
            }
        }
        
        
        l2 = cJSON_GetObjectItem(root, "chParas");
        if (l2 && cJSON_IsArray(l1) ){
            cnt = cJSON_GetArraySize(l2);
            if(cnt>CH_MAX) {
                cJSON_Delete(root);
                LOGE(" json array size too large!\n");
                return -1;
            }
            
            for(i=0; i<CH_MAX; i++) {
                usr->smp.ch[i].ch = 0xff;
            }
            
            for(i=0; i<cnt; i++) {
                pch = NULL;
                
                a1 = cJSON_GetArrayItem(l2, i);
                if(!a1) {
                    r = -1; LOGE("json array %d not found!\n", i); continue;
                }
                
                l3 = cJSON_GetObjectItem(a1, "ch");
                if(!l3) {
                    r = -1; LOGE("ch not found!\n"); continue; 
                }
                
                ch = l3->valueint;
                if(ch>=CH_MAX) {
                    r = -1; LOGE(" ch: %d is error\n", ch); continue; 
                }
                pchs = &usr->smp.ch[ch];
                pchs->ch = ch;
                
                l3 = cJSON_GetObjectItem(a1, "enable");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 0, 1, "ch[%d].enable is wrong, value: 0,1\n", ch)
                    pchs->enable = l3->valueint;
                }
                
                //ch_para_t
                l3 = cJSON_GetObjectItem(a1, "chPara");
                if(l3) {
                    U8 mode=usr->smp.mode;
                    pch = &pchs->para[mode];
                    
                    a2 = cJSON_GetArrayItem(l2, i);
                    if(!a2) {
                        r = -1; LOGE("ch_para_t array %d not found!\n", i); continue;
                    }
                    
                    l4 = cJSON_GetObjectItem(a1, "para");
                    if(!l4) {
                        r = -1; LOGE("para not found!\n"); continue; 
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "smpMode");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 0, 2, "ch[%d].smpMode is wrong, value: 0,1,2\n", ch)
                        pch->smpMode = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "smpFreq");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 1, 250, "ch[%d].smpFreq is wrong, range: 1~250(kps)\n", ch)
                        pch->smpFreq = l4->valueint*1000;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "smpPoints");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 1000, 10000, "ch[%d].smpPoints is smpPoints, range: 1000~10000\n", ch)
                        pch->smpPoints = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "smpInterval");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 0, 10000, "ch[%d].smpInterval is wrong, range: 0~10000000(us)\n", ch)
                        pch->smpInterval = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "smpTimes");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 1, 100, "ch[%d].smpTimes is wrong, range: 1~100\n", ch)
                        pch->smpTimes = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "ampThreshold");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 1, 10, "ch[%d].ampThreshold is wrong, range: 1~10(mv)\n", ch)
                        pch->ampThreshold = l3->valuedouble;
                    }

                    l3 = cJSON_GetObjectItem(a2, "messDuration");
                    if (l4) {
                        RANGE_CHECK(l4->valueint, 1, 10, "ch[%d].messDuration is wrong, range: 1~10(s)\n", ch)
                        pch->messDuration = l4->valueint*1000;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "trigDelay");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 10, 60, "ch[%d].trigDelay is wrong, range: 10~60(s)\n", ch)
                        pch->trigDelay = l4->valueint*1000;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "evCalcCnt");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 1000, 10000, "ch[%d].evCalcCnt is wrong, range: 1000~10000\n", ch)
                        pch->evCalcCnt = l4->valueint;
                    } 
                    
                    l4 = cJSON_GetObjectItem(a2, "upway");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 0, 1, "ch[%d].upway is wrong, value: 0,1\n", ch)
                        pch->upway = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "upwav");
                    if(l4) {
                        RANGE_CHECK(l4->valueint, 0, 1, "ch[%d].upwav is wrong, value: 0,1\n", ch)
                        pch->upwav = l4->valueint;
                    }

                    l4 = cJSON_GetObjectItem(a2, "savwav");
                    if (l4) {
                        RANGE_CHECK(l4->valueint, 0, 1, "ch[%d].savwav is wrong, value: 0,1\n", ch)
                            pch->savwav = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(a2, "ev");
                    if (l4 && cJSON_IsArray(l4)){
                        memset(pch->ev, 0xff, EV_NUM);
                        
                        int n_ev = cJSON_GetArraySize(l4);
                        RANGE_CHECK(n_ev, 1, EV_NUM, "ch[%d].ev array size %d is wrong, range: 1~%d\n", ch, n_ev, EV_NUM)
                        {
                            pch->n_ev = n_ev;

                            for (j = 0; j < pch->n_ev; j++) {
                                cJSON* it = cJSON_GetArrayItem(l4, j);
                                if (it && cJSON_IsNumber(it)) {
                                    RANGE_CHECK(it->valueint, 0, EV_NUM-1, "ch[%d].ev[%d] value %d is wrong, range: 1~%d\n", ch, j, it->valueint, EV_NUM-1)
                                    pch->ev[j] = it->valueint;
                                }
                            }
                        }
                    }
                    
                } 

                l3 = cJSON_GetObjectItem(a1, "coef");
                if (l3) {
                    l4 = cJSON_GetObjectItem(l3, "a");
                    if (l4) {
                        pchs->coef.a = l4->valuedouble;
                    }

                    l4 = cJSON_GetObjectItem(l3, "b");
                    if (l4) {
                        pchs->coef.b = l4->valuedouble;
                    }
                }
            }
        }
    }
    
    cJSON_Delete(root);
    
    return r;
}









