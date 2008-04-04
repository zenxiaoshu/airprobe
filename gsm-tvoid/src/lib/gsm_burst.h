#ifndef INCLUDED_GSM_BURST_H
#define INCLUDED_GSM_BURST_H

//TODO: rename to gsm_burst_receiver ?  use gsm_burst as encapsulation of an actual burst, incl bbuf, etc.
//			need to determine what is a decoder vs. burst function.  E.g. calc_freq_offset
//			everything but I/O & clocking & sync_state?
//			What about handling complex&diff&bin data?

#include "gsm_constants.h"
#include <gr_math.h>
//#include <Python.h>		//for callback testing
#include <gr_feval.h>

//Console printing options
#define PRINT_NOTHING		0x00000000
#define PRINT_EVERYTHING	0x7FFFFFFF		//7 for SWIG overflow check work around
#define PRINT_BITS			0x00000001
#define PRINT_ALL_BITS		0x00000002
#define PRINT_CORR_BITS		0x00000004
#define PRINT_STATE			0x00000008

#define PRINT_ALL_TYPES		0x00000FF0
#define PRINT_KNOWN			0x00000FE0
#define PRINT_UNKNOWN		0x00000010
#define PRINT_TS0			0x00000020
#define PRINT_FCCH			0x00000040
#define PRINT_SCH			0x00000080
#define PRINT_DUMMY			0x00000100
#define PRINT_NORMAL		0x00000200

#define PRINT_HEX			0x00001000

//Timing/clock options
#define QB_NONE				0x00000000
#define QB_QUARTER			0x00000001		//only for internal clocked versions
#define QB_FULL04			0x00000003
#define QB_MASK				0x0000000F

#define CLK_CORR_TRACK		0x00000010		//adjust timing based on correlation offsets

#define DEFAULT_CLK_OPTS	( QB_QUARTER | CLK_CORR_TRACK )

#define SIGNUM(x)	((x>0)-(x<0))
		  
#define BBUF_SIZE	   TS_BITS

// Center bursts in the TS, splitting the guard period
//
// +--+--+---...-----+--...---+----...----+--+--+
//  G  T     D1         TS         D2      T   G
// Start ^

#define MAX_SYNC_WAIT	32	//Number of missed bursts before reverting to WAIT_FCCH. 

#define MAX_CORR_DIST   7	// 4 + 3 =  1/2 GUARD + TAIL
#define SCH_CORR_THRESHOLD	0.80
#define FCCH_CORR_THRESHOLD 0.90
#define NORM_CORR_THRESHOLD 0.80

#define	FCCH_HITS_NEEDED	(USEFUL_BITS - 4)
#define FCCH_MAX_MISSES		1

enum EQ_TYPE {
	EQ_NONE,
	EQ_FIXED_LINEAR,
	EQ_ADAPTIVE_LINEAR,
	EQ_FIXED_DFE,
	EQ_ADAPTIVE_DFE,
	EQ_VITERBI
};

//typedef void (*PSTAT_FUNC)(int, void *);
//#define STAT_GOT_BURST	1
//double gr_feval_callback(gr_feval_dd *f, double x);

/*
class gsm_tuner_callback {
protected:
  virtual void tune(double x);

public:
  virtual void do_tune(double x);
};
*/
//void do_tuner_callback(gsm_tuner_callback *t, double f);

class gsm_burst;

class gsm_burst
{
protected:
	
	gsm_burst(gr_feval_dd *t);  

	//Burst Buffer: Storage for burst data
	float			d_burst_buffer[BBUF_SIZE];
	int				d_bbuf_pos;			//write position
	int				d_burst_start;		//first useful bit (beginning of output)
	unsigned long   d_sample_count;		//sample count at end (TODO:beginning) of BBUF (bit count if external clock)
	unsigned long   d_last_burst_s_count;		//sample count from previous burst

	unsigned char	d_decoded_burst[USEFUL_BITS];	//Differentially Decoded burst buffer {0,1}
	
	///// Sync/training sequence correlation
	float			corr_sync[N_SYNC_BITS];	//encoded sync bits for correlation
	float			corr_train_seq[10][N_TRAIN_BITS];
	const float		*d_corr_pattern;
	int				d_corr_pat_size;
	float 			d_corr_max;
	int 			d_corr_maxpos;
	int				d_corr_center;
		
	///// Burst information
	SYNC_STATE		d_sync_state;
	SYNC_STATE		d_last_sync_state;
	BURST_TYPE		d_burst_type;
	unsigned		d_ts;					//timeslot 0-7
	unsigned long	d_last_good;			//Burst count of last good burst
	unsigned long   d_burst_count;			//Bursts received starting w/ initial FCCH reset after lost sync
	unsigned long   d_last_sch;				//Burst count of last SCH
	int				d_color_code;

	float			d_freq_offset;
	double			d_freq_off_sum;
	double			d_freq_off_weight;

	//PSTAT_FUNC		p_stat_func;
	//void       		*stat_func_data;

	gr_feval_dd 	*p_tuner;
	//gsm_tuner_callback	*p_tuner;
	
	//////// Methods
	int				get_burst(void);
	BURST_TYPE		get_fcch_burst(void);
	BURST_TYPE		get_sch_burst(void);
	BURST_TYPE		get_norm_burst(void);
	
	void	shift_burst(int);
	void	calc_freq_offset(void); 
	void	equalize(void);
	float	correlate_pattern(const float *,const int,const int,const int);
	void	diff_decode_burst(void);

	void	sync_reset(void);

	void	print_bits(const float *data,int length);
	void	print_hex(const unsigned char *data,int length);
	void	print_burst(void);

	void	diff_encode(const float *in,float *out,int length,float lastbit = 1.0);	
	void	diff_decode(const float *in,float *out,int length,float lastbit = 1.0);	

public:
	~gsm_burst ();	

	//Set status callback function, needed for quick tune()
	//void py_set_status_callback(PyObject *pyfunc);
	//void set_status_callback(PSTAT_FUNC func, void *clientdata);

	//void set_tuner_callback(gr_feval_dd *t);
	
	//use swig directors to privide a python override
	//virtual void notify_status(int status);

	//void set_tuner_callback(gsm_tuner_callback *f);
	
	////// General Stats
	//TODO: Maybe there should be a burst_stats class?
	long			d_sync_loss_count;
	long			d_fcch_count;
	long			d_part_sch_count;
	long			d_sch_count;
	long			d_normal_count;
	long			d_dummy_count;
	long			d_unknown_count;
	
	////// Options
	unsigned long	d_clock_options;
	unsigned long	d_print_options;
	EQ_TYPE			d_equalizer_type;
	
	int sync_state() { return d_sync_state;}
	float last_freq_offset() {return d_freq_offset;}
	double mean_freq_offset(void);
	
	//Methods
	void full_reset(void);
};


#endif /* INCLUDED_GSM_BURST_H */
