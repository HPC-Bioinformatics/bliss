/**
 * @file    test_threads.cpp
 * @ingroup
 * @author  tpan
 * @brief
 * @details
 *
 * Copyright (c) 2014 Georgia Institute of Technology.  All Rights Reserved.
 *
 * TODO add License
 */
#include "config.hpp"


#include "utils/logging.h"

#include "common/alphabets.hpp"


#include "index/KmerIndex.hpp"
#include "common/Kmer.hpp"
#include "common/base_types.hpp"
#include <string>
#include <sstream>
#include "utils/KmerUtils.hpp"
/*
 * TYPE DEFINITIONS
 */


/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {

  //////////////// init logging
  LOG_INIT();

  //////////////// parse parameters

  int nthreads = 1;
#ifdef USE_OPENMP
  if (argc > 1)
  {
    nthreads = atoi(argv[1]);
    if (nthreads == -1)
      nthreads = omp_get_max_threads();
  }
  omp_set_nested(1);
  omp_set_dynamic(0);
#else
  FATALF("NOT compiled with OPENMP")
#endif

  int chunkSize = sysconf(_SC_PAGE_SIZE);
  if (argc > 2)
  {
    chunkSize = atoi(argv[2]);
  }

  //std::string filename("/home/tpan/src/bliss/test/data/test.medium.fastq");
  std::string filename("/home/tpan/src/bliss/test/data/test.fastq");
  //std::string filename("/mnt/data/1000genome/HG00096/sequence_read/SRR077487_1.filt.fastq");
  if (argc > 3)
  {
    filename.assign(argv[3]);
  }

  int nprocs = 1;
  int rank = 0;
  //////////////// initialize MPI and openMP
#ifdef USE_MPI


  if (nthreads > 1) {

    int provided;

    // one thread will be making all MPI calls.
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    if (provided < MPI_THREAD_FUNNELED) {
      ERRORF("The MPI Library Does not have thread support.");
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  } else {
    MPI_Init(&argc, &argv);
  }

  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Comm_size(comm, &nprocs);
  MPI_Comm_rank(comm, &rank);


  if (rank == 0)
    INFOF("USE_MPI is set");
#else
  static_assert(false, "MPI used although compilation is not set to use MPI");
#endif


  std::chrono::high_resolution_clock::time_point t1, t2;
  std::chrono::duration<double> time_span;
#if defined(KMOLECULEINDEX)
  {
    t1 = std::chrono::high_resolution_clock::now();
    // initialize index
    INFOF("***** initializing index.");
    bliss::index::KmerPositionIndexOld<21, DNA, bliss::io::FASTQ, true> kmer_index(comm, nprocs, nthreads);

    // start processing.  enclosing with braces to make sure loader is destroyed before MPI finalize.
    INFOF("***** building index first pass.");

    kmer_index.build(filename, nthreads, chunkSize);
    //kmer_index.flush();

  //  MPI_Barrier(comm);
    //INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());
    DEBUG("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());

    INFOF("COUNT %d index built pass 1 with index size: %ld", rank, kmer_index.local_size());


    kmer_index.finalize();


    t2 = std::chrono::high_resolution_clock::now();
    time_span =
        std::chrono::duration_cast<std::chrono::duration<double>>(
            t2 - t1);
    INFO("old position index: time " << time_span.count());
  }
#elif defined(KMERINDEX)
  {
    t1 = std::chrono::high_resolution_clock::now();
  // initialize index
  INFOF("***** initializing index.");
  bliss::index::KmerPositionIndex<21, DNA, bliss::io::FASTQ, true> kmer_index(comm, nprocs, nthreads);

  // start processing.  enclosing with braces to make sure loader is destroyed before MPI finalize.
  INFOF("***** building index first pass.");

  kmer_index.build(filename, nthreads, chunkSize);
  //kmer_index.flush();

//  MPI_Barrier(comm);
  //INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());
  INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());


  INFOF("COUNT %d index built pass 1 with index size: %ld", rank, kmer_index.local_size());


  kmer_index.finalize();
  MPI_Barrier(comm);
  t2 = std::chrono::high_resolution_clock::now();
  time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(
          t2 - t1);
  INFO("new position index: time " << time_span.count());

  }


#endif
  MPI_Barrier(comm);

#if defined(KMOLECULEINDEX)
  {
    t1 = std::chrono::high_resolution_clock::now();
  // initialize index
  INFOF("***** initializing index.");
  bliss::index::KmerCountIndexOld<21, DNA, bliss::io::FASTQ, true> kmer_index(comm, nprocs, nthreads);

  // start processing.  enclosing with braces to make sure loader is destroyed before MPI finalize.
  INFOF("***** building index first pass.");

  kmer_index.build(filename, nthreads, chunkSize);
  //kmer_index.flush();

//  MPI_Barrier(comm);
  //INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());
  INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());

  INFOF("COUNT %d index built pass 1 with index size: %ld", rank, kmer_index.local_size());



  for (auto it = kmer_index.getLocalIndex().cbegin(); it != kmer_index.getLocalIndex().cend(); ++it) {
    INFOF("Entry: Key=%s (%s), val = %d", it->first.toString().c_str(), bliss::utils::KmerUtils::toASCIIString(it->first).c_str(), it->second);
  }


  kmer_index.finalize();
  MPI_Barrier(comm);
  t2 = std::chrono::high_resolution_clock::now();
  time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(
          t2 - t1);
  INFO("old count index: time " << time_span.count());

  }
#elif defined(KMERINDEX)
  {
    t1 = std::chrono::high_resolution_clock::now();
  // initialize index
  INFOF("***** initializing index.");
  bliss::index::KmerCountIndex<21, DNA, bliss::io::FASTQ, true> kmer_index(comm, nprocs, nthreads);

  // start processing.  enclosing with braces to make sure loader is destroyed before MPI finalize.
  INFOF("***** building index first pass.");

  kmer_index.build(filename, nthreads, chunkSize);
  //kmer_index.flush();

//  MPI_Barrier(comm);
  //INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());
  INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());


  INFOF("COUNT %d index built pass 1 with index size: %ld", rank, kmer_index.local_size());

  for (auto it = kmer_index.getLocalIndex().cbegin(); it != kmer_index.getLocalIndex().cend(); ++it) {
    INFOF("Entry: Key=%s (%s), val = %d", it->first.toString().c_str(), bliss::utils::KmerUtils::toASCIIString(it->first).c_str(), it->second);
  }


  kmer_index.finalize();
  MPI_Barrier(comm);
  t2 = std::chrono::high_resolution_clock::now();
  time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(
          t2 - t1);
  INFO("new count index: time " << time_span.count());

  }

