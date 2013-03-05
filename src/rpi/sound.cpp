/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2013 - Hans-Kristian Arntzen
 *  Copyright (C) 2012-2013 - Michael Lelli
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "driver.h"
#include "minimal.h"

//sq #include <SDL/SDL.h>
#include <alsa/asoundlib.h>

extern "C" {
#include "fifo_buffer.h"
#include "thread.h"
}

int soundcard,usestereo;
int master_volume = 100;

static INT16 *stream_cache_data;
static int stream_cache_len;
static int stream_cache_channels;
static int samples_per_frame;

//============================================================
//      LOCAL VARIABLES
//============================================================

int                attenuation = 0;

static int                buf_locked;

static INT8               *stream_buffer;
static volatile INT32     stream_playpos;

static UINT32                   stream_buffer_in;

// buffer over/underflow counts
static int                              fifo_underrun;
static int                              fifo_overrun;
static int                              snd_underrun;

// sound enable
static int snd_enabled;

typedef struct alsa
{
    snd_pcm_t *pcm;
    bool nonblock;
    bool has_float;
    volatile bool thread_dead;
    
    size_t buffer_size;
    size_t period_size;
    snd_pcm_uframes_t period_frames;
    
    fifo_buffer_t *buffer;
    sthread_t *worker_thread;
    slock_t *fifo_lock;
    scond_t *cond;
    slock_t *cond_lock;
} alsa_t;

static alsa_t *g_alsa;

static alsa_t *alsa_init(void);
static void alsa_worker_thread(void *data);
static void alsa_free(void *data);


typedef struct audio_driver
{
    void *(*init)(const char *device, unsigned rate, unsigned latency);
    ssize_t (*write)(void *data, const void *buf, size_t size);
    bool (*stop)(void *data);
    bool (*start)(void *data);
    void (*set_nonblock_state)(void *data, bool toggle); // Should we care about blocking in audio thread? Fast forwarding.
    void (*free)(void *data);
    bool (*use_float)(void *data); // Defines if driver will take standard floating point samples, or int16_t samples.
    const char *ident;
    
    size_t (*write_avail)(void *data); // Optional
    size_t (*buffer_size)(void *data); // Optional
} audio_driver_t;

//============================================================

int msdos_init_sound(void)
{
	/* Ask the user if no soundcard was chosen */
	if (soundcard == -1)
	{
		soundcard=1;
	}

	if (soundcard == 0)     /* silence */
	{
		/* update the Machine structure to show that sound is disabled */
		Machine->sample_rate = 0;
		return 0;
	}

	stream_cache_data = 0;
	stream_cache_len = 0;
	stream_cache_channels = 0;

//sq	gp2x_sound_rate=options.samplerate;

	/* update the Machine structure to reflect the actual sample rate */
//sq	Machine->sample_rate = gp2x_sound_rate;

	logerror("set sample rate: %d\n",Machine->sample_rate);

	osd_set_mastervolume(attenuation);	/* set the startup volume */

	return 0;
}

void msdos_shutdown_sound(void)
{
	return;
}

int osd_start_audio_stream(int stereo)
{
	if (stereo) stereo = 1;	/* make sure it's either 0 or 1 */

	stream_cache_channels = (stereo?2:1);

	/* determine the number of samples per frame */
	samples_per_frame = Machine->sample_rate / Machine->drv->frames_per_second;

	//Sound switched off?
	if (Machine->sample_rate == 0)
		return samples_per_frame;

    // attempt to initialize SDL
    g_alsa = alsa_init();

	return samples_per_frame;
}

void osd_stop_audio_stream(void)
{
    // if nothing to do, don't do it
    if (Machine->sample_rate == 0)
        return;
    
	alsa_free(g_alsa);
    
    // print out over/underflow stats
	logerror("Sound buffer: fifo_overrun=%d fifo_underrun=%d snd_underrun=%d\n", fifo_overrun, fifo_underrun, snd_underrun);	

}

#define min(a, b) ((a) < (b) ? (a) : (b))

