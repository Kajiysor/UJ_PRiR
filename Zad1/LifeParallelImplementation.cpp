// LifeParallelImplementation.cpp
#include "LifeParallelImplementation.h"
#include <mpi.h>

LifeParallelImplementation::LifeParallelImplementation()
{
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &procSize_);
}

LifeParallelImplementation::~LifeParallelImplementation()
{
}

void LifeParallelImplementation::exchangeBorderRowsInfo()
{
    // send the first row to the previous process
    if (rank_ != 0)
    {
        MPI_Send(cells[firstRow_], size, MPI_INT, rank_ - 1, 0, MPI_COMM_WORLD);
        MPI_Send(pollution[firstRow_], size, MPI_INT, rank_ - 1, 0, MPI_COMM_WORLD);
    }
    // receive the first row from the next process
    if (rank_ != procSize_ - 1)
    {
        MPI_Recv(cells[lastRow_], size, MPI_INT, rank_ + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(pollution[lastRow_], size, MPI_INT, rank_ + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    // send the last row to the next process
    if (rank_ != procSize_ - 1)
    {
        MPI_Send(cells[lastRow_ - 1], size, MPI_INT, rank_ + 1, 0, MPI_COMM_WORLD);
        MPI_Send(pollution[lastRow_ - 1], size, MPI_INT, rank_ + 1, 0, MPI_COMM_WORLD);
    }
    // receive the last row from the previous process
    if (rank_ != 0)
    {
        MPI_Recv(cells[firstRow_ - 1], size, MPI_INT, rank_ - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(pollution[firstRow_ - 1], size, MPI_INT, rank_ - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

void LifeParallelImplementation::realStep()
{
    if (procSize_ > 1)
    {
        exchangeBorderRowsInfo(); // exchange borders before updating the cells
    }

    int currentState, currentPollution;
    for (int row = firstRow_; row < lastRow_; row++)
        for (int col = 1; col < size_1; col++)
        {
            currentState = cells[row][col];
            currentPollution = pollution[row][col];
            cellsNext[row][col] = rules->cellNextState(currentState, liveNeighbours(row, col),
                                                       currentPollution);
            pollutionNext[row][col] =
                rules->nextPollution(currentState, currentPollution, pollution[row + 1][col] + pollution[row - 1][col] + pollution[row][col - 1] + pollution[row][col + 1],
                                     pollution[row - 1][col - 1] + pollution[row - 1][col + 1] + pollution[row + 1][col - 1] + pollution[row + 1][col + 1]);
        }
}

void LifeParallelImplementation::oneStep()
{
    realStep();
    swapTables();
}

int LifeParallelImplementation::numberOfLivingCells()
{
    if (!afterLastStep_)
    {
        afterLastStep();
    }
    return sumTable(cells);
}

double LifeParallelImplementation::averagePollution()
{
    if (!afterLastStep_)
    {
        afterLastStep();
    }
    return (double)sumTable(pollution) / size_1_squared / rules->getMaxPollution();
}

void LifeParallelImplementation::beforeFirstStep()
{
    if (procSize_ > 1)
    {
        if (rank_ == 0)
        {
            // split the board into equal parts for each process
            int rowsPerProcess = size / procSize_;
            int rowsLeft = size % procSize_;
            firstRow_ = 0;
            lastRow_ = 0;
            for (int procNum = 0; procNum < procSize_; procNum++)
            {
                firstRow_ = lastRow_;
                lastRow_ = firstRow_ + rowsPerProcess;
                if (procNum == procSize_ - 1)
                {
                    lastRow_ += rowsLeft - 1;
                }
                if (procNum == 0)
                {
                    continue;
                }
                // send the information to all other processes
                MPI_Send(&firstRow_, 1, MPI_INT, procNum, 0, MPI_COMM_WORLD);
                MPI_Send(&lastRow_, 1, MPI_INT, procNum, 0, MPI_COMM_WORLD);
                for (int j = firstRow_; j < lastRow_; j++)
                {
                    MPI_Send(cells[j], size, MPI_INT, procNum, 0, MPI_COMM_WORLD);
                    MPI_Send(pollution[j], size, MPI_INT, procNum, 0, MPI_COMM_WORLD);
                }
            }
            firstRow_ = 1;
            lastRow_ = rowsPerProcess;
        }
        else
        {
            // receive all the information from the root process
            MPI_Recv(&firstRow_, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&lastRow_, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i = firstRow_; i < lastRow_; i++)
            {
                MPI_Recv(cells[i], size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(pollution[i], size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }
    else
    {
        firstRow_ = 1;
        lastRow_ = size_1;
    }
}

void LifeParallelImplementation::afterLastStep()
{
    if (procSize_ > 1)
    {
        // send the table from all processes to the root process
        if (rank_ == 0)
        {
            // receive the table from all other processes
            for (int procNum = 1; procNum < procSize_; procNum++)
            {
                MPI_Recv(&firstRow_, 1, MPI_INT, procNum, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&lastRow_, 1, MPI_INT, procNum, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int i = firstRow_; i < lastRow_; i++)
                {
                    MPI_Recv(cells[i], size, MPI_INT, procNum, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(pollution[i], size, MPI_INT, procNum, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
            afterLastStep_ = true;
        }
        else
        {
            // send the table from all processes to the root process
            MPI_Send(&firstRow_, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&lastRow_, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            for (int i = firstRow_; i < lastRow_; i++)
            {
                MPI_Send(cells[i], size, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(pollution[i], size, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        }
    }
}
