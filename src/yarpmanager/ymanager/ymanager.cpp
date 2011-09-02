/*
 *  Yarp Modules Manager
 *  Copyright: 2011 (C) Robotics, Brain and Cognitive Sciences - Italian Institute of Technology (IIT)
 *  Authors: Ali Paikan <ali.paikan@iit.it>
 * 
 *  Copy Policy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 *
 */


#include "ymanager.h"
#include "xmlapploader.h"
#include "application.h"

using namespace yarp::os;


#define UNIX

#ifdef UNIX
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <errno.h>
	#include <string.h>
	#include <sys/types.h>
	#include <dirent.h>
	#include <signal.h>

	string HEADER = "";
	string OKBLUE = "";
	string OKGREEN = "";
	string WARNING = "";
	string FAIL = "";
	string INFO = "";
	string ENDC = "";

#else
	#define HEADER		""
	#define OKBLUE		""
	#define OKGREEN		""
	#define WARNING		""
	#define FAIL		""
	#define INFO		""
	#define ENDC		""
#endif


#ifdef WITH_READLINE
	#include <readline/readline.h>
	#include <readline/history.h>
	const char* commands[21] = {"help", "exit","list mod", "list app", "add mod",
				  "add app", "load app", "run", "stop", "kill", 
				  "connect", "disconnect", "which", "check state",
				  "check cnn", "check dep", "set", "get", "export",
				  "show mod", " "};
	char* command_generator (const char* text, int state);
	char* appname_generator (const char* text, int state);
	char ** my_completion (const char* text, int start, int end);
	vector<string> appnames;	
#endif

#define DEF_CONFIG_FILE		"./ymanager.ini"

#define WELCOME_MESSAGE		"\
Yarp module manager 0.9 (BETA)\n\
==============================\n\n\
type \"help\" for more information."

#define HELP_MESSAGE		"\
Usage:\n\
      ymanager [option...]\n\n\
Options:\n\
  --help                  Show help\n\
  --config                Configuration file name\n"


/**
 * Class YConsoleManager
 */

YConsoleManager::YConsoleManager(int argc, char* argv[]) : Manager()
{
	bShouldRun = false;
	cmdline.fromCommand(argc, argv);

	if(cmdline.check("help"))
	{
		cout<<HELP_MESSAGE<<endl;
		return;
	}
	
	cout<<endl<<WELCOME_MESSAGE<<endl<<endl;
	
	if(cmdline.check("config"))
	{
		if(cmdline.find("config").asString() == "")
		{
			cout<<HELP_MESSAGE<<endl;
			return;			
		}
		if(!config.fromConfigFile(cmdline.find("config").asString().c_str()))
			cout<<WARNING<<"WARNING: "<<INFO<<cmdline.find("config").asString().c_str()<<" cannot be loaded."<<ENDC<<endl;
	}
	else 
		config.fromConfigFile(DEF_CONFIG_FILE);
	//		cout<<WARNING<<"WARNING: "<<INFO<<DEF_CONFIG_FILE<<" cannot be loaded. configuration is set to default."<<ENDC<<endl;

	/**
	 *  preparing default options
	 */
	if(!config.check("apppath"))
		config.put("apppath", "./");

	if(!config.check("modpath"))
		config.put("modpath", "./");
	
	if(!config.check("load_subfolders"))
		config.put("load_subfolders", "no");
	
	if(!config.check("watchdog"))
		config.put("watchdog", "no");

	if(!config.check("module_failure"))
		config.put("module_failure", "prompt");
	
	if(!config.check("connection_failure"))
		config.put("connection_failure", "prompt");

	if(!config.check("auto_connect"))
		config.put("auto_connect", "no");
	
	if(!config.check("auto_dependency"))
		config.put("auto_dependency", "no");

	if(!config.check("color_theme"))
		config.put("color_theme", "light");

	

	/**
	 * Set configuration
	 */
	if(config.find("color_theme").asString() == "dark")
		setColorTheme(THEME_DARK);
	else if(config.find("color_theme").asString() == "light")
		setColorTheme(THEME_LIGHT);
	else
		setColorTheme(THEME_NONE);

	if(config.find("watchdog").asString() == "yes")
		enableWatchDog();
	else
		disableWatchod();

	if(config.find("auto_dependency").asString() == "yes")
		enableAutoDependency();
	else
		disableAutoDependency();

	if(config.find("auto_connect").asString() == "yes")
		enableAutoConnect();
	else
		disableAutoConnect();

	if(config.check("modpath"))
		addModules(config.find("modpath").asString().c_str());

	if(config.check("apppath"))
	{
		if(config.find("load_subfolders").asString() == "yes")
			loadRecursiveApplications(config.find("apppath").asString().c_str());
		else
			addApplications(config.find("apppath").asString().c_str());	
	}

	reportErrors();	

#ifdef WITH_READLINE
	updateAppNames(&appnames);
#endif

#ifdef UNIX
	struct sigaction new_action, old_action;
     
	/* Set up the structure to specify the new action. */
	new_action.sa_handler = YConsoleManager::onSignal;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;

	sigaction (SIGINT, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGINT, &new_action, NULL);
	sigaction (SIGHUP, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGHUP, &new_action, NULL);
	sigaction (SIGTERM, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN)
		sigaction (SIGTERM, &new_action, NULL);

//	signal(SIGTERM, YConsoleManager::onExit());
//    signal(SIGINT, YConsoleManager::onExit());
#endif

	YConsoleManager::main();	
}

