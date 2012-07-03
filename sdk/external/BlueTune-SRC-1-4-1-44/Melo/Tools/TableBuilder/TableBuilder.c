/*----------------------------------------------------------------------
|       Includes
+---------------------------------------------------------------------*/



#include "MloDebug.h"
#include "MloElementCce.h"
#include "MloFloat.h"
#include "MloIcsInfo.h"
#include "MloFft.h"
#include "MloScaleFactor.h"
#include "MloTns.h"
#include "MloUtils.h"

#include <math.h>
#include <stdio.h>



/*----------------------------------------------------------------------
|       Prototypes
+---------------------------------------------------------------------*/



static void TableBuilder_BuildFloatScale ();
static void TableBuilder_BuildFftTableCos ();
static void TableBuilder_BuildImdctTables ();
static void TableBuilder_BuildWindowTables ();
static void TableBuilder_BuildP43 ();
static void TableBuilder_BuildCceP2 ();
static void TableBuilder_BuildScaleFactorP2 ();
static void TableBuilder_BuildTnsFilterCoef ();

static void TableBuilder_DumpTableFloat (const char *name_0, const double table [], long len);
static void TableBuilder_DumpTableFloatPartial (int indent, const double table [], long len);
static void TableBuilder_Indent (int indent);

static void TableBuilder_BuildTableWinSine (double table_ptr [], long len);
static void TableBuilder_BuildTableWinKaiser (double table_ptr [], long len, float alpha);
static double  TableBuilder_ComputeWp (long pos, long len, float alpha);
static double  TableBuilder_ComputeI0 (double x);



/*----------------------------------------------------------------------
|       Functions
+---------------------------------------------------------------------*/



int main (int argc, char *argv [])
{
   TableBuilder_BuildFloatScale ();
   TableBuilder_BuildFftTableCos ();
   TableBuilder_BuildImdctTables ();
   TableBuilder_BuildWindowTables ();
   TableBuilder_BuildP43 ();
   TableBuilder_BuildCceP2 ();
   TableBuilder_BuildScaleFactorP2 ();
   TableBuilder_BuildTnsFilterCoef ();

   return (0);
}



static void TableBuilder_BuildFloatScale ()
{
   int            p2;
   double         table [MLO_FLOAT_TABLE_P2_LEN];
   double         cur_val;

   cur_val = 1;
   for (p2 = -MLO_FLOAT_TABLE_P2_MIN; p2 < MLO_FLOAT_TABLE_P2_LEN; ++p2)
   {
      table [p2] = cur_val;
      cur_val *= 2;
   }

   cur_val = 1;
   for (p2 = -MLO_FLOAT_TABLE_P2_MIN; p2 >= 0; --p2)
   {
      table [p2] = cur_val;
      cur_val *= 0.5;
   }

   TableBuilder_DumpTableFloat (
      "static const MLO_Float MLO_Float_table_p2 [MLO_FLOAT_TABLE_P2_LEN]",
      &table [0],
      MLO_FLOAT_TABLE_P2_LEN
   );
}



static void TableBuilder_BuildFftTableCos ()
{
   int            i;
   const double   mul = (0.5 * MLO_DEFS_PI) / MLO_FFT_TABLE_LEN_COS;
   double         table [MLO_FFT_TABLE_LEN_COS];

	for (i = 0; i < MLO_FFT_TABLE_LEN_COS; ++ i)
	{
		table [i] = cos (i * mul);
	}

   TableBuilder_DumpTableFloat (
      "static const MLO_Float MLO_Fft_table_cos [MLO_FFT_TABLE_LEN_COS]",
      &table [0],
      MLO_FFT_TABLE_LEN_COS
   );
}



static void TableBuilder_BuildImdctTables ()
{
   double         table_1 [MLO_DEFS_FRAME_LEN_LONG];
   double         table_2 [MLO_DEFS_FRAME_LEN_LONG];
   const double   r_1 = MLO_DEFS_PI / (MLO_DEFS_FRAME_LEN_LONG * 4);
   const double   r_2 = MLO_DEFS_PI / (MLO_DEFS_FRAME_LEN_LONG * 2);
   int            k;

   for (k = 0; k < MLO_DEFS_FRAME_LEN_LONG; ++k)
   {
      table_1 [k] = cos ((k*2 + 1) * r_1);
      table_2 [k] = cos ( k        * r_2);
   }

   TableBuilder_DumpTableFloat (
      "static const MLO_Float MLO_Imdct_cos_table_1 [MLO_DEFS_FRAME_LEN_LONG]",
      &table_1 [0],
      MLO_DEFS_FRAME_LEN_LONG
   );

   TableBuilder_DumpTableFloat (
      "static const MLO_Float MLO_Imdct_cos_table_2 [MLO_DEFS_FRAME_LEN_LONG]",
      &table_2 [0],
      MLO_DEFS_FRAME_LEN_LONG
   );
}



