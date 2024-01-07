#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
void sigintHandler(int signum) { std::exit(0); }

struct functionData {
	std::string name;
	long double time;
};

class TimeChecker
{
public:
	TimeChecker(std::string filename, std::string otherFilename)
	{
		std::ifstream file(filename);
		if (!file.is_open()) {
			std::cout << "Cannot open file " << filename << std::endl;
			return;
		}
		std::string line;
		while (std::getline(file, line)) {
			original.push_back(line);
		}
		file.close();

		std::ifstream otherFile(otherFilename);
		if (!otherFile.is_open()) {
			std::cout << "Cannot open file " << otherFilename << std::endl;
			return;
		}
		while (std::getline(otherFile, line)) {
			solution.push_back(line);
		}
		otherFile.close();
	}

	void check()
	{
		auto getData = [](const std::string &line) -> functionData {
			std::regex pattern(R"(.*(Simulation::.*): (.*)s)");
			std::smatch match;
			if (!std::regex_search(line, match, pattern)) {
				return {};
			}
			std::regex_search(line, match, pattern);
			return {match[1], std::stod(match[2])};
		};

		auto getTimes = [&](const std::vector<std::string> &data) {
			std::map<std::string, long double> times;
			for (int i = 0; i < data.size() - 1; i++) {
				auto fnData = getData(data[i]);
				if (times.find(fnData.name) == times.end()) {
					times[fnData.name] = fnData.time;
				} else {
					times[fnData.name] += fnData.time;
				}
			}
			return times;
		};

		auto originalTimes = getTimes(original);
		auto solutionTimes = getTimes(solution);

		std::vector<std::string> keys;
		for (const auto &key : originalTimes) {
			keys.push_back(key.first);
		}

		auto tokenizeTimeOutput = [&](const std::string &input) {
			std::regex pattern("\\s+");
			std::sregex_token_iterator tokens(input.begin(), input.end(),
											  pattern, -1);
			std::sregex_token_iterator end;
			std::vector<std::string> result(tokens, end);
			return result;
		};

		const auto tokenizedTimeOutputOriginal =
			tokenizeTimeOutput(original.back());
		const auto tokenizedTimeOutputSolution =
			tokenizeTimeOutput(solution.back());

		std::vector<std::string> timeKeys = {"userTime", "systemTime",
											 "elapsedTime", "CPU"};
		int i = 0;
		for (const auto &key : timeKeys) {
			keys.push_back(key);
			originalTimes[key] = std::stold(tokenizedTimeOutputOriginal[i]);
			solutionTimes[key] = std::stold(tokenizedTimeOutputSolution[i]);
			i++;
		}

		auto maxLength =
			std::max_element(keys.begin(), keys.end(),
							 [](const std::string &a, const std::string &b) {
								 return a.length() < b.length();
							 });

		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		const auto fullLineSeparator = std::string(w.ws_col, '-');

		std::cout << "\033[35m" << fullLineSeparator << "\033[0m\n";
		std::cout << std::left << std::setw(maxLength->length() + 12)
				  << " \033[1;34m" << std::setw(10)
				  << "Original <?> Solution (s)\033[0m\n";
		for (const auto &key : keys) {
			bool originalFaster = (originalTimes[key] > solutionTimes[key]);
			if (key == "userTime") {
				std::cout << "\033[35m" << fullLineSeparator << "\033[0m\n";
			}
			if (key == "CPU") {
				std::cout << (originalFaster ? "\033[1;31m" : "\033[1;32m");
			} else {
				std::cout << (originalFaster ? "\033[1;32m" : "\033[1;33m");
			}
			std::cout << std::left << std::setw(maxLength->length() + 2) << key
					  << ": " << originalTimes[key]
					  << (originalFaster ? " > " : " < ") << solutionTimes[key]
					  << std::right << std::setw(5) << "("
					  << std::setprecision(5) << std::fixed << std::left
					  << (std::abs(solutionTimes[key] - originalTimes[key]) /
						  originalTimes[key]) *
							 100
					  << "%)\033[0m\n";
		}
		std::cout << "\033[35m" << fullLineSeparator << "\033[0m\n";
	}

private:
	std::vector<std::string> original;
	std::vector<std::string> solution;
};

int main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cout << "Usage: " << argv[0]
				  << " <original_time_output_file> <solution_time_output_file>"
				  << std::endl;
		return 1;
	}
	signal(SIGINT, sigintHandler);

	TimeChecker checker(argv[1], argv[2]);
	checker.check();
	return 0;
}