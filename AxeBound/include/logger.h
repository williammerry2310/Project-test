#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <queue>
#include <filesystem>   // used to create the logs folder if it is missing


// 15/10/26 03/11/26
// How to use
// init - logger log(int mode, "path to logs folder")
// modes - 0=all, 1=all-debug, 2=all-debug&warn, 3=all-debug&error, 4=info, 5=debug+error, 6=disabled
// message commands - info("msg"); error("msg"); warn("msg"); debug("msg");
// then once per frame/second/updateTime log.writeQueue() dumps the last 10 items to the file
// the max ouput may be set to a var on init?
// then log.dispose() will close the file

class logger{
	private:
		// colour codes
		const std::string c_default = "\033[0m";
		const std::string c_red = "\033[31m";
		const std::string c_green = "\033[32m";
		const std::string c_yellow = "\033[33m";
		const std::string c_blue = "\033[34m";
		
		// modes                   // type a binary string for which ones you want to ouput eg 011111111 (everything but debug)
		bool debugEnabled;         // 0
		bool infoEnabled;          // 1 
		bool warnEnabled;          // 2
		bool errorEnabled;         // 3
		bool fatalEnabled;         // 4
		bool criticalEnabled;      // 5
		bool violationEnabled;     // 6
		bool valueErrorEnabled;    // 7
		bool typeErrorEnabled;     // 8
		
		std::string timeCreated;
		std::queue<std::string> outputQueue;//output queue
		std::ofstream outFile;
		const int maxOutput = 10; // limits max output per frame to prevent massive lag
		
		void output(const std::string& msg, const std::string colour){
			std::cout << colour << msg << c_default << std::endl; //writes to console
			if (outFile.is_open()) outputQueue.push(msg); // only queue if there is a file to write to (stops the queue growing forever)
		}
	public:
		logger(const std::string& logModes = "011111000", std::string logsPath = "logs/"){ // default remove any errors that the user doesnt need to see as they are used in testing/dev or should be! eg there SHOULD NOT be a logic error in a release!!!
			debugEnabled = (logModes.length() > 0 && logModes[0] == '1');
			infoEnabled = (logModes.length() > 1 && logModes[1] == '1');
			warnEnabled = (logModes.length() > 2 && logModes[2] == '1');
			errorEnabled = (logModes.length() > 3 && logModes[3] == '1');
			fatalEnabled = (logModes.length() > 4 && logModes[4] == '1');
			criticalEnabled = (logModes.length() > 5 && logModes[5] == '1');
			violationEnabled = (logModes.length() > 6 && logModes[6] == '1');
			valueErrorEnabled = (logModes.length() > 7 && logModes[7] == '1');
			typeErrorEnabled = (logModes.length() > 8 && logModes[8] == '1');
			
			std::time_t now = std::time(nullptr);
			char buffer[25];
			std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
			timeCreated = std::string(buffer);
			
			// CHANGED: file names cannot contain ':' on Windows, and the folder must exist before the file can be made
			std::string safeTime = timeCreated;
			for (char &c : safeTime) { if (c == ':' || c == ' ') c = '-'; }
			std::error_code ec;
			std::filesystem::create_directories(logsPath, ec);
			if (!logsPath.empty() && logsPath.back() != '/' && logsPath.back() != '\\') logsPath += "/";
			outFile.clear();
			outFile.open(logsPath + std::string("log-") + safeTime + std::string(".txt"), std::ios::trunc); // dont really need time created to be stored?
			if (!outFile.is_open()) {critical("Can NOT Create Log File!");}
		}
		// error tpyes
		void info(const std::string& msg){ // used for info, like assets loaded succesfully
			if (infoEnabled) {
				output("[INFO] " + msg, c_green);
			}
		}
		void error(const std::string& msg){ // used for general errors eg cant save game. not critical or fatal as game still works mostly apart from (EXAMPLE)  it cant save which doesnt break the program in anyway.
			if (errorEnabled) {
				output("[ERROR] " + msg, c_red);  // WHEN WRITING ERRORS IS IT RECORMENDED TO SAY WHICH FUNCTION/CLASS IT WAS FLAGGED FROM <<< 
			}
		}
		void warn(const std::string& msg){ // used for warnings eg cant connect to a server?
			if (warnEnabled) {
				output("[WARN] " + msg, c_yellow);
			}
		}
		void debug(const std::string& msg){ // used for debuging - eg: log.debug(value) > [debug] 2
			if (debugEnabled) {
				output("[DEBUG] " + msg, c_blue);
			}
		}
		void fatal(const std::string& msg){ // used for FATAL errors - eg: game.loadAssets(tilePath) > [FATAL] STOPPING! No Assetts found, assetts required for game to function!
			if (fatalEnabled) {
				output("[FATAL] " + msg, c_red);
			}
		}
		void critical(const std::string& msg){ // used for critical errors - eg: game.loadFont(fontPath) > [CRITICAL] No font found, this will cause missing text! // for bad errors that wont cause the program to crash but will cause issues eg with no text
			if (criticalEnabled) {
				output("[CRITICAL] " + msg, c_red);
			}
		}
		void violation(const std::string& msg){ // used for logic errors - eg: ui.reSize(newSize) > [VIOLATION] Size has not changed!
			if (violationEnabled) {
				output("[VIOLATION] " + msg, c_red);
			}
		}
		void valueError(const std::string& msg){ // used for value errors - eg: ui.reSize(newSize) > [VALUE] Size is wrong! Can NOT be Larger than the screen! // probably not used often msotly for incorrect values
			if (valueErrorEnabled) {
				output("[VALUE] " + msg, c_red);
			}
		}
		void typeError(const std::string& msg){ // used for type errors eg: ui.reSize(newSize) > [TYPE] Was passed string NOT a Vector2!
			if (typeErrorEnabled) {
				output("[TYPE] " + msg, c_red);
			}
		}
		void writeQueue(){ // call once per frame
			if (!outFile.is_open()) {return;} // file not open
			if (outputQueue.empty()) {return;} //queue is empty, nothing to write
			int loopCounter = 0;
			while (!outputQueue.empty()){
				loopCounter+=1;
				if (loopCounter >= maxOutput) {return;}
				//write to file
				if (outFile.is_open()){
					outFile << outputQueue.front() << "\n";
					outputQueue.pop();
				}
			}
			outFile.flush(); // so the log survives a crash
		}
		void dispose(){ // MUST BE CALLED BEFORE PROGRAM EXIT, also this not needed anymore
			if (outFile.is_open()){outFile.close();}
		}
		~logger(){ //replaces above code
			if (outFile.is_open()){outFile.close();}
		}
};