YConsoleManager::~YConsoleManager()
{
}



void YConsoleManager::onSignal(int signum)
{
	cout<<endl<<INFO<<"use"<<OKGREEN<<" 'exit' "<<INFO<<"to quit!"<<ENDC<<endl;
}


void YConsoleManager::main(void)
{

		
#ifdef WITH_READLINE
	rl_attempted_completion_function = my_completion;
#endif

	while(!cin.eof())
	{
		string temp;

#ifdef WITH_READLINE
		static char* szLine = (char*)NULL;
		if(szLine)
		{
			free(szLine);
			szLine = (char*)NULL;
		}
			
		szLine = readline(">>");
		if(szLine && *szLine)
		{
			temp = szLine;
			add_history(szLine);
		}
		else 
			temp = "";  
#else
		cout << ">> ";
		getline(cin, temp);
#endif

		//Break string into separate strings on whitespace
		vector<string> cmdList;
		stringstream foo(temp);
		string s;
		while (foo >> s)
		{
			if (s[0]=='~') s = getenv("HOME") + s.substr(1);
			cmdList.push_back(s);
		}
		if(!process(cmdList))
		{
			if(cmdList[0] == "exit")
			{
				if(YConsoleManager::exit())
					break;
			}
			else
			{
				cout<<"'"<<cmdList[0]<<"'"<<INFO<<" is not correct. ";
				cout<<"type \"help\" for more information."<<ENDC<<endl;
			}
		}
	}
	cout<<"bye."<<endl;

}

bool YConsoleManager::exit(void)
{
	if(!bShouldRun)
		return true;

	string ans;
	cout<<WARNING<<"WARNING: ";
	cout<<INFO<<"yarpmanager will terminate all of the running modules on exit. Are you sure? [No/yes] ";
	getline(cin, ans);
	if(compareString(ans.c_str(),"yes"))
	{
		bShouldRun = false; 
		kill();
		reportErrors();
		return true;
	}
	return false;
}


