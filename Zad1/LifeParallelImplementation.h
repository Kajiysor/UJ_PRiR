// LifeParallelImplementation.h
#ifndef LIFEPARALLELIMPLEMENTATION_H_
#define LIFEPARALLELIMPLEMENTATION_H_

#include "Life.h"
#include <mpi.h>

class LifeParallelImplementation : public Life
{
private:
    MPI_Comm comm_; // MPI communicator
    int rank_;      // Rank of the current process
    int procSize_;  // Total number of processes
    int firstRow_;  // Index of the first row in the current process
    int lastRow_;   // Index of the last row in the current process
    bool afterLastStep_;

    void exchangeBorders();

public:
    LifeParallelImplementation();
    virtual ~LifeParallelImplementation();

    int numberOfLivingCells();
    double averagePollution();
    void oneStep() override;
    void realStep() override;
    void beforeFirstStep() override;
    void afterLastStep() override;
};

#endif /* LIFEPARALLELIMPLEMENTATION_H_ */
