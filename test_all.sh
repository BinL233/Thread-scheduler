# Test all sample/task/test cases

echo "[info] Start testing all given inputs"
while read line; do
    # bash ./consistency_test.sh $line | grep '\[.*\]' && read -p 'Press anything to continue testing ...' </dev/tty;
    bash ./consistency_test.sh $line | grep '\[.*\]';
done < test_cases.txt
echo "[info] End testing all given inputs"