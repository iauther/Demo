#include "data.h"
#include "icfg.h"


paras_t DEFAULT_PARAS={
    
    {
        FW_MAGIC,
        VERSION,
        __DATE__,
    },
    
    {
        MODE_CONTINUS,
        {
            {
                MODE_CONTINUS,
                {0,0,0},
                90.0F,
                100.0F
             },
            
             {
                MODE_CONTINUS,
                {0,0,0},
                90.0F,
                100.0F
             },
             
             {
                MODE_CONTINUS,
                {0,0,0},
                90.0F,
                100.0F
             },
             
             {
                MODE_CONTINUS,
                {0,0,0},
                90.0F,
                100.0F
             },
        },
    }
};

paras_t curParas;


