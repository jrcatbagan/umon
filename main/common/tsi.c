/**************************************************************************
 *
 * Copyright (c) 2013 Alcatel-Lucent
 *
 * Alcatel Lucent licenses this file to You under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  A copy of the License is contained the
 * file LICENSE at the top level of this repository.
 * You may also obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************
 *
 * tsi.c:
 *
 * Touch Screen Interface...
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_TSI & INCLUDE_FBI
#include "genlib.h"
#include "cli.h"
#include "fbi.h"
#include "tsi.h"
#include "timer.h"


int
tsi_scribble(int width, long color)
{
    int x, y, xx, yy, done, last_x, last_y;
    static int xadjust, yadjust;

    printf("Entering 'scribble' mode, hit any key to terminate...\n");

    last_x = last_y = done = 0;
    while(!done) {
        /* Use 'a', 'd', 'w' and 'x' as special input to adjust the
         * coordinates between touch and frame buffer.
         */
        if(gotachar()) {
            switch(getchar()) {
            case 'a':       // LEFT
                xadjust--;
                break;
            case 'd':       // RIGHT
                xadjust++;
                break;
            case 'w':       // UP
                yadjust--;
                break;
            case 'x':       // DOWN
                yadjust++;
                break;
            default:
                done = 1;
                break;
            }
        }
        if(tsi_active()) {
            x = tsi_getx();
            y = tsi_gety();

            /* If incoming coordinate is ever more than 'PEN_MOVE_FILTER'
             * pixels away from the previous position, then assume it
             * to be noise and don't display it.
             */
            if((x < (last_x + PEN_MOVE_FILTER)) &&
                    (x > (last_x - PEN_MOVE_FILTER)) &&
                    (y < (last_y + PEN_MOVE_FILTER)) &&
                    (y > (last_y - PEN_MOVE_FILTER))) {
                x += xadjust;
                y += yadjust;
                for(xx = x-width; xx <= x+width; xx++) {
                    for(yy = y-width; yy <= y+width; yy++) {
                        fbi_setpixel(xx,yy,color);
                    }
                }
            }
            last_x = x;
            last_y = y;
        }
    }
    if((xadjust != 0) || (yadjust != 0)) {
        printf("Xadjust: %d, Yadjust: %d\n",xadjust, yadjust);
    }
    return(0);
}

#define WFNT_FILTER 200

char *TsiHelp[] = {
    "Touch screen interface",
    "-[v] {cmd} [args...]",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -v           additive verbosity",
    "",
    "Commands:",
    " scribble {width} {color}    # draw on the frame buffer",
    " wft [msec-timeout]          # wait for touch",
    " wfnt [msec-timeout]         # wait for no touch",
    "",
    "Notes:",
    " Shell variable 'TSI_X' and 'TSI_Y' are populated by 'wft'.",
#endif
    0,
};

int
TsiCmd(int argc,char *argv[])
{
    static int tsiInitialized;
    int opt, verbose;
    char *cmd, *arg1;
    static struct elapsed_tmr tmr;

    // Make sure the touch-screen interface is initialized...
    if(!tsiInitialized) {
        if(tsi_init() == -1) {
            printf("Error intializing touch screen!\n");
            return(CMD_FAILURE);
        }
        tsiInitialized = 1;
    }
    verbose = 0;
    while((opt=getopt(argc,argv,"v")) != -1) {
        switch(opt) {
        case 'v':
            verbose++;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }
    if(argc < optind + 1) {
        return(CMD_FAILURE);
    }

    cmd = argv[optind];
    arg1 = argv[optind+1];

    if(strcmp(cmd,"scribble") == 0) {
        int width;
        long color;

        if(argc != optind+3) {
            return(CMD_PARAM_ERROR);
        }

        width = strtol(arg1,0,0);
        color = strtol(argv[optind+2],0,0);
        tsi_scribble(width,color);
    } else if(strcmp(cmd,"wfnt") == 0) {
        int x, y;
        int filter, ftot;

        if(argc == optind+2) {
            int timeout = strtol(arg1,0,0);

            startElapsedTimer(&tmr,timeout);
            while(1) {
                x = tsi_getx();
                y = tsi_gety();
                if(!tsi_active()) {
                    for(ftot=0,filter=0; filter<WFNT_FILTER; filter++) {
                        ftot += tsi_active() ? 0 : 1;
                    }
                    if(ftot == WFNT_FILTER) {
                        break;
                    }
                }
                if(msecElapsed(&tmr)) {
                    x = y = -1;
                    break;
                }
            }
        } else {
            while(1) {
                x = tsi_getx();
                y = tsi_gety();
                if(!tsi_active()) {
                    for(ftot=0,filter=0; filter<WFNT_FILTER; filter++) {
                        ftot += tsi_active() ? 0 : 1;
                    }
                    if(ftot == WFNT_FILTER) {
                        break;
                    }
                }
            }
        }
        shell_sprintf("TSI_X","%d",x);
        shell_sprintf("TSI_Y","%d",y);
    } else if(strcmp(cmd,"wft") == 0) {
        int x, y;

        if(argc == optind+2) {
            int timeout = strtol(arg1,0,0);

            startElapsedTimer(&tmr,timeout);
            while(1) {
                if(tsi_active()) {
                    x = tsi_getx();
                    y = tsi_gety();
                    break;
                }
                if(msecElapsed(&tmr)) {
                    x = y = -1;
                    break;
                }
            }
        } else {
            while(!tsi_active());
            x = tsi_getx();
            y = tsi_gety();
        }
        shell_sprintf("TSI_X","%d",x);
        shell_sprintf("TSI_Y","%d",y);
    } else {
        return(CMD_PARAM_ERROR);
    }

    return(CMD_SUCCESS);
}
#endif
