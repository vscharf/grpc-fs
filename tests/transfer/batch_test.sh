#!/bin/bash -e


SCRIPT=`mktemp`

cat <<'EOF' > $SCRIPT
#/bin/bash -xe
/afsuser/vscharf/WorkArea/GrpcFileServer/grpc-fs/tests/transfer/transfer_server 50511 > /dev/null
EOF
N=${1:-5}
for I in `eval echo {1..$N}`
do
  echo $I
  qsub -q Short -V -l nodes=hen0$I.atlas-farm.kip.uni-heidelberg.de $SCRIPT
done
