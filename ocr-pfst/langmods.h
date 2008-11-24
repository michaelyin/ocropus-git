#ifndef langmods_h__
#define langmods_h__

#ifdef HAVE_FST
#undef CHECK
#ifdef GOOGLE_INTERNAL
#include "nlp/fst/lib/fst-decl.h"
#include "nlp/fst/lib/fst-inl.h"
#include "nlp/fst/lib/fstlib-inl.h"
namespace fst = nlp_fst;
#else
#include "fst/lib/fst.h"
#include "fst/lib/fstlib.h"
#endif
#undef CHECK
#include "fstutil.h"
#include "fstmodels.h"
#include "fstbuilder.h"
#else
#warning building without HAVE_FST
#endif
#include "a-star.h"
#include "beam-search.h"
#include "fst-em.h"
#include "fst-io.h"
#include "langmod-shortest-path.h"
#include "lattice.h"

#endif
