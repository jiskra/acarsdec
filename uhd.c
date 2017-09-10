/*
 * Copyright 2015 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef WITH_UHD

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <uhd.h>
#include "acarsdec.h"

#define INTRATE 12500
#define UHDMULT 200
#define UHDINRATE (INTRATE*UHDMULT)
#define	EXIT_SUCCESS	0	/* Successful exit status.  */

static uhd_usrp_handle usrp;
static uhd_rx_streamer_handle rx_streamer;
static uhd_rx_metadata_handle md;
static char uhd_error_string[512];
static float *buff = NULL;
static float *buff_start = NULL;
static void **buffs_ptr = NULL;
static size_t samps_per_buff;
static int return_code = EXIT_SUCCESS;

//static rtlsdr_dev_t *dev = NULL;
static int status = 0;

#define UHDOUTBUFSZ 1024
#define UHDINBUFSZ (UHDOUTBUFSZ*UHDMULT*2)

static unsigned int chooseFc(unsigned int *Fd, unsigned int nbch)
{
	int n;
	int ne;
	int Fc;
	do {
		ne = 0;
		for (n = 0; n < nbch - 1; n++) {
			if (Fd[n] > Fd[n + 1]) {
				unsigned int t;
				t = Fd[n + 1];
				Fd[n + 1] = Fd[n];
				Fd[n] = t;
				ne = 1;
			}
		}
	} while (ne);

	if ((Fd[nbch - 1] - Fd[0]) > UHDINRATE - 4 * INTRATE) {
		fprintf(stderr, "Frequencies too far apart\n");
		return -1;
	}

	for (Fc = Fd[nbch - 1] + 2 * INTRATE; Fc > Fd[0] - 2 * INTRATE; Fc--) {
		for (n = 0; n < nbch; n++) {
			if (abs(Fc - Fd[n]) > UHDINRATE / 2 - 2 * INTRATE)
				break;
			if (abs(Fc - Fd[n]) < 2 * INTRATE)
				break;
			if (n > 0 && Fc - Fd[n - 1] == Fd[n] - Fc)
				break;
		}
		if (n == nbch)
			break;
	}

	return Fc;
}

int initUHD(char **argv, int optind){
	int r, n;
    double freq_bk = 500e6;
    double rate_bk = 1e6;
    double gain_bk = 5.0;
    double clock_bk = 16e6;
    char* device_args = "";
    size_t signal_channel = 0;
    char *argF;
    unsigned int Fc;
    unsigned int Fd[MAXNBCHANNELS];
    
    if(uhd_set_thread_priority(uhd_default_thread_priority, true)){
            fprintf(stderr, "Unable to set thread priority. Continuing anyway.\n");
        }
    fprintf(stderr, "Creating USRP with args \"%s\"...\n", device_args);
            
    if (argv[optind] == NULL) {
    		fprintf(stderr, "Use the default device.\n");
    	}
    	optind++;
    	nbch = 0;
    		while ((argF = argv[optind]) && nbch < MAXNBCHANNELS) {
    			Fd[nbch] =
    			    ((int)(1000000 * atof(argF) + INTRATE / 2) / INTRATE) *
    			    INTRATE;
    			optind++;
    			if (Fd[nbch] < 118000000 || Fd[nbch] > 138000000) {
    				fprintf(stderr, "WARNING: Invalid frequency %d\n",
    					Fd[nbch]);
    				continue;
    			}
    			channel[nbch].chn = nbch;
    			channel[nbch].Fr = (float)Fd[nbch];
    			nbch++;
    		};
    		if (nbch >= MAXNBCHANNELS)
    			fprintf(stderr,
    				"WARNING: too many frequencies, using only the first %d\n",
    				MAXNBCHANNELS);

    		if (nbch == 0) {
    			fprintf(stderr, "Need a least one frequency\n");
    			return 1;
    		}

    		Fc = chooseFc(Fd, nbch);
    		if (Fc == 0)
    			return 1;

    		for (n = 0; n < nbch; n++) {
    			channel_t *ch = &(channel[n]);
    			int ind;
    			float AMFreq;

    			ch->wf = malloc(UHDMULT*sizeof(float complex));

    			ch->dm_buffer=malloc(UHDOUTBUFSZ*sizeof(float));

    			AMFreq = (ch->Fr - (float)Fc) / (float)(UHDINRATE) * 2.0 * M_PI;
    			for (ind = 0; ind < UHDMULT; ind++) {
    				ch->wf[ind]=cexpf(AMFreq*ind*-I)/UHDMULT/127.5;
    			}
    		}
    		
    if (uhd_usrp_make(&usrp, device_args)) return 1;
        // Create RX streamer
    if (uhd_usrp_set_master_clock_rate(usrp,50e6,0)) return 1;

    if (uhd_usrp_get_master_clock_rate(usrp,0,&clock_bk)) return 1;
    fprintf(stderr,"Master clock rate is %f.\n",clock_bk);
        
    if (uhd_rx_streamer_make(&rx_streamer)) return 1;

        // Create RX metadata
        
    if (uhd_rx_metadata_make(&md)) return 1;

        // Create other necessary structs
       
        uhd_tune_result_t tune_result;

        uhd_stream_args_t stream_args = {
            .cpu_format = "fc32",
            .otw_format = "sc16",
            .args = "",
            .channel_list = &signal_channel,
            .n_channels = 1
        };

        uhd_stream_cmd_t stream_cmd = {
            .stream_mode = UHD_STREAM_MODE_START_CONTINUOUS,
            .num_samps = 0,
            .stream_now = true
        };



        // Set rate
        if (verbose) fprintf(stderr, "Setting RX Rate: %f...\n", (double)UHDINRATE);
        if (uhd_usrp_set_rx_rate(usrp, (double) UHDINRATE, signal_channel) ) return 1;

        // See what rate actually is
        if (uhd_usrp_get_rx_rate(usrp, signal_channel, &rate_bk)) return 1;
        fprintf(stderr, "Actual RX Rate: %f...\n", rate_bk);

        // Set gain
        if (verbose) fprintf(stderr, "Setting RX Gain: %f dB...\n", (double)gain);
        if (uhd_usrp_set_rx_gain(usrp, (double)gain, signal_channel, "")) return 1;

        // See what gain actually is
        if (uhd_usrp_get_rx_gain(usrp, signal_channel, "", &gain_bk)) return 1;
        fprintf(stderr, "Actual RX Gain: %f...\n", gain_bk);
        
        uhd_tune_request_t tune_request = {
                   .target_freq = (float)Fc,
                   .rf_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO,
                   .dsp_freq_policy = UHD_TUNE_REQUEST_POLICY_AUTO
               };

        // Set frequency
        fprintf(stderr, "Setting RX frequency: %f MHz...\n", Fc/1e6);
        if (uhd_usrp_set_rx_freq(usrp, &tune_request, signal_channel, &tune_result)) return 1;

        // See what frequency actually is
        if (uhd_usrp_get_rx_freq(usrp, signal_channel, &freq_bk)) return 1;
        fprintf(stderr, "Actual RX frequency: %f MHz...\n", freq_bk / 1e6);

        // Set up streamer
        stream_args.channel_list = &signal_channel;
        if (uhd_usrp_get_rx_stream(usrp, &stream_args, rx_streamer)) return 1;

        // Set up buffer
        if (uhd_rx_streamer_max_num_samps(rx_streamer, &samps_per_buff)) return 1;
        if (verbose) fprintf(stderr, "Buffer size in samples: %zu\n", samps_per_buff);
        buff_start  = buff = malloc((UHDINBUFSZ+2*2*samps_per_buff)* sizeof(float));
        buffs_ptr = (void**)&buff;

        // Issue stream command
        if (verbose) fprintf(stderr, "Issuing stream command.\n");
        if (uhd_rx_streamer_issue_stream_cmd(rx_streamer, &stream_cmd)) return 1;
        return 0;

}




