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
#FILE_ARR[5]="$BASEDIR/211867.0.root"
#FILE_ARR[1]="$BASEDIR/212721.0.root"
#FILE_ARR[2]="$BASEDIR/209864.0.root"
#FILE_ARR[3]="$BASEDIR/213039.0.root"
#FILE_ARR[4]="$BASEDIR/207982.0.root"
#FILE_ARR[0]="$BASEIDR/214680.0.root $BASEDIR/212199.0.root"
FILE_ARR[1]="$BASEDIR/213951.0.root $BASEDIR/211787.0.root"
FILE_ARR[2]="$BASEDIR/215473.0.root $BASEDIR/209580.0.root"
FILE_ARR[3]="$BASEDIR/214777.0.root $BASEDIR/209995.0.root"
FILE_ARR[4]="$BASEDIR/208781.0.root $BASEDIR/209254.0.root"
FILE_ARR[5]="$BASEDIR/209629.0.root $BASEDIR/208485.0.root"
FILE_ARR[6]="$BASEDIR/207447.0.root $BASEDIR/203602.0.root"
FILE_ARR[7]="$BASEDIR/204763.0.root $BASEDIR/205071.1.root"
FILE_ARR[8]="$BASEDIR/204265.0.root $BASEDIR/209736.0.root"
FILE_ARR[9]="$BASEDIR/205055.0.root $BASEDIR/212172.0.root"

for I in {1..9}
do
  qsub -q Short -l nodes=hen0$I.atlas-farm.kip.uni-heidelberg.de -v files="${FILE_ARR[I]}" $SCRIPT
#  cat <<EOF | qsub -q Short -l nodes=hen0$I.atlas-farm.kip.uni-heidelberg.de
#!/bin/bash -e
#source /afsuser/vscharf/WorkArea/GrpcFileServer/setup.sh
#echo "Files: ${FILE_ARR[I]}"
#for F in ${FILE_ARR[I]}; do
#  echo "Transferring \$F ..."
#  /lib64/ld-linux-x86-64.so.2 /afsuser/vscharf/WorkArea/GrpcFileServer/grpc-fs/tests/performance-tests-remote/transfer_disk_disk_static \$F /dev/null || exit 1
#done
#EOF
done
