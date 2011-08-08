#!/bin/bash

# $1 die Anzahl der Durchlaufe eines einzelnen Tests
# $2 die Anzahl der Tests

serWHmacData=ser_w_hmac.dat
serWOHmacData=ser_wo_hmac.dat
deserWHmacData=deser_w_hmac.dat
deserWOHmacData=deser_wo_hmac.dat
hmacCreate=hmac_create.dat
hmacVerify=hmac_verify.dat

function usage() {
	echo "$0 <runs> <iterations>"
}

export runs=$1
export iterations=$2

if [ -z $runs ]
then
	usage
	exit 0
fi

if [ -z $iterations ]
then
	usage
	exit 0
fi

rm -f $serWHmacData $serWOHmacData $deserWHmacData $deserWOHmacData $hmacCreate $hmacVerify

for (( i=0; i<$iterations; i++ ))
do
	./auth_benchmark $runs 2>&1 > /dev/null
	# Create HMAC
	./eval_journal.py 15 16 < benchmark_journal.txt >> $hmacCreate
	# Verify HMAC
	./eval_journal.py 17 18 < benchmark_journal.txt >> $hmacVerify
	# ser w hmac and calc & ser hmac
	./eval_journal.py 21 22 < benchmark_journal.txt >> $serWHmacData
	# ser wo hmac
	./eval_journal.py 25 26 < benchmark_journal.txt >> $serWOHmacData
	# deser w hmac and calc & check hmac 
	./eval_journal.py 23 24 < benchmark_journal.txt >> $deserWHmacData
	# deser wo hmac
	./eval_journal.py 27 28 < benchmark_journal.txt >> $deserWOHmacData
done

# Sum of all Min/Max/Mean/Stdev values for HMAC calculation
sum_min_hmac_calc=$(awk 'BEGIN{sum=0} $3!="Min" {sum+=$3} END{print sum}' < $hmacCreate)
sum_max_hmac_calc=$(awk 'BEGIN{sum=0} $4!="Max" {sum+=$4} END{print sum}' < $hmacCreate)
sum_mean_hmac_calc=$(awk 'BEGIN{sum=0} $5!="Mean" {sum+=$5} END{print sum}' < $hmacCreate)
sum_stdev_hmac_calc=$(awk 'BEGIN{sum=0} $6!="Stdev" {sum+=$6} END{print sum}' < $hmacCreate)

# Sum of all Min/Max/Mean/Stdev values for HMAC verification
sum_min_hmac_verify=$(awk 'BEGIN{sum=0} $3!="Min" {sum+=$3} END{print sum}' < $hmacVerify)
sum_max_hmac_verify=$(awk 'BEGIN{sum=0} $4!="Max" {sum+=$4} END{print sum}' < $hmacVerify)
sum_mean_hmac_verify=$(awk 'BEGIN{sum=0} $5!="Mean" {sum+=$5} END{print sum}' < $hmacVerify)
sum_stdev_hmac_verify=$(awk 'BEGIN{sum=0} $6!="Stdev" {sum+=$6} END{print sum}' < $hmacVerify)

# Sum of all Min/Max/Mean/Stdev values for serialization with HMAC
sum_min_ser_w_hmac=$(awk 'BEGIN{sum=0} $3!="Min" {sum+=$3} END{print sum}' < $serWHmacData)
sum_max_ser_w_hmac=$(awk 'BEGIN{sum=0} $4!="Max" {sum+=$4} END{print sum}' < $serWHmacData)
sum_mean_ser_w_hmac=$(awk 'BEGIN{sum=0} $5!="Mean" {sum+=$5} END{print sum}' < $serWHmacData)
sum_stdev_ser_w_hmac=$(awk 'BEGIN{sum=0} $6!="Stdev" {sum+=$6} END{print sum}' < $serWHmacData)

# Sum of all Min/Max/Mean/Stdev values for serialization without HMAC
sum_min_ser_wo_hmac=$(awk 'BEGIN{sum=0} $3!="Min" {sum+=$3} END{print sum}' < $serWOHmacData)
sum_max_ser_wo_hmac=$(awk 'BEGIN{sum=0} $4!="Max" {sum+=$4} END{print sum}' < $serWOHmacData)
sum_mean_ser_wo_hmac=$(awk 'BEGIN{sum=0} $5!="Mean" {sum+=$5} END{print sum}' < $serWOHmacData)
sum_stdev_ser_wo_hmac=$(awk 'BEGIN{sum=0} $6!="Stdev" {sum+=$6} END{print sum}' < $serWOHmacData)

# Sum of all Min/Max/Mean/Stdev values for deserialization with HMAC
sum_min_deser_w_hmac=$(awk 'BEGIN{sum=0} $3!="Min" {sum+=$3} END{print sum}' < $deserWHmacData)
sum_max_deser_w_hmac=$(awk 'BEGIN{sum=0} $4!="Max" {sum+=$4} END{print sum}' < $deserWHmacData)
sum_mean_deser_w_hmac=$(awk 'BEGIN{sum=0} $5!="Mean" {sum+=$5} END{print sum}' < $deserWHmacData)
sum_stdev_deser_w_hmac=$(awk 'BEGIN{sum=0} $6!="Stdev" {sum+=$6} END{print sum}' < $deserWHmacData)

