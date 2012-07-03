/*****************************************************************
|
|    Copyright 2004-2006 Axiomatic Systems LLC
|
|    This file is part of Melo (Melo AAC Decoder).
|
|    Unless you have obtained Melo under a difference license,
|    this version of Melo is Melo|GPL.
|    Melo|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Melo|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Melo|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/
#include "MloDebug.h"
#include "MloIndivChnPool.h"
#include "MloIndivChnStream.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/
static void  MLO_IndivChnPool_EraseChannels (MLO_IndivChnPool *pool_ptr);

/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/
/*
==============================================================================
Name: MLO_IndivChnPool_Create
Description:
   Initialises the object, and allocates memory for the given number of
   channels. Call this function only once, before any other.
Input parameters:
	- nbr_chn: Initial channel capacity, > 0
Output parameters:
	- pool_ptr: Object to initialise.
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_OUT_OF_MEMORY if... guess what?
==============================================================================
*/

MLO_Result  MLO_IndivChnPool_Create (MLO_IndivChnPool *pool_ptr, int nbr_chn)
{
    int            chn_cnt;

    MLO_ASSERT (pool_ptr != NULL);
	MLO_ASSERT (nbr_chn > 0);
    MLO_ASSERT (nbr_chn <= MLO_DEFS_MAX_CHN);

    pool_ptr->nbr_alloc_chn = 0;
    pool_ptr->nbr_chn = 0;
    for (chn_cnt = 0
      ;  chn_cnt < (int)MLO_ARRAY_SIZE (pool_ptr->chn_ptr_arr)
      ;  ++chn_cnt)
   {
      pool_ptr->chn_ptr_arr [chn_cnt] = 0;
   }

    return (MLO_IndivChnPool_Allocate (pool_ptr, nbr_chn));
}



/*
==============================================================================
Name: MLO_IndivChnPool_Destroy
Description:
   Releases any memory. Call this function when the object is no longer used,
   but only if call to MLO_IndivChnPool_Create() was successful.
Input/output parameters:
	- pool_ptr: object to destroy.
==============================================================================
*/

void  MLO_IndivChnPool_Destroy (MLO_IndivChnPool *pool_ptr)
{
	MLO_ASSERT (pool_ptr != NULL);

   MLO_IndivChnPool_EraseChannels (pool_ptr);
}



/*
==============================================================================
Name: MLO_IndivChnPool_Allocate
Description:
   Changes the pool capacity. Pool is cleared.
Input parameters:
	- nbr_chn: Maximum number of channel, > 0
Input/output parameters:
	- pool_ptr: MLO_IndivChnPool object
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_OUT_OF_MEMORY if out of memory. The pool remains valid, with its
      previous capacity.
==============================================================================
*/

MLO_Result  
MLO_IndivChnPool_Allocate (MLO_IndivChnPool *pool_ptr, int nbr_chn)
{
    MLO_Result          result = MLO_SUCCESS;
    MLO_IndivChnStream* mem_ptr = 0;

	MLO_ASSERT (pool_ptr != NULL);
	MLO_ASSERT (nbr_chn >= 0);
    MLO_ASSERT (nbr_chn <= MLO_DEFS_MAX_CHN);

   if (nbr_chn != pool_ptr->nbr_alloc_chn)
   {
      mem_ptr = MLO_AllocateMemory (sizeof (*mem_ptr) * nbr_chn);
      if (mem_ptr == 0)
      {
         result = MLO_ERROR_OUT_OF_MEMORY;
      }

      else
      {
         int            chn_cnt;

         MLO_IndivChnPool_EraseChannels (pool_ptr);

         pool_ptr->nbr_alloc_chn = nbr_chn;
         for (chn_cnt = 0; chn_cnt < nbr_chn; ++chn_cnt)
         {
            pool_ptr->chn_ptr_arr [chn_cnt] = &mem_ptr [chn_cnt];
            MLO_IndivChnStream_ClearBuffers (pool_ptr->chn_ptr_arr [chn_cnt]);
         }
      }
   }

   MLO_IndivChnPool_Clear (pool_ptr);

   return (result);
}



/*
==============================================================================
Name: MLO_IndivChnPool_EraseChannels
Description:
   Frees memory. Channel capacity becomes 0.
Input/output parameters:
	- pool_ptr: MLO_IndivChnPool object
==============================================================================
*/

void  MLO_IndivChnPool_EraseChannels (MLO_IndivChnPool *pool_ptr)
{
   MLO_ASSERT (pool_ptr != NULL);

   if (pool_ptr->nbr_alloc_chn > 0)
   {
      int            chn_cnt;

      MLO_ASSERT (pool_ptr->chn_ptr_arr [0] != 0);
      MLO_FreeMemory (pool_ptr->chn_ptr_arr [0]);
      pool_ptr->nbr_alloc_chn = 0;

      for (chn_cnt = 0; chn_cnt < pool_ptr->nbr_alloc_chn; ++chn_cnt)
      {
         pool_ptr->chn_ptr_arr [chn_cnt] = 0;
      }

      MLO_IndivChnPool_Clear (pool_ptr);
   }
}



/*
==============================================================================
Name: MLO_IndivChnPool_Clear
Description:
   Clears the pool (beginning of a frame). Memory stays allocated and capacity
   doesn't change.
Output parameters:
	- pool_ptr: MLO_IndivChnPool object
==============================================================================
*/

void  MLO_IndivChnPool_Clear (MLO_IndivChnPool *pool_ptr)
{
   MLO_ASSERT (pool_ptr != NULL);

   pool_ptr->nbr_chn = 0;
}



/*
==============================================================================
Name: MLO_IndivChnPool_AddChn
Description:
   Reserves a channel in the pool.
Output parameters:
	- index_ptr: Index of the allocated channel (if success)
Input/output parameters:
	- pool_ptr: MLO_IndivChnPool object
Returns:
   MLO_SUCCESS if ok
   MLO_ERROR_NO_CHN_AVAILABLE if there is no room in the channel pool
==============================================================================
*/

MLO_Result  MLO_IndivChnPool_AddChn (MLO_IndivChnPool *pool_ptr, int *index_ptr)
{
   MLO_Result     result = MLO_SUCCESS;

   MLO_ASSERT (pool_ptr != NULL);

   if (MLO_IndivChnPool_GetNbrChnFree (pool_ptr) > 0)
   {
      *index_ptr = pool_ptr->nbr_chn;
      ++ pool_ptr->nbr_chn;
   }
   else
   {
      result = MLO_ERROR_NO_CHN_AVAILABLE;
   }

   return (result);
}



/*
==============================================================================
Name: MLO_IndivChnPool_GetNbrChnFree
Description:
   Returns the number of available channels in the pool.
Input parameters:
	- pool_ptr: MLO_IndivChnPool object
Returns: Number of available channels, >= 0
==============================================================================
*/

int   MLO_IndivChnPool_GetNbrChnFree (const MLO_IndivChnPool *pool_ptr)
{
   MLO_ASSERT (pool_ptr != NULL);

   return (pool_ptr->nbr_alloc_chn - pool_ptr->nbr_chn);
}
