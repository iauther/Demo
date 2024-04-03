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
    cJSON *l1,*l2,*l3,*l4,*a1,*a2;
    ch_para_t *pch;

    if (!js || !js_len || !usr) {
        return -1; 
    }
    
    root = cJSON_CreateObject();
    if(!root) {
        return -1;
    }
    
    //add usr->takeff
    cJSON_AddNumberToObject(root, "takeff",  usr->takeff);
    
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
    if (l1) {
        int h, m, s;
        char tmp[100], ht[20], mt[20], st[20];
        U8  mode = usr->smp.mode;

        cJSON_AddNumberToObject(l1, "mode", usr->smp.mode);
        cJSON_AddNumberToObject(l1, "port", usr->smp.port);
        cJSON_AddNumberToObject(l1, "pwrmode", usr->smp.pwrmode);

        h = usr->smp.invTime / 3600;
        m = usr->smp.invTime / 60;
        s = usr->smp.invTime % 60;

        ht[0] = mt[0] = st[0] = 0;
        snprintf(tmp, sizeof(tmp), "%dh%dm%ds", h, m, s);
        if (h > 0) sprintf(ht, "%dh", h);
        if (m > 0) sprintf(mt, "%dm", m);
        if (s > 0) sprintf(st, "%ds", s);
        sprintf(tmp, "%s%s%s", ht, mt, st);

        cJSON_AddStringToObject(l1, "invTime", tmp);

        a1 = cJSON_CreateArray();
        if (a1) {
            for (i = 0; i < CH_MAX; i++) {
                pch = &usr->smp.ch[i];
                l2 = cJSON_CreateObject();  //ch
                if (l2) {
                    cJSON_AddNumberToObject(l2, "ch", i);
                    cJSON_AddNumberToObject(l2, "enable", pch->enable);

                    cJSON_AddNumberToObject(l2, "smpMode",      pch->smpMode);
                    cJSON_AddNumberToObject(l2, "smpFreq",      pch->smpFreq / 1000);
                    cJSON_AddNumberToObject(l2, "smpPoints",    pch->smpPoints);
                    cJSON_AddNumberToObject(l2, "smpInterval",  pch->smpInterval);
                    cJSON_AddNumberToObject(l2, "smpTimes",     pch->smpTimes);
                    cJSON_AddNumberToObject(l2, "trigEvType",   pch->trigEvType);
                    cJSON_AddNumberToObject(l2, "trigThreshold",  pch->trigThreshold);
                    
                    l3 = cJSON_CreateObject();
                    if(l3) {
                        cJSON_AddNumberToObject(l3, "preTime",   pch->trigTime.preTime);
                        cJSON_AddNumberToObject(l3, "postTime",  pch->trigTime.postTime);
                        cJSON_AddNumberToObject(l3, "PDT",       pch->trigTime.HDT);
                        cJSON_AddNumberToObject(l3, "HDT",       pch->trigTime.HDT);
                        cJSON_AddNumberToObject(l3, "HLT",       pch->trigTime.HLT);
                        cJSON_AddNumberToObject(l3, "MDT",       pch->trigTime.MDT);
                         cJSON_AddItemToObject(l2, "trigTime", l3);
                    }
                    

                    cJSON_AddNumberToObject(l2, "n_ev", pch->n_ev);
                    cJSON* a2 = cJSON_CreateArray();
                    if (a2) {
                        for (j = 0; j < pch->n_ev; j++) {
                            cJSON_AddItemToArray(a2, cJSON_CreateNumber(pch->ev[j]));
                        }
                        cJSON_AddItemToObject(l2, "ev", a2);
                    }

                    cJSON_AddNumberToObject(l2, "upway", pch->upway);
                    cJSON_AddNumberToObject(l2, "upwav", pch->upwav);
                    cJSON_AddNumberToObject(l2, "savwav", pch->savwav);
                    cJSON_AddNumberToObject(l2, "evCalcPoints", pch->evCalcPoints);

                    l3 = cJSON_CreateObject();
                    if (l3) {
                        cJSON_AddNumberToObject(l3, "a", pch->coef.a);
                        cJSON_AddNumberToObject(l3, "b", pch->coef.b);
                        cJSON_AddItemToObject(l2, "coef", l3);
                    }

                    cJSON_AddItemToArray(a1, l2);
                }
            }
            cJSON_AddItemToObject(l1, "chParas", a1);
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
    ch_para_t  *pch;

    if (!js || !usr) {
        return -1;
    }

    root = cJSON_Parse(js);
    if(!root) {
        LOGE("json file is wrong, %s\n", cJSON_GetErrorPtr());
        return -1;
    }
    
    l1 = cJSON_GetObjectItem(root, "takeff");
    if (l1) {
        RANGE_CHECK(l1->valueint, 0, 1, "takeff is wrong, range: 0,1\n")
        usr->takeff = l1->valueint;
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
        
        l2 = cJSON_GetObjectItem(l1, "invTime");
        if(l2) {
            
            int h,m,s,n;
            h = m = s = 0;
            n = get_hms(l2->valuestring, &h, &m, &s);
            if(n>0) {
                usr->smp.invTime = h*3600+m*60+s;
            }
        }
        
        l2 = cJSON_GetObjectItem(l1, "chParas");
        if (l2 && cJSON_IsArray(l2) ){
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
                pch = &usr->smp.ch[ch];
                pch->ch = ch;
                
                l3 = cJSON_GetObjectItem(a1, "enable");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 0, 1, "ch[%d].enable is wrong, value: 0,1\n", ch)
                    pch->enable = l3->valueint;
                }
                
                //ch_para_t
                U8 mode=usr->smp.mode;
                
                l3 = cJSON_GetObjectItem(a1, "smpMode");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 0, 2, "ch[%d].smpMode is wrong, value: 0,1,2\n", ch)
                    pch->smpMode = l3->valueint;
                }
                
                l3 = cJSON_GetObjectItem(a1, "smpFreq");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 1, 2500, "ch[%d].smpFreq is wrong, range: 1~2500(kps)\n", ch)
                    pch->smpFreq = l3->valueint*1000;
                }
                
                l3 = cJSON_GetObjectItem(a1, "smpPoints");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 1000, 10000, "ch[%d].smpPoints is smpPoints, range: 1000~10000\n", ch)
                    pch->smpPoints = l3->valueint;
                }
                
                l3 = cJSON_GetObjectItem(a1, "smpInterval");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 0, 10000000, "ch[%d].smpInterval is wrong, range: 0~10000000(us)\n", ch)
                    pch->smpInterval = l3->valueint;
                }
                
                l3 = cJSON_GetObjectItem(a1, "smpTimes");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 1, 100, "ch[%d].smpTimes is wrong, range: 1~100\n", ch)
                    pch->smpTimes = l3->valueint;
                }
                
                l3 = cJSON_GetObjectItem(a1, "trigEvType");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 1, 1, "ch[%d].trigEvType is wrong, range: 1(amp)\n", ch)
                    pch->trigEvType = l3->valueint;
                }

                l3 = cJSON_GetObjectItem(a1, "trigThreshold");
                if (l3) {
                    //RANGE_CHECK(l3->valuedouble, 0, 6, "ch[%d].ampThreshold is wrong, range: 0~6(mv)\n", ch)
                    pch->trigThreshold = l3->valuedouble;
                }

                //////////////////
                l3 = cJSON_GetObjectItem(a1, "trigTime");
                if(l3) {
                    
                    l4 = cJSON_GetObjectItem(l3, "preTime");
                    if(l4) {
                        //RANGE_CHECK(l4->valueint, 1, 10, "ch[%d].ampThreshold is wrong, range: 1~10(mv)\n", ch)
                        pch->trigTime.preTime = l4->valueint;
                    }

                    
                    l4 = cJSON_GetObjectItem(l3, "postTime");
                    if(l4) {
                        //RANGE_CHECK(l4->valueint, 1, 10, "ch[%d].ampThreshold is wrong, range: 1~10(mv)\n", ch)
                        pch->trigTime.postTime = l4->valueint;
                    }

                    l4 = cJSON_GetObjectItem(l3, "PDT");
                    if(l4) {
                        //RANGE_CHECK(l4->valueint, 10, 60, "ch[%d].PDT is wrong, range: 10~60(s)\n", ch)
                        pch->trigTime.PDT = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(l3, "HDT");
                    if(l4) {
                        //RANGE_CHECK(l4->valueint, 10, 60, "ch[%d].HDT is wrong, range: 10~60(s)\n", ch)
                        pch->trigTime.HDT = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(l3, "HLT");
                    if (l4) {
                        //RANGE_CHECK(l4->valueint, 1, 10, "ch[%d].HLT is wrong, range: 1~10(s)\n", ch)
                        pch->trigTime.HLT = l4->valueint;
                    }
                    
                    l4 = cJSON_GetObjectItem(l3, "MDT");
                    if (l4) {
                        //RANGE_CHECK(l4->valueint, 1, 10, "ch[%d].MDT is wrong, range: 1~10(s)\n", ch)
                        pch->trigTime.MDT = l4->valueint;
                    }
                }
                
                l3 = cJSON_GetObjectItem(a1, "evCalcPoints");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 1000, 10000, "ch[%d].evCalcPoints is wrong, range: 1000~10000\n", ch)
                    pch->evCalcPoints = l3->valueint;
                } 
                
                l3 = cJSON_GetObjectItem(a1, "upway");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 0, 1, "ch[%d].upway is wrong, value: 0,1\n", ch)
                    pch->upway = l3->valueint;
                }
                
                l3 = cJSON_GetObjectItem(a1, "upwav");
                if(l3) {
                    RANGE_CHECK(l3->valueint, 0, 1, "ch[%d].upwav is wrong, value: 0,1\n", ch)
                    pch->upwav = l3->valueint;
                }

                l3 = cJSON_GetObjectItem(a1, "savwav");
                if (l3) {
                    RANGE_CHECK(l3->valueint, 0, 1, "ch[%d].savwav is wrong, value: 0,1\n", ch)
                        pch->savwav = l3->valueint;
                }
                
                l3 = cJSON_GetObjectItem(a1, "ev");
                if (l3 && cJSON_IsArray(l3)){
                    memset(pch->ev, 0xff, EV_NUM);
                    
                    int n_ev = cJSON_GetArraySize(l3);
                    RANGE_CHECK(n_ev, 1, EV_NUM, "ch[%d].ev array size %d is wrong, range: 1~%d\n", ch, n_ev, EV_NUM)
                    {
                        pch->n_ev = n_ev;

                        for (j = 0; j < pch->n_ev; j++) {
                            cJSON* it = cJSON_GetArrayItem(l3, j);
                            if (it && cJSON_IsNumber(it)) {
                                RANGE_CHECK(it->valueint, 0, EV_NUM-1, "ch[%d].ev[%d] value %d is wrong, range: 1~%d\n", ch, j, it->valueint, EV_NUM-1)
                                pch->ev[j] = it->valueint;
                            }
                        }
                    }
                }

                l3 = cJSON_GetObjectItem(a1, "coef");
                if (l3) {
                    l4 = cJSON_GetObjectItem(l3, "a");
                    if (l4) {
                        pch->coef.a = l4->valuedouble;
                    }

                    l4 = cJSON_GetObjectItem(l3, "b");
                    if (l4) {
                        pch->coef.b = l4->valuedouble;
                    }
                }
            }
        }
    }
    
    
    cJSON_Delete(root);
    
    return r;
}









