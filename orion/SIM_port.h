/*******************************************************************
*Unit :
*	voltage : V
* 	frequency : Hz
*                  
********************************************************************/

#include "booksim_config.hpp"

#ifndef _SIM_PORT_H
#define _SIM_PORT_H
extern int Flexus_Orion_init(const Configuration &BookCfg);
/*Technology related parameters */
#define PARM_TECH_POINT 65
#define PARM_TRANSISTOR_TYPE NVT /* transistor type, HVT, NVT, or LVT */
//#define PARM_Vdd              1.0
extern double PARM_Vdd; //             1.0
//double PARM_Vdd =            1.0;
extern double PARM_Freq; //             0.746e9
#define PARM_VDD_V PARM_Vdd
#define PARM_FREQ_Hz PARM_Freq
extern double PARM_tr;

/* router module parameters */
extern int PARM_in_port;      // 		5	/* # of router input ports */
#define PARM_cache_in_port 0  /* # of cache input ports */
#define PARM_mc_in_port 0     /* # of memory controller input ports */
#define PARM_io_in_port 0     /* # of I/O device input ports */
extern int PARM_out_port;     //		5
#define PARM_cache_out_port 0 /* # of cache output ports */
#define PARM_mc_out_port 0    /* # of memory controller output ports */
#define PARM_io_out_port 0    /* # of I/O device output ports */
extern int PARM_flit_width;   //		64	/* flit width in bits */
extern float bitwidth64x;

/* virtual channel parameters */
extern int PARM_v_class;   //       1   /* # of total message classes */
extern int PARM_v_channel; //      1	/* # of virtual channels per virtual message class*/
#define PARM_cache_class 0 /* # of cache port virtual classes */
#define PARM_mc_class 0    /* # of memory controller port virtual classes */
#define PARM_io_class 0    /* # of I/O device port virtual classes */
/* ?? */
extern int PARM_in_share_buf;  //	0	/* do input virtual channels physically share buffers? */
extern int PARM_out_share_buf; //	0	/* do output virtual channels physically share buffers? */
/* ?? */
#define PARM_in_share_switch 1  /* do input virtual channels share crossbar input ports? */
#define PARM_out_share_switch 1 /* do output virtual channels share crossbar output ports? */

/* crossbar parameters */
extern int PARM_crossbar_model; //	MULTREE_CROSSBAR	/* crossbar model type MATRIX_CROSSBAR, MULTREE_CROSSBAR, or TRISTATE_CROSSBAR (only for Orion3.0)*/
extern int PARM_crsbar_degree;  //	4					/* crossbar mux degree */
extern int PARM_connect_type;   //	TRISTATE_GATE		/* crossbar connector type */
extern int PARM_trans_type;     //		NP_GATE				/* crossbar transmission gate type */
#define PARM_crossbar_in_len 0  /* crossbar input line length, if known */
#define PARM_crossbar_out_len 0 /* crossbar output line length, if known */
#define PARM_xb_in_seg 0
#define PARM_xb_out_seg 0
/* HACK HACK HACK */
#define PARM_exp_xb_model SIM_NO_MODEL /* the other parameter is MATRIX_CROSSBAR */
#define PARM_exp_in_seg 2
#define PARM_exp_out_seg 2

/* input buffer parameters */
extern int PARM_in_buf;         //			1		/* have input buffer? */
extern int PARM_in_buf_set;     //		1
#define PARM_in_buf_rport 1     /* # of read ports */
extern int PARM_in_buffer_type; //SRAM	/*buffer model type, SRAM or REGISTER*/

#define PARM_cache_in_buf 0
#define PARM_cache_in_buf_set 0
#define PARM_cache_in_buf_rport 0

#define PARM_mc_in_buf 0
#define PARM_mc_in_buf_set 0
#define PARM_mc_in_buf_rport 0

#define PARM_io_in_buf 0
#define PARM_io_in_buf_set 0
#define PARM_io_in_buf_rport 0

/* output buffer parameters */
extern int PARM_out_buf;     //		0
extern int PARM_out_buf_set; //		2
#define PARM_out_buf_wport 1
extern int PARM_out_buffer_type; //  	SRAM		/*buffer model type, SRAM or REGISTER*/

/* central buffer parameters */
#define PARM_central_buf 0 /* have central buffer? */
#define PARM_cbuf_set 2560 /* # of rows */
#define PARM_cbuf_rport 2  /* # of read ports */
#define PARM_cbuf_wport 2  /* # of write ports */
#define PARM_cbuf_width 4  /* # of flits in one row */
#define PARM_pipe_depth 4  /* # of banks */