# Sum of all Min/Max/Mean/Stdev values for deserialization without HMAC
sum_min_deser_wo_hmac=$(awk 'BEGIN{sum=0} $3!="Min" {sum+=$3} END{print sum}' < $deserWOHmacData)
sum_max_deser_wo_hmac=$(awk 'BEGIN{sum=0} $4!="Max" {sum+=$4} END{print sum}' < $deserWOHmacData)
sum_mean_deser_wo_hmac=$(awk 'BEGIN{sum=0} $5!="Mean" {sum+=$5} END{print sum}' < $deserWOHmacData)
sum_stdev_deser_wo_hmac=$(awk 'BEGIN{sum=0} $6!="Stdev" {sum+=$6} END{print sum}' < $deserWOHmacData)

#####################################################################
# print statistics
#####################################################################

### All Min Values
minTimeCreateHmac=$(( $sum_min_hmac_calc / $iterations ))
minTimeVerifyHmac=$(( $sum_min_hmac_verify / $iterations ))
minTimeSerWithHmac=$(( $sum_min_ser_w_hmac / $iterations ))
minTimeSerWithoutHmac=$(( $sum_min_ser_wo_hmac / $iterations ))
minTimeDeserWithHmac=$(( $sum_min_deser_w_hmac / $iterations ))
minTimeDeserWithoutHmac=$(( $sum_min_deser_wo_hmac / $iterations ))

### All Max Values
maxTimeCreateHmac=$(( $sum_max_hmac_calc / $iterations ))
maxTimeVerifyHmac=$(( $sum_max_hmac_verify / $iterations ))
maxTimeSerWithHmac=$(( $sum_max_ser_w_hmac / $iterations ))
maxTimeSerWithoutHmac=$(( $sum_max_ser_wo_hmac / $iterations ))
maxTimeDeserWithHmac=$(( $sum_max_deser_w_hmac / $iterations ))
maxTimeDeserWithoutHmac=$(( $sum_max_deser_wo_hmac / $iterations ))

### All Mean Values
meanTimeCreateHmac=$(( $sum_mean_hmac_calc / $iterations ))
meanTimeVerifyHmac=$(( $sum_mean_hmac_verify / $iterations ))
meanTimeSerWithHmac=$(( $sum_mean_ser_w_hmac / $iterations ))
meanTimeSerWithoutHmac=$(( $sum_mean_ser_wo_hmac / $iterations ))
meanTimeDeserWithHmac=$(( $sum_mean_deser_w_hmac / $iterations ))
meanTimeDeserWithoutHmac=$(( $sum_mean_deser_wo_hmac / $iterations ))

### All Stdev Values
stdevTimeCreateHmac=$(( $sum_stdev_hmac_calc / $iterations ))
stdevTimeVerifyHmac=$(( $sum_stdev_hmac_verify / $iterations ))
stdevTimeSerWithHmac=$(( $sum_stdev_ser_w_hmac / $iterations ))
stdevTimeSerWithoutHmac=$(( $sum_stdev_ser_wo_hmac / $iterations ))
stdevTimeDeserWithHmac=$(( $sum_stdev_deser_w_hmac / $iterations ))
stdevTimeDeserWithoutHmac=$(( $sum_stdev_deser_wo_hmac / $iterations ))

percentageSerWithHmac=$(echo "scale=4; (($sum_mean_ser_w_hmac / $sum_mean_ser_wo_hmac) - 1) * 100" | bc -l)
percentageDeserWithHmac=$(echo "scale=4; (($sum_mean_deser_w_hmac / $sum_mean_deser_wo_hmac) - 1) * 100" | bc -l)

echo -e "Action\t\t\t\tMin\tMax\tMean\tStdev"
echo -e "Serialization\t\t\t$minTimeSerWithoutHmac\t$maxTimeSerWithoutHmac\t$meanTimeSerWithoutHmac\t$stdevTimeSerWithoutHmac"
echo -e "Serialization with HMAC\t\t$minTimeSerWithHmac\t$maxTimeSerWithHmac\t$meanTimeSerWithHmac\t$stdevTimeSerWithHmac"
echo -e "Deserialization\t\t\t$minTimeDeserWithoutHmac\t$maxTimeDeserWithoutHmac\t$meanTimeDeserWithoutHmac\t$stdevTimeDeserWithoutHmac"
echo -e "Deserialization with HMAC\t$minTimeDeserWithHmac\t$maxTimeDeserWithHmac\t$meanTimeDeserWithHmac\t$stdevTimeDeserWithHmac"

echo -e "Overhead serialization AuthSession object with HMAC:\t\t\t$percentageSerWithHmac"
echo -e "Overhead deserialization AuthSession object with HMAC verification:\t$percentageDeserWithHmac"
