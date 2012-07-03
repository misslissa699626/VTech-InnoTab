/*****************************************************************
|
|   BlueTune - Bento4 Adapters
|
|   (c) 2002-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltBento4Adapters.h"
#include "BltErrors.h"

/*----------------------------------------------------------------------
|   BLT_Ap4CipherAdapter::GetDirection
+---------------------------------------------------------------------*/
AP4_BlockCipher::CipherDirection
BLT_Ap4CipherAdapter::GetDirection()
{
    return BLT_Cipher_GetDirection(m_Delegate) == BLT_CIPHER_DIRECTION_DECRYPT ?
                                                  AP4_BlockCipher::DECRYPT : 
                                                  AP4_BlockCipher::ENCRYPT;
}

/*----------------------------------------------------------------------
|   BLT_Ap4CipherAdapter::Process
+---------------------------------------------------------------------*/
AP4_Result
BLT_Ap4CipherAdapter::Process(const AP4_UI08* input, 
                              AP4_Size        input_size,
                              AP4_UI08*       output,
                              const AP4_UI08* iv)
{
    return BLT_Cipher_Process(m_Delegate, input, input_size, output, iv);
}

/*----------------------------------------------------------------------
|   BLT_Ap4CipherFactoryAdapter::CreateCipher
+---------------------------------------------------------------------*/
AP4_Result 
BLT_Ap4CipherFactoryAdapter::CreateCipher(AP4_BlockCipher::CipherType      type,
                                          AP4_BlockCipher::CipherDirection direction,
                                          AP4_BlockCipher::CipherMode      mode,
                                          const void*                      params,
                                          const AP4_UI08*                  key,
                                          AP4_Size                         key_size,
                                          AP4_BlockCipher*&                cipher)
{
    BLT_CipherAlgorithm blt_algorithm;
    switch (type) {
        case AP4_BlockCipher::AES_128: blt_algorithm = BLT_CIPHER_ALGORITHM_AES_128; break;
        default: return AP4_ERROR_NOT_SUPPORTED;
    }
    
    BLT_CipherDirection blt_direction;
    switch (direction) {
        case AP4_BlockCipher::ENCRYPT: blt_direction = BLT_CIPHER_DIRECTION_ENCRYPT; break;
        case AP4_BlockCipher::DECRYPT: blt_direction = BLT_CIPHER_DIRECTION_DECRYPT; break;
        default: return AP4_ERROR_NOT_SUPPORTED;
    }
    
    BLT_CipherMode blt_mode;
    switch (mode) {
        case AP4_BlockCipher::CBC: blt_mode = BLT_CIPHER_MODE_CBC; break;
        case AP4_BlockCipher::CTR: blt_mode = BLT_CIPHER_MODE_CTR; break;
        default: return AP4_ERROR_NOT_SUPPORTED;
    }
    
    // create the cipher
    BLT_Cipher* blt_cipher;
    BLT_Result result = BLT_CipherFactory_CreateCipher(m_Delegate, 
                                                       blt_algorithm,
                                                       blt_direction,
                                                       blt_mode,
                                                       params,
                                                       key,
                                                       key_size,
                                                       &blt_cipher);
    if (BLT_FAILED(result)) return result;
    
    // create a cipher adapter
    cipher = new BLT_Ap4CipherAdapter(blt_cipher);
    
    return AP4_SUCCESS;
}
