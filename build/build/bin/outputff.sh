#!/bin/sh

init_ff_table()
{
ff[0]=$(echo -ne "\xff")
local i=0;
local j=1;
while [ $i -lt 20 ] ; do
let j=$i+1;
ff[$j]=$(echo -ne "${ff[$i]}${ff[$i]}");
let i=$j;
done
}

output_ff_stdout()
{
local appendlen=$1
local fbit;

while [ ${appendlen} -ge $((1<<20)) ] ; do
echo -ne ${ff[20]};
appendlen=$(($appendlen-(1<<20)))
done

local index=0;
while [ ${appendlen} -gt 0 ] ; do

fbit=$((1<<$index)); let "bit=($appendlen&$fbit)";
[ $bit -ne 0 ] && appendlen=$(($appendlen-$fbit)) && echo -ne ${ff[$index]};
let index++;

done
}

output_ff_file()
{
local appendlen=$1
local appendFile="$2"
local fbit;

rm -rf ${appendFile};

while [ ${appendlen} -ge $((1<<20)) ] ; do
echo -ne ${ff[20]};
appendlen=$(($appendlen-(1<<20)))
done

local index=0;
while [ ${appendlen} -gt 0 ] ; do

fbit=$((1<<$index)); let "bit=($appendlen&$fbit)";
[ $bit -ne 0 ] && appendlen=$(($appendlen-$fbit)) && echo -ne ${ff[$index]} >> ${appendFile};
let index++;

done
}

PrintHelp()
{
echo -e "Usage:\n" "\t$0 appendLen  -- Output to stdout.\n" "\t$0 appendLen appendFile -- Output to file.\n"
}

if [ $# -eq 1 ] ; then
init_ff_table
output_ff_stdout $@
elif [ $# -eq 2 ] ; then
init_ff_table
output_ff_file $@
else
PrintHelp $0; 
fi