bool YConsoleManager::process(const vector<string> &cmdList)
{
	if (!cmdList.size() || cmdList[0] == "") 
		return true;
	
	/**
	 * help 
	 */
	 if((cmdList.size() == 1) && 
		(cmdList[0] == "help"))
	 {
		 help();
		 return true;
	 }


	/**
	 * add application 
	 */ 
	 if((cmdList.size() == 3) && 
		(cmdList[0] == "add") && (cmdList[1] == "app"))
	 {
		 if(addApplication(cmdList[2].c_str()))
			cout<<INFO<<cmdList[2]<<" is successfully added."<<ENDC<<endl;
		 reportErrors();
		#ifdef WITH_READLINE
		updateAppNames(&appnames);
		#endif
		 return true;
	 }

	/**
	 * add module
	 */ 
	 if((cmdList.size() == 3) && 
		(cmdList[0] == "add") && (cmdList[1] == "mod"))
	 {
		 if(addModule(cmdList[2].c_str()))
			cout<<INFO<<cmdList[2]<<" is successfully added."<<ENDC<<endl;
		 reportErrors();
		 return true;
	 }


	/**
	 * load application 
	 */ 
	 if((cmdList.size() == 3) && 
		(cmdList[0] == "load") && (cmdList[1] == "app"))
	 {
		 if(loadApplication(cmdList[2].c_str()))
		 {
			//cout<<cmdList[2]<<" is successfully loaded."<<endl;
		 	which();
		 }
		 reportErrors();
		 return true;
	 }

	/**
	 * load module
	 */ 
	/*
	 if((cmdList.size() >= 3) && 
		(cmdList[0] == "load") && (cmdList[1] == "mod"))
	 {
		 if(cmdList.size() > 3)
		 {
			if(manager.loadModule(cmdList[2].c_str(), cmdList[3].c_str()))
				cout<<cmdList[2]<<" is successfully loaded."<<endl;
		 }
		else
			if(manager.loadModule(cmdList[2].c_str()))
				cout<<cmdList[2]<<" is successfully loaded."<<endl;		
		 reportErrors();
		 return true;
	 }
	*/

	/**
	 * run 
	 */ 
	 if((cmdList.size() == 1) && 
		(cmdList[0] == "run"))
	 {
		 bShouldRun = run();
		 reportErrors();
		 return true;
	 }
	 if((cmdList.size() == 2) && 
		(cmdList[0] == "run"))
	 {
		 bShouldRun = run(atoi(cmdList[1].c_str()));
		 reportErrors();
		 return true;
	 }

	/**
	 * stop 
	 */ 
	 if((cmdList.size() == 1) && 
		(cmdList[0] == "stop"))
	 {
		 bShouldRun = false;
		 stop();
		 reportErrors();
		 return true;
	 }
	 if((cmdList.size() == 2) && 
		(cmdList[0] == "stop"))
	 {
		 bShouldRun = false;
		 stop(atoi(cmdList[1].c_str()));
		 reportErrors();
		 return true;
	 }

	/**
	 * kill 
	 */ 
	 if((cmdList.size() == 1) && 
		(cmdList[0] == "kill"))
	 {
		 bShouldRun = false; 
		 kill();
		 reportErrors();
		 return true;
	 }
	 if((cmdList.size() == 2) && 
		(cmdList[0] == "kill"))
	 {
		 bShouldRun = false;
		 kill(atoi(cmdList[1].c_str()));
		 reportErrors();
		 return true;
	 }

	/**
	 * connect
	 */ 
	 if((cmdList.size() == 1) && 
		(cmdList[0] == "connect"))
	 {
		 connect();
		 reportErrors();
		 return true;
	 }
	 if((cmdList.size() == 2) && 
		(cmdList[0] == "connect"))
	 {
		 connect(atoi(cmdList[1].c_str()));
		 reportErrors();
		 return true;
	 }


	/**
	 * disconnect
	 */ 
	 if((cmdList.size() == 1) && 
		(cmdList[0] == "disconnect"))
	 {
		 disconnect();
		 reportErrors();
		 return true;
	 }
	 if((cmdList.size() == 2) && 
		(cmdList[0] == "disconnect"))
	 {
		 disconnect(atoi(cmdList[1].c_str()));
		 reportErrors();
		 return true;
	 }


	/**
	 * which 
	 */ 
	 if((cmdList.size() == 1) && 
		(cmdList[0] == "which"))
	 {
	 	which();
		return true;
	 }

	/**
	 * check for dependencies
	 */
	if((cmdList.size() == 2) && 
		(cmdList[0] == "check") && (cmdList[1] == "dep"))
	{
		if(checkDependency())
			cout<<INFO<<"All of resource dependencies are satisfied."<<ENDC<<endl;
		reportErrors();
		return true;
	}
	
	
	/**
	 * check for running state
	 */
	if((cmdList.size() == 3) && 
		(cmdList[0] == "check") && (cmdList[1] == "state"))
	{
		ExecutablePContainer modules = getExecutables();
		unsigned int id = (unsigned int)atoi(cmdList[2].c_str());
		if(id>=modules.size())
		{
			cout<<FAIL<<"ERROR:   "<<INFO<<"Module id is out of range."<<ENDC<<endl;
			return true;
		}

		if(running(id))
			cout<<OKGREEN<<"<RUNNING> ";
		else
			cout<<FAIL<<"<STOPPED> ";
		cout<<INFO<<"("<<id<<") ";
		cout<<modules[id]->getCommand();
		cout<<" ["<<modules[id]->getHost()<<"]"<<ENDC<<endl;
		reportErrors();
		return true;
	}
	if((cmdList.size() == 2) && 
		(cmdList[0] == "check") && (cmdList[1] == "state"))
	{

		ExecutablePContainer modules = getExecutables();
		ExecutablePIterator moditr;
		unsigned int id = 0;
		for(moditr=modules.begin(); moditr<modules.end(); moditr++)
		{
			if(running(id))
				cout<<OKGREEN<<"<RUNNING> ";
			else
				cout<<FAIL<<"<STOPPED> ";
			cout<<INFO<<"("<<id<<") ";
			cout<<(*moditr)->getCommand();
			cout<<" ["<<(*moditr)->getHost()<<"]"<<ENDC<<endl;
			id++;
		}

		reportErrors();
		return true;
	}


	/**
	 * check for connection state
	 */
	if((cmdList.size() == 3) && 
		(cmdList[0] == "check") && (cmdList[1] == "cnn"))
	{

		CnnContainer connections  = getConnections();
		unsigned int id = (unsigned int)atoi(cmdList[2].c_str());
		if(id>=connections.size())
		{
			cout<<FAIL<<"ERROR:   "<<INFO<<"Connection id is out of range."<<ENDC<<endl;
			return true;
		}

		if(connected(id))
			cout<<OKGREEN<<"<CONNECTED> ";
		else
			cout<<FAIL<<"<DISCONNECTED> ";

		cout<<INFO<<"("<<id<<") ";
		cout<<connections[id].from()<<" - "<<connections[id].to();
				cout<<" ["<<carrierToStr(connections[id].carrier())<<"]"<<ENDC<<endl;
		reportErrors();
		return true;
	}
	if((cmdList.size() == 2) && 
		(cmdList[0] == "check") && (cmdList[1] == "cnn"))
	{
		CnnContainer connections  = getConnections();
		CnnIterator cnnitr;
		int id = 0;
		for(cnnitr=connections.begin(); cnnitr<connections.end(); cnnitr++)
		{
			if(connected(id))
				cout<<OKGREEN<<"<CONNECTED> ";
			else
				cout<<FAIL<<"<DISCONNECTED> ";

			cout<<INFO<<"("<<id<<") ";
			cout<<(*cnnitr).from()<<" - "<<(*cnnitr).to();
				cout<<" ["<<carrierToStr((*cnnitr).carrier())<<"]"<<ENDC<<endl;
			id++;
		}
		return true;
	}



	/**
	 *  list available modules
	 */
	if((cmdList.size() == 2) && 
		(cmdList[0] == "list") && (cmdList[1] == "mod"))
	{
		KnowledgeBase* kb = getKnowledgeBase();
		ModulePContainer mods =  kb->getModules();
		int id = 0;
		for(ModulePIterator itr=mods.begin(); itr!=mods.end(); itr++)
		{
			string fname;
			string fpath = (*itr)->getXmlFile();

#ifdef __WINDOWS__
			size_t pos = fpath.rfind("\\");
#else
			size_t pos = fpath.rfind("/");
#endif
			if(pos!=string::npos)
				fname = fpath.substr(pos);
			else
				fname = fpath;
			cout<<INFO<<"("<<id++<<") ";
			cout<<OKBLUE<<(*itr)->getName()<<ENDC;
			cout<<INFO<<" ["<<fname<<"]"<<ENDC<<endl;
		}
		return true;
	}	


	/**
	 *  list available applications
	 */
	if((cmdList.size() == 2) && 
		(cmdList[0] == "list") && (cmdList[1] == "app"))
	{
		KnowledgeBase* kb = getKnowledgeBase();
		ApplicaitonPContainer apps =  kb->getApplications();
		int id = 0;
		for(ApplicationPIterator itr=apps.begin(); itr!=apps.end(); itr++)
		{
			string fname;
			string fpath = (*itr)->getXmlFile();

#ifdef __WINDOWS__
			size_t pos = fpath.rfind("\\");
#else
			size_t pos = fpath.rfind("/");
#endif
			if(pos!=string::npos)
				fname = fpath.substr(pos);
			else
				fname = fpath;
			cout<<INFO<<"("<<id++<<") ";
			cout<<OKBLUE<<(*itr)->getName()<<ENDC;
			cout<<INFO<<" ["<<fname<<"]"<<ENDC<<endl;
		}
		return true;
	}	

	/**
	 *  export knowledgebase graph 
	 */
	if((cmdList.size() == 2) && 
		(cmdList[0] == "export") )
	{
		if(!exportDependencyGraph(cmdList[1].c_str()))
			cout<<FAIL<<"ERROR:   "<<INFO<<"Cannot export graph to "<<cmdList[1]<<"."<<ENDC<<endl;
		return true;
	}	
	
	/**
	 * edit module xml file 
	 */
	/*
	if((cmdList.size() == 3) && 
		(cmdList[0] == "edit") && (cmdList[1] == "mod"))
	{
		if(!config.check("xmleditor"))
		{
			cout<<"please set 'xmleditro' first."<<endl;
			return true; 
		}
		
		XmlModLoader modloader(config.find("modpath").asString().c_str() ,
							   cmdList[2].c_str());
		if(!modloader.init())
		{
			reportErrors();
			return true;	
		}
	
		Module* module;
		if(!(module=modloader.getNextModule()))
		{
			cout<<"cannot find module "<<cmdList[2]<<"."<<endl;
			reportErrors();
			return true;
		}
		editXmlFile(module->getXmlFile());
		return true;
	}	
	*/

	/**
	 * show module's insformation 
	 */
	if((cmdList.size() == 3) && 
		(cmdList[0] == "show") && (cmdList[1] == "mod"))
	{
		KnowledgeBase* kb = getKnowledgeBase();
		if(!kb->getModule(cmdList[2].c_str()))
		{
			cout<<FAIL<<"ERROR:   "<<INFO<<"'"<<cmdList[2].c_str()<<"' not found."<<ENDC<<endl;
			return true;
		}
		cout<<INFO;
		PRINT_MODULE(kb->getModule(cmdList[2].c_str()));
		cout<<ENDC;
		return true;
	}	

	/**
	 * edit application xml file 
	 */
	/*
	if((cmdList.size() == 3) && 
		(cmdList[0] == "edit") && (cmdList[1] == "app"))
	{
		if(!config.check("xmleditor"))
		{
			cout<<"please set 'xmleditor' first."<<endl;
			return true; 
		}
		
		XmlAppLoader apploader(config.find("apppath").asString().c_str() ,
							   cmdList[2].c_str());
		if(!apploader.init())
		{
			reportErrors();
			return true;	
		}
	
		Application* app;
		if(!(app=apploader.getNextApplication()))
		{
			cout<<"cannot find application "<<cmdList[2]<<"."<<endl;
			reportErrors();
			return true;
		}
		editXmlFile(app->getXmlFile());
		return true;
	}	
	*/

	/**
	 * set an option  
	 */ 
	 if((cmdList.size() == 3) && 
		(cmdList[0] == "set"))
	 {
		 config.unput(cmdList[1].c_str());
		 config.put(cmdList[1].c_str(), cmdList[2].c_str());
		 
		if(cmdList[1] == string("watchdog"))
		{
			if(cmdList[2] == string("yes"))
				enableWatchDog();
			else
				disableWatchod();
		}

		if(cmdList[1] == string("auto_dependency"))
		{
			if(cmdList[2] == string("yes"))
				enableAutoDependency();
			else
				disableAutoDependency();
		}

		if(cmdList[1] == string("auto_connect"))
		{
			if(cmdList[2] == string("yes"))
				enableAutoConnect();
			else
				disableAutoConnect();
		}
	
		if(cmdList[1] == string("color_theme"))
		{
			if(cmdList[2] == string("dark"))
				setColorTheme(THEME_DARK);
			else if(cmdList[2] == string("light"))
				setColorTheme(THEME_LIGHT);
			else
				setColorTheme(THEME_NONE);
		}

		return true;
	 }


	/**
	 * get an option
	 */ 
	 if((cmdList.size() == 2) && 
		(cmdList[0] == "get"))
	 {
		 if(config.check(cmdList[1].c_str()))
		 {
		 	cout<<OKBLUE<<cmdList[1]<<INFO<<" = ";
			cout<<OKGREEN<<config.find(cmdList[1].c_str()).asString()<<ENDC<<endl;
		 }
		 else
			cout<<FAIL<<"ERROR:   "<<INFO<<"'"<<cmdList[1].c_str()<<"' not found."<<ENDC<<endl;
		 return true;
	 }
	
	return false;
}


