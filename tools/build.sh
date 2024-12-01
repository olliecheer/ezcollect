#!/bin/bash

set -ex

SCRIPT_DIR=$(readlink -f $(dirname $0))

APPDIR=${SCRIPT_DIR}/AppDir
rm -rf $APPDIR

cd ..
rm -rf build
mkdir build
cd build

cmake ..
make -j$(nproc)
make install DESTDIR=${APPDIR}


cd $SCRIPT_DIR

filelist=($(${SCRIPT_DIR}/resolve_ldd.py ${APPDIR}/usr/local/bin/ezcollect))
for it in ${filelist[@]}; do
  mkdir -p ${APPDIR}/$(dirname $it)
  cp $it ${APPDIR}/$it
  # echo $it
done

mkdir -p ${APPDIR}/etc
touch ${APPDIR}/etc/ld.so.conf

declare -A LIB_DIRS

for it in ${filelist[@]}; do
  LIB_DIRS[$(dirname $it)]=1
done


for it in "${!LIB_DIRS[@]}"; do
  LD_LIBRARY_PATH_VALUE="\${APPDIR}/$it:$LD_LIBRARY_PATH_VALUE"

  # echo "$it" >> ${APPDIR}/etc/ld.so.conf
done


touch ${APPDIR}/ezcollect.svg
cat <<EOF > ${APPDIR}/ezcollect.desktop
[Desktop Entry]
Name=ezcollect
Exec=ezcollect
Icon=ezcollect
Type=Application
Categories=Utility;
EOF

cat <<EOF > ${APPDIR}/AppRun
#!/bin/bash
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH_VALUE}

\${APPDIR}/lib/$(uname -m)-linux-gnu/ld-linux-$(uname -m).so* \${APPDIR}/usr/local/bin/ezcollect \$@
EOF

chmod +x ${APPDIR}/AppRun

./appimagetool-$(uname -m).AppImage ./AppDir

