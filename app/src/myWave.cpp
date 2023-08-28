#include <WS2tcpip.h>
#include <stdio.h>
#include "myWave.h"



#pragma comment (lib, "GuiLite.lib")
#pragma comment (lib, "plplot.lib")

static PLINT pl_errcode= 0;
static char  errmsg[160]= "";
myWave::myWave()
{
    pls = new plstream();
}

myWave::~myWave()
{
    pls->stripd(chartId);
    delete pls;
}


int myWave::set_print(myWave_print_t fn)
{
	
	return 0;
}


int myWave::init(void)
{
    bool            autoy, acc;
    PLFLT           init_ymin, init_ymax, xlab, ylab;
    PLFLT           t, init_tmin, init_tmax, tjump, noise;
    PLINT           colbox, collab, colline[4]={0}, styline[4]={0};
    const char      *legline[4] ;

    //Parse and process command line arguments.
    pls->parseopts(&argc, argv, PL_PARSE_FULL);

    //Specify some reasonable defaults for ymin and ymax
    //The plot will grow automatically if needed (but not shrink)
    init_ymin = -0.1;
    init_ymax = 0.1;
        
    //Specify initial tmin and tmax -- this determines length of window.
    //Also specify maximum jump in t
    //This can accomodate adaptive timesteps
    init_tmin  = 0.;
    init_tmax  = 10.;
    tjump = 0.3;      // percentage of plot to jump

    //Axes options same as plbox.
    //Only automatic tick generation and label placement allowed
    //Eventually I'll make this fancier
 
    colbox     = 1;
    collab     = 3;
    styline[0] = colline[0] = 2;      // pens color and line style
    styline[1] = colline[1] = 2;
    styline[2] = colline[2] = 2;
    styline[3] = colline[3] = 2;
 
    legline[0] = "sin";                       // pens legend
    legline[1] = "";
    legline[2] = "";
    legline[3] = "";
 
    xlab = 0.; ylab = 0.25;   // legend position
 
    autoy = true;             // autoscale y
    acc   = true;             // don't scrip, accumulate
    
    pls->sdev("qtwidget");
    pls->init();
    pls->adv(0);
    pls->vsta();
    
    pls->sError(&errCode, errmsg);
     
    pls->stripc(&chartId, "bcnst", "bcnstv",
                init_tmin, init_tmax, tjump, init_ymin, init_ymax,
                xlab, ylab, autoy, acc, colbox, collab, colline, styline, legline,
                "t", "", "Strip chart demo");
    
    pls->sError( NULL, NULL );
    
    autoy = false; // autoscale y
    acc   = true;  // accumulate/
    
    return 0;
}


int myWave::update(void)
{
    PLFLT y1, y2, y3, y4,dt;
    PLINT n,nsteps = 1000;
    
    y1 = y2 = y3 = y4 = 0.0;
    dt = 0.1;
    for (n = 0; n<nsteps; n++)
    {
        t  = (double) n * dt;
        y1 = sin( t * M_PI / 18. );
        // There is no need for all pens to have the same number of
        // points or beeing equally time spaced.
        pls->stripa(chartId, 0, t, y1);
    }
    
    return 0;
}







