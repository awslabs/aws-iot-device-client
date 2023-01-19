#!/usr/bin/env bash


if [[ -z $CLANG_FORMAT ]] ; then
    CLANG_FORMAT=clang-format
fi

if NOT type $CLANG_FORMAT 2> /dev/null ; then
    echo "No appropriate clang-format found."
    exit 1
fi

FAIL=0
SOURCE_FILES=`find source test integration-tests/source -type f \( -name '*.h' -o -name '*.cpp' \)`

if [ "$1" == "--auto" ]; then
  # Passing the --auto flag will automatically format the code
  for i in $SOURCE_FILES
  do
    $CLANG_FORMAT -i $i
  done
fi

for i in $SOURCE_FILES
do
    $CLANG_FORMAT -output-replacements-xml $i | grep -c "<replacement " > /dev/null
    if [ $? -ne 1 ]
    then
        # Output side-by-side diff of the current code and reformatted code.
        $CLANG_FORMAT $i > /tmp/$(basename $0)-diff
        diff --suppress-common-lines --side-by-side --width 200 $i /tmp/$(basename $0)-diff
        echo "$i failed clang-format check."
        FAIL=1
    fi
done

exit $FAIL
