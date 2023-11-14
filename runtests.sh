#!/usr/bin/env bash
make clean
make bank
echo "spec:"
./bank -v -f spec-reg.txt<spec-commands.txt> out.txt
diff out.txt spec-output-verbose.txt
echo 

echo "test 11"
./bank -v -f test-11-reg.txt<test-11-commands.txt > out.txt
echo

echo "test 10"
./bank -v -f test-10-reg.txt<test-10-commands.txt > out.txt
echo

echo "test 9"
./bank -v -f test-9-reg.txt<test-9-commands.txt > out.txt
echo

echo "test 8"
./bank -v -f test-8-reg.txt<test-8-commands.txt > out.txt
echo

echo "test 7"
./bank -v -f test-7-reg.txt<test-7-commands.txt > out.txt
echo

echo "test 6"
./bank -v -f test-6-reg.txt<test-6-commands.txt > out.txt
diff out.txt t6-correct.txt
echo

echo "test 5"
./bank -v -f test-5-reg.txt<test-5-commands.txt > out.txt
diff out.txt t5-correct.txt
echo

echo "test 4"
./bank -v -f test-3-reg.txt<test-3-commands.txt > out.txt
echo

echo "test 3"
./bank -v -f test-3-reg.txt<test-3-commands.txt > out.txt
diff out.txt t3-correct.txt
echo

echo "test 2"
./bank -v -f test-2-reg.txt<test-2-commands.txt > out.txt
diff out.txt t2-correct.txt

echo "test 1"
./bank -v -f test-1-reg.txt<test-1-commands.txt > out.txt
echo