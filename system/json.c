#include "json.h"
#include "cjson.h"
#include "log.h"



int json_init(void)
{
    cJSON_InitHooks(NULL);
    return 0;
}



int json_from_ch(char *js, int js_len, ch_cfg_t *cfg, int cfg_cnt)
{
    int i;
    char *json;
    cJSON *root = NULL;
    cJSON *item = NULL;

    if (!js || !cfg)  {
        return -1; 
    }   

    root = cJSON_CreateObject();
    if(!root) {
        return -1;
    }
    
    for(i=0; i<cfg_cnt; i++) {
        item = cJSON_CreateObject();
        if(!item) {
            break;
        }
        
        cJSON_AddNumberToObject(item, "chID",           cfg->chID);
        cJSON_AddNumberToObject(item, "caliParaA",      cfg->caliParaA);
        cJSON_AddNumberToObject(item, "caliParaB",      cfg->caliParaB);
        cJSON_AddNumberToObject(item, "coef",           cfg->coef);
        cJSON_AddNumberToObject(item, "sampleTime",     cfg->sampleTime);
        cJSON_AddNumberToObject(item, "samplePoints",   cfg->samplePoints);
        
        cJSON_AddItemToObject(root, "subobject_1", item);
    }

#if 0
    //cJSON *data = cJSON_CreateObject();
    //cJSON_AddItemToObject(root, "data", data);
    //cJSON *rows = cJSON_CreateArray();
    //cJSON_AddItemToObject(data, "rows", rows);
    //cJSON *row = cJSON_CreateObject();
    //cJSON_AddItemToArray(rows, row);
#endif
    
    json = cJSON_Print(root);
    if(!json) {
        cJSON_Delete(root);
        return -1;
    }
    
    if(js_len<strlen(json)) {
        return -1;
    }
    
    strcpy(js, json);
    cJSON_Delete(root);
    
    return 0;
}



int json_from_ev(char *js, int js_len, ev_cfg_t *cfg, int cfg_cnt)
{
    int i;
    char *json;
    cJSON *root = NULL;

    if (!js || !cfg)  {
        return -1; 
    }   

    root = cJSON_CreateObject();
    if(!root) {
        return -1;
    }
    
    cJSON_AddNumberToObject(root, "type", cfg->type);
    
    json = cJSON_Print(root);
    if(!json) {
        cJSON_Delete(root);
        return -1;
    }
    
    if(js_len<strlen(json)) {
        return -1;
    }
    
    strcpy(js, json);
    cJSON_Delete(root);
    
    return 0;
}



int json_to_ch(char *js, ch_cfg_t *cfg, int cfg_cnt)
{
    int r=0;
    int i,cnt,chID;
    cJSON *root,*params;
    cJSON *tmp,*item;
    ch_cfg_t *pcfg;

    if (!js || !cfg || !cfg_cnt) {
        return -1;
    }

    root = cJSON_Parse(js);
    if(!root) {
        LOGE("%s\n", cJSON_GetErrorPtr());
        return -1;
    }
    
    params = cJSON_GetObjectItem(root, "chParams");
    if (cJSON_IsArray(params)){
        cnt = cJSON_GetArraySize(params);
        if(cnt>cfg_cnt) {
            cJSON_Delete(root);
            LOGE(" json array size too large!\n");
            return -1;
        }
        
        for(i=0; i<cnt; i++) {
            pcfg = NULL;
            item = cJSON_GetArrayItem(params, i);
            if(!item) {
                r = -1; LOGE("json array %d not found!\n", i); break;
            }
            
            tmp = cJSON_GetObjectItem(item, "chID"); 
            if(!tmp) {
                r = -1; LOGE("chID not found!\n"); break; 
            }
            
            chID = tmp->valueint;
            if(chID>=cfg_cnt) {
                r = -1; LOGE(" chID: %d is error\n", chID); break; 
            }
            pcfg = cfg+chID;
            
            pcfg->chID = chID;
            
            tmp = cJSON_GetObjectItem(item, "caliParaA");      if(!tmp) {r = -1; LOGE("caliParaA not found!\n");    break;}
            pcfg->caliParaA = tmp->valuedouble;
            
            tmp = cJSON_GetObjectItem(item, "caliParaB");      if(!tmp) {r = -1; LOGE("caliParaB not found!\n");    break;}
            pcfg->caliParaB = tmp->valuedouble;
            
            tmp = cJSON_GetObjectItem(item, "coef");           if(!tmp) {r = -1; LOGE("coef not found!\n");         break;}
            pcfg->coef = tmp->valuedouble;
            
            tmp = cJSON_GetObjectItem(item, "freq");           if(!tmp) {r = -1; LOGE("freq not found!\n");         break;}
            pcfg->freq = tmp->valueint;
            
            tmp = cJSON_GetObjectItem(item, "sampleTime");     if(!tmp) {r = -1; LOGE("sampleTime not found!\n");   break;}
            pcfg->sampleTime = tmp->valueint;
            
            tmp = cJSON_GetObjectItem(item, "samplePoints");   if(!tmp) {r = -1; LOGE("samplePoints not found!\n"); break;}
            pcfg->samplePoints = tmp->valueint;
        }
    }

    //sub = cJSON_GetObjectItem(root, "chParams");
    
    cJSON_Delete(root);
    
    return r;
}


int json_to_ev(char *js, ev_cfg_t *cfg, int cfg_cnt)
{
    int i;
    cJSON *root;
    cJSON *item;

    if (!js || !cfg) {
        LOGE("Invalid input argument\n");
        return -1;
    }

    root = cJSON_Parse(js);
    item = cJSON_GetObjectItem(root, "type");

    cfg->type = item->valueint;
    
    cJSON_Delete(root);
    
    return 0;
}