static void alsa_worker_thread(void *data)
{
    alsa_t *alsa = (alsa_t*)data;
    
    UINT8 *buf = (UINT8 *)calloc(1, alsa->period_size);
    if (!buf)
    {
        logerror("failed to allocate audio buffer");
        goto end;
    }
    
    while (!alsa->thread_dead)
    {
        slock_lock(alsa->fifo_lock);
        size_t avail = fifo_read_avail(alsa->buffer);
        size_t fifo_size = min(alsa->period_size, avail);
        fifo_read(alsa->buffer, buf, fifo_size);
        scond_signal(alsa->cond);
        slock_unlock(alsa->fifo_lock);
        
        // If underrun, fill rest with silence.
        memset(buf + fifo_size, 0, alsa->period_size - fifo_size);

 		if(alsa->period_size != fifo_size) {
 			fifo_underrun++;
 		}
        
        snd_pcm_sframes_t frames = snd_pcm_writei(alsa->pcm, buf, alsa->period_frames);
        
        if (frames == -EPIPE || frames == -EINTR || frames == -ESTRPIPE)
        {
			snd_underrun++;
            if (snd_pcm_recover(alsa->pcm, frames, 1) < 0)
            {
                logerror("[ALSA]: (#2) Failed to recover from error (%s)\n",
                          snd_strerror(frames));
                break;
            }
            
            continue;
        }
        else if (frames < 0)
        {
            logerror("[ALSA]: Unknown error occured (%s).\n", snd_strerror(frames));
            break;
        }
    }
    
end:
    slock_lock(alsa->cond_lock);
    alsa->thread_dead = true;
    scond_signal(alsa->cond);
    slock_unlock(alsa->cond_lock);
    free(buf);
}

static ssize_t alsa_write(void *data, const void *buf, size_t size)
{
    alsa_t *alsa = (alsa_t*)data;
    
    if (alsa->thread_dead)
        return -1;
    
//sq    if (alsa->nonblock)
   //sq {
        slock_lock(alsa->fifo_lock);
        size_t avail = fifo_write_avail(alsa->buffer);
        size_t write_amt = min(avail, size);
		if(write_amt) 
        	fifo_write(alsa->buffer, buf, write_amt);
        slock_unlock(alsa->fifo_lock);
		if(write_amt<size)	
			fifo_overrun++;
        return write_amt;
//sq     }
//sq     else
//sq     {
//sq         size_t written = 0;
//sq         while (written < size && !alsa->thread_dead)
//sq         {
//sq             slock_lock(alsa->fifo_lock);
//sq             size_t avail = fifo_write_avail(alsa->buffer);
//sq             
//sq             if (avail == 0)
//sq             {
//sq                 slock_unlock(alsa->fifo_lock);
//sq                 slock_lock(alsa->cond_lock);
//sq                 if (!alsa->thread_dead)
//sq                     scond_wait(alsa->cond, alsa->cond_lock);
//sq                 slock_unlock(alsa->cond_lock);
//sq             }
//sq             else
//sq             {
//sq                 size_t write_amt = min(size - written, avail);
//sq                 fifo_write(alsa->buffer, (const char*)buf + written, write_amt);
//sq                 slock_unlock(alsa->fifo_lock);
//sq                 written += write_amt;
//sq             }
//sq         }
//sq         return written;
//sq     }

}

static int counter=0;

int osd_update_audio_stream(INT16 *buffer)
{
	//Soundcard switch off?
	if (Machine->sample_rate == 0) return samples_per_frame;

	stream_cache_data = buffer;
	stream_cache_len = samples_per_frame;
    
	profiler_mark(PROFILER_USER1);
    alsa_write(g_alsa, buffer, (samples_per_frame*stream_cache_channels*sizeof(signed short)));
	profiler_mark(PROFILER_END);

	return samples_per_frame;
}

int msdos_update_audio(void)
{
	if (Machine->sample_rate == 0 || stream_cache_data == 0) return 0;
//	profiler_mark(PROFILER_MIXER);
//	updateaudiostream();
//	profiler_mark(PROFILER_END);
	return 0;
}

/* attenuation in dB */
void osd_set_mastervolume(int _attenuation)
{
}

int osd_get_mastervolume(void)
{
	return attenuation;
}

void osd_sound_enable(int enable_it)
{
}

#define TRY_ALSA(x) if (x < 0) { \
goto error; \
}

