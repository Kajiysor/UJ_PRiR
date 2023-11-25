// LifeParallelImplementation.h
#ifndef LIFEPARALLELIMPLEMENTATION_H_
#define LIFEPARALLELIMPLEMENTATION_H_

#include "Life.h"
#include <mpi.h>

class LifeParallelImplementation : public Life
{
private:
    MPI_Comm comm_;      // MPI communicator
    int rank_;           // rank of the current process
    int procSize_;       // total number of processes
    int firstRow_;       // index of the first row in the current process
    int lastRow_;        // index of the last row in the current process
    bool afterLastStep_; // true if the last step has been performed

    void exchangeBorderRowsInfo();

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