#endif
  MPI_Barrier(comm);




#if defined(KMOLECULEINDEX)

  // with quality score....
  {
    t1 = std::chrono::high_resolution_clock::now();
  // initialize index
  INFOF("***** initializing index.");
  bliss::index::KmerPositionAndQualityIndexOld<21, DNA, bliss::io::FASTQ, true> kmer_index(comm, nprocs, nthreads);

  // start processing.  enclosing with braces to make sure loader is destroyed before MPI finalize.
  INFOF("***** building index first pass.");

  kmer_index.build(filename, nthreads, chunkSize);
  //kmer_index.flush();

//  MPI_Barrier(comm);
  //INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());
  INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());


  INFOF("COUNT %d index built pass 1 with index size: %ld", rank, kmer_index.local_size());



  kmer_index.finalize();
  MPI_Barrier(comm);
  t2 = std::chrono::high_resolution_clock::now();
  time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(
          t2 - t1);
  INFO("old position/quality index: time " << time_span.count());

  }
#elif defined(KMERINDEX)

  {
    t1 = std::chrono::high_resolution_clock::now();
  // initialize index
  INFOF("***** initializing index.");
  bliss::index::KmerPositionAndQualityIndex<21, DNA, bliss::io::FASTQ, true> kmer_index(comm, nprocs, nthreads);

  // start processing.  enclosing with braces to make sure loader is destroyed before MPI finalize.
  INFOF("***** building index first pass.");

  kmer_index.build(filename, nthreads, chunkSize);
  //kmer_index.flush();

//  MPI_Barrier(comm);
  //INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());
  INFO("COUNT " << rank << " Index Building 1 for " << filename << " using " << nthreads << " threads, index size " << kmer_index.local_size());


  INFOF("COUNT %d index built pass 1 with index size: %ld", rank, kmer_index.local_size());


  kmer_index.finalize();
  MPI_Barrier(comm);
  t2 = std::chrono::high_resolution_clock::now();
  time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(
          t2 - t1);
  INFO("new positionAndQuality index: time " << time_span.count());

  }

#endif
  MPI_Barrier(comm);

  //////////////  clean up MPI.
  MPI_Finalize();

  DEBUGF("M Rank %d called MPI_Finalize", rank);



  return 0;
}