void YConsoleManager::help(void)
{
	cout<<"Here is a list of Yarp manager keywords.\n"<<endl;	
	cout<<OKGREEN<<"help"<<INFO<<"                    : show help."<<ENDC<<endl;
	cout<<OKGREEN<<"exit"<<INFO<<"                    : exit yarp manager."<<ENDC<<endl;
	cout<<OKGREEN<<"list mod"<<INFO<<"                : list available modules."<<ENDC<<endl;
	cout<<OKGREEN<<"list app"<<INFO<<"                : list available applications."<<ENDC<<endl;
	cout<<OKGREEN<<"add mod <filename>"<<INFO<<"      : add a module from its description file."<<ENDC<<endl;
	cout<<OKGREEN<<"add app <filename>"<<INFO<<"      : add an application from its description file."<<ENDC<<endl;
	cout<<OKGREEN<<"load app <application>"<<INFO<<"  : load an application to run."<<endl;
//	cout<<"load mod <module> <host>: load a module to run on an optional host."<<endl;
	cout<<OKGREEN<<"run [id]"<<INFO<<"                : run application or a module indicated by id."<<ENDC<<endl;
	cout<<OKGREEN<<"stop [id]"<<INFO<<"               : stop running application or module indicated by id."<<ENDC<<endl;
	cout<<OKGREEN<<"kill [id]"<<INFO<<"               : kill running application or module indicated by id."<<ENDC<<endl;
	cout<<OKGREEN<<"connect [id]"<<INFO<<"            : stablish all connections or just one connection indicated by id."<<ENDC<<endl;
	cout<<OKGREEN<<"disconnect [id]"<<INFO<<"         : remove all connections or just one connection indicated by id."<<ENDC<<endl;
	cout<<OKGREEN<<"which"<<INFO<<"                   : list loaded modules, connections and resource dependencies."<<ENDC<<endl;	
	cout<<OKGREEN<<"check dep"<<INFO<<"               : check for all resource dependencies."<<ENDC<<endl;
	cout<<OKGREEN<<"check state [id]"<<INFO<<"        : check for running state of application or a module indicated by id."<<ENDC<<endl;
	cout<<OKGREEN<<"check cnn [id]"<<INFO<<"          : check for all connections state or just one connection indicated by id."<<ENDC<<endl;
	cout<<OKGREEN<<"set <option> <value>"<<INFO<<"    : set value to an option."<<ENDC<<endl;
	cout<<OKGREEN<<"get <option>"<<INFO<<"            : show value of an option."<<ENDC<<endl;	
	cout<<OKGREEN<<"export <filename>"<<INFO<<"       : export application's graph as Graphviz dot format."<<ENDC<<endl;
//	cout<<"edit mod <modname>      : open module relevant xml file to edit."<<endl;
//	cout<<"edit app <appname>      : open application relevant xml file to edit."<<endl;
	cout<<OKGREEN<<"show mod <modname>"<<INFO<<"      : display module information (description, input, output,...)."<<ENDC<<endl;
	
	cout<<endl;
}