int runUHDSample(void){
	size_t num_acc_samps = 0;
	size_t num_rx_samps = 0;
	while(1){
	buff=(void*)buff_start+2*sizeof(float)*num_acc_samps;
	if (uhd_rx_streamer_recv(rx_streamer, buffs_ptr, samps_per_buff, &md, 3.0, false, &num_rx_samps)) return 1;
	uhd_rx_metadata_error_code_t error_code;
	if (uhd_rx_metadata_error_code(md, &error_code)) return 1;
	if(error_code != UHD_RX_METADATA_ERROR_CODE_NONE){
	   fprintf(stderr, "Error code 0x%x was returned during streaming. Aborting.\n", error_code);
	   return 1;
	 }

	        // Handle data
	 if (verbose) {
	   time_t full_secs;
	   double frac_secs;
	   uhd_rx_metadata_time_spec(md, &full_secs, &frac_secs);
	   fprintf(stderr, "Received packet: %zu samples, %.f full secs, %f frac secs\n",
	            num_rx_samps,
	            difftime(full_secs, (time_t) 0),
	            frac_secs);
	   }

	  num_acc_samps += num_rx_samps;
	  if ((2*num_acc_samps)>UHDINBUFSZ) {
		  int r, n;

		  	status=0;

		  	for (n = 0; n < nbch; n++) {
		  		channel_t *ch = &(channel[n]);
		  		int i,m;
		  		float complex D,*wf;

		  		wf = ch->wf;
		  		m=0;
		  		for (i = 0; i < UHDINBUFSZ;) {
		  			int ind;

		  			D = 0;
		  			for (ind = 0; ind < UHDMULT; ind++) {
		  				float r, g;
		  				float complex v;

		  				r = buff_start[i]; i++;
		  				g = buff_start[i]; i++;

		  				v=r+g*I;
		  				D+=v*wf[ind];
		  			}
		  			ch->dm_buffer[m++]=cabsf(D);
		  		}
		  		demodMSK(ch,m);
		  	}
	  		num_acc_samps=num_acc_samps-UHDINBUFSZ/2;
	  		memcpy((void*)buff_start, (void*)(buff_start+UHDINBUFSZ*sizeof(float)), num_acc_samps*sizeof(float)*2);
	  }//*/

   }
}
int FreeUHD(void){
        if(buff){
            if(verbose){
                fprintf(stderr, "Freeing buffer.\n");
            }
            free(buff);
        }
        buff = NULL;
        buffs_ptr = NULL;

        if(verbose){
            fprintf(stderr, "Cleaning up RX streamer.\n");
        }
        uhd_rx_streamer_free(&rx_streamer);

        if(verbose){
            fprintf(stderr, "Cleaning up RX metadata.\n");
        }
        uhd_rx_metadata_free(&md);

        if(verbose){
            fprintf(stderr, "Cleaning up USRP.\n");
        }
        if(return_code != EXIT_SUCCESS && usrp != NULL){
            uhd_usrp_last_error(usrp, uhd_error_string, 512);
            fprintf(stderr, "USRP reported the following error: %s\n", uhd_error_string);
        }
        uhd_usrp_free(&usrp);

    return 0;
}
#endif
