#!/bin/bash
# $1 $2 mit authsession
# $3 $4 ohne authsession


if (( $# != 6 ))
then
  echo "Not enough arguments need 6 arguments" >&2
  exit
fi

run=10000
#key_count=40000
out_data=$5
img_file=$6
rm -f $out_data
for qspec_count in `seq 10 10 4000`
do
  ./auth_benchmark $run 1 $qspec_count 2>&1 > /dev/null
  mean=`./eval_journal.py $1 $2 < benchmark_journal.txt | awk '$5!="Mean" {print $5}'`
  mean2=`./eval_journal.py $3 $4 < benchmark_journal.txt | awk '$5!="Mean" {print $5}'`
  overhead_proz=`awk 'BEGIN { y = ('$mean'*100 / '$mean2') - 100; print y }'`
  hmac_buf_size=`echo "170 + $qspec_count * 4 " | bc`
  echo "$hmac_buf_size $overhead_proz" >> $out_data
  echo "$hmac_buf_size $overhead_proz"
#  awk ' $1=="'$1'" {x= ( ($3 * 1000000000) + $4)} $1=="'$2'" {y=((($3 * 1000000000) + $4) - x); print y} ' < benchmark_journal.txt > $out_data
#  max=`awk ' BEGIN {max=0} { if(max < $1) max= $1 } END {print max}'< $out_data `
#  min=`awk ' BEGIN {min=1000000} { if(min > $1) min = $1 } END {print min}'< $out_data`
done
  cat > "plot$img_file" << END
set terminal postscript eps color enhanced
set output "$img_file"
set xlabel "Größe der Objekte, die in die HMAC-Berechnung eingehen [Byte]"
set ylabel "Mehraufwand in Prozent"
plot "$out_data" with lines t "Messdaten"
END
gnuplot "plot$img_file"
#done 
