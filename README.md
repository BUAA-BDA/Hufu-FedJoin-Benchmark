![image](https://github.com/BUAA-BDA/Hufu-FedJoin-Benchmark/assets/60478900/ecd62e56-7938-42e0-8843-a1146a2546e3)# FEJ-Bench: A Benchmark for Federated Equi-Joins

The federated equi-join is commonly used in a data federation. It enables the combination of two or more relations held by distinct data owners, depending on the equality of attribute values specified in the join condition. Moreover, the query user remains unaware of any information beyond the query result itself or any details that can be directly inferred from it. Similarly, any data owner is limited to the knowledge of his own input and output, and has no clue about the others’ sensitive data.

<p align="center">
  <img src="https://github.com/BUAA-BDA/Hufu-FedJoin-Benchmark/blob/main/Resources/framework.png" />
</p>

However, previous methods lack a unified comparison within the same experimental benchmark. One reason is that their implementations are either unavailable or non-uniform. Moreover, their methods are evaluated on nearly completely different datasets.

Thus, we have built an open-sourced Benchmark for Federated Equi-Joins, called FEJ-Bench. To foster future research studies. This benchmark is the first of its kind for federated equi-joins, to the best of our knowledge.



## Algorithms Evaluated

Many existing studies have designed novel solutions to answer a federated equi-join, and their proposed solutions can be classified into four categories. The first three are inspired by the seminal ideas of equi-joins over plaintext data: nested-loop based federated equi-joins, sort-merge based federated equi-joins, and hash based federated equijoins. The other line of research leverages the rich techniques of private set intersection (PSI) that securely computes the intersection of two attribute sets (e.g., primary keys) from different data owners.

<p align="center">
  <img src="https://github.com/BUAA-BDA/Hufu-FedJoin-Benchmark/blob/main/Resources/taxonomy2.png" />
</p>

* Nested-Loop based Federated Binary Equi-Joins
  * [VLDB'17] SMCQL: paper[1], code[14]
  * [VLDB'18] Shrinkwrap: paper[2]
  * [EuroSys'19] Conclave: paper[3], code[15]

* Sort-Merge based Federated Binary Equi-Joins
  * [SIGMOD'22] SMJ: paper[4]
  * [VLDB'20] SFD20: paper[5], code[16]

* Hash based Federated Binary Equi-Joins
  * [CCS'21] PPM20: paper[6]

* PSI based Federated Binary Equi-Joins
  * [SIGMOD'21] SecYan: paper[7], code[17]
  * [CCS'22] VOLE-PSI: paper[8], code[18]

<p align="center">
  <img src="https://github.com/BUAA-BDA/Hufu-FedJoin-Benchmark/blob/main/Resources/alg-cmp.png" />
</p>


## Dataset Used

Our experiment study consists of five datasets that have been widely used in existing research, as shown in Table II. Four of them, Slashdot, Jokes, Amazon, and IMDB datasets, are collected from real-world companies. The other one, TPC-H, is one of the most popular benchmark datasets.
* Dataset
  * Slashdot dataset [9]. This dataset is collected by Slashdot, a technology news website with friend/foe links between users.
  * Jokes dataset [10]. The Jokes dataset contains anonymous ratings of jokes by different users of the recommender system Jester developed by UC Berkeley.
  * Amazon dataset [11]. This dataset records the frequently co-purchased products on Amazon’s website.
  * IMDB dataset [12]. Two tables from the original IMDB dataset are used: “title.basics” and “name.basics”. The first table contains information about movies, such as their titles. The second table contains information about actors, such as their representative movies’ names.
  * TPC-H dataset [13]. The TPC-H dataset is a commonlyused benchmark dataset that simulates the real-world business data and queries.
* Data Partition. We consider two ways to determine the partitioned data of the above datasets in each owner.
    * Horizontal Partition. Since the Slashdot, Jokes, and Amazon datasets contain only one table, we randomly separate the tuples into two relations R and S that have the same schema but are held by different data owners.
    * Vertical Partition. By contrast, the IMDB and TPCH datasets have more than one table. Thus, following a vertical partition in existing work, each data owner holds a complete and different table, and these data owners consist a vertical data federation.  


## Performance Evaluation

Based on the pros/cons of these evaluated algorithms, we provide our recommendations for the best solution to federated equi-joins under different scenarios:

<p align="center">
  <img src="https://github.com/BUAA-BDA/Hufu-FedJoin-Benchmark/blob/main/Resources/recommendation.png" />
</p>

For detailed performance evaluation, please refer to our paper *An Experimental Study on Federated Equi-Joins*.


## Uasge
### Requirements

* gcc/g++ >= 8
* cmake >= 3.16
* Boost >= 1.75

### Compile

```shell
git clone --recurse-submodules git@github.com:BUAA-BDA/Hufu-FedJoin-Benchmark.git
cd Hufu-FedJoin-Benchmark
make build
cd build
cmake ..
make -j 8
```

### Run

```shell
# build directory
# parameters role(S/C) + input_file + data_size + algorithm_name

# Server:
./join SERVER ../../Data/TPCH/part.txt 20 80 Conclave

# Client:
./join CLIENT ../../Data/TPCH/partsupp.txt 20 80 Conclave
```



## References

[1] J. Bater, G. Elliott, C. Eggen, S. Goel, A. N. Kho, and J. Rogers, “SMCQL: secure query processing for private data networks,” PVLDB, vol. 10, no. 6, pp. 673–684, 2017.

[2] J. Bater, X. He, W. Ehrich, A. Machanavajjhala, and J. Rogers, “Shrinkwrap: Efficient SQL query processing in differentially private
data federations,” PVLDB, vol. 12, no. 3, pp. 307–320, 2018.

[3] N. Volgushev, M. Schwarzkopf, B. Getchell, M. Varia, A. Lapets, and A. Bestavros, “Conclave: secure multi-party computation on big data,” in EuroSys, 2019, pp. 3:1–3:18.

[4] S. Krastnikov, F. Kerschbaum, and D. Stebila, “Efficient oblivious database joins,” PVLDB, vol. 13, no. 11, pp. 2132–2145, 2020.

[5] Z. Chang, D. Xie, S. Wang, and F. Li, “Towards practical oblivious join,” in SIGMOD, 2022, pp. 803–817.

[6] P. Mohassel, P. Rindal, and M. Rosulek, “Fast database joins and PSI for secret shared data,” in CCS, 2020, pp. 1271–1287.

[7] Y. Wang and K. Yi, “Secure yannakakis: Join-aggregate queries over private data,” in SIGMOD, 2021, pp. 1969–1981.

[8] S. Raghuraman and P. Rindal, “Blazing fast PSI from improved OKVS and subfield VOLE,” in CCS, 2022, pp. 2505–2517.

[9] J. Leskovec, K. J. Lang, A. Dasgupta, and M. W. Mahoney, “Community structure in large networks: Natural cluster sizes and the absence of large well-defined clusters,” Internet Mathematics, vol. 6, no. 1, pp. 29–123, 2009.

[10] K. Y. Goldberg, T. Roeder, D. Gupta, and C. Perkins, “Eigentaste: Aconstant time collaborative filtering algorithm,” Information Retrieval, vol. 4, no. 2, pp. 133–151, 2001.

[11] J. Leskovec, L. A. Adamic, and B. A. Huberman, “The dynamics of viral marketing,” ACM Trans. Web, vol. 1, no. 1, p. 5, 2007.

[12] K. J. Maliszewski, J. Quiane-Ruiz, J. Traub, and V. Markl, “What is the price for joining securely? benchmarking equi-joins in trusted execution environments,” PVLDB, vol. 15, no. 3, pp. 659–672, 2021.

[13] TPC-H, 2023. [Online]. Available: https://www.tpc.org/tpch/.

[14] https://github.com/smcql/smcql

[15] https://github.com/multiparty/conclave

[16] https://git.uwaterloo.ca/skrastni/obliv-join-impl

[17] https://github.com/hkustDB/SECYAN

[18] https://github.com/Visa-Research/volepsi
