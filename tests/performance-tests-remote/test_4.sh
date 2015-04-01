#!/bin/bash -e


SCRIPT=`mktemp`

cat <<'EOF' > $SCRIPT
#/bin/bash -xe
source /afsuser/vscharf/WorkArea/GrpcFileServer/setup.sh
cd /afsuser/vscharf/WorkArea/GrpcFileServer/grpc-fs
echo $files
for A in $files
do
  echo $A
  /lib64/ld-linux-x86-64.so.2 /afsuser/vscharf/WorkArea/GrpcFileServer/grpc-fs/bins/grpcfs_client_static $A /dev/null
done
EOF

BASEDIR=/data/users/vscharf/WyyAnalysis/skims/data

# files
declare -a FILE_ARR
FILE_ARR[1]="/data/users/vscharf/WyyAnalysis/skims/mc/105200/105200.6.root /data/users/vscharf/WyyAnalysis/skims/mc/146436/146436.2.root /data/users/vscharf/WyyAnalysis/skims/mc/126893/126893.1.root /data/users/vscharf/WyyAnalysis/skims/mc/146437/146437.0.root /data/users/vscharf/WyyAnalysis/skims/mc/129819/129819.0.root"
FILE_ARR[2]="/data/users/vscharf/WyyAnalysis/skims/mc/105200/105200.18.root /data/users/vscharf/WyyAnalysis/skims/mc/129823/129823.2.root /data/users/vscharf/WyyAnalysis/skims/mc/105985/105985.0.root /data/users/vscharf/WyyAnalysis/skims/mc/146437/146437.1.root /data/users/vscharf/WyyAnalysis/skims/mc/126893/126893.2.root"
FILE_ARR[3]="/data/users/vscharf/WyyAnalysis/skims/mc/126939/126939.0.root /data/users/vscharf/WyyAnalysis/skims/mc/126893/126893.0.root /data/users/vscharf/WyyAnalysis/skims/mc/126938/126938.14.root /data/users/vscharf/WyyAnalysis/skims/mc/105200/105200.17.root /data/users/vscharf/WyyAnalysis/skims/mc/105200/105200.16.root"
FILE_ARR[4]="/data/users/vscharf/WyyAnalysis/skims/mc/129823/129823.1.root /data/users/vscharf/WyyAnalysis/skims/mc/126939/126939.1.root /data/users/vscharf/WyyAnalysis/skims/mc/105200/105200.5.root /data/users/vscharf/WyyAnalysis/skims/mc/146436/146436.1.root /data/users/vscharf/WyyAnalysis/skims/mc/108346/108346.0.root"

for I in {1..4}
do
  qsub -q Short -l nodes=hen0$I.atlas-farm.kip.uni-heidelberg.de -v files="${FILE_ARR[I]}" $SCRIPT
done