static void TableBuilder_BuildWindowTables ()
{
   double         table_s [MLO_DEFS_FRAME_LEN_LONG * 2];
   double         table_k [MLO_DEFS_FRAME_LEN_LONG * 2];

   MLO_ASSERT (MLO_ICS_INFO_WINDOW_SHAPE_SINE   == 0);
   MLO_ASSERT (MLO_ICS_INFO_WINDOW_SHAPE_KAISER == 1);

   /* Sine, long */
   TableBuilder_BuildTableWinSine (
      &table_s [0],
      MLO_DEFS_FRAME_LEN_LONG * 2
   );

   /* Kaiser, long */
   TableBuilder_BuildTableWinKaiser (
      &table_k [0],
      MLO_DEFS_FRAME_LEN_LONG * 2,
      4
   );

   /* Prints */
   printf ("static const MLO_Float MloFilterBank_table_long [MLO_ICS_INFO_WINDOW_SHAPE_NBR_ELT] [MLO_DEFS_FRAME_LEN_LONG * 2] =\n{\n");
   TableBuilder_DumpTableFloatPartial (3, table_s, MLO_DEFS_FRAME_LEN_LONG * 2);
   printf (",\n");
   TableBuilder_DumpTableFloatPartial (3, table_k, MLO_DEFS_FRAME_LEN_LONG * 2);
   printf ("\n};\n");

   /* Sine, short */
   TableBuilder_BuildTableWinSine (
      &table_s [0],
      MLO_DEFS_FRAME_LEN_SHORT * 2
   );

   /* Kaiser, short */
   TableBuilder_BuildTableWinKaiser (
      &table_k [0],
      MLO_DEFS_FRAME_LEN_SHORT * 2,
      6
   );

   /* Prints */
   printf ("static const MLO_Float MloFilterBank_table_short [MLO_ICS_INFO_WINDOW_SHAPE_NBR_ELT] [MLO_DEFS_FRAME_LEN_SHORT * 2] =\n{\n");
   TableBuilder_DumpTableFloatPartial (3, table_s, MLO_DEFS_FRAME_LEN_SHORT * 2);
   printf (",\n");
   TableBuilder_DumpTableFloatPartial (3, table_k, MLO_DEFS_FRAME_LEN_SHORT * 2);
   printf ("\n};\n");
}



static void TableBuilder_BuildP43 ()
{
   double         table [8192 / 8];
   int            index;
   int            len = MLO_ARRAY_SIZE (table);

   table [0] = 0;
   for (index = 1; index < len; ++index)
   {
      table [index] = 16 * pow (index, 4.0 / 3);
   }

   TableBuilder_DumpTableFloat (
      "static const MLO_Float MLO_InvQuant_table_pow43 [8192 / 8]",
      &table [0],
      len
   );
}



static void TableBuilder_BuildCceP2 ()
{
   double         table [MLO_ELEMENT_CCE_POW2_RES];
   int            k;
   const double   mult = 1.0 / MLO_ELEMENT_CCE_POW2_RES;
   for (k = 0; k < MLO_ELEMENT_CCE_POW2_RES; ++k)
   {
      table [k] = pow (2.0, k * mult);
   }

   TableBuilder_DumpTableFloat (
      "static const MLO_Float MLO_ElementCce_table_pow2 [MLO_ELEMENT_CCE_POW2_RES]",
      &table [0],
      MLO_ELEMENT_CCE_POW2_RES
   );
}



static void TableBuilder_BuildScaleFactorP2 ()
{
   double         table [1 << 2];
   const int      len = MLO_ARRAY_SIZE (table);
   int            k;
   for (k = 0; k < len; ++k)
   {
      table [k] = pow (2.0, 0.25 * k);
   }

   TableBuilder_DumpTableFloat (
      "static const MLO_Float MLO_ScaleFactor_table_pow2  [1 << 2]",
      &table [0],
      len
   );
}



/* Ref: 4.6.9.3 */
static void TableBuilder_BuildTnsFilterCoef ()
{
   double         table [2] [2] [1 << MLO_TNS_MAX_COEF_RES];
   const int      len = MLO_ARRAY_SIZE (table [0] [0]);
   int            resol;

   printf ("static const MLO_Float MLO_Tns_table_coef [2] [2] [1 << MLO_TNS_MAX_COEF_RES] =\n{\n");

   for (resol = 0; resol < 2; ++resol)
   {
      int            compress;

      TableBuilder_Indent (3);
      printf ("{\n");

      for (compress = 0; compress < 2; ++compress)
      {
         const int      r_2 = resol - compress;
         const int      iqr = 8 << resol;
         const double   iqfac   = MLO_DEFS_PI / (iqr - 1);
         const double   iqfac_m = MLO_DEFS_PI / (iqr + 1);
         const int      s = 1 << (2 + r_2);
         const int      max_nbr_coef = 1 << (3 + r_2);
         int            coef;

         MLO_ASSERT (max_nbr_coef <= len);

         for (coef = 0; coef < max_nbr_coef; ++coef)
         {
            /* Conversion to signed integer */
            const int      tmp = coef - (((coef & s) != 0) ? (s * 2) : 0);

            /* Inverse quantization */
            const double   iqfac_d = (tmp >= 0) ? iqfac : iqfac_m;
            const double   phase = tmp * iqfac_d;

            table [resol] [compress] [coef] = sin (phase);
         }

         TableBuilder_DumpTableFloatPartial (
            6,
            &table [resol] [compress] [0],
            max_nbr_coef
         );
         if (compress < 1)
         {
            printf (",");
         }
         printf ("\n");
      }

      TableBuilder_Indent (3);
      printf ("}");
      if (resol < 1)
      {
         printf (",");
      }
      printf ("\n");
   }

   printf ("};\n");
}



