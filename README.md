# IPC
Benchmark of shared memory and socket IPC mechanisms on inter-process SPSC  

Run benchmark.sh script to run the benchmark comparison on your device  

Sample result:
<pre>
Comparing build/IPC_benchmark/shm_benchmark to build/IPC_benchmark/socket_benchmark
Benchmark                                 Time             CPU      Time Old      Time New       CPU Old       CPU New
----------------------------------------------------------------------------------------------------------------------
Bench_simulate/1                       +0.1074         +0.1747         25819         28593         16192         19022
Bench_simulate/2                       +0.1296         +0.1625         23730         26805         15079         17529
Bench_simulate/4                       +0.1265         +0.1689         26305         29632         16713         19535
Bench_simulate/8                       +0.1577         +0.2808         25675         29724         16327         20913
Bench_simulate/16                      +0.3663         +0.5304         23790         32503         15329         23460
Bench_simulate/32                      +0.3402         +0.5821         27615         37010         17469         27639
Bench_simulate/64                      +1.0111         +1.6256         25414         51110         16579         43531
Bench_simulate/128                     +1.8977         +2.6039         28342         82126         19419         69984
Bench_simulate/256                     +2.9248         +3.7762         36855        144648         26447        126316
Bench_simulate/512                     +3.4574         +4.0785         60145        268088         46638        236850
Bench_simulate/1024                    +2.3244         +2.4934        152751        507804        137897        481731
Bench_simulate/2048                    +1.8380         +1.9094        338291        960074        318059        925374
Bench_simulate/4096                    +2.0129         +2.2271        632227       1904833        578591       1867184
Bench_simulate/8192                    +1.7883         +1.8431       1358271       3787278       1315481       3740021
Bench_simulate/16384                   +1.5421         +1.5709       2827771       7188576       2756004       7085459
Bench_simulate/32768                   +1.3710         +1.4027       5913399      14020382       5761952      13844136
Bench_simulate/65536                   +1.5018         +1.5206      11850521      29647103      11682219      29446226
Bench_simulate/131072                  +1.3214         +1.3246      24575544      57049877      24364044      56637087
Bench_simulate/262144                  +1.1866         +1.1940      49848305     109000584      49440515     108471103
Bench_simulate/524288                  +1.2671         +1.2790      99466476     225499897      98520061     224530356
Bench_simulate/1048576                 +1.3175         +1.3019     194203411     450066544     193101927     444509644
Bench_simulate/2097152                 +1.2389         +1.2434     393951382     882004007     390270190     875514027
Bench_simulate/4194304                 +1.2272         +1.2315     789095574    1757458681     781008940    1742820005
Bench_simulate/8388608                 +1.3753         +1.3763    1520150286    3610860488    1504725672    3575688034
Bench_simulate/10000000                +1.2580         +1.2656    1868896325    4220058942    1844562513    4179033718
OVERALL_GEOMEAN                        +1.1706         +1.3068             0             0             0             0
</pre>