nOk=0
nKo=0
nTotal=0
nMis=0

for n in $(seq 4)
do
  find . -name "*.dot" -delete

  if [ $n -lt 4 ]
  then
    rm test.log
    ./mkRandomLog.py > test.log
  else
    ln -sf /tmp/anonyme.log test.log
  fi

  echo "Test ID;Return code validation;Out result;StdErr result;File creation result;Global result" >results$n.csv

  for i in Test*
  do
    ./test.sh "$i" results$n.csv
    result=$?
    if [ $result -eq 0 ]
    then
      let "nKo=$nKo+1"
    elif [ $result -eq 1 ]
    then
      let "nOk=$nOk+1"
    else
      let "nMis=$nMis+1"
    fi
    let "nTotal=$nTotal+1"
  done

  if [ $nKo -gt 0 ]
  then
      echo "Error at run     : $n"
      break
  fi
done

echo "Number of runs   : $n"
echo "-----------------------"
echo "Passed tests     : $nOk"
echo "Failed tests     : $nKo"
echo "Misformed tests  : $nMis"
echo "-----------------------"
echo "Total            : $nTotal"
