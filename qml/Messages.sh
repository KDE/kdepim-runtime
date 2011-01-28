#! /bin/sh
$EXTRACTRC --ignore-no-input `find . -name '*.ui' -or -name '*.rc' -or -name '*.kcfg' -or -name '*.kcfg.cmake'` >> rc.cpp || exit 11
$XGETTEXT -ktranslate `find . -name '*.cpp' -o -name '*.h' | grep -v '/tests/'` -o $podir/kdepim-runtime-qml.pot
if [ -e $podir/kdepim-runtime-qml.pot ]; then
	extraparam="-j"
fi
$XGETTEXT -ktranslate `find . -name '*.qml' | grep -v '/tests/'` $extraparam -L Java -o $podir/kdepim-runtime-qml.pot
rm -f rc.cpp