static alsa_t *alsa_init(void)
{
    
    alsa_t *alsa = (alsa_t*)calloc(1, sizeof(alsa_t));
    if (!alsa)
        return NULL;

	fifo_underrun=0;
	fifo_overrun=0;
	snd_underrun=0;
    
    snd_pcm_hw_params_t *params = NULL;
    snd_pcm_sw_params_t *sw_params = NULL;
    
    const char *alsa_dev = "default";
    
    unsigned latency_usec = 100 * 1000;
    unsigned periods = 2;
    snd_pcm_uframes_t buffer_size;
    
    TRY_ALSA(snd_pcm_open(&alsa->pcm, alsa_dev, SND_PCM_STREAM_PLAYBACK, 0));
    
    TRY_ALSA(snd_pcm_hw_params_malloc(&params));
    
    TRY_ALSA(snd_pcm_hw_params_any(alsa->pcm, params));
    TRY_ALSA(snd_pcm_hw_params_set_access(alsa->pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED));
    TRY_ALSA(snd_pcm_hw_params_set_format(alsa->pcm, params, SND_PCM_FORMAT_S16));
    TRY_ALSA(snd_pcm_hw_params_set_channels(alsa->pcm, params, stream_cache_channels));
    TRY_ALSA(snd_pcm_hw_params_set_rate(alsa->pcm, params, Machine->sample_rate, 0));
    
    TRY_ALSA(snd_pcm_hw_params_set_buffer_time_near(alsa->pcm, params, &latency_usec, NULL));
    TRY_ALSA(snd_pcm_hw_params_set_periods_near(alsa->pcm, params, &periods, NULL));
    
    TRY_ALSA(snd_pcm_hw_params(alsa->pcm, params));
    
    snd_pcm_hw_params_get_period_size(params, &alsa->period_frames, NULL);
    logerror("ALSA: Period size: %d frames\n", (int)alsa->period_frames);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    logerror("ALSA: Buffer size: %d frames\n", (int)buffer_size);
    alsa->buffer_size = snd_pcm_frames_to_bytes(alsa->pcm, buffer_size);
    alsa->period_size = snd_pcm_frames_to_bytes(alsa->pcm, alsa->period_frames);
    
    TRY_ALSA(snd_pcm_sw_params_malloc(&sw_params));
    TRY_ALSA(snd_pcm_sw_params_current(alsa->pcm, sw_params));
    TRY_ALSA(snd_pcm_sw_params_set_start_threshold(alsa->pcm, sw_params, buffer_size / 2));
    TRY_ALSA(snd_pcm_sw_params(alsa->pcm, sw_params));
    
    snd_pcm_hw_params_free(params);
    snd_pcm_sw_params_free(sw_params);

	TRY_ALSA(snd_pcm_prepare(alsa->pcm));

	//Write initial blank sound to stop underruns?
	{
		void *tempbuf;
		tempbuf=calloc(1, alsa->period_size*2);
		snd_pcm_writei (alsa->pcm, tempbuf, 2 * alsa->period_frames);
		free(tempbuf);
	}

    alsa->fifo_lock = slock_new();
    alsa->cond_lock = slock_new();
    alsa->cond = scond_new();
    alsa->buffer = fifo_new(alsa->buffer_size);
    if (!alsa->fifo_lock || !alsa->cond_lock || !alsa->cond || !alsa->buffer)
        goto error;
    
    alsa->worker_thread = sthread_create(alsa_worker_thread, alsa);
    if (!alsa->worker_thread)
    {
        logerror("error initializing worker thread\n");
        goto error;
    }
    
    snd_enabled = 1;

    return alsa;
    
error:
    logerror("ALSA: Failed to initialize...\n");
    if (params)
        snd_pcm_hw_params_free(params);
    
    if (sw_params)
        snd_pcm_sw_params_free(sw_params);
    
    alsa_free(alsa);
    
    return NULL;

}

static bool alsa_use_float(void *data)
{
    alsa_t *alsa = (alsa_t*)data;
    return alsa->has_float;
}


static void alsa_free(void *data)
{
    alsa_t *alsa = (alsa_t*)data;
    
    if (alsa)
    {
        if (alsa->worker_thread)
        {
            alsa->thread_dead = true;
            sthread_join(alsa->worker_thread);
        }
        if (alsa->buffer)
            fifo_free(alsa->buffer);
        if (alsa->cond)
            scond_free(alsa->cond);
        if (alsa->fifo_lock)
            slock_free(alsa->fifo_lock);
        if (alsa->cond_lock)
            slock_free(alsa->cond_lock);
        if (alsa->pcm)
        {
            snd_pcm_drop(alsa->pcm);
            snd_pcm_close(alsa->pcm);
        }
        free(alsa);
    }
}


static bool alsa_stop(void *data)
{
    (void)data;
    return true;
}

static void alsa_set_nonblock_state(void *data, bool state)
{
    alsa_t *alsa = (alsa_t*)data;
    alsa->nonblock = state;
}

static bool alsa_start(void *data)
{
    (void)data;
    return true;
}

static size_t alsa_write_avail(void *data)
{
    alsa_t *alsa = (alsa_t*)data;
    
    if (alsa->thread_dead)
        return 0;
    slock_lock(alsa->fifo_lock);
    size_t val = fifo_write_avail(alsa->buffer);
    slock_unlock(alsa->fifo_lock);
    return val;
}

static size_t alsa_buffer_size(void *data)
{
    alsa_t *alsa = (alsa_t*)data;
    return alsa->buffer_size;
}

//const audio_driver_t audio_alsathread = {
//    alsa_init,
//    alsa_write,
//    alsa_stop,
//    alsa_start,
//    alsa_set_nonblock_state,
//    alsa_free,
//    alsa_use_float,
//    "alsathread",
//    alsa_write_avail,
//    alsa_buffer_size,
//};


void osd_opl_control(int chip,int reg)
{
}

void osd_opl_write(int chip,int data)
{
}
