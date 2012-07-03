#!/bin/bash
echo "=== OpenPlatform Release ==="
echo ""


###########################################################
### OpenPlatform
###########################################################
mkdir -p ./release/
rm -rf ./release/openplatform

mkdir -p ./release/openplatform
cp -a ./Makefile ./release/openplatform/
cp -a ./README.TXT ./release/openplatform/


###########################################################
### SDK
###########################################################
echo "Release sdk"

mkdir -p ./release/openplatform/sdk/
mkdir -p ./release/openplatform/sdk/middleware
cp -aR ./sdk/bin ./release/openplatform/sdk/
cp -aR ./sdk/build ./release/openplatform/sdk/
cp -aR ./sdk/external ./release/openplatform/sdk/
cp -aR ./sdk/middleware/libflash ./release/openplatform/sdk/middleware/
cp -aR ./sdk/include ./release/openplatform/sdk/
cp -aR ./sdk/lib ./release/openplatform/sdk/
cp -aR ./sdk/lua ./release/openplatform/sdk/
cp -aR ./sdk/prebuild ./release/openplatform/sdk/
cp -a ./sdk/Makefile ./release/openplatform/sdk/
cp -a ./sdk/mkconfig ./release/openplatform/sdk/
cp -a ./sdk/middleware/Makefile ./release/openplatform/sdk/middleware/

mkdir -p ./release/openplatform/sdk/os
cp -aR ./sdk/os/Makefile ./release/openplatform/sdk/os/

find ./release/openplatform/sdk -name ".svn" | xargs rm -rf


cp -a ./release/openplatform/sdk/build/core/main.mak ./release/openplatform/sdk/build/core/main.mak.old
cp -a ./release/openplatform/sdk/build/core/main.mak.release ./release/openplatform/sdk/build/core/main.mak

###########################################################
### Project
###########################################################
echo "Release project"

mkdir -p ./release/openplatform/project
cp -aR ./project/common ./release/openplatform/project/
cp -a ./project/mkconfig ./release/openplatform/project/

mkdir -p ./release/openplatform/project/gplus
cp -aR ./project/gplus/sampleCode ./release/openplatform/project/gplus/
#cp -aR ./project/gplus/flashUI ./release/openplatform/project/gplus/
#rm -rf ./release/openplatform/project/gplus/flashUI/app/
#cp -aR ./project/gplus/pmpUI/app/ ./release/openplatform/project/gplus/flashUI/

#---------------------------------------------------------
# for Zhihua
#---------------------------------------------------------
#mkdir -p ./release/openplatform/project/jxd
#cp -aR ./project/jxd/jxd3000d ./release/openplatform/project/jxd/
#---------------------------------------------------------

find ./release/openplatform/project -name ".svn" | xargs rm -rf


###########################################################
### Platform
###########################################################
echo "Release platform"

mkdir -p ./release/openplatform/platform
cp -aR ./platform/common ./release/openplatform/platform/
cp -a ./platform/mkconfig ./release/openplatform/platform/

mkdir -p ./release/openplatform/platform/gplus
cp -aR ./platform/gplus/evm32900 ./release/openplatform/platform/gplus/
#cp -aR ./platform/gplus/evm8050 ./release/openplatform/platform/gplus/

#---------------------------------------------------------
# for Zhihua
#---------------------------------------------------------
#mkdir -p ./release/openplatform/platform/jxd
#cp -aR ./platform/jxd/jxd2000 ./release/openplatform/platform/jxd/
#cp -aR ./platform/jxd/jxd3000d ./release/openplatform/platform/jxd/
#cp -aR ./platform/jxd/jxd3000d_v1.3 ./release/openplatform/platform/jxd/
#mkdir -p ./release/openplatform/platform/dreamfone
#cp -aR ./platform/dreamfone/dd900 ./release/openplatform/platform/dreamfone/
#---------------------------------------------------------

find ./release/openplatform/platform -name ".svn" | xargs rm -rf


###########################################################
### Clean
###########################################################
cp -aR ./out ./release/openplatform/
cp -a ./product_config.mak ./release/openplatform/
cd ./release/openplatform/sdk/os
ln -s ../../../../sdk/os/kernel-2.6.32 kernel-2.6.32
cd - > /dev/null

cd ./release/openplatform/
make clean
cd - > /dev/null

rm -rf ./release/openplatform/out
rm -rf ./release/openplatform/product_config.mak
rm -rf ./release/openplatform/sdk/os/kernel-2.6.32


###########################################################
### Finished
###########################################################
du -s -h ./release/openplatform
echo "Done."
