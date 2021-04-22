//
//
/*
    Copyright (C) 2003- 2011, Hammersmith Imanet Ltd
    This file is part of STIR.

    SPDX-License-Identifier: Apache-2.0

    See STIR/LICENSE.txt for details
*/
/*!
  \file
  \ingroup normalisation

  \brief Implementation for class ChainedBinNormalisation

  \author Kris Thielemans
*/


#include "stir/recon_buildblock/ChainedBinNormalisation.h"
#include "stir/is_null_ptr.h"
#include "stir/Succeeded.h"

START_NAMESPACE_STIR

const char * const 
ChainedBinNormalisation::registered_name = "Chained"; 

void 
ChainedBinNormalisation::set_defaults()
{
  apply_first.reset();
  apply_second.reset();
}

void 
ChainedBinNormalisation::
initialise_keymap()
{
  parser.add_start_key("Chained Bin Normalisation Parameters");
  parser.add_parsing_key("Bin Normalisation to apply first", &apply_first);
  parser.add_parsing_key("Bin Normalisation to apply second", &apply_second);
  parser.add_stop_key("END Chained Bin Normalisation Parameters");}

bool 
ChainedBinNormalisation::
post_processing()
{
  return false;
}


ChainedBinNormalisation::
ChainedBinNormalisation()
{
  set_defaults();
}

ChainedBinNormalisation::
ChainedBinNormalisation(shared_ptr<BinNormalisation> const& apply_first_v,
		        shared_ptr<BinNormalisation> const& apply_second_v)
  : apply_first(apply_first_v),
    apply_second(apply_second_v)
{
  post_processing();
}

Succeeded
ChainedBinNormalisation::
set_up(const shared_ptr<ProjDataInfo>& proj_data_info_ptr)
{
  BinNormalisation::set_up(proj_data_info_ptr);
  if (!is_null_ptr(apply_first))
    if (apply_first->set_up(proj_data_info_ptr) == Succeeded::no)
      return  Succeeded::no;
  if (!is_null_ptr(apply_second))
    return apply_second->set_up(proj_data_info_ptr);
  else
    return Succeeded::yes;  
}


void 
ChainedBinNormalisation::apply(RelatedViewgrams<float>& viewgrams,const double start_time, const double end_time) const 
{
  if (!is_null_ptr(apply_first))
    apply_first->apply(viewgrams,start_time,end_time);
  if (!is_null_ptr(apply_second))
    apply_second->apply(viewgrams,start_time,end_time);
}

void
ChainedBinNormalisation::apply(ProjData& proj_data,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_first))
    apply_first->apply(proj_data,start_time,end_time);
  if (!is_null_ptr(apply_second))
    apply_second->apply(proj_data,start_time,end_time);
}

void
ChainedBinNormalisation::apply_only_first(RelatedViewgrams<float>& viewgrams,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_first))
    apply_first->apply(viewgrams,start_time,end_time);
}

void
ChainedBinNormalisation::apply_only_first(ProjData& proj_data,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_first))
    apply_first->apply(proj_data,start_time,end_time);
}

void
ChainedBinNormalisation::apply_only_second(RelatedViewgrams<float>& viewgrams,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_second))
    apply_second->apply(viewgrams,start_time,end_time);
}

void
ChainedBinNormalisation::apply_only_second(ProjData& proj_data,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_second))
    apply_second->apply(proj_data,start_time,end_time);
}

void 
ChainedBinNormalisation::
undo(RelatedViewgrams<float>& viewgrams,const double start_time, const double end_time) const 
{
  if (!is_null_ptr(apply_first))
    apply_first->undo(viewgrams,start_time,end_time);
  if (!is_null_ptr(apply_second))
    apply_second->undo(viewgrams,start_time,end_time);
}

void
ChainedBinNormalisation::
undo(ProjData& proj_data,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_first))
    apply_first->undo(proj_data,start_time,end_time);
  if (!is_null_ptr(apply_second))
    apply_second->undo(proj_data,start_time,end_time);
}

void
ChainedBinNormalisation::
undo_only_first(RelatedViewgrams<float>& viewgrams,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_first))
    apply_first->undo(viewgrams,start_time,end_time);
}

void
ChainedBinNormalisation::
undo_only_first(ProjData& proj_data,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_first))
    apply_first->undo(proj_data,start_time,end_time);
}

void
ChainedBinNormalisation::
undo_only_second(RelatedViewgrams<float>& viewgrams,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_second))
    apply_second->undo(viewgrams,start_time,end_time);
}

void
ChainedBinNormalisation::
undo_only_second(ProjData& proj_data,const double start_time, const double end_time) const
{
  if (!is_null_ptr(apply_second))
    apply_second->undo(proj_data,start_time,end_time);
}

float
ChainedBinNormalisation:: get_bin_efficiency(const Bin& bin,const double start_time, const double end_time) const
{
  return 
    (!is_null_ptr(apply_first) 
     ? apply_first->get_bin_efficiency(bin,start_time,end_time)
     : 1)
    *
    (!is_null_ptr(apply_second) 
     ? apply_second->get_bin_efficiency(bin,start_time,end_time)
     : 1);
}
 
bool
ChainedBinNormalisation::is_first_trivial() const
{
    if (is_null_ptr(apply_first))
        error("First Normalisation object has not been set.");
    return apply_first->is_trivial();
}

bool
ChainedBinNormalisation::is_second_trivial() const
{
    if (is_null_ptr(apply_second))
        error("Second Normalisation object has not been set.");
    return apply_second->is_trivial();
}

shared_ptr<BinNormalisation>
ChainedBinNormalisation::get_first_norm() const
{
    if (is_null_ptr(apply_first))
        error("First Normalisation object has not been set.");
    return apply_first;
}

shared_ptr<BinNormalisation>
ChainedBinNormalisation::get_second_norm() const
{
    if (is_null_ptr(apply_second))
        error("Second Normalisation object has not been set.");
    return apply_second;
}
 
END_NAMESPACE_STIR

