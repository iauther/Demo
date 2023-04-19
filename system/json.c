#include "json.h"
#include "cjson.h"
#include "log.h"



int json_init(void)
{
    cJSON_InitHooks(NULL);
    return 0;
}



int json_from_ch(int id, char *js, int jslen, ch_cfg_t *cfg)
{
    cJSON *root = NULL;

    if (!js || !cfg)  {
        return -1; 
    }   

    root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(root, "chID",           cfg->chID);
    cJSON_AddNumberToObject(root, "caliParaA",      cfg->caliParaA);
    cJSON_AddNumberToObject(root, "caliParaB",      cfg->caliParaB);
    cJSON_AddNumberToObject(root, "coef",           cfg->coef);
    cJSON_AddNumberToObject(root, "sampleTime",     cfg->sampleTime);
    cJSON_AddNumberToObject(root, "samplePoints",   cfg->samplePoints);


#if 0
    //cJSON *data = cJSON_CreateObject();
    //cJSON_AddItemToObject(root, "data", data);
    //cJSON *rows = cJSON_CreateArray();
    //cJSON_AddItemToObject(data, "rows", rows);
    //cJSON *row = cJSON_CreateObject();
    //cJSON_AddItemToArray(rows, row);
#endif
    
    strncpy(js, cJSON_Print(root), jslen);
    cJSON_Delete(root);
    
    return 0;
}



int json_from_ev(int id, char *js, int jslen, ev_cfg_t *cfg)
{
    cJSON *root = NULL;

    if (!js || !cfg)  {
        return -1; 
    }   

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "type", cfg->type);
    strncpy(js, cJSON_Print(root), jslen);

    cJSON_Delete(root);
    
    return 0;
}



int json_to_ch(int id, char *js, ch_cfg_t *cfg)
{
    cJSON *root;
    cJSON *item;

    if (!js || !cfg) {
        return -1;
    }

    root = cJSON_Parse(js);
    
    item = cJSON_GetObjectItem(root, "chID");           if(item) cfg->chID = item->valueint;
    item = cJSON_GetObjectItem(root, "caliParaA");      if(item) cfg->caliParaA = item->valueint;
    item = cJSON_GetObjectItem(root, "caliParaB");      if(item) cfg->caliParaB = item->valueint;
    item = cJSON_GetObjectItem(root, "coef");           if(item) cfg->coef = item->valueint;
    item = cJSON_GetObjectItem(root, "sampleTime");     if(item) cfg->sampleTime = item->valueint;
    item = cJSON_GetObjectItem(root, "samplePoints");   if(item) cfg->samplePoints = item->valueint;
    
    cJSON_Delete(root);
    
    return 0;
}


int json_to_ev(int id, char *js, ev_cfg_t *cfg)
{
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








