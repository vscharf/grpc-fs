#!/bin/bash -e

SCRIPT=`mktemp`

cat <<'EOF' > $SCRIPT
#/bin/bash -xe
/afsuser/vscharf/WorkArea/GrpcFileServer/grpc-fs/bins/grpcfs_client $file /dev/null
EOF

N=9 #${1:-5}

ARR=(0 /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23181790/user.vlang.5198776._000027.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23186009/user.vlang.5198776._000091.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23186009/user.vlang.5198776._000095.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23218066/user.vlang.5198776._000114.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23164353/user.vlang.5198776._000004.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23179758/user.vlang.5198776._000008.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23181752/user.vlang.5198776._000034.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23181909/user.vlang.5198776._000040.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23181909/user.vlang.5198776._000074.data12_8TeV.periodC.slim_Egamma.root /data/users/vlang/sm/data/gridFiles/user.vlang.slim.periodC.20150327_data12_8TeV.periodC.slim_Egamma.root.23182009/user.vlang.5198776._000042.data12_8TeV.periodC.slim_Egamma.root)

for I in `eval echo 0{1..$N}` 10
do
  i=${I#0}
  qsub -q Short -V -v 'file='"${ARR[i]}" -l nodes=hen$I.atlas-farm.kip.uni-heidelberg.de $SCRIPT
done
