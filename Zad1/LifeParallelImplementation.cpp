// LifeParallelImplementation.cpp
#include "LifeParallelImplementation.h"
#include <iostream>

LifeParallelImplementation::LifeParallelImplementation()
{
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &procSize_);
}

LifeParallelImplementation::~LifeParallelImplementation()
{
}

void LifeParallelImplementation::exchangeBorders()
{
}

void LifeParallelImplementation::realStep()
{
    exchangeBorders(); // Exchange borders before updating the cells

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
    if (afterLastStep_)
    {
        return sumTable(cells);
    }
    return -1;
}

double LifeParallelImplementation::averagePollution()
{
    if (afterLastStep_)
    {
        return (double)sumTable(pollution) / size_1_squared / rules->getMaxPollution();
    }
    return -1;
}

void LifeParallelImplementation::beforeFirstStep()
{
    // distribute the table from the root process to all other processes, the table should be split into as many parts as there are processes
    if (rank_ == 0)
    {
        // split the board into equal parts for each process
        int rowsPerProcess = size / procSize_;
        int rowsLeft = size % procSize_;
        int startRow = 0;
        int endRow = 0;
        for (int procNum = 0; procNum < procSize_; procNum++)
        {
            startRow = endRow;
            endRow = startRow + rowsPerProcess;
            firstRow_ = startRow;
            if (procNum == procSize_ - 1)
            {
                endRow += rowsLeft - 1;
            }
            lastRow_ = endRow;
            if (procNum == 0)
            {
                continue;
            }
            std::cout << "Root process sending to process " << procNum << "\n";
            std::cout << "First row: " << firstRow_ << "\n";
            std::cout << "Last row: " << lastRow_ << "\n";
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
        std::cout << "Process " << rank_ << " receiving from root process\n";
        std::cout << "First row: " << firstRow_ << "\n";
        std::cout << "Last row: " << lastRow_ << "\n";
    }
}

void LifeParallelImplementation::afterLastStep()
{
}