static void TableBuilder_DumpTableFloat (const char *name_0, const double table [], long len)
{
   printf ("%s =\n", name_0);
   TableBuilder_DumpTableFloatPartial (0, table, len);
   printf (";\n");
}



static void TableBuilder_DumpTableFloatPartial (int indent, const double table [], long len)
{
   long           pos;

   TableBuilder_Indent (indent);
   printf ("{\n");

   for (pos = 0; pos < len; ++pos)
   {
      const double   val_flt = table [pos];
      const int      val_int = (int) val_flt;

      TableBuilder_Indent (indent + 3);
      printf ("MLO_FLOAT_C (%.9g)", table [pos]);

      if (pos < len - 1)
      {
         printf (",");
      }
      printf ("\n");
   }

   TableBuilder_Indent (indent);
   printf ("}");
}



static void TableBuilder_Indent (int indent)
{
   int            k;

   for (k = 0; k < indent; ++k)
   {
      printf (" ");
   }
}



/* Ref: 4.6.11.3.2 */
static void TableBuilder_BuildTableWinSine (double table_ptr [], long len)
{
   long           pos;
   const long     half_len = len / 2;

   MLO_ASSERT (table_ptr != 0);
   MLO_ASSERT (len > 0);

   for (pos = 0; pos < half_len; ++pos)
   {
      const double   phase = MLO_DEFS_PI * (pos + 0.5) / len;
      const double    val = sin (phase);
      table_ptr [          pos] = val;
      table_ptr [len - 1 - pos] = val;
   }
}



/* Ref: 4.6.11.3.2 */
static void TableBuilder_BuildTableWinKaiser (double table_ptr [], long len, float alpha)
{
   long           pos;
   const long     half_len = len / 2;
   double         wp_sum [MLO_DEFS_FRAME_LEN_LONG + 1];
   double         den = 0;

   MLO_ASSERT (table_ptr != 0);
   MLO_ASSERT (len > 0);
   MLO_ASSERT (len/2 <= MLO_ARRAY_SIZE (wp_sum));
   MLO_ASSERT (alpha > 0);

   for (pos = 0; pos <= half_len; ++pos)
   {
      den += TableBuilder_ComputeWp (pos, len, alpha);
      wp_sum [pos] = den;
   }

   for (pos = 0; pos < half_len; ++pos)
   {
      const double   num = wp_sum [pos];
      const double   val = sqrt (num / den);
      table_ptr [          pos] = val;
      table_ptr [len - 1 - pos] = val;
   }
}



/*
==============================================================================
Name: TableBuilder_ComputeWp
Description:
   Computes W'(n, alpha) * I0 (Pi * alpha).
   The original formula for kaiser window uses just W', but because it is
   used in a ratio of sums of W' with constant alpha, I0 (Pi * alpha) can
   be factored and thus removed - there is no need to compute it.

                                                             n - N/4 2
W'(n, alpha) * I0 (Pi * alpha) = I0 (Pi * alpha * sqrt (1 - (-------) ) )
                                                               N/4

   Ref: 4.6.11.3.2
Input parameters:
	- pos: n in the formula. [0 ; len/2]
	- len: N in the formula. > 0
	- alpha: Kernel Alpha Factor, > 0
Returns: Value of W'(n, alpha) * I0 (Pi * alpha) 
==============================================================================
*/

static double  TableBuilder_ComputeWp (long pos, long len, float alpha)
{
   double         len_q;
   double         r;
   double         x;
   double         wp;

   MLO_ASSERT (pos >= 0);
   MLO_ASSERT (pos <= len/2);
   MLO_ASSERT (len > 0);
   MLO_ASSERT (alpha > 0);

   len_q = len * 0.25;
   r = (pos - len_q) / len_q;
   x = MLO_DEFS_PI * alpha * sqrt (1 - r*r);
   wp = TableBuilder_ComputeI0 (x);

   return (wp);
}


static double  TableBuilder_ComputeI0 (double x)
{
   long           k = 1;
   double         sum = 1;
   double         prod = 1;
   double         fact = 1;
   double         ratio;

   x *= 0.5;
   do
   {
      prod *= x;
      fact *= k;
      ratio = prod / fact;
      ratio *= ratio;
      sum += ratio;
      ++ k;
   }
   while (sum < ratio * 1e10);

   return (sum);
}
