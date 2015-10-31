#Odd Even Sort
Implement parallel Odd-Even Sort with MPI.

##Experiment System Spec 
8 nodes, each has:
- Intel Xeon CPU L5640@2.27GHz (2x6 cores)
- 4 GB memory
- 2TB HDD Storage

##Difference implementation
###Basic:
Basic version of odd-even sort is strictly limited, each element can only be swapped with its adjacent elements in each operation.
In other words, take the entire list of numbers and apply classic odd-even sort directly in parallel.
###Advanced:
The only restriction is that each MPI process can only send messages to its neighbor processes. The number of elements sent in each message can also be arbitrary. 
Advanced version should achieve better performance than basic version.

##Tools
- parse binary integers and print out
```
./tools/parse_int.py
```

- generate experiment shell job
```
./tools/experiment_gen.py ($testcase_number)
```

- validate the ouput and generate report of testing jobs
```
./tools/report.py ($testcase_number)
```