/* array parameters shared by various buffers */
#define PARM_wordline_model CACHE_RW_WORDLINE
#define PARM_bitline_model RW_BITLINE
#define PARM_mem_model NORMAL_MEM
#define PARM_row_dec_model GENERIC_DEC
#define PARM_row_dec_pre_model SINGLE_OTHER
#define PARM_col_dec_model SIM_NO_MODEL
#define PARM_col_dec_pre_model SIM_NO_MODEL
#define PARM_mux_model SIM_NO_MODEL
#define PARM_outdrv_model REG_OUTDRV

/* these 3 should be changed together */
/* use double-ended bitline because the array is too large */
#define PARM_data_end 2
#define PARM_amp_model GENERIC_AMP
#define PARM_bitline_pre_model EQU_BITLINE
//#define PARM_data_end			1
//#define PARM_amp_model		SIM_NO_MODEL
//#define PARM_bitline_pre_model	SINGLE_OTHER

/* switch allocator arbiter parameters */
extern int PARM_sw_in_arb_model;         //	RR_ARBITER	/* input side arbiter model type, MATRIX_ARBITER , RR_ARBITER, QUEUE_ARBITER*/
#define PARM_sw_in_arb_ff_model NEG_DFF  /* input side arbiter flip-flop model type */
extern int PARM_sw_out_arb_model;        //	RR_ARBITER	/* output side arbiter model type, MATRIX_ARBITER */
#define PARM_sw_out_arb_ff_model NEG_DFF /* output side arbiter flip-flop model type */

/* virtual channel allocator arbiter parameters */
extern int PARM_vc_allocator_type;       //	TWO_STAGE_ARB	/*vc allocator type, ONE_STAGE_ARB, TWO_STAGE_ARB, VC_SELECT*/
extern int PARM_vc_in_arb_model;         //   	RR_ARBITER  /*input side arbiter model type for TWO_STAGE_ARB. MATRIX_ARBITER, RR_ARBITER, QUEUE_ARBITER*/
#define PARM_vc_in_arb_ff_model NEG_DFF  /* input side arbiter flip-flop model type */
extern int PARM_vc_out_arb_model;        //  	RR_ARBITER 	/*output side arbiter model type (for both ONE_STAGE_ARB and TWO_STAGE_ARB). MATRIX_ARBITER, RR_ARBITER, QUEUE_ARBITER */
#define PARM_vc_out_arb_ff_model NEG_DFF /* output side arbiter flip-flop model type */
#define PARM_vc_select_buf_type REGISTER /* vc_select buffer type, SRAM or REGISTER */

// link wire parameters//
#define WIRE_LAYER_TYPE INTERMEDIATE     //wire layer type, INTERMEDIATE or GLOBAL
#define PARM_width_spacing DWIDTH_DSPACE //choices are SWIDTH_SSPACE, SWIDTH_DSPACE, DWIDTH_SSPACE, DWIDTH_DSPACE
#define PARM_buffering_scheme MIN_DELAY  //choices are MIN_DELAY, STAGGERED
#define PARM_shielding FALSE             //choices are TRUE, FALSE

//clock power parameters//
#define PARM_pipeline_stages 4   //number of pipeline stages
#define PARM_H_tree_clock 0      //1 means calculate H_tree_clock power, 0 means not calculate H_tree_clock
#define PARM_router_diagonal 442 //router diagonal in micro-meter
/*
//link wire parameters
extern int WIRE_LAYER_TYPE ;//        INTERMEDIATE //wire layer type, INTERMEDIATE or GLOBAL
extern int PARM_width_spacing ;//     DWIDTH_DSPACE   //choices are SWIDTH_SSPACE, SWIDTH_DSPACE, DWIDTH_SSPACE, DWIDTH_DSPACE
extern int PARM_buffering_scheme;//   MIN_DELAY   	//choices are MIN_DELAY, STAGGERED 
extern int PARM_shielding   ;//       FALSE   		//choices are TRUE, FALSE 

clock power parameters
extern int PARM_pipeline_stages;//    4   	//number of pipeline stages//
extern int PARM_H_tree_clock    ;//   0       //1 means calculate H_tree_clock power, 0 means not calculate H_tree_clock
extern int PARM_router_diagonal ;//   442  	//router diagonal in micro-meter //

*/
/* RF module parameters */
#define PARM_read_port 1
#define PARM_write_port 1
#define PARM_n_regs 64
#define PARM_reg_width 32

#define PARM_ndwl 1
#define PARM_ndbl 1
#define PARM_nspd 1

#define PARM_POWER_STATS 1

#endif /* _SIM_PORT_H */