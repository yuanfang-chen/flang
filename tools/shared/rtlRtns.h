/*
 * Copyright (c) 2016-2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef FORTRAN_RTLRTNS_H_
#define FORTRAN_RTLRTNS_H_

/**
   \file
   \brief Declarations used to access the run time library structure defined in
   rte_rtns.h.
 */

#include "rtlRtnsDesc.h"

/** \brief Enumerator for some (eventually all) the RTL library routines.
 *
 * NOTE: within each section (E.g., NO RTN to END_OF_PFX_F90,
 * END_OF_PFX_F90+1 to ...) the ftnRtlRtns entries must be sorted on the
 * baseNm field of the ftnRtlRtns or, equivalently, the order of the enum
 * names.
 */
typedef enum {
  RTE_no_rtn,
  RTE_achar,
  RTE_addr_1_dim_1st_elem,
  RTE_adjustl,
  RTE_adjustr,
  RTE_alloc,
  RTE_alloc03,
  RTE_alloc03_chk,
  RTE_alloc04,
  RTE_alloc04_chk,
  RTE_alloc04_chkm,
  RTE_alloc04_chkp,
  RTE_alloc04m,
  RTE_alloc04p,
  RTE_allocated,
  RTE_allocated2,
  RTE_allocated_lhs,
  RTE_allocated_lhs2,
  RTE_amodulev,
  RTE_amodulov,
  RTE_asn_closure,
  RTE_auto_alloc,
  RTE_auto_alloc04,
  RTE_auto_alloc04m,
  RTE_auto_alloc04p,
  RTE_auto_allocv,
  RTE_auto_calloc,
  RTE_auto_calloc04,
  RTE_auto_calloc04m,
  RTE_auto_calloc04p,
  RTE_auto_dealloc,
  RTE_auto_deallocm,
  RTE_auto_deallocp,
  RTE_c_f_procptr,
  RTE_c_f_ptr,
  RTE_calloc03,
  RTE_calloc04,
  RTE_calloc04m,
  RTE_calloc04p,
  RTE_ceiling,
  RTE_ceilingv,
  RTE_class_obj_size,
  RTE_cmd_arg_cnt,
  RTE_cmplx16,
  RTE_cmplx32,
  RTE_cmplx8,
  RTE_conformable,
  /* diff from previous three is to
   * pass value, not address */
  RTE_conformable_11v,
  RTE_conformable_1dv,
  RTE_conformable_22v,
  RTE_conformable_2dv,
  RTE_conformable_33v,
  RTE_conformable_3dv,
  RTE_conformable_d1v,
  RTE_conformable_d2v,
  RTE_conformable_d3v,
  RTE_conformable_dd,
  RTE_conformable_dnv,
  RTE_conformable_ndv,
  RTE_conformable_nnv,
  RTE_copy_f77_argl,
  RTE_copy_f77_argsl,
  RTE_copy_f90_argl,
  RTE_copy_proc_desc,
  RTE_dble,
  RTE_dceiling,
  RTE_dceilingv,
  RTE_dealloc,
  RTE_dealloc03,
  RTE_dealloc03m,
  RTE_dealloc03p,
  RTE_dealloc_mbr03,
  RTE_dealloc_mbr03m,
  RTE_dealloc_mbr03p,
  RTE_dealloc_poly03,
  RTE_dealloc_poly_mbr03,
  RTE_deallocx,
  RTE_dfloor,
  RTE_dfloorv,
  RTE_dmodulev,
  RTE_dmodulov,
  RTE_exit,
  RTE_expon,
  RTE_expond,
  RTE_expondx,
  RTE_exponx,
  RTE_extends_type_of,
  RTE_finalize,
  RTE_floor,
  RTE_floorv,
  RTE_frac,
  RTE_fracd,
  RTE_fracdx,
  RTE_fracx,
  RTE_get_cmd,
  RTE_get_cmd_arg,
  RTE_get_env_var,
  RTE_hypot,
  RTE_hypotd,
  RTE_i8modulov,
  RTE_iachar,
  RTE_ichar,
  RTE_imodulov,
  RTE_index,
  RTE_init_from_desc,
  RTE_init_unl_poly_desc,
  RTE_int,
  RTE_int1,
  RTE_int2,
  RTE_int4,
  RTE_int8,
  RTE_is_contiguous,
  RTE_is_iostat_end,
  RTE_is_iostat_eor,
  RTE_kexpondx,
  RTE_ksize,
  RTE_lb,
  RTE_lb1,
  RTE_lb2,
  RTE_lb4,
  RTE_lb8,
  RTE_lba,
  RTE_lba1,
  RTE_lba2,
  RTE_lba4,
  RTE_lba8,
  RTE_lbaz,
  RTE_lbaz1,
  RTE_lbaz2,
  RTE_lbaz4,
  RTE_lbaz8,
  RTE_lbound,
  RTE_lbound1,
  RTE_lbound2,
  RTE_lbound4,
  RTE_lbound8,
  RTE_lbounda,
  RTE_lbounda1,
  RTE_lbounda2,
  RTE_lbounda4,
  RTE_lbounda8,
  RTE_lboundaz,
  RTE_lboundaz1,
  RTE_lboundaz2,
  RTE_lboundaz4,
  RTE_lboundaz8,
  RTE_len,
  RTE_lentrim,
  RTE_loc,
  RTE_log1,
  RTE_log2,
  RTE_log4,
  RTE_log8,
  RTE_matmul_cplx16,
  RTE_matmul_cplx16mxv_t,
  RTE_matmul_cplx32,
  RTE_matmul_cplx8,
  RTE_matmul_cplx8mxv_t,
  RTE_matmul_int1,
  RTE_matmul_int2,
  RTE_matmul_int4,
  RTE_matmul_int8,
  RTE_matmul_log1,
  RTE_matmul_log2,
  RTE_matmul_log4,
  RTE_matmul_log8,
  RTE_matmul_real16,
  RTE_matmul_real4,
  RTE_matmul_real4mxv_t,
  RTE_matmul_real8,
  RTE_matmul_real8mxv_t,
  RTE_max,
  RTE_mcopy1,
  RTE_mcopy2,
  RTE_mcopy4,
  RTE_mcopy8,
  RTE_mcopyz16,
  RTE_mcopyz4,
  RTE_mcopyz8,
  RTE_mergec,
  RTE_mergech,
  RTE_merged,
  RTE_mergedc,
  RTE_mergedt,
  RTE_mergei,
  RTE_mergei1,
  RTE_mergei2,
  RTE_mergei8,
  RTE_mergel,
  RTE_mergel1,
  RTE_mergel2,
  RTE_mergel8,
  RTE_mergeq,
  RTE_merger,
  RTE_min,
  RTE_mmul_cmplx16,
  RTE_mmul_cmplx8,
  RTE_mmul_real4,
  RTE_mmul_real8,
  RTE_modulov,
  RTE_move_alloc,
  RTE_mp_bcs_nest,
  RTE_mp_ecs_nest,
  RTE_mset1,
  RTE_mset2,
  RTE_mset4,
  RTE_mset8,
  RTE_msetz16,
  RTE_msetz4,
  RTE_msetz8,
  RTE_mvbits,
  RTE_mzero1,
  RTE_mzero2,
  RTE_mzero4,
  RTE_mzero8,
  RTE_mzeroz16,
  RTE_mzeroz4,
  RTE_mzeroz8,
  RTE_nadjustl,
  RTE_nadjustr,
  RTE_name,
  RTE_nearest,
  RTE_nearestd,
  RTE_nearestdx,
  RTE_nearestx,
  RTE_nlen,
  RTE_nlentrim,
  RTE_nrepeat,
  RTE_nscan,
  RTE_nstr_copy,
  RTE_nstr_copy_klen,
  RTE_nstr_index,
  RTE_nstr_index_klen,
  RTE_nstrcmp,
  RTE_nstrcmp_klen,
  RTE_ntrim,
  RTE_nverify,
  RTE_pause,
  RTE_poly_asn,
  RTE_present,
  RTE_present_ptr,
  RTE_presentc,
  RTE_ptr_alloc,
  RTE_ptr_alloc03,
  RTE_ptr_alloc04,
  RTE_ptr_alloc04m,
  RTE_ptr_alloc04p,
  RTE_ptr_calloc03,
  RTE_ptr_calloc04,
  RTE_ptr_calloc04m,
  RTE_ptr_calloc04p,
  RTE_ptr_src_alloc03,
  RTE_ptr_src_alloc04,
  RTE_ptr_src_alloc04m,
  RTE_ptr_src_alloc04p,
  RTE_ptr_src_calloc03,
  RTE_ptr_src_calloc04,
  RTE_ptr_src_calloc04m,
  RTE_ptr_src_calloc04p,
  RTE_ptrchk,
  RTE_ptrcp,
  RTE_real,
  RTE_real16,
  RTE_real4,
  RTE_real8,
  RTE_repeat,
  RTE_rrspacing,
  RTE_rrspacingd,
  RTE_rrspacingdx,
  RTE_rrspacingx,
  RTE_rtn_name,
  RTE_same_intrin_type_as,
  RTE_same_type_as,
  RTE_scale,
  RTE_scaled,
  RTE_scaledx,
  RTE_scalex,
  RTE_scan,
  RTE_sect,
  RTE_sect1,
  RTE_sect1v,
  RTE_sect2,
  RTE_sect2v,
  RTE_sect3,
  RTE_sect3v,
  RTE_sel_char_kind,
  RTE_sel_int_kind,
  RTE_sel_real_kind,
  RTE_set_intrin_type,
  RTE_set_type,
  RTE_setexp,
  RTE_setexpd,
  RTE_setexpdx,
  RTE_setexpx,
  RTE_shape,
  RTE_shape1,
  RTE_shape2,
  RTE_shape4,
  RTE_shape8,
  RTE_show,
  RTE_size,
  RTE_spacing,
  RTE_spacingd,
  RTE_spacingdx,
  RTE_spacingx,
  RTE_stop,
  RTE_stop08,
  RTE_str_copy,
  RTE_str_copy_klen,
  RTE_str_cpy1,
  RTE_str_free,
  RTE_str_index,
  RTE_str_index_klen,
  RTE_str_malloc,
  RTE_str_malloc_klen,
  RTE_strcmp,
  RTE_strcmp_klen,
  RTE_subchk,
  RTE_subchk64,
  RTE_template,
  RTE_template1,
  RTE_template1v,
  RTE_template2,
  RTE_template2v,
  RTE_template3,
  RTE_template3v,
  RTE_test_and_set_type,
  RTE_trim,
  RTE_ub,
  RTE_ub1,
  RTE_ub2,
  RTE_ub4,
  RTE_ub8,
  RTE_uba,
  RTE_uba1,
  RTE_uba2,
  RTE_uba4,
  RTE_uba8,
  RTE_ubaz,
  RTE_ubaz1,
  RTE_ubaz2,
  RTE_ubaz4,
  RTE_ubaz8,
  RTE_ubound,
  RTE_ubound1,
  RTE_ubound2,
  RTE_ubound4,
  RTE_ubound8,
  RTE_ubounda,
  RTE_ubounda1,
  RTE_ubounda2,
  RTE_ubounda4,
  RTE_ubounda8,
  RTE_uboundaz,
  RTE_uboundaz1,
  RTE_uboundaz2,
  RTE_uboundaz4,
  RTE_uboundaz8,
  RTE_verify,
  END_OF_PFX_F90,
  RTE_all,
  RTE_all_scatterx,
  RTE_alls,
  RTE_any,
  RTE_any_scatterx,
  RTE_anys,
  RTE_associated,
  RTE_associated_char,
  RTE_associated_t,
  RTE_associated_tchar,
  RTE_barrier,
  RTE_block_loop,
  RTE_comm_copy,
  RTE_comm_free,
  RTE_comm_gatherx,
  RTE_comm_scatterx,
  RTE_copy_out,
  RTE_count,
  RTE_counts,
  RTE_cpu_time,
  RTE_cpu_timed,
  RTE_cshift,
  RTE_cshiftc,
  RTE_cshifts,
  RTE_cshiftsc,
  RTE_cyclic_loop,
  RTE_dandt,
  RTE_date,
  RTE_datew,
  RTE_dotpr,
  RTE_eoshift,
  RTE_eoshiftc,
  RTE_eoshifts,
  RTE_eoshiftsa,
  RTE_eoshiftsac,
  RTE_eoshiftsc,
  RTE_eoshiftss,
  RTE_eoshiftssc,
  RTE_eoshiftsz,
  RTE_eoshiftszc,
  RTE_eoshiftz,
  RTE_eoshiftzc,
  RTE_extent,
  RTE_findloc,
  RTE_findlocs,
  RTE_findlocstr,
  RTE_findlocstrs,
  RTE_free,
  RTE_freen,
  RTE_ftime,
  RTE_ftimew,
  RTE_function_entry,
  RTE_function_exit,
  RTE_get_scalar,
  RTE_global_all,
  RTE_global_any,
  RTE_global_firstmax,
  RTE_global_firstmin,
  RTE_global_iall,
  RTE_global_iany,
  RTE_global_iparity,
  RTE_global_lastmax,
  RTE_global_lastmin,
  RTE_global_maxval,
  RTE_global_minval,
  RTE_global_parity,
  RTE_global_product,
  RTE_global_sum,
  RTE_globalize,
  RTE_iall_scatterx,
  RTE_iany_scatterx,
  RTE_idate,
  RTE_ilen,
  RTE_indexDsc,
  RTE_indexx,
  RTE_indexx_cr,
  RTE_indexx_cr_nm,
  RTE_init,
  RTE_instance,
  RTE_iparity_scatterx,
  RTE_islocal_idx,
  RTE_jdate,
  RTE_lastval,
  RTE_lbound1Dsc,
  RTE_lbound2Dsc,
  RTE_lbound4Dsc,
  RTE_lbound8Dsc,
  RTE_lboundDsc,
  RTE_lbounda1Dsc,
  RTE_lbounda2Dsc,
  RTE_lbounda4Dsc,
  RTE_lbounda8Dsc,
  RTE_lboundaDsc,
  RTE_lboundaz1Dsc,
  RTE_lboundaz2Dsc,
  RTE_lboundaz4Dsc,
  RTE_lboundaz8Dsc,
  RTE_lboundazDsc,
  RTE_leadz,
  RTE_lenDsc,
  RTE_lenx,
  RTE_lenx_cr,
  RTE_lenx_cr_nm,
  RTE_line_entry,
  RTE_lineno,
  RTE_localize_bounds,
  RTE_localize_index,
  RTE_matmul,
  RTE_maxloc,
  RTE_maxlocs,
  RTE_maxval,
  RTE_maxval_scatterx,
  RTE_maxvals,
  RTE_member_base,
  RTE_minloc,
  RTE_minlocs,
  RTE_minval,
  RTE_minval_scatterx,
  RTE_minvals,
  RTE_np,
  RTE_nullify,
  RTE_nullify_char,
  RTE_nullifyx,
  RTE_number_of_processors,
  RTE_olap_cshift,
  RTE_olap_eoshift,
  RTE_olap_shift,
  RTE_pack,
  RTE_packc,
  RTE_packz,
  RTE_packzc,
  RTE_parity_scatterx,
  RTE_permute_section,
  RTE_popcnt,
  RTE_poppar,
  RTE_processors,
  RTE_processors_rank,
  RTE_product,
  RTE_product_scatterx,
  RTE_products,
  RTE_ptr_asgn,
  RTE_ptr_asgn_char,
  RTE_ptr_assign,
  RTE_ptr_assign_char,
  RTE_ptr_assign_charx,
  RTE_ptr_assignx,
  RTE_ptr_assn,
  RTE_ptr_assn_assumeshp,
  RTE_ptr_assn_char,
  RTE_ptr_assn_char_assumeshp,
  RTE_ptr_assn_charx,
  RTE_ptr_assn_dchar,
  RTE_ptr_assn_dchar_assumeshp,
  RTE_ptr_assn_dcharx,
  RTE_ptr_assnx,
  RTE_ptr_fix_assumeshp,
  RTE_ptr_fix_assumeshp1,
  RTE_ptr_fix_assumeshp2,
  RTE_ptr_fix_assumeshp3,
  RTE_ptr_in,
  RTE_ptr_in_char,
  RTE_ptr_offset,
  RTE_ptr_out,
  RTE_ptr_out_char,
  RTE_ptr_shape_assn,
  RTE_ptr_shape_assnx,
  RTE_qopy_in,
  RTE_realign,
  RTE_redistribute,
  RTE_reduce_descriptor,
  RTE_reshape,
  RTE_reshapec,
  RTE_rnum,
  RTE_rnumd,
  RTE_rseed,
  RTE_secnds,
  RTE_secndsd,
  RTE_shapeDsc,
  RTE_sizeDsc,
  RTE_spread,
  RTE_spread_descriptor,
  RTE_spreadc,
  RTE_spreadcs,
  RTE_spreads,
  RTE_sum,
  RTE_sum_scatterx,
  RTE_sums,
  RTE_sysclk,
  RTE_templateDsc,
  RTE_transfer,
  RTE_type,
  RTE_typep,
  RTE_ubound1Dsc,
  RTE_ubound2Dsc,
  RTE_ubound4Dsc,
  RTE_ubound8Dsc,
  RTE_uboundDsc,
  RTE_ubounda1Dsc,
  RTE_ubounda2Dsc,
  RTE_ubounda4Dsc,
  RTE_ubounda8Dsc,
  RTE_uboundaDsc,
  RTE_uboundaz1Dsc,
  RTE_uboundaz2Dsc,
  RTE_uboundaz4Dsc,
  RTE_uboundaz8Dsc,
  RTE_uboundazDsc,
  RTE_unpack,
  RTE_unpackc,
  END_OF_PFX_FTN,
  RTE_f90io_aux_init,
  RTE_f90io_backspace,
  RTE_f90io_begin,
  RTE_f90io_byte_read,
  RTE_f90io_byte_read64,
  RTE_f90io_byte_write,
  RTE_f90io_byte_write64,
  RTE_f90io_close,
  RTE_f90io_dts_fmtr,
  RTE_f90io_dts_fmtw,
  RTE_f90io_dts_stat,
  RTE_f90io_encode_fmt,
  RTE_f90io_encode_fmtv,
  RTE_f90io_end,
  RTE_f90io_endfile,
  RTE_f90io_flush,
  RTE_f90io_fmt_read,
  RTE_f90io_fmt_read64_a,
  RTE_f90io_fmt_read_a,
  RTE_f90io_fmt_write,
  RTE_f90io_fmt_write64_a,
  RTE_f90io_fmt_write_a,
  RTE_f90io_fmtr_end,
  RTE_f90io_fmtr_init,
  RTE_f90io_fmtr_init03,
  RTE_f90io_fmtr_init2003,
  RTE_f90io_fmtr_initv,
  RTE_f90io_fmtr_initv2003,
  RTE_f90io_fmtr_intern_init,
  RTE_f90io_fmtr_intern_inite,
  RTE_f90io_fmtr_intern_initev,
  RTE_f90io_fmtr_intern_initv,
  RTE_f90io_fmtw_end,
  RTE_f90io_fmtw_init,
  RTE_f90io_fmtw_init03,
  RTE_f90io_fmtw_initv,
  RTE_f90io_fmtw_intern_init,
  RTE_f90io_fmtw_intern_inite,
  RTE_f90io_fmtw_intern_initev,
  RTE_f90io_fmtw_intern_initv,
  RTE_f90io_get_newunit,
  RTE_f90io_inquire,
  RTE_f90io_inquire03,
  RTE_f90io_inquire03_2,
  RTE_f90io_inquire2003,
  RTE_f90io_iomsg,
  RTE_f90io_iomsg_,
  RTE_f90io_ldr,
  RTE_f90io_ldr64_a,
  RTE_f90io_ldr_a,
  RTE_f90io_ldr_end,
  RTE_f90io_ldr_init,
  RTE_f90io_ldr_init03,
  RTE_f90io_ldr_intern_init,
  RTE_f90io_ldr_intern_inite,
  RTE_f90io_ldw,
  RTE_f90io_ldw64_a,
  RTE_f90io_ldw_a,
  RTE_f90io_ldw_end,
  RTE_f90io_ldw_init,
  RTE_f90io_ldw_init03,
  RTE_f90io_ldw_intern_init,
  RTE_f90io_ldw_intern_inite,
  RTE_f90io_nml_read,
  RTE_f90io_nml_write,
  RTE_f90io_nmlr,
  RTE_f90io_nmlr_end,
  RTE_f90io_nmlr_init,
  RTE_f90io_nmlr_init03,
  RTE_f90io_nmlr_intern_init,
  RTE_f90io_nmlw,
  RTE_f90io_nmlw_end,
  RTE_f90io_nmlw_init,
  RTE_f90io_nmlw_init03,
  RTE_f90io_nmlw_intern_init,
  RTE_f90io_open03,
  RTE_f90io_open2003,
  RTE_f90io_open_async,
  RTE_f90io_open_cvt,
  RTE_f90io_open_share,
  RTE_f90io_print_init,
  RTE_f90io_rewind,
  RTE_f90io_sc_cd_fmt_write,
  RTE_f90io_sc_cd_ldw,
  RTE_f90io_sc_cf_fmt_write,
  RTE_f90io_sc_cf_ldw,
  RTE_f90io_sc_ch_fmt_write,
  RTE_f90io_sc_ch_ldw,
  RTE_f90io_sc_d_fmt_write,
  RTE_f90io_sc_d_ldw,
  RTE_f90io_sc_f_fmt_write,
  RTE_f90io_sc_f_ldw,
  RTE_f90io_sc_fmt_write,
  RTE_f90io_sc_i_fmt_write,
  RTE_f90io_sc_i_ldw,
  RTE_f90io_sc_l_fmt_write,
  RTE_f90io_sc_l_ldw,
  RTE_f90io_sc_ldw,
  RTE_f90io_src_info03,
  RTE_f90io_src_infox03,
  RTE_f90io_swbackspace,
  RTE_f90io_unf_async,
  RTE_f90io_unf_end,
  RTE_f90io_unf_init,
  RTE_f90io_unf_read,
  RTE_f90io_unf_read64_a,
  RTE_f90io_unf_read_a,
  RTE_f90io_unf_write,
  RTE_f90io_unf_write64_a,
  RTE_f90io_unf_write_a,
  RTE_f90io_usw_end,
  RTE_f90io_usw_init,
  RTE_f90io_usw_read,
  RTE_f90io_usw_read64_a,
  RTE_f90io_usw_read_a,
  RTE_f90io_usw_write,
  RTE_f90io_usw_write64_a,
  RTE_f90io_usw_write_a,
  RTE_f90io_wait,
  END_OF_IO,
  RTE_io_fmt_read,
  RTE_io_fmt_read64,
  RTE_io_fmt_write,
  RTE_io_fmt_write64,
  RTE_io_ldr,
  RTE_io_ldr64,
  RTE_io_ldw,
  RTE_io_ldw64,
  RTE_io_unf_read,
  RTE_io_unf_read64,
  RTE_io_unf_write,
  RTE_io_unf_write64,
  RTE_io_usw_read,
  RTE_io_usw_read64,
  RTE_io_usw_write,
  RTE_io_usw_write64,
  END_OF_FTNIO
} FtnRtlEnum;


/** \brief "ftype" information about those RTL routines where the 1st argument
 *  is a section descriptor that is being created/modified.  The int return by
 *  getF90TmplSectRtn is a bitfield composed of some combination of these
 *  values.
 */
#define FTYPE_SECT 1
#define FTYPE_TEMPLATE 2
#define FTYPE_TEMPLATE1 3
#define FTYPE_TEMPLATE1V 4
#define FTYPE_TEMPLATE2 5
#define FTYPE_TEMPLATE2V 6
#define FTYPE_TEMPLATE3 7
#define FTYPE_TEMPLATE3V 8
#define FTYPE_I8 0x10
#define FTYPE_MASK 0x0f

/**
   \brief ...
 */
char *mkRteRtnNm(FtnRtlEnum rteRtn);

/** \brief Returns an integer bitfield contain information about a RTL routine
 *  that creates or modifies an array descriptor.  If the routine does not
 *  create or modify an array descriptor, zero is returned.
 */
int getF90TmplSectRtn(char *rtnNm);

/**
   \brief ...
 */
void dump_FtnRteRtn(FtnRtlEnum rteRtn);

#endif /* FORTRAN_RTLRTNS_H_ */
