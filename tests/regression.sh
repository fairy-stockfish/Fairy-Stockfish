#!/bin/bash
# regression test variant bench numbers
# arguments: ./old_engine ./new_engine "bench_args" [variant1 variant2 variant3 ...]
# examples:
#   Pure bench: ./regression.sh ./old_engine ./new_engine "" chess xiangqi shogi
#   Perft bench: ./regression.sh ./old_engine ./new_engine "16 1 5 default perft" chess xiangqi shogi
#   Auto-detect common variants: ./regression.sh ./old_engine ./new_engine ""

echo "variant $1 $2"
bench_args="$3"

# If no variants provided, extract them from UCI output
if [ $# -eq 3 ]; then
    echo "No variants specified, extracting from UCI output..."
    
    # Extract variants from first engine
    variants1=$(echo "uci" | $1 2>/dev/null | grep "option name UCI_Variant" | grep -o 'var [^[:space:]]*' | sed 's/var //' | sort)
    
    # Extract variants from second engine
    variants2=$(echo "uci" | $2 2>/dev/null | grep "option name UCI_Variant" | grep -o 'var [^[:space:]]*' | sed 's/var //' | sort)
    
    # Find intersection of variants (common to both engines)
    variants=$(comm -12 <(echo "$variants1") <(echo "$variants2"))
    
    echo "Common variants found: $(echo $variants | wc -w)"

    # Convert to array for processing
    variant_array=($variants)
else
    # Use provided variants
    variant_array=("${@:4}")
fi

for var in "${variant_array[@]}"
do
    ref=`$1 bench $var $bench_args 2>&1 | grep "Nodes searched  : " | awk '{print $4}'`
    signature=`$2 bench $var $bench_args 2>&1 | grep "Nodes searched  : " | awk '{print $4}'`
    if [ -z "$ref" ]; then
        echo "${var} none ${signature} <-- no reference"
    elif [ -z "$signature" ]; then
        echo "${var} ${ref} none <-- no new"
    elif [ "$ref" != "$signature" ]; then
        echo "${var} ${ref} ${signature} <-- mismatch"
    else
        echo "${var} ${ref} OK"
    fi
done
