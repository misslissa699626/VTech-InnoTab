/*****************************************************************
|
|   Key Manager Interface
|
|   (c) 2008-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Key Manager Interface
 */

#ifndef _BLT_KEY_MANAGER_H_
#define _BLT_KEY_MANAGER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltTypes.h"
#include "Atomix.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_KEY_MANAGER_PROPERTY     "KeyManager"
#define BLT_CIPHER_FACTORY_PROPERTY  "CipherFactory"

#define BLT_ERROR_CRYPTO_FAILURE (BLT_ERROR_BASE_KEY_MANAGER-0)

/*----------------------------------------------------------------------
|   BLT_KeyManager Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_KeyManager)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_KeyManager)
    BLT_Result (*GetKeyByName)(BLT_KeyManager* self,
                               const char*     name,
                               unsigned char*  key, 
                               unsigned int*   key_size);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   BLT_Cipher Interface
+---------------------------------------------------------------------*/
typedef enum {
    BLT_CIPHER_ALGORITHM_AES_128
} BLT_CipherAlgorithm;

typedef enum {
    BLT_CIPHER_DIRECTION_ENCRYPT,
    BLT_CIPHER_DIRECTION_DECRYPT
} BLT_CipherDirection;

typedef enum {
    BLT_CIPHER_MODE_CBC,
    BLT_CIPHER_MODE_CTR
} BLT_CipherMode;

ATX_DECLARE_INTERFACE(BLT_Cipher)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_Cipher)
    BLT_CipherDirection (*GetDirection)(BLT_Cipher* self);
    BLT_Result          (*Process)(BLT_Cipher*      self, 
                                   const BLT_UInt8* input,
                                   BLT_Size         input_size,
                                   BLT_UInt8*       output,
                                   const BLT_UInt8* iv);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   BLT_CipherFactory Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_CipherFactory)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_CipherFactory)
    BLT_Result (*CreateCipher)(BLT_CipherFactory*  self,
                               BLT_CipherAlgorithm algorithm,
                               BLT_CipherDirection direction,
                               BLT_CipherMode      mode,
                               const void*         params,
                               const BLT_UInt8*    key,
                               BLT_Size            key_size,
                               BLT_Cipher**        cipher);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_KeyManager_GetKeyByName(object, name, key, key_size) \
    ATX_INTERFACE(object)->GetKeyByName(object, name, key, key_size)

#define BLT_Cipher_GetDirection(object) \
    ATX_INTERFACE(object)->GetDirection(object)

#define BLT_Cipher_Process(object, input, input_size, output, iv) \
    ATX_INTERFACE(object)->Process(object, input, input_size, output, iv)

#define BLT_CipherFactory_CreateCipher(object, algorithm, direction, mode, params, key, key_size, cipher) \
    ATX_INTERFACE(object)->CreateCipher(object, algorithm, direction, mode, params, key, key_size, cipher)

#endif /* _BLT_KEY_MANAGER_H_ */