void YConsoleManager::which(void)
{
	ExecutablePContainer modules = getExecutables();
	CnnContainer connections  = getConnections();
	ExecutablePIterator moditr;
	CnnIterator cnnitr;
	
	cout<<endl<<HEADER<<"Application: "<<ENDC<<endl;
	cout<<OKBLUE<<getApplicationName()<<ENDC<<endl;

	cout<<endl<<HEADER<<"Modules: "<<ENDC<<endl;
	int id = 0;
	for(moditr=modules.begin(); moditr<modules.end(); moditr++)
	{
		cout<<INFO<<"("<<id++<<") ";
		cout<<OKBLUE<<(*moditr)->getCommand()<<INFO;
		cout<<" ["<<(*moditr)->getHost()<<"] ["<<(*moditr)->getParam()<<"]";
		cout<<" ["<<(*moditr)->getEnv()<<"]"<<ENDC<<endl; 
	}
	cout<<endl<<HEADER<<"Connections: "<<ENDC<<endl;
	id = 0;
	for(cnnitr=connections.begin(); cnnitr<connections.end(); cnnitr++)
	{
		cout<<INFO<<"("<<id++<<") ";
		cout<<OKBLUE<<(*cnnitr).from()<<" - "<<(*cnnitr).to()<<INFO;
			cout<<" ["<<carrierToStr((*cnnitr).carrier())<<"]"<<ENDC<<endl;
	}		
	
	cout<<endl<<HEADER<<"Resources:"<<ENDC<<endl;
	id = 0;
	ResourcePIterator itrS;
	for(itrS=getResources().begin(); itrS!=getResources().end(); itrS++)
	{
		cout<<INFO<<"("<<id++<<") ";
		cout<<OKBLUE<<(*itrS)->getName()<<INFO<<" ["<<(*itrS)->getPort()<<"]"<<ENDC<<endl;
	}
	cout<<endl;
}


