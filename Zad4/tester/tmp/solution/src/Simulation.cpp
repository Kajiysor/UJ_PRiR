/*
 * Simulation.cpp
 *
 *  Created on: 10 gru 2023
 *      Author: oramus
 */

#include "Simulation.h"
#include <chrono>
#include <omp.h>

#include <math.h>
#include <omp.h>

#include <iostream>

using namespace std;

Simulation::Simulation(Force *_force, double _dt, bool _molecularStatic)
{
    force = _force;
    particles = {};
    x = y = m = Fx = Fy = Vx = Vy = {};

    dt = _dt;
    dt_2 = dt / 2.0;
    molecularStatic = _molecularStatic;
}

void Simulation::initialize(DataSupplier *supplier)
{
	const auto start = std::chrono::system_clock::now();
    particles = supplier->points();
    allocateMemory();

#pragma omp parallel for
    for (int idx = 0; idx < particles; idx++)
    {
        x[idx] = supplier->x(idx);
        y[idx] = supplier->y(idx);
        m[idx] = supplier->m(idx);
        Vx[idx] = Vy[idx] = Fx[idx] = Fy[idx] = {0.0};
    }
#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "void Simulation::initialize(DataSupplier *supplier): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
}

void Simulation::allocateMemory()
{
	const auto start = std::chrono::system_clock::now();
    x = new double[particles];
    y = new double[particles];
    m = new double[particles];
    Fx = new double[particles];
    Fy = new double[particles];
    Vx = new double[particles];
    Vy = new double[particles];
#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "void Simulation::allocateMemory(): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
}

void Simulation::step()
{
	const auto start = std::chrono::system_clock::now();
    updateVelocity();
    updatePosition();
    if (molecularStatic)
        preventMoveAgainstForce();
#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "void Simulation::step(): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
}

void Simulation::updateVelocity()
{
	const auto start = std::chrono::system_clock::now();
    double oldFx, oldFy;
    double dx, dy, distance, frc;

#pragma omp parallel for private(oldFx, oldFy, dx, dy, distance, frc)
    for (int idx = 0; idx < particles; idx++)
    {
        oldFx = Fx[idx];
        oldFy = Fy[idx];
        Fx[idx] = Fy[idx] = 0.0;
        for (int idx2 = 0; idx2 < idx; idx2++)
        {
            dx = x[idx2] - x[idx];
            dy = y[idx2] - y[idx];

            distance = sqrt(dx * dx + dy * dy);

            frc = force->value(distance);

            Fx[idx] += frc * dx / distance;
            Fy[idx] += frc * dy / distance;
        }

        for (int idx2 = idx + 1; idx2 < particles; idx2++)
        {
            dx = x[idx2] - x[idx];
            dy = y[idx2] - y[idx];

            distance = sqrt(dx * dx + dy * dy);

            frc = force->value(distance);

            Fx[idx] += frc * dx / distance;
            Fy[idx] += frc * dy / distance;
        }
        Vx[idx] += dt_2 * (Fx[idx] + oldFx) / m[idx];
        Vy[idx] += dt_2 * (Fy[idx] + oldFy) / m[idx];
    }
#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "void Simulation::updateVelocity(): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
}

void Simulation::updatePosition()
{
	const auto start = std::chrono::system_clock::now();
#pragma omp parallel for
    for (int idx = 0; idx < particles; idx++)
    {
        x[idx] += dt * (Vx[idx] + dt_2 * Fx[idx] / m[idx]);
        y[idx] += dt * (Vy[idx] + dt_2 * Fy[idx] / m[idx]);
    }
#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "void Simulation::updatePosition(): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
}

void Simulation::preventMoveAgainstForce()
{
	const auto start = std::chrono::system_clock::now();
    double dotProduct;
#pragma omp parallel for private(dotProduct)
    for (int idx = 0; idx < particles; idx++)
    {
        dotProduct = Vx[idx] * Fx[idx] + Vy[idx] * Fy[idx];
        if (dotProduct < 0.0)
        {
            Vx[idx] = Vy[idx] = {0.0};
        }
    }
#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "void Simulation::preventMoveAgainstForce(): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
}

double Simulation::Ekin()
{
	const auto start = std::chrono::system_clock::now();
    double ek = 0.0;

#pragma omp parallel for reduction(+ : ek)
    for (int idx = 0; idx < particles; idx++)
    {
        ek += m[idx] * (Vx[idx] * Vx[idx] + Vy[idx] * Vy[idx]) * 0.5;
    }

#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "double Simulation::Ekin(): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
    return ek;
}

void Simulation::pairDistribution(double *histogram, int size, double coef)
{
	const auto start = std::chrono::system_clock::now();
    for (int i = 0; i < size; i++)
        histogram[i] = 0;

    const double maxDistanceSQ = size * coef * size * coef;
    double dx, dy;
    double distance;
    int idx;

#pragma omp parallel for private(dx, dy, distance, idx)
    for (int idx1 = 0; idx1 < particles; idx1++)
    {
        for (int idx2 = 0; idx2 < idx1; idx2++)
        {
            dx = x[idx2] - x[idx1];
            dy = y[idx2] - y[idx1];
            distance = dx * dx + dy * dy;
            if (distance < maxDistanceSQ)
            {
                distance = sqrt(distance);
                idx = (int)(distance / coef);
#pragma omp atomic
                histogram[idx]++;
            }
        }
    }

#pragma omp parallel for
    for (int i = 0; i < size; i++)
    {
        distance = (i + 0.5) * coef;
        histogram[i] *= 1.0 / (2.0 * M_PI * distance * coef);
    }
#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "void Simulation::pairDistribution(double *histogram, int size, double coef): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
}

double Simulation::avgMinDistance()
{
	const auto start = std::chrono::system_clock::now();
    double sum = {};

#pragma omp parallel for reduction(+ : sum)
    for (int i = 0; i < particles; i++)
        sum += minDistance(i);

#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "double Simulation::avgMinDistance(): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
    return sum / particles;
}

double Simulation::minDistance(int idx)
{
	const auto start = std::chrono::system_clock::now();
    double dSqMin = 10000000.0;
    double dx, dy, distanceSQ;

    double xx = x[idx];
    double yy = y[idx];

    for (int i = 0; i < idx; i++)
    {
        dx = xx - x[i];
        dy = yy - y[i];
        distanceSQ = dx * dx + dy * dy;
        if (distanceSQ < dSqMin)
            dSqMin = distanceSQ;
    }
    for (int i = idx + 1; i < particles; i++)
    {
        dx = xx - x[i];
        dy = yy - y[i];
        distanceSQ = dx * dx + dy * dy;
        if (distanceSQ < dSqMin)
            dSqMin = distanceSQ;
    }

#pragma omp barrier
	const auto end = std::chrono::system_clock::now();
	const std::chrono::duration<double> elapsed_seconds = end - start;
	fprintf(stderr, "double Simulation::minDistance(int idx): %.10fs\n", elapsed_seconds.count());
	std::cerr.flush();
    return sqrt(dSqMin);
}

Simulation::~Simulation() {}
