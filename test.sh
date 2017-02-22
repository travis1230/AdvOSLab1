echo "ANON MAP PRIVATE SEQUENTIAL"
./print_map 1 0 0 0 0 0
./print_map 1 0 0 0 0 0
./print_map 1 0 0 0 0 0
./print_map 1 0 0 0 0 0
./print_map 1 0 0 0 0 0
./print_map 1 0 0 0 0 0
./print_map 1 0 0 0 0 0
echo "ANON MAP PRIVATE RANDOM"
./print_map 1 0 0 0 0 1 
./print_map 1 0 0 0 0 1 
./print_map 1 0 0 0 0 1 
./print_map 1 0 0 0 0 1 
./print_map 1 0 0 0 0 1 
./print_map 1 0 0 0 0 1 
./print_map 1 0 0 0 0 1 
echo "FILE BACKED MAP PRIVATE SEQUENTIAL"
./print_map 0 0 0 0 0 0
./print_map 0 0 0 0 0 0
./print_map 0 0 0 0 0 0
./print_map 0 0 0 0 0 0
./print_map 0 0 0 0 0 0
./print_map 0 0 0 0 0 0
./print_map 0 0 0 0 0 0
echo "FILE BACKED MAP PRIVATE RANDOM"
./print_map 0 0 0 0 0 1
./print_map 0 0 0 0 0 1
./print_map 0 0 0 0 0 1
./print_map 0 0 0 0 0 1
./print_map 0 0 0 0 0 1
./print_map 0 0 0 0 0 1
./print_map 0 0 0 0 0 1
echo "FILE BACKED SEQUENTIAL MAP SHARED"
./print_map 0 1 0 0 0 0
./print_map 0 1 0 0 0 0
./print_map 0 1 0 0 0 0
./print_map 0 1 0 0 0 0
./print_map 0 1 0 0 0 0
./print_map 0 1 0 0 0 0
./print_map 0 1 0 0 0 0
echo "FILE BACKED RANDOM MAP SHARED"
./print_map 0 1 0 0 0 1
./print_map 0 1 0 0 0 1
./print_map 0 1 0 0 0 1
./print_map 0 1 0 0 0 1
./print_map 0 1 0 0 0 1
./print_map 0 1 0 0 0 1
./print_map 0 1 0 0 0 1
echo "FILE BACKED SEQUENTIAL MAP PRIVATE MAP POPULATE"
./print_map 0 0 1 0 0 0
./print_map 0 0 1 0 0 0
./print_map 0 0 1 0 0 0
./print_map 0 0 1 0 0 0
./print_map 0 0 1 0 0 0
./print_map 0 0 1 0 0 0
./print_map 0 0 1 0 0 0
echo "FILE BACKED RANDOM MAP PRIVATE MAP POPULATE"
./print_map 0 0 1 0 0 1
./print_map 0 0 1 0 0 1
./print_map 0 0 1 0 0 1
./print_map 0 0 1 0 0 1
./print_map 0 0 1 0 0 1
./print_map 0 0 1 0 0 1
./print_map 0 0 1 0 0 1
echo "FILE BACKED SEQUENTIAL MAP PRIVATE MEMSET"
./print_map 0 0 0 1 0 0
./print_map 0 0 0 1 0 0
./print_map 0 0 0 1 0 0
./print_map 0 0 0 1 0 0
./print_map 0 0 0 1 0 0
./print_map 0 0 0 1 0 0
./print_map 0 0 0 1 0 0
echo "FILE BACKED RANDOM MAP PRIVATE MEMSET"
./print_map 0 0 0 1 0 1
./print_map 0 0 0 1 0 1
./print_map 0 0 0 1 0 1
./print_map 0 0 0 1 0 1
./print_map 0 0 0 1 0 1
./print_map 0 0 0 1 0 1
./print_map 0 0 0 1 0 1 
echo "FILE BACKED SEQUENTIAL MAP PRIVATE MEMSET MEMSYNC" 
./print_map 0 0 0 1 1 0
./print_map 0 0 0 1 1 0
./print_map 0 0 0 1 1 0
./print_map 0 0 0 1 1 0
./print_map 0 0 0 1 1 0
./print_map 0 0 0 1 1 0
./print_map 0 0 0 1 1 0
echo "FILE BACKED RANDOM MAP PRIVATE MEMSET MEMSYNC"
./print_map 0 0 0 1 1 1
./print_map 0 0 0 1 1 1
./print_map 0 0 0 1 1 1
./print_map 0 0 0 1 1 1
./print_map 0 0 0 1 1 1
./print_map 0 0 0 1 1 1
./print_map 0 0 0 1 1 1