void YConsoleManager::reportErrors(void)
{
	ErrorLogger* logger  = ErrorLogger::Instance();	
	if(logger->errorCount() || logger->warningCount())
	{
		const char* msg;
		while((msg=logger->getLastError()))
			cout<<FAIL<<"ERROR:   "<<INFO<<msg<<ENDC<<endl;

		while((msg=logger->getLastWarning()))
			cout<<WARNING<<"WARNING: "<<INFO<<msg<<ENDC<<endl;
	}	
}
	
	
void YConsoleManager::editXmlFile( const char* filename)
{

#ifdef UNIX	

	pid_t kidpid;
	if((kidpid = fork()))
	{
		//Parent
		int status = 0;
		waitpid(kidpid, &status, 0);
		return;
	}
	//Child - execute the command
	execlp(config.find("xmleditor").asString().c_str(), 
		   config.find("xmleditor").asString().c_str(),
		   filename, (char*)NULL);

	//Program will never reach here unless execvp failed
	cout<<"Failed to run editor "<<config.find("xmleditor").asString()<<". ";
	cout<<strerror(errno)<<endl; 
	::exit(0);  
#endif //UNIX	

}

void YConsoleManager::onExecutableStart(void* which) { }

void YConsoleManager::onExecutableStop(void* which) { }

