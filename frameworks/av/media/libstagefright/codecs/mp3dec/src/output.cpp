

#include "frame.h"
#include "synth.h"
#include "utils/Log.h"
#if 0
/* default error handler */  
enum mad_flow error_default(void *data, struct mad_stream *stream,
			    struct mad_frame *frame)
{
  int *bad_last_frame = data;

  switch (stream->error) {
  case MAD_ERROR_BADCRC:
    if (*bad_last_frame)
      mad_frame_mute(frame);
    else
      *bad_last_frame = 1;

    return MAD_FLOW_IGNORE;

  default:
    return MAD_FLOW_CONTINUE;
  }
}

#endif



/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

#if 0
static
enum mad_flow input(void *data,
		    struct mad_stream *stream , void * decoder )
{
  struct buffer *buffer = data;
  unsigned int lb , rb = 0;
  
  static MY_FILE* pRawFileCache = NULL;
  static size_t (*RKFIO_FRead)(MY_FILE *, void *, size_t) ;
  static int (*RKFIO_FSeek)(MY_FILE * /*stream*/, long int /*offset*/, int /*whence*/);
  static unsigned int (*RKFIO_FLength)(MY_FILE *in);
  
  tMP3 *pMP3 = (tMP3 *)decoder;
  
  pRawFileCache = (MY_FILE *)pMP3->hFile;
  RKFIO_FRead = (size_t (*)(MY_FILE *, void *, size_t)) pMP3->fRead;
  RKFIO_FSeek = (int (*)(MY_FILE * , long int , int )) pMP3->fSeek;
  RKFIO_FLength = (unsigned int (*)(MY_FILE *)) pMP3->fLength;

  if (!buffer->length)
    return MAD_FLOW_STOP;

  // READ 
  if (stream->this_frame && stream->next_frame)
  {
    rb = (unsigned int)(buffer->length) - 
         (unsigned int)(stream->next_frame - stream->buffer);
	
	if (stream->buffer == stream->next_frame)
	{
		rb = 0;
		lb = RKFIO_FRead(pRawFileCache,(void *)(stream->buffer),buffer->length);
		printf("read a : %d\n",buffer->length);
	}
	else
	{
	    //memmove((void *)stream->buffer, (void *)stream->next_frame, rb);
	    memcpy((void *)stream->buffer, (void *)stream->next_frame, rb);
	    lb = RKFIO_FRead(pRawFileCache,(void *)(stream->buffer + rb),buffer->length - rb);
	    printf("read b : %d\n",buffer->length - rb);
	    if (lb < buffer->length - rb)
	    {
	    	buffer->length = lb + rb;
	    	if (lb == 0)
	    		return MAD_FLOW_STOP;
	    }//lb = 0;
	}
  }
  else  
  {
    lb = RKFIO_FRead(pRawFileCache,(void *)buffer->start , buffer->length - rb);
    printf("read c : %d\n",buffer->length - rb);
  }

  if (lb == 0)
  {	
    / ffer->length = 0;
    return MAD_FLOW_STOP;
  }
  else 
    buffer->length = lb + rb;

  mad_stream_buffer(stream, buffer->start, buffer->length);
  
  return MAD_FLOW_CONTINUE;  
}
#endif


/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static __inline
signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

#if 0
static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
#else
void output(struct mad_header const * /*header*/,
		     struct mad_pcm *pcm)

#endif

{
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;
  /*
  short * pLeft  = pcm->pOutLeft ;  
  short * pRight = pcm->pOutRight;
  */
  short * pLeft  = pcm->pOutput ;  
  short * pRight = pcm->pOutput + 1;
  
  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  while (nsamples--) {
  
    signed int sample;

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = scale(*left_ch++);

    *pLeft++ = sample;
   

#ifdef MERGE//one channel ->two channel
    if (nchannels == 2) {
      sample = scale(*right_ch++);      
      *pRight++ = sample;
	}	
    else
    {
      *pRight++ = sample;
    }   
	pRight++;	//for merge
#else
	 
	  if (nchannels == 2) {
	  pLeft ++;	//for merge
      sample = scale(*right_ch++);      
      *pRight++ = sample;
		pRight++;	//for merge
	 }
#endif
        
  }

  return ;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */
#if 0
static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct buffer *buffer = data;
/*
  fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);
*/
  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

  return MAD_FLOW_CONTINUE;
}

#endif



#if 0


void main()
{
	
	unsigned char inputdata[192];
	short outputdata[1152*2];
	struct mad_frame frame;
	struct mad_stream stream;
	struct mad_synth synth;

	FILE* inputfp = fopen("test.mp1","rb");
	FILE* outputfp = fopen("test.pcm","wb");
	if(inputfp == NULL || outputfp == NULL)
		return;

	  mad_stream_init(&stream);
	  mad_frame_init(&frame);
	  mad_synth_init(&synth);
	  synth.pcm.pOutput = outputdata;
	while(fread(inputdata , 1, 192, inputfp)== 192)//此处读书需先知道没帧编码的大小
	{
		 stream.this_frame = inputdata;
		 mad_frame_decode(&frame, &stream);
		 mad_synth_frame(&synth, &frame);
		 output(&frame.header, &synth.pcm);
		 fwrite(outputdata,2,synth.pcm.length*2,outputfp);
	}

	fclose(inputfp);
	fclose(outputfp);	
}
#endif
