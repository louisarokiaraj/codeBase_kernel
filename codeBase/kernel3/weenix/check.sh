#!/bin/bash
grep "GRADING3" typescript | cut -f1 -d' ' | grep -o "[a-z]*.c:[0-9]*$" | sort | uniq > DbgPrinted
grep -nr  "GRADING3" . | grep "GRADING"  | cut -f1,2 -d':' | grep -o "[a-z]*.c:[0-9]*$" | sort | uniq > DbgInCode
difflines=$(diff DbgPrinted DbgInCode | wc -l )
if [ $difflines -eq 0 ]
then
    echo "PASS: All the dbg()s have successfully been printed."
else
   
diff DbgPrinted DbgInCode
fi
rm DbgPrinted DbgInCode