void YConsoleManager::onExecutableDied(void* which) 
{
	Executable* exe = (Executable*) which;
	if(config.check("module_failure") &&
	 config.find("module_failure").asString() == "prompt")
		cout<<exe->getCommand()<<" from "<<exe->getHost()<<" is dead!"<<endl;	

	if(bShouldRun && config.check("module_failure") &&
	 config.find("module_failure").asString() == "recover")
	 {
	 	cout<<endl<<exe->getCommand()<<" from "<<exe->getHost()<<" is dead! (restarting...)"<<endl;
	 	exe->start();
	 }

	if(bShouldRun && config.check("module_failure") &&
	 config.find("module_failure").asString() == "terminate")
	 {
	 	cout<<endl<<exe->getCommand()<<" from "<<exe->getHost()<<" is dead! (terminating application...)"<<endl;	
		bShouldRun = false;
	 	stop();
		reportErrors();
	 }
}

void YConsoleManager::onCnnStablished(void* which) { }

void YConsoleManager::onCnnFailed(void* which) 
{
	Connection* cnn = (Connection*) which;
	if(config.check("connection_failure") &&
	 config.find("connection_failure").asString() == "prompt")
		cout<<endl<<"connection failed between "<<cnn->from()<<" and "<<cnn->to()<<endl;

	if(bShouldRun && config.check("connection_failure") &&
	 config.find("connection_failure").asString() == "terminate")
	 {
		cout<<endl<<"connection failed between "<<cnn->from()<<" and "<<cnn->to()<<"(terminating application...)"<<endl;
		bShouldRun = false;
		stop();
		reportErrors();
	 }
}


bool YConsoleManager::loadRecursiveApplications(const char* szPath)
{
#ifdef UNIX

	string strPath = szPath;
	if((strPath.rfind("/")==string::npos) || 
			(strPath.rfind("/")!=strPath.size()-1))
			strPath = strPath + string("/");

	DIR *dir;
	struct dirent *entry;
	if ((dir = opendir(strPath.c_str())) == NULL)
		return false;

	addApplications(strPath.c_str());

	while((entry = readdir(dir)))
	{
		if((string(entry->d_name) != string(".")) 
		&& (string(entry->d_name) != string("..")))
		{
			string name = strPath + string(entry->d_name);
			loadRecursiveApplications(name.c_str());
		}
	}
	closedir(dir);
#endif
	return true;
}



void YConsoleManager::updateAppNames(vector<string>* names)
{
	names->clear();
	KnowledgeBase* kb = getKnowledgeBase();
	ApplicaitonPContainer apps =  kb->getApplications();
	for(ApplicationPIterator itr=apps.begin(); itr!=apps.end(); itr++)
		names->push_back((*itr)->getName());	
}



void YConsoleManager::setColorTheme(ColorTheme theme)
{
#ifdef UNIX
	switch(theme) {
		case THEME_DARK : { 
			HEADER = "\033[01;95m";
			OKBLUE = "\033[94m";
			OKGREEN = "\033[92m";
			WARNING = "\033[93m";
			FAIL = "\033[91m";
			INFO = "\033[37m";
			ENDC = "\033[0m";
			break;
		}
		case THEME_LIGHT: { 
			HEADER = "\033[01;35m";
			OKBLUE = "\033[34m";
			OKGREEN = "\033[32m";
			WARNING = "\033[33m";
			FAIL = "\033[31m";
			INFO = "\033[0m";
			ENDC = "\033[0m";
			break;
		}
		default: { 
			HEADER = "";
			OKBLUE = "";
			OKGREEN = "";
			WARNING = "";
			FAIL = "";
			INFO = "";
			ENDC = "";
			break;
		}
	};
#endif

}



#ifdef WITH_READLINE

char* dupstr(char* s) 
{
  char *r;
  r = (char*) malloc ((strlen (s) + 1));
  strcpy (r, s);
  return (r);
}

/* Attempt to complete on the contents of TEXT.  START and END show the
   region of TEXT that contains the word to complete.  We can use the
   entire line in case we want to do some simple parsing.  Return the
   array of matches, or NULL if there aren't any. */
char ** my_completion (const char* text, int start, int end)
{
	char **matches;

	matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
	if (start == 0)
		matches = rl_completion_matches(text, &command_generator);
   else
		matches = rl_completion_matches(text, &appname_generator);

 	return (matches);
}


/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char* command_generator (const char* text, int state)
{
  //printf("commmand_genrator\n");
  
  static int list_index, len;
  char *name;

  /* if this is a new word to complete, initialize now.  this includes
     saving the length of text for efficiency, and initializing the index
     variable to 0. */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  while ((list_index<21) && (name = (char*)commands[list_index]))
    {
      list_index++;
      if (strncmp (name, text, len) == 0)
        return (dupstr(name));
    }
  
  /* if no names matched, then return null. */
  return ((char *)NULL);
}


char* appname_generator (const char* text, int state)
{
  
  static unsigned int list_index, len;
  char *name;

   if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  while ((list_index<appnames.size()) && (name = (char*)appnames[list_index].c_str()))
    {
      list_index++;
      if (strncmp (name, text, len) == 0)
        return (dupstr(name));
    }
  
  return ((char *)NULL);
}



#endif 