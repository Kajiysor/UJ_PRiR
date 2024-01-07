#include "DataSupplier.h"
#include "MyForce.h"
#include "SimpleDataSupplier.h"
#include "Simulation.h"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

void sigintHandler(int signum) { std::exit(0); }

class Config
{
public:
	Config(std::string filename)
	{
		std::ifstream file(filename);
		std::string line;
		while (std::getline(file, line)) {
			std::istringstream is_line(line);
			std::string key;
			if (std::getline(is_line, key, '=')) {
				std::string value;
				if (std::getline(is_line, value)) {
					config[key] = value;
				}
			}
		}
		file.close();
	}

	std::string operator[](std::string key) { return config[key]; }
	std::string get(std::string key)
	{
		return config.find(key) != config.end() ? "" : config[key];
	}
	std::string print()
	{
		std::string result = "";
		for (auto const &pair : config) {
			result += pair.first + " = " + pair.second + "\n";
		}
		return result;
	}

private:
	std::map<std::string, std::string> config;
};

void showLoadingBar(int currentStep, int maxSteps,
					std::chrono::duration<double> time)
{
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	int barWidth = w.ws_col - 40;

	float progress = static_cast<float>(currentStep) / maxSteps;

	std::cout << "[";
	int pos = barWidth * progress;
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) {
			std::cout << "=";
		} else if (i == pos) {
			std::cout << ">";
		} else {
			std::cout << " ";
		}
	}
	std::cout << "] " << std::setw(3) << static_cast<int>(progress * 100.0)
			  << "%\t\033[35mRunning for: " << std::setw(5)
			  << std::setprecision(3) << std::fixed << time.count()
			  << "s\033[0m\r";
	std::cout.flush();
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		std::cout << "Usage: " << argv[0]
				  << " <config_file_path> <original/solution>" << std::endl;
		return 1;
	}

	if (!std::filesystem::exists(argv[1])) {
		std::cout << "File " << argv[1] << " does not exist" << std::endl;
		return 1;
	}

	signal(SIGINT, sigintHandler);

	auto configFilepath = std::filesystem::path(argv[1]);
	Config config(configFilepath.string());

	auto reportFilePath =
		std::filesystem::path(configFilepath).parent_path().parent_path() /
		"reports" / argv[2] / configFilepath.filename().string();
	if (!std::filesystem::exists(reportFilePath)) {
		std::filesystem::create_directories(reportFilePath.parent_path());
	}

	std::fstream reportFile(reportFilePath.replace_extension("txt"),
							std::ios::out);

	auto histogramSize = std::stoi(config["HISTOGRAM_SIZE"]);
	double *v = new double[histogramSize];
	Force *force = new MyForce();
	auto *supplier = new SimpleDataSupplier(std::stoi(config["PARTICLES_SQRT"]),
											std::stod(config["DISTANCE"]),
											std::stod(config["MASS"]));

	supplier->initializeData();

	Simulation *simulation =
		new Simulation(force, std::stod(config["DT"]), true);
	simulation->initialize(supplier);

	auto steps = std::stoi(config["STEPS"]);
	int reportPeriod;
	if (config["REPORT_PERIOD"][0] == '/') {
		reportPeriod = steps / std::stoi(config["REPORT_PERIOD"].substr(1));
	} else {
		reportPeriod = std::stoi(config["REPORT_PERIOD"]);
	}

	auto startTime = std::chrono::high_resolution_clock::now();
	auto histogramLengthPerBin = std::stod(config["HISTOGRAM_LENGTH_PER_BIN"]);
	auto showReport = [&](int step, Simulation *s, double *v) {
		s->pairDistribution(v, histogramSize, histogramLengthPerBin);
		auto time = std::chrono::high_resolution_clock::now() - startTime;
		showLoadingBar(step, steps, time);
		reportFile << "Step: " << step << " Ekin = " << s->Ekin()
				   << " <min(NNdistance)> = " << s->avgMinDistance()
				   << std::endl;
		for (int j = 0; j < histogramSize; j++) {
			reportFile << "v[" << j << "] = " << v[j] << std::endl;
		}
	};

	for (int step = 0; step < steps; step++) {
		if (step % reportPeriod == 0) {
			showReport(step, simulation, v);
		}
		simulation->step();
	}
	showReport(steps, simulation, v);
	std::cout << "\033[2K\r";

	delete[] v;
	delete simulation;
	delete supplier;
	delete force;
	reportFile.close();
}