#ifndef __stir_listmode_LmToTimeDiffs_H__
#define __stir_listmode_LmToTimeDiffs_H__

#include "stir/listmode/LmToProjData.h"


START_NAMESPACE_STIR


class LmToTimeDiffs : public LmToProjData
{
public:

  //! Constructor taking a filename for a parameter file
  /*! Will attempt to open and parse the file. */
  LmToTimeDiffs(const char * const par_filename);

  //! Default constructor
  /*! \warning leaves parameters ill-defined. Set them by parsing. */
  LmToTimeDiffs();

  virtual void process_data();

};

END_NAMESPACE_STIR

#endif
