//
//
/*!

  \file
  \ingroup projection

  \brief Implementations of inline functions for class stir::ProjMatrixByBin

  \author Nikos Efthimiou
  \author Mustapha Sadki 
  \author Kris Thielemans
  \author PARAPET project

*/
/*
    Copyright (C) 2000 PARAPET partners
    Copyright (C) 2000- 2013, Hammersmith Imanet Ltd
    Copyright (C) 2016, University of Hull
    This file is part of STIR.

    SPDX-License-Identifier: Apache-2.0 AND License-ref-PARAPET-license

    See STIR/LICENSE.txt for details
*/
#include "stir/Succeeded.h"
#include "stir/recon_buildblock/SymmetryOperation.h"
#include "stir/geometry/line_distances.h"
#include "stir/numerics/erf.h"

START_NAMESPACE_STIR

const DataSymmetriesForBins*
ProjMatrixByBin:: get_symmetries_ptr() const
{
  return  symmetries_sptr.get();
}

const shared_ptr<DataSymmetriesForBins>
ProjMatrixByBin:: get_symmetries_sptr() const
{
  return  symmetries_sptr;
}

inline void 
ProjMatrixByBin::
get_proj_matrix_elems_for_one_bin(
                                  ProjMatrixElemsForOneBin& probabilities,
                                  const Bin& bin) STIR_MUTABLE_CONST
{  
  // start_timers(); TODO, can't do this in a const member
  
  // set to empty
  probabilities.erase();

  if (cache_stores_only_basic_bins)
  {
    // find basic bin
    Bin basic_bin = bin;    
    unique_ptr<SymmetryOperation> symm_ptr = 
      symmetries_sptr->find_symmetry_operation_from_basic_bin(basic_bin);
    
    probabilities.set_bin(basic_bin);
    // check if basic bin is in cache  
    if (get_cached_proj_matrix_elems_for_one_bin(probabilities) ==
      Succeeded::no)
    {
      // call 'calculate' just for the basic bin
      calculate_proj_matrix_elems_for_one_bin(probabilities);
#ifndef NDEBUG
      probabilities.check_state();
#endif
      cache_proj_matrix_elems_for_one_bin(probabilities);
    }
    if ( proj_data_info_sptr->is_tof_data() &&
                                 this->tof_enabled)
    {
        LORInAxialAndNoArcCorrSinogramCoordinates<float> lor;
        proj_data_info_sptr->get_LOR(lor, bin);
        LORAs2Points<float> lor2(lor);
        probabilities.set_bin(bin);
        // now apply TOF kernel and transform to original bin
        apply_tof_kernel_and_symm_transformation(probabilities, lor2.p1(), lor2.p2(), symm_ptr);
    }
    else
    {
        // now transform to original bin
        symm_ptr->transform_proj_matrix_elems_for_one_bin(probabilities);
    }
  }
  else // !cache_stores_only_basic_bins
  {
    probabilities.set_bin(bin);
    // check if in cache  
    if (get_cached_proj_matrix_elems_for_one_bin(probabilities) ==
      Succeeded::no)
    {
      // find basic bin
      Bin basic_bin = bin;  
      unique_ptr<SymmetryOperation> symm_ptr = 
        symmetries_sptr->find_symmetry_operation_from_basic_bin(basic_bin);

      probabilities.set_bin(basic_bin);
      // check if basic bin is in cache
      if (get_cached_proj_matrix_elems_for_one_bin(probabilities) ==
        Succeeded::no)
      {
        // call 'calculate' just for the basic bin
        calculate_proj_matrix_elems_for_one_bin(probabilities);
#ifndef NDEBUG
        probabilities.check_state();
#endif
        cache_proj_matrix_elems_for_one_bin(probabilities);
      }
      if ( proj_data_info_sptr->is_tof_data() &&
                                   this->tof_enabled)
      {
          LORInAxialAndNoArcCorrSinogramCoordinates<float> lor;
          proj_data_info_sptr->get_LOR(lor, bin);
          LORAs2Points<float> lor2(lor);
          probabilities.set_bin(bin);
          // now apply TOF kernel and transform to original bin
          apply_tof_kernel_and_symm_transformation(probabilities, lor2.p1(), lor2.p2(), symm_ptr);

      }
      else
      {
          // now transform to original bin
          symm_ptr->transform_proj_matrix_elems_for_one_bin(probabilities);
      }
      cache_proj_matrix_elems_for_one_bin(probabilities);      
    }
  }  
  // stop_timers(); TODO, can't do this in a const member
}

void
ProjMatrixByBin::apply_tof_kernel_and_symm_transformation(ProjMatrixElemsForOneBin& probabilities,
                                  const CartesianCoordinate3D<float>& point1,
                                  const CartesianCoordinate3D<float>& point2,
                                  const unique_ptr<SymmetryOperation>& symm_ptr)  STIR_MUTABLE_CONST
{

    float new_value = 0.f;
    float low_dist = 0.f;
    float high_dist = 0.f;

    // The direction can be from 1 -> 2 depending on the bin sign.
    const CartesianCoordinate3D<float> middle = (point1 + point2)*0.5f;
    const CartesianCoordinate3D<float> diff = point2 - middle;

    const CartesianCoordinate3D<float> diff_unit_vector(diff/static_cast<float>(norm(diff)));

    for (ProjMatrixElemsForOneBin::iterator element_ptr = probabilities.begin();
         element_ptr != probabilities.end(); ++element_ptr)
    {
        Coordinate3D<int> c(element_ptr->get_coords());
        symm_ptr->transform_image_coordinates(c);

        const float d2 = -inner_product(image_info_sptr->get_physical_coordinates_for_indices (c) - middle, diff_unit_vector);

        low_dist = ((proj_data_info_sptr->tof_bin_boundaries_mm[probabilities.get_bin_ptr()->timing_pos_num()].low_lim - d2) * r_sqrt2_gauss_sigma);
        high_dist = ((proj_data_info_sptr->tof_bin_boundaries_mm[probabilities.get_bin_ptr()->timing_pos_num()].high_lim - d2) * r_sqrt2_gauss_sigma);


        if ((low_dist >= 4.f && high_dist >= 4.f) ||
                (low_dist <= -4.f && high_dist <= -4.f))
        {
            *element_ptr = ProjMatrixElemsForOneBin::value_type(c, 0.0f);
            continue;
        }

        new_value = get_tof_value(low_dist, high_dist);
        new_value *= element_ptr->get_value();
        *element_ptr = ProjMatrixElemsForOneBin::value_type(c, new_value);
    }
}

float
ProjMatrixByBin::
get_tof_value(const float d1, const float d2) const
{
//    val = 0.5f * (erf(d2) - erf(d1));
    return 0.5f * (erf_interpolation(d2) - erf_interpolation(d1));
}

END_NAMESPACE_STIR
