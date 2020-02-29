#!/bin/sh

prog='ns_shell'
echo '<<<<<'PROGRAM: $prog.c'>>>>>';
echo "Mm: make / M: make+run / Rr: run / Vv: valgrind / Ee: exit"

while true; do
	read -p "option: " in
	case $in in
		[Mm]* )
			make
			;;
		[Rr]* ) 
			./$prog
			;;
		[Vv]* )
			echo 'valgrind --tool=memcheck --leak-check=full --track-origins=yes ./'"$prog"
			valgrind --tool=memcheck --leak-check=full --track-origins=yes ./"$prog"
			;;
		[Ee]* ) 
			exit 0
			;;
	esac
done