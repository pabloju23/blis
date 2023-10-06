/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2022, The University of Texas at Austin
   Copyright (C) 2022, Oracle Labs, Oracle Corporation

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name(s) of the copyright holder(s) nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "blis.h"

#ifdef BLIS_ENABLE_LEVEL4

err_t bli_chol_l_opt_var1
     (
       const obj_t*  a,
       const cntx_t* cntx,
             rntm_t* rntm,
             cntl_t* cntl
     )
{
	num_t     dt        = bli_obj_dt( a );

	uplo_t    uploa     = bli_obj_uplo( a );
	dim_t     m         = bli_obj_length( a );
	void*     buf_a     = bli_obj_buffer_at_off( a );
	inc_t     rs_a      = bli_obj_row_stride( a );
	inc_t     cs_a      = bli_obj_col_stride( a );

	if ( bli_error_checking_is_enabled() )
		PASTEMAC(chol,_check)( a, cntx );

	// Query a type-specific function pointer, except one that uses
	// void* for function arguments instead of typed pointers.
	chol_opt_vft f = PASTEMAC(chol_l_opt_var1,_qfp)( dt );

	return
	f
	(
	  uploa,
	  m,
	  buf_a, rs_a, cs_a,
	  cntx,
	  rntm
	);
}

#undef  GENTFUNCR
#define GENTFUNCR( ctype, ctype_r, ch, chr, varname ) \
\
err_t PASTEMAC(ch,varname) \
     ( \
             uplo_t  uploa, \
             dim_t   m, \
             ctype*  a, inc_t rs_a, inc_t cs_a, \
       const cntx_t* cntx, \
             rntm_t* rntm  \
     ) \
{ \
	const ctype minus_one = *PASTEMAC(ch,m1); \
	const ctype one       = *PASTEMAC(ch,1); \
\
	for ( dim_t i = 0; i < m; ++i ) \
	{ \
		const dim_t mn_behind = i; \
\
		/* Identify subpartitions: /  a00                \
		                           |  a10  alpha11       |
		                           \  ---  ---      ---  / */ \
		ctype*   a00       = a + (0  )*rs_a + (0  )*cs_a; \
		ctype*   a10       = a + (i  )*rs_a + (0  )*cs_a; \
		ctype*   alpha11   = a + (i  )*rs_a + (i  )*cs_a; \
\
		ctype_r* alpha11_r = &PASTEMAC(ch,real)( *alpha11 ); \
\
		/* a10   = a10 / tril( A00 )';
		   a10^T = conj( tril( A00 ) ) / a10^T */ \
		PASTEMAC2(ch,trsv,BLIS_TAPI_EX_SUF) \
		( \
		  uploa, \
		  BLIS_CONJ_NO_TRANSPOSE, \
		  BLIS_NONUNIT_DIAG, \
		  mn_behind, \
		  &one, \
		  a00, rs_a, cs_a, \
		  a10, cs_a, \
		  cntx, \
		  rntm \
		); \
\
		/* [ alpha11, 0.0 ] = alpha11 - a10 * a10'; */ \
		PASTEMAC2(ch,dotxv,BLIS_TAPI_EX_SUF) \
		( \
		  BLIS_NO_CONJUGATE, \
		  BLIS_CONJUGATE, \
		  mn_behind, \
		  &minus_one, \
		  a10, cs_a, \
		  a10, cs_a, \
		  &one, \
		  alpha11, \
		  cntx, \
		  rntm \
		); \
		PASTEMAC(ch,seti0s)( *alpha11 ); \
\
		/* Return an error code if the matrix is not Hermitian positive
		   definite. */ \
		if ( PASTEMAC(chr,lte0)( *alpha11_r ) ) return mn_behind + 1; \
\
		/* alpha11 = sqrt( real(alpha11) ); */ \
		PASTEMAC(chr,sqrt2s)( *alpha11_r, *alpha11_r ); \
	} \
\
	return BLIS_SUCCESS; \
}

INSERT_GENTFUNCR_BASIC0( chol_l_opt_var1 )

#endif