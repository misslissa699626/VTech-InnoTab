/*****************************************************************
|
|   BlueTune - Stream Controller
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "Atomix.h"
#include "Neptune.h"
#include "BlueTune.h"

/*----------------------------------------------------------------------
|    BtStreamController
+---------------------------------------------------------------------*/
class BtStreamController : public NPT_Thread
{
public:
    // methods
    BtStreamController(NPT_InputStreamReference& input,
                       BLT_Player&               player);
    virtual ~BtStreamController() {}

    // NPT_Thread methods
    void Run();

protected:
    // methods
    void DoSeekToTimeStamp(const char* time);
    void DoSetProperty(const char* property);
    void DoLoadPlugin(const char* cmd);
    void DoLoadPlugins(const char* cmd);
    
    // members
    NPT_InputStreamReference m_InputStream;
    BLT_Player&              m_Player;
};

