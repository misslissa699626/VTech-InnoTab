/*****************************************************************
|
|   BlueTune - Bento4 Adapters
|
|   (c) 2002-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
#ifndef _BLT_BENTO4_ADAPTERS_H_
#define _BLT_BENTO4_ADAPTERS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "BltKeyManager.h"

/*----------------------------------------------------------------------
|   BLT_Ap4CipherAdapter
+---------------------------------------------------------------------*/
class BLT_Ap4CipherAdapter : public AP4_BlockCipher
{
public:
    // constructor and destructor
    BLT_Ap4CipherAdapter(BLT_Cipher* delegate) : m_Delegate(delegate) {}
    virtual ~BLT_Ap4CipherAdapter() {
        ATX_DESTROY_OBJECT(m_Delegate);
    }
    
    // methods
    virtual CipherDirection GetDirection();
    virtual AP4_Result Process(const AP4_UI08* input, 
                               AP4_Size        input_size,
                               AP4_UI08*       output,
                               const AP4_UI08* iv);
                               
private:
    // members
    BLT_Cipher* m_Delegate;
};

/*----------------------------------------------------------------------
|   BLT_Ap4CipherFactoryAdapter
+---------------------------------------------------------------------*/
class BLT_Ap4CipherFactoryAdapter : public AP4_BlockCipherFactory 
{
public:
    BLT_Ap4CipherFactoryAdapter(BLT_CipherFactory* delegate) :
        m_Delegate(delegate) {}
    virtual AP4_Result CreateCipher(AP4_BlockCipher::CipherType      type,
                                    AP4_BlockCipher::CipherDirection direction,
                                    AP4_BlockCipher::CipherMode      mode,
                                    const void*                      params,
                                    const AP4_UI08*                  key,
                                    AP4_Size                         key_size,
                                    AP4_BlockCipher*&                cipher);
                                    
private:
    BLT_CipherFactory* m_Delegate;
};

#endif /* _BLT_BENTO4_ADAPTERS_H_ */